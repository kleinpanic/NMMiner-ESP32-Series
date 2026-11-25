---
phase: 03-rebuild-core-screens-uimanager-carousel
audited: 2026-06-18T00:00:00Z
asvs_level: 1
block_on: high
status: secured
threats_total: 15
threats_closed: 15
threats_open: 0
unregistered_flags: 0
---

# Phase 3 Security Audit — Rebuild Core Screens (LVGL UI on CYD fork)

**Verdict:** SECURED. 15/15 threats CLOSED. No open BLOCKERs. No unregistered flags.

Attack surface is small and local: on-device LVGL UI rendering local mining/time
data. No new network endpoints, no credentials, no untrusted external input. The
only new surface that appeared during implementation was a dual-SPI-writer
concurrency race + active-screen teardown (caught by 03-REVIEW.md as CR-01/CR-02);
both are now fixed in the shipped code, mapping back to the T-03-02/T-03-04 DoS
dispositions. Verification below is against the **current source**, not the review's
findings — the review flagged CR-01/CR-02 as open; the code now closes them.

## Threat Verification

| Threat ID | Category | Disposition | Verdict | Evidence |
|-----------|----------|-------------|---------|----------|
| T-03-01 | Info disclosure | mitigate | CLOSED | `recovered/nmminer_palette.h` = numeric RGB565/RGB888 `#define`s only; `FUN_400f24ac`-style refs are provenance comments (IROM addresses = facts), not decompiled bodies. No closed-firmware bytes. |
| T-03-02 | Denial of service | accept | CLOSED | Build links SUCCESS at RAM 35.3% / Flash 87.3% (03-VALIDATION.md). OOM surfaces as build/boot failure; integration gate caught it. LV_MEM_SIZE bounded (lv_conf.h). |
| T-03-SC | Tampering (supply chain) | mitigate | CLOSED | `lvgl/lvgl@^8.4.0` at platformio.ini:1026 (CYD env) — identical pin to wt32-sc01 (:147) and wt32-sc01-plus (:184). Vetted upstream; no new/[ASSUMED] package. |
| T-03-03 | Tampering (NVS LastPage) | mitigate | CLOSED | Stored index clamped to `[0, NM_PAGE_COUNT)` before `pages[]` deref: nm_uimanager.cpp:61-63 (goto path) and :127-129 (boot read). |
| T-03-04 | Denial of service (UI pump) | mitigate | CLOSED | `nm_uimanager_tick()` non-blocking — `lv_timer_handler` pump + label update only, no delay()/busy-wait in page path: nm_uimanager.cpp:138-149. |
| T-03-05 | Elevation of privilege | accept | CLOSED | Grep for `WiFiClient`/`HTTPClient`/`.connect(`/`esp_http` across `src/ui_nm/` returns empty. No new network surface or credentials. |
| T-03-06 | Info disclosure (geometry) | mitigate | CLOSED | Screen modules carry only coordinates/font names/`NMMINER_COLOR_*_HEX` constants; SPEC stays gitignored (RE-04). |
| T-03-07 | Tampering (label text) | accept | CLOSED | Device-local text rendered via `lv_label_set_text`; no sink, no injection path. |
| T-03-08 | Info disclosure (Miner geometry) | mitigate | CLOSED | Same as T-03-06; Miner module is geometry facts only. |
| T-03-09 | Tampering (format string) | mitigate | CLOSED | Both `lv_label_set_text_fmt` calls (nm_screen_miner.cpp:161,166) use fixed literal format strings (`"%sC"`, `"%lddBm"`) with matching arg types. No variable-as-format sink found. |
| T-03-10 | Denial of service (per-tick update) | accept | CLOSED | `update()` is O(slots) label-set only; no alloc/network. Bounded. |
| T-03-11 | Info disclosure (Clock geometry) | mitigate | CLOSED | Coordinates/font/color facts only; SPEC gitignored. |
| T-03-12 | Tampering (NTP source) | accept | CLOSED | Clock reads system clock read-only via `getLocalTime()`/`strftime` (nm_screen_clock.cpp:14-15,119-133); NO new NTP client created. Reuses fork's existing setup. |
| T-03-13 | Spoofing (stub screens) | accept | CLOSED | Weather/SPrice render static placeholders only; no input. |
| T-03-14 | Info disclosure (final binary) | mitigate | CLOSED | Only open-upstream fonts + numeric geometry linked. `fa_nmminer_subset.c` = regenerated FA6-Free-Solid (SIL OFL, SHA-256 documented); Montserrat = LVGL built-in OFL. Build SUCCESS confirms link. SPEC/decompiled gitignored. |
| T-03-15 | Tampering (identity strings) | mitigate | CLOSED | `"NMMiner"` (nm_screen_loading.cpp:103) and `"Make it better"` (:94) present in committed source — survive branding pass (DATA-06). |

## Concurrency / Memory-Safety Fixes (new surface from implementation)

| ID | Risk | Verdict | Evidence |
|----|------|---------|----------|
| CR-01 | Dual-SPI-writer race (legacy Monitor renderer vs LVGL flush on shared `tft`) | FIXED | `drawCurrentScreen`/`animateCurrentScreen` env-gated `return;` no-op for CYD env: display.cpp:114-119, 129-135. Legacy renderer off; LVGL owns the panel (D-32). |
| CR-02 | `lv_obj_del` of active screen inside `lv_timer_handler` | FIXED | `goto_page_ex` builds + `lv_scr_load`s the NEW screen first, THEN destroys the old: nm_uimanager.cpp:65-87. Active-screen pointer never dangling mid-handler. |
| WR-04 | `LV_ASSERT_HANDLER while(1)` silent watchdog-starved brick | FIXED | `LV_ASSERT_HANDLER esp_restart();` (lv_conf.h:139-140). Fail-safe reset, not infinite loop. |
| WR-01 | NVS write every 6s (flash wear) | FIXED | auto-advance path passes `persist=false`; write gated on value-changed + `s_prefs_ready`: nm_uimanager.cpp:91-94, 112. |
| WR-02 | `putUInt` on unopened NVS handle | FIXED | `s_prefs_ready = s_prefs.begin(...)`; persist gated on it: nm_uimanager.cpp:124, 91. |
| WR-03 | Build-NULL corrupts destroy bookkeeping | FIXED | `s_have_page`/`s_current` only set when `new_obj != NULL`: nm_uimanager.cpp:77-87. |

WR-05 (unbounded `lv_timer_handler` work per pass) and IN-02 (`getMiningData` cross-task
String read) remain as the code review's qualified non-blockers — label-only screens,
short windows, and out of v1 scope per 03-REVIEW.md. Neither is a declared Phase 3
threat-model item; both are documented accepted residual risk here, not BLOCKERs at ASVS L1.

## Accepted Risks Log

- **T-03-02 / T-03-10 (DoS):** OOM / per-tick load accepted — bounded LV_MEM_SIZE,
  partial buffers, build gate catches OOM, asserts now reset (not brick) per WR-04.
- **T-03-05 / T-03-12 / T-03-13 (EoP / NTP / stubs):** no new network surface or
  credentials this phase; price/weather feeds are Phase 5.
- **T-03-07 (label text):** device-local render, no injection sink.
- **WR-05 / IN-02 (residual concurrency):** qualified non-blockers, v1 scope, see 03-REVIEW.md.

## Unregistered Flags

None. No SUMMARY carries a `## Threat Flags` section; the implementation-introduced
concurrency surface (CR-01/CR-02) maps to the T-03-02/T-03-04 DoS dispositions and is fixed.

## Commands Run

```
# T-03-01/06/08/11/14 — closed-firmware byte/decompile marker scan
grep -rniE "decompil|ghidra|sub_[0-9a-f]{4}|loc_[0-9a-f]{4}|FUN_[0-9a-f]{6}|undefined[0-9]|__fastcall" src/ui_nm/ recovered/
#   -> only FUN_* provenance comments (IROM addr facts), no decompiled bodies

# T-03-SC — lvgl pin parity across envs
grep -n "lvgl/lvgl" platformio.ini        # :147 :184 :1026 all @^8.4.0

# CR-01 — legacy renderer env-gated no-op
Read src/drivers/displays/display.cpp:112-136

# CR-02/WR-01/02/03 + T-03-03/04 — goto_page load-then-destroy, clamp, gating
Read src/ui_nm/nm_uimanager.cpp (full)

# WR-04 — assert handler
grep -n "LV_ASSERT_HANDLER" src/ui_nm/lv_conf.h   # esp_restart();

# T-03-09 — format strings
grep -rn "lv_label_set_text_fmt" src/ui_nm/        # both literal fmt

# T-03-05/12 — no new network surface
grep -rniE "WiFiClient|HTTPClient|\.connect\(|esp_http" src/ui_nm/   # empty

# T-03-15 — identity strings
grep -rni "NMMiner|Make it better" src/ui_nm/      # both present
```

_Audited: 2026-06-18 — gsd-security-auditor — ASVS L1, block on high._
