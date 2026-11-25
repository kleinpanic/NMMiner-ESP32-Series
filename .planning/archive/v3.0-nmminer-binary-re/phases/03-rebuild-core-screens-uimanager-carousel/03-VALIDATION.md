---
phase: 03-rebuild-core-screens-uimanager-carousel
plan: 06
kind: validation
nyquist_compliant: true
build_gate: PASS
build_command: "cd firmware/custom/NerdMiner-CYD-Custom && pio run -e ESP32-2432S028R"
build_result: SUCCESS
ram_used_pct: 35.3
flash_used_pct: 87.3
fork_commit: a12b1ee
date: 2026-06-18
---

# Phase 3 Validation — NMMiner UI Rebuild (CYD)

This is the Phase 3 acceptance gate. On-device flash + visual parity are Phase 7
(out of scope here). This validates that the recovered NMMiner look compiles and
LINKS into a real CYD firmware image — the firmware that fixes the user's black
display (D-35 backlight ramp).

## Gate A — Build SUCCESS (the phase gate)

```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R
```

Result (build log `/tmp/nm_build_5.log`):

```
RAM:   [====      ]  35.3% (used 115744 bytes from 327680 bytes)
Flash: [========= ]  87.3% (used 2746781 bytes from 3145728 bytes)
========================= [SUCCESS] Took 39.28 seconds =========================
ESP32-2432S028R  SUCCESS
```

Artifacts produced:
- `.pio/build/ESP32-2432S028R/firmware.bin` (2753072 bytes)
- `.pio/build/ESP32-2432S028R/firmware.elf`
- `ESP32-2432S028R_factory.bin` (2822144 bytes)

All 11 `src/ui_nm/*` modules (nm_display, nm_uimanager, 6 nm_screen_*) + the
open-upstream `recovered/fonts/fa_nmminer_subset.c` compile and link. **PASS.**

## Gate B — Per-screen widget presence (grep facts)

Greps run against `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/`:

| Screen | Required widget / string | Check | Result |
|--------|--------------------------|-------|--------|
| Loading | `lv_bar_create` progress bar | `grep -c lv_bar_create nm_screen_loading.cpp` | 1 — PASS |
| Loading | "Make it better" tagline | `grep -c 'Make it better'` | 2 — PASS |
| Loading | Montserrat font usage | `grep -c lv_font_montserrat` | 7 — PASS |
| Miner | FA icon symbols (`NMMINER_SYMBOL_*`) | `grep -c NMMINER_SYMBOL_` | 7 — PASS |
| Miner | static "H/s" unit tag | `grep -c '"H/s"'` | 3 — PASS |
| Clock | price up/down colors (`PRICE_UP/DOWN`) | `grep -c 'PRICE_UP\|PRICE_DOWN'` | 2 — PASS |
| Clock | NTP time via `getLocalTime` | `grep -c getLocalTime` | 4 — PASS |
| Config | `Config320x240` descriptor name | `grep -c Config320x240` | 3 — PASS |
| Config | NO captive-portal SSID widget (`nmap-2.4g`) | `grep -ci 'nmap-2.4g\|nmap_2'` | 0 — PASS |
| SPrice | class-name stub title "SPrice" | `grep -c '"SPrice"'` | 1 — PASS (honest stub) |
| Weather | class-name stub title "Weather" | `grep -c '"Weather"'` | 1 — PASS (honest stub) |

## Gate C — 6-page list completeness (UI-02 structural identity)

`nm_ui.h` enum defines all 6 stock pages + count:
`NM_PAGE_LOADING, NM_PAGE_MINER, NM_PAGE_CLOCK, NM_PAGE_SPRICE, NM_PAGE_WEATHER,
NM_PAGE_CONFIG, NM_PAGE_COUNT` (== 6).

`nm_uimanager.cpp` page registry references all 6 descriptors; all 6
`const nm_screen_t nm_screen_*` definitions exist (loading, miner, clock,
sprice, weather, config). **PASS.**

## Gate D — Identity strings preserved (DATA-06 / T-03-15)

`nm_screen_loading.cpp`: `NMMiner` (4 hits), `Make it better` (2 hits) — stock
identity intact. `kleinpanic` branding (3 hits) layered additively (D-37).
Controllers key off `model="NMMiner"`; the string survives the branding pass.
**PASS.**

## Gate E — Clock price color resolved (D-36)

`recovered/nmminer_palette.h` defines `NMMINER_COLOR_PRICE_UP_HEX 0x84DB31`
(green) and `NMMINER_COLOR_PRICE_DOWN_HEX 0xFF0000` (red). The spurious magenta
`0xF700EF` / `0xf01d` appears ONLY in the evidence comment documenting its
removal — no active color use (grep of all `src/ui_nm/*` source: 0 hits).
**PASS.**

## Gate F — Threat-model dispositions (T-03-14 / T-03-SC)

- T-03-14 (info disclosure): the build links only open-upstream fonts
  (`fa_nmminer_subset.c` from Font Awesome OFL + LVGL built-in Montserrat) and
  numeric geometry/palette `#define`s. No closed-firmware bytes; SPEC/decompiled
  assets stay gitignored. **mitigated.**
- T-03-SC (supply chain): `lvgl@^8.4.0` pinned in lib_deps, already vetted in
  plan 01; no new package introduced at integration. **mitigated.**

## Deferred to Phase 7 (out of this gate)
- On-device flash to the ESP32-2432S028R.
- Pixel-parity visual compare against decoded .ss screenshots (D-23).
- SPrice/Weather live data + full SPrice geometry (Phase 5, D-33).
- Network/pool diff wiring on the Miner screen (Phase 5).

## Nyquist task map
- Task 1 (branding finalize, D-37) -> Gate D.
- Task 2 (build gate + validation) -> Gates A/B/C/E/F.

`nyquist_compliant: true` — every task maps to a recorded check above.
