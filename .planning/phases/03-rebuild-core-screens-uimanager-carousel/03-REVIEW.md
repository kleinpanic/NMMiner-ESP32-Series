---
phase: 03-rebuild-core-screens-uimanager-carousel
reviewed: 2026-06-18T00:00:00Z
depth: standard
files_reviewed: 14
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_miner.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_clock.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_sprice.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_weather.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/lv_conf.h
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
findings:
  critical: 2
  warning: 5
  info: 4
  total: 11
status: issues_found
---

# Phase 3: Code Review Report

**Reviewed:** 2026-06-18
**Depth:** standard
**Files Reviewed:** 14
**Status:** issues_found

## Summary

The new LVGL-over-TFT_eSPI UI is well structured: env-guarding is consistent and
correct across all 12 modules, the SPEC geometry transcription is faithful (Miner
hashrate TOP_MID @ 1,3; Clock price BOTTOM_RIGHT @ -2,0; AM/PM CENTER @ 80,-31 all
match SPEC.md), the destroy()/build() pattern frees LVGL trees per navigate so the
6s carousel does not leak per cycle, and mining.cpp is genuinely untouched. No
firmware-literal/closed-binary bytes leaked (D-11 clean).

However the central safety claim of the phase — "the UI runs on its own path
without racing the mining tasks" — is **false as shipped**. The legacy TFT_eSPI
cyclic-screen renderer was never disabled for the CYD env: the Monitor FreeRTOS
task still pushes full-screen images to the *same* `tft` instance that LVGL flushes
to, from a different core/task, with no mutex. This is a concurrent SPI-bus data
race (CR-01) that will corrupt the display and can wedge the SPI peripheral. A
second blocker (CR-02) is that screen teardown/rebuild is driven from inside a
running LVGL timer callback, deleting the active screen object mid-`lv_timer_handler`.

## Critical Issues

### CR-01: Legacy Monitor-task screen renderer races LVGL on the shared `tft`/SPI bus

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:160`,
`src/mining.cpp` (runMonitor, untouched), `src/drivers/displays/display.cpp:112-122`,
`src/ui_nm/nm_display.cpp:52-63`

**Issue:** For the CYD env, `setup()` skips `initDisplay()` and calls
`nm_uimanager_init()` instead, but it does **not** stop the legacy display path.
The Monitor task is still created unconditionally:

```
xTaskCreatePinnedToCore(runMonitor, "Monitor", ..., 5, NULL, 1);  // core 1, prio 5
```

`runMonitor` (mining.cpp) loops calling `drawCurrentScreen(mElapsed)` and
`animateCurrentScreen(frame)`. For this env `currentDisplayDriver =
&esp32_2432S028RDriver` (display.cpp:36), whose cyclic-screen callbacks push pixels
straight to the global `tft` (`tft.pushImage(...)`, `tft.fillScreen(...)` — see
esp23_2432s028r.cpp:215/298/360/454/514). Meanwhile the Arduino `loop()` (also core
1) calls `nm_uimanager_tick()` -> `lv_timer_handler()` -> `nm_disp_flush()`, which
does `tft.startWrite()/setAddrWindow/pushColors/endWrite` on the *same* `tft`.

Two tasks issue interleaved SPI transactions to one device with no `startWrite`
serialization, no mutex, and no `kpScreensaverActive`-style guard. Expected symptoms:
torn/garbled frames, half-drawn legacy bitmaps over the LVGL UI, and at worst a
deadlocked SPI bus if a `startWrite` from one task is interrupted by another's
`setAddrWindow`. The `kpScreensaverActive` guard at display.cpp:114 only short-
circuits the screensaver, not the cyclic screens.

**Fix:** Disable the legacy renderer for the CYD env. Either gate the Monitor task's
draw calls or skip the draw entirely. Minimal, at the routing seam:

```cpp
// src/drivers/displays/display.cpp
void drawCurrentScreen(unsigned long mElapsed) {
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  return;  // CYD env: LVGL UIManager owns the panel (D-32)
#else
  if (kpScreensaverActive) return;
  currentDisplayDriver->cyclic_screens[currentDisplayDriver->current_cyclic_screen](mElapsed);
#endif
}
void animateCurrentScreen(unsigned long frame) {
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  return;
#else
  currentDisplayDriver->animateCurrentScreen(frame);
#endif
}
```

If any legacy draw must coexist later, both writers must share a single SPI mutex
taken around each `startWrite/endWrite` span — but for Phase 3 the legacy path
should simply be off.

### CR-02: Screen teardown/rebuild runs inside the LVGL timer callback (deletes the active screen mid-`lv_timer_handler`)

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:52-93,111`

**Issue:** `nm_autoadvance_cb` is an `lv_timer_t` callback, so it executes *inside*
`lv_timer_handler()` (called from `nm_display_tick`). It calls
`nm_uimanager_next_page()` -> `nm_uimanager_goto_page()`, which:

1. calls `prev->destroy()` -> `lv_obj_del(active_screen)` while that object is still
   `lv_disp_get_scr_act()`, then
2. `next->build()`, then
3. `lv_scr_load(new)`.

Deleting the currently-active screen object from within a running LVGL timer pass,
before the replacement is loaded, leaves the display's active-screen pointer dangling
for the remainder of that handler iteration and mutates the object tree LVGL may
still be iterating. LVGL v8 expects screen swaps via `lv_scr_load`/`lv_scr_load_anim`
(which defers the old-screen delete), not a raw `lv_obj_del` of the active screen
from inside `lv_timer_handler`. This is undefined-order behavior that can crash or
corrupt the heap; it is masked today only because the screens are static and the
window is short, but it is not safe and will surface under load/animation.

**Fix:** Do not delete-then-build. Load the new screen first, then delete the old one
*after* the swap, and prefer `lv_scr_load_anim(..., auto_del=true)` which LVGL
defers safely:

```cpp
void nm_uimanager_goto_page(uint8_t index) {
    if (index >= NM_PAGE_COUNT) index = NM_PAGE_LOADING;

    const nm_screen_t* next = pages[index];
    lv_obj_t* new_obj = (next && next->build) ? next->build() : NULL;
    lv_obj_t* old_obj = s_current_obj;

    if (new_obj) lv_scr_load(new_obj);   // swap active screen first
    if (s_have_page) {                   // then tear down what we left
        const nm_screen_t* prev = pages[s_current];
        if (prev && prev->destroy) prev->destroy();  // lv_obj_del now safe (no longer active)
    }
    s_current_obj = new_obj;
    s_current = index;
    s_have_page = true;
    s_prefs.putUInt(NM_UI_KEY, index);
}
```

Note this also requires the per-screen `destroy()` to delete `s_scr` (it does) and
the build to have populated a fresh `s_scr` — fine since build runs before destroy
of the *other* page's globals. (Each page owns distinct static handles, so building
page B then destroying page A does not clobber B's `s_scr`.)

## Warnings

### WR-01: NVS write on every auto-advance — flash wear, 6s cadence forever

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:80`

**Issue:** `goto_page` calls `s_prefs.putUInt(NM_UI_KEY, index)` on *every* navigation.
With auto-advance enabled (`NM_UI_AUTOADVANCE 1`, 6000 ms), that is a NVS write every
6 seconds, forever — ~14,400 writes/day. NVS sits on the same flash as the firmware;
ESP-IDF NVS does wear-leveling but this is gratuitous wear for a value that only
matters across reboots. The "LastPage" intent is to persist *user-chosen* pages, not
debug auto-cycle steps.

**Fix:** Skip the persist when the move came from auto-advance, or only write when the
value actually changed and debounce. Simplest: pass a `persist` flag, or guard:

```cpp
if (index != s_current_persisted) {   // and skip from autoadvance path
    s_prefs.putUInt(NM_UI_KEY, index);
    s_current_persisted = index;
}
```

Phase 4 disables auto-advance, but this code ships now and runs the wear loop until then.

### WR-02: `s_prefs` may be written before `begin()` on the first `goto_page`

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:96-107`

**Issue:** `nm_uimanager_init()` calls `nm_display_init()` first, then
`s_prefs.begin(NM_UI_NS, false)`, then `nm_uimanager_goto_page(last)`. Ordering is
correct here, but `goto_page` is a public API (header-exported, intended for Phase 4
touch nav) that unconditionally calls `s_prefs.putUInt(...)`. If any caller invokes
`goto_page` before `init()` (or the begin fails silently), the `putUInt` is a no-op
on an unopened handle and the persist silently fails. There is no return-value check
on `s_prefs.begin()`.

**Fix:** Check `begin()` success and gate persistence on it; document that
`goto_page` requires `init()` first, or make persistence defensive:

```cpp
if (!s_prefs_ready) return;  // set s_prefs_ready = s_prefs.begin(...) in init
```

### WR-03: No null check on `build()` / `lv_obj_create` returns under OOM

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:70-74`;
all `*_build()` in nm_screen_*.cpp

**Issue:** With LV_MEM_SIZE capped at 32 KB and no PSRAM (review focus #5), allocation
under fragmentation is plausible. `goto_page` does guard `if (obj)` before
`lv_scr_load`, but if `build()` returns NULL the manager sets `s_have_page = true`
with `s_current_obj = NULL`, leaving the display showing nothing and the next
`destroy()` operating on a half-built page (the screen module's static handles like
`s_w.scr` may be non-NULL from a partially-completed build, since `lv_obj_create`
failures inside build are not checked either). The screen builders never check
`lv_obj_create(NULL)` / `lv_label_create` returns; under LVGL's
`LV_USE_ASSERT_MALLOC 1` + `LV_ASSERT_HANDLER while(1);` an alloc failure becomes a
**hard hang** rather than a recoverable path.

**Fix:** Either (a) leave asserts but accept hard-hang-on-OOM as the documented
failure mode, or (b) check the top-level `lv_obj_create(NULL)` in each build and
return NULL early, and in `goto_page` only set `s_have_page`/`s_current` when `obj`
is non-NULL so a failed build doesn't corrupt the destroy bookkeeping. At minimum,
do not flip `s_have_page = true` when `obj == NULL`.

### WR-04: 24-hour-mode AM/PM widget left as dead state; `LV_ASSERT_HANDLER while(1)` is a silent brick

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/lv_conf.h:130-136`

**Issue:** `LV_USE_ASSERT_NULL 1` and `LV_USE_ASSERT_MALLOC 1` with
`#define LV_ASSERT_HANDLER while(1);` means any LVGL assert (null parent, malloc
failure) is an **infinite busy-loop with no watchdog feed and no log** (`LV_USE_LOG 0`).
On a miner the practical result is a frozen UI core and a confusing field failure with
zero diagnostics — particularly bad combined with WR-03's unchecked allocations on a
32 KB pool. This is a robustness landmine for a no-PSRAM target.

**Fix:** Provide a real handler that logs and resets (or at least feeds the WDT), e.g.
route to `esp_restart()` or a `Serial.printf` + `abort()`, instead of `while(1);`.
Or enable `LV_USE_LOG` for at least error level so the freeze is diagnosable.

### WR-05: `nm_display_tick()` calls `lv_timer_handler()` which can do unbounded work in one `loop()` pass

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp:101-104`,
`src/NerdMinerV2.ino.cpp:281`

**Issue:** The phase claims the UI pump "never starves the stratum/miner tasks
(T-03-04)" because it runs from the non-mining loop. But `lv_timer_handler()` renders
*and flushes* synchronously: a full-screen refresh (e.g. right after a screen swap)
flushes 240/40 = 6 tiles, each a blocking SPI `pushColors` of 320×40×2 bytes. With
the legacy renderer also on the bus (CR-01) and miner tasks at equal/higher priority
on the same core, the worst-case single `nm_uimanager_tick()` can hold core 1 long
enough to matter. The `vTaskDelay(50)` after it yields, but the flush itself is
uninterruptible SPI. This is acceptable for label-only screens but the "never starves"
claim is stronger than the code guarantees, especially on full redraws.

**Fix:** No code change strictly required for label screens, but the claim should be
qualified. If starvation appears, cap work per tick (LVGL returns the time to next
timer; you can bound flush tiles) or move LVGL to a dedicated low-priority task with
the SPI mutex from CR-01.

## Info

### IN-01: Stale comments contradict the actual buffer/pool sizes

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp:14-15`,
`src/ui_nm/lv_conf.h:11-13`

**Issue:** nm_display.cpp header comment says "two 320*40 partial draw buffers" but
the code uses a single buffer (`s_buf1`, `buf2 = NULL` at line 74) — the inline
comment at line 42 correctly says ONE buffer, so the file contradicts itself.
lv_conf.h:11-13 still documents "2 partial draw buffers (~51 KB) + LV_MEM_SIZE
(48 KB) ≈ 99 KB" while the actual config is one buffer + 32 KB (line 55). Misleading
for the next maintainer reasoning about the DRAM budget.

**Fix:** Update both comments to the shipped reality: one 320×40 buffer (~25.6 KB) +
32 KB LV_MEM_SIZE.

### IN-02: `getMiningData()` shared-global read from a second task (loop) without locking

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_miner.cpp:153`,
`src/monitor.cpp:323`

**Issue:** `miner_update()` (Arduino loop) now calls `getMiningData(0)` in addition to
the Monitor task. It returns a copy of Arduino `String` fields read from shared globals
updated by the miner/monitor tasks. Concurrent String read while another task reassigns
the same String can produce a torn read (dangling internal buffer pointer) → garbage or
crash, since Arduino String is not thread-safe. Low likelihood (short windows, copy
semantics) and the fork already reads these globally, but the UI adds a second reader on
a different schedule.

**Fix:** Out of v1 scope to fully fix, but note it. Proper fix is a snapshot mutex around
the mining_data globals, or have the Monitor task publish an atomic snapshot the UI reads.

### IN-03: Clock header gray palette constant does not match SPEC exactly

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_clock.cpp:47`,
`hardware/recovered-assets/SPEC.md:128`

**Issue:** SPEC Clock header gray is #838183 (RGB565 0x8410); the code uses
`NMMINER_COLOR_CLOCK_GRAY_HEX` documented in the file comment as #848284. One LSB off
per channel — visually negligible but a fidelity deviation from the recovered fact.
Verify the palette constant resolves to 0x8410.

**Fix:** Confirm `NMMINER_COLOR_CLOCK_GRAY_HEX` == 0x838183/0x8410; reconcile the comment.

### IN-04: Loading screen seeds a hardcoded IP and a "TENTATIVE" title geometry

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp:78,106,114`

**Issue:** The IP label is seeded with a literal `"192.168.124.222"` (someone's real
LAN IP from capture) rather than empty/`0.0.0.0`; it persists on screen until monitor
wiring lands (Phase 5), showing a misleading address. The "NMMiner" title and
"kleinpanic" accent are placed at TENTATIVE coordinates (SPEC row 7 geometry not
extracted) and self-document as such — acceptable for this phase but flag so they are
revisited, not frozen.

**Fix:** Seed the IP label empty or `"--"` until real IP is available; track the
TENTATIVE title/accent coords as a follow-up against the unextracted SPEC row.

---

_Reviewed: 2026-06-18_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
