---
phase: 05-data-driven-screens-http-api-parity
plan: 04
subsystem: firmware-cyd-screensaver-swarm
tags: [screensaver, ss-decoder, lzss, rle, rgb565, carousel, swarm, lan-scan, findme, idle-state-machine]
requires:
  - "nm_settings (05-01): NM_SET_SCREENSAVER / NM_SET_SCREENSAVER_MODE + nm_settings_ss_timeout_ms token->ms"
provides:
  - "nm_screensaver: .ss C decoder (SN/LZSS W10L4/RLE/RGB565-direct) + LittleFS /ss/ carousel + black mode + FindMe flash"
  - "nm_swarm: Miner-gated /24 TCP-probe LAN scan + bounded dedup IP list + raise-once/consume-once FindMe flag"
  - "nm_uimanager: stage-2 idle->screensaver state machine, resume-on-touch, market/swarm page-gating, FindMe override"
  - "nm_uimanager_trigger_findme() C entry point for the 05-05 /api/swarm/find route"
affects:
  - "05-05 (HTTP API): /alive serves nm_swarm_get_list + nm_swarm_note_alive; /api/swarm/find calls nm_swarm_request_findme (or nm_uimanager_trigger_findme)"
  - "05-06 (loop integration): owns ino.cpp; calls nm_swarm_tick from loop if not pumped via nm_uimanager_tick"
tech-stack:
  added: []
  patterns:
    - "streaming single-pass decode (bitreader -> LZSS ring -> RLE state machine -> row blit) avoids a full framebuffer"
    - "weak extern fallback for a sibling-plan symbol (nm_market_set_page_allowed) so the fork links in isolation"
    - "env-guarded .cpp (#if ESP32_2432S028R || ESP32_2432S028_2USB), extern C headers (05-01 idiom)"
key-files:
  created:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.h
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screensaver.cpp
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_swarm.h
    - firmware/custom/NerdMiner-CYD-Custom/src/nm_swarm.cpp
  modified:
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
    - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
decisions:
  - "LZSS implemented as a 1024-byte ring window (10-bit offset reaches <=1023 bytes back) feeding the RLE sink one byte at a time — no full decoded-output buffer (T-05-SSRAM)."
  - "RGB565 pushed DIRECTLY to the native panel via the shared TFT_eSPI instance; the spec's RGB565->RGB888 step is host-only and skipped on device (RESEARCH Pattern 7)."
  - "nm_market_set_page_allowed declared as a weak no-op in nm_uimanager so this plan builds/links standalone (sibling 05-02 nm_market.cpp not yet present); the linker binds the strong definition once 05-02 lands."
  - "Swarm scan starts awake on Miner-page entry and sleeps after 30s with no /alive (D-56); probes the /24 one host/tick with a 60ms TCP connect timeout to stay inside the WDT window."
  - "FindMe drives the CYD RGB LED active-LOW (common-anode) on pins 4/16/17, blinking ~5Hz across a 1.2s white flash."
metrics:
  duration: "~18 min"
  completed: 2026-06-18
---

# Phase 5 Plan 04: Screensaver + Swarm + Idle State Machine Summary

Idle screensaver subsystem (DATA-03/D-54), Miner-gated LAN swarm scan + FindMe (DATA-04/D-56),
and the nm_uimanager extension that turns the Phase-4 dim timer into a two-stage
idle->dim->screensaver state machine — all behind one file-ownership boundary so there is no
Wave-2 conflict with the market/weather plans.

## What Was Built

- **`src/ui_nm/nm_screensaver.{h,cpp}`** — fresh C port of the `.ss` algorithm
  (SS-ALGORITHM-SPEC.md, host ref `scripts/nmminer_ss_decode.py`). `nm_ss_decode` validates the
  `"SN"` magic + width/height bounds (`w!=0`, `w<=800`, `1<=h<=600`) and the hs_params nibble
  guards (stock 0xA4 -> W=10,L=4) BEFORE any allocation (T-05-SS), then runs a streaming
  pipeline: a buffered MSB-first bit reader -> LZSS (1024-byte ring window, T-05-SSRAM) -> an RLE
  control-byte state machine -> one `width*2`-byte RGB565 row pushed DIRECTLY to the panel
  (no RGB888). Centers sub-panel images, pads a short stream with black, stops on a malformed
  back-ref (`start<0`). Carousel: `nm_ss_begin` enumerates `/ss/ss_saver_320_240_*.ss` and reads
  ScreenSaverMode (image/black); `nm_ss_tick` advances every `NM_SS_FRAME_MS` (8s); `nm_ss_stop`
  releases the state. `nm_ss_findme_start/flash` paint a full-screen white flash + blink the
  active-LOW RGB LED for 1.2s (D-56).
- **`src/nm_swarm.{h,cpp}`** — `nm_swarm_add_ip` (idempotent insert into a bounded 16-entry array,
  T-05-SWARM), `nm_swarm_get_list`/`nm_swarm_count` (serialize for 05-05 `/alive`),
  `nm_swarm_set_enabled` (Miner-page gate), `nm_swarm_note_alive` (reset the 30s sleep timer),
  `nm_swarm_tick` (one /24 host/tick TCP connect probe with a 60ms timeout, sleeps after 30s no
  /alive), and `nm_swarm_request_findme`/`nm_swarm_findme_pending` (raise-once/consume-once). No
  `lv_*` in the module.
- **`src/ui_nm/nm_uimanager.{h,cpp}`** — extended the Phase-4 idle timer. Added `s_ss_active` +
  `s_nav_in_progress` + `s_findme_active`. `nm_uimanager_tick` now: (stage 1) dims after
  `NM_UI_IDLE_MS`; polls the FindMe flag -> `nm_ss_findme_start/flash` (overrides everything,
  D-56/OQ3); (stage 2) after the runtime ScreenSaver timeout (05-01 token->ms; `never`=0 disables)
  enters `nm_ss_begin`/`nm_ss_tick`; pumps `nm_swarm_tick` when not in the carousel decode.
  `nm_input_wake_if_dimmed` exits an active screensaver (repaint page + wake) as a consumed wake.
  `nm_uimanager_goto_page_ex` guards with `s_nav_in_progress` (Pitfall 6) and gates
  `nm_market_set_page_allowed` (Miner/Clock/SPrice) + `nm_swarm_set_enabled` (Miner only).
  `nm_uimanager_trigger_findme` exposed for the 05-05 API.

## API Surface (for 05-05 + 05-06)

```c
/* nm_screensaver.h — carousel + FindMe (pumped by nm_uimanager_tick today) */
bool nm_ss_decode(const char* path);     /* validated SN/LZSS/RLE -> RGB565-direct blit */
int  nm_ss_begin(void);                  /* enumerate /ss/ + read image/black mode; returns image count */
void nm_ss_tick(uint32_t now_ms);        /* advance the carousel / hold black */
void nm_ss_stop(void);                   /* release on resume */
void nm_ss_findme_start(void);           /* begin the white flash + LED blink */
bool nm_ss_findme_flash(uint32_t now_ms);/* animate; false when finished */

/* nm_swarm.h — 05-05 /alive + /api/swarm/find */
void nm_swarm_add_ip(const char* ip);            /* idempotent dedup insert */
int  nm_swarm_get_list(char* out, size_t outlen);/* CSV peer list for /alive */
int  nm_swarm_count(void);
void nm_swarm_set_enabled(bool enabled);         /* Miner-page gate (UIManager-driven) */
void nm_swarm_note_alive(uint32_t now_ms);       /* 05-05 /alive calls this each request */
void nm_swarm_tick(uint32_t now_ms);             /* loop pump (off LVGL) */
void nm_swarm_request_findme(void);              /* 05-05 /api/swarm/find calls this */
bool nm_swarm_findme_pending(void);              /* consumed once by the UIManager */

/* nm_uimanager.h — state machine entry */
void nm_uimanager_trigger_findme(void);  /* 05-05 alt route -> raises the swarm flag */
```

### Wiring notes for 05-05 / 05-06

- `/alive` handler: call `nm_swarm_note_alive(millis())` then serialize self + `nm_swarm_get_list()`.
- `/api/swarm/find`: call `nm_swarm_request_findme()` (or `nm_uimanager_trigger_findme()`); the
  UIManager tick consumes the flag next pump and runs the flash.
- The swarm + carousel + FindMe are all pumped from `nm_uimanager_tick()` today. 05-06 owns ino.cpp
  and must call `nm_uimanager_tick()` from a non-mining loop (it already does for the UI). No extra
  loop wiring is required for this plan; `nm_swarm_tick` is also exported if 05-06 prefers a direct
  loop pump.

## Per-Task Status

| Task | Name                                                  | Status | Commit(s)          | Files                                   |
|------|-------------------------------------------------------|--------|--------------------|-----------------------------------------|
| 1    | .ss decoder + carousel (nm_screensaver)               | done   | 953468c            | nm_screensaver.{h,cpp}                  |
| 2    | Swarm LAN scan + dedup list + FindMe flag (nm_swarm)  | done   | d18fce4            | nm_swarm.{h,cpp}                        |
| 3    | Wire idle->screensaver state machine (nm_uimanager)   | done   | 3810d9b, e3d53f4   | nm_screensaver.cpp (fix), nm_uimanager.{h,cpp} |

Commits are in the nested fork repo (`firmware/custom/NerdMiner-CYD-Custom`, branch `main`).
Gitlink bump in the parent is the orchestrator's responsibility.

## Verification

- `pio run -e ESP32-2432S028R` — **SUCCESS** (64s). RAM 36.6% (119976 B), Flash 89.5% (2816373 B).
- Task 1 gate: `grep LittleFS && grep -E '0x53|0x4E|"SN"'` on nm_screensaver.cpp — **SS_OK**.
- Task 2 gate: `grep -E 'nm_swarm_add_ip|nm_swarm_request_findme'` on nm_swarm.cpp — **SWARM_OK**;
  zero `lv_*` calls (only a comment mentions it).
- Task 3 gate: build SUCCESS + `grep s_ss_active` + `grep nm_market_set_page_allowed` — **UIM_OK**.
- key_links: uimanager pattern matches 22x; screensaver `LittleFS|/ss/` matches 10x;
  `s_ss_active` present 7x; `nm_market_set_page_allowed` present 2x.

## RGB565-direct + memory-bounded confirmation

- `.ss` rows are pushed via `tft.pushColors(s->row, s->col, true)` straight from the RGB565 row
  buffer — the spec's RGB565->RGB888 bit-replication is NOT performed on device (RESEARCH Pattern 7).
- Decode RAM is bounded: a single 320-pixel (`640`-byte) row buffer + a static 1024-byte LZSS ring
  window; output is capped at `width*height` pixels. No full-image buffer is ever allocated
  (T-05-SSRAM). Header bounds are validated before the first allocation (T-05-SS).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] `fs::File` namespace qualification in nm_screensaver.cpp**
- **Found during:** Task 3 build.
- **Issue:** `File` is declared in the `fs::` namespace and is not pulled into global scope by the
  include chain here, so `File f = LittleFS.open(...)` failed to compile.
- **Fix:** Added `#include <FS.h>` + `using fs::File;`.
- **Files modified:** src/ui_nm/nm_screensaver.cpp
- **Commit:** 3810d9b

**2. [Rule 3 - Blocking] Weak fallback for the sibling-owned `nm_market_set_page_allowed`**
- **Found during:** Task 3 (planning the page-gating call).
- **Issue:** The plan requires calling `nm_market_set_page_allowed` (owned by sibling plan 05-02,
  nm_market). `nm_market.h` exists in the tree but `nm_market.cpp` is not yet present/linked, so a
  direct call would fail to link when this plan is built in isolation — yet Task 3's gate requires a
  build SUCCESS. Package-install exclusion does not apply (no package).
- **Fix:** Declared `extern "C" __attribute__((weak)) void nm_market_set_page_allowed(bool){...}` as
  a no-op in nm_uimanager.cpp. The linker binds the strong definition from 05-02's nm_market.cpp once
  it lands, and falls back to this stub otherwise. Behavior is correct in both orderings.
- **Files modified:** src/ui_nm/nm_uimanager.cpp
- **Commit:** e3d53f4

### TDD gate note (consistent with 05-01)

Tasks 1-2 are marked `tdd="true"`, but the fork has no native unit-test harness (documented in
05-01; D-58 minimal-code keeps a native harness out of scope). The behavior functions are pure ports
of the validated host reference (`nmminer_ss_decode.py`) and the documented swarm contract; their
correctness is gated by the ESP32 build + the structural grep gates rather than a RED/GREEN native
cycle. No `test(...)`/`feat(...)` native gate commits were produced for Tasks 1-2 for this reason.

## TDD Gate Compliance

Plan type is `execute` (not plan-level `tdd`); Tasks 1-2 carry `tdd="true"`. Per the 05-01-established
constraint, no native test harness exists in this fork, so the per-task RED/GREEN native gate cannot
run. Behavior is verified by the ESP32 build + grep gates. This is a known, plan-anticipated
limitation, not a skipped gate.

## Self-Check: PASSED

- All 6 owned files exist on disk (line counts: screensaver.h 88, screensaver.cpp 362, swarm.h 75,
  swarm.cpp 139, uimanager.h 76, uimanager.cpp 329; min_lines satisfied: screensaver.cpp>=120,
  swarm.cpp>=50).
- All 4 commits exist in the nested fork repo: 953468c, d18fce4, 3810d9b, e3d53f4.
- Build green; all verification + key_link patterns match.
