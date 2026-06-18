# 01-03 SUMMARY — Decompile 6 screens + write SPEC.md (RE-03, RE-04)

**Status:** complete
**Requirements:** RE-03, RE-04

## Key RE finding (premise correction)
The functions located via the class-name string xref (`Clock320x240`@`0x400d1864`, `Miner320x240`@`0x400d1960`, …) are **class-name accessor stubs** (`return PTR_s_<Name>`), NOT the screen builders. The real layout code is reached through each screen's **vtable** (in DROM): the get-name stub is one slot; the large `vt[±]` slots are the builders. Recorded so Phase 3 builds from the right functions.

## Screen builders located + decompiled (gitignored `recovered-assets/decompiled/`)
| Screen | builder | result |
|--------|---------|--------|
| Miner320x240 | FUN_400f2f5c | 23 widgets; stat slots mapped (hashrate/shares/net+pool+best diff/temp/RSSI/uptime); RGB565 colors decoded; FontAwesome glyphs ID'd |
| Clock320x240 | FUN_400f1f34 | 9 labels + WiFi compound + 500ms timer; "00:00"/"AM" seeds; green/red price colors applied in FUN_400f24ac (runtime) |
| Loading320x240 | FUN_400f2c5c (+FUN_400f4830) | 6 widgets + "Make it better"/"v2.0.02"; "NMMiner" title in sub-builder |
| SPrice320x240 | FUN_400f3570 | 6 widgets + chart surface; dual kline / pricewall modes |
| Config320x240 | FUN_400f2b58 | 2 labels; `nmap-2.4g`/`192.168.4.1` confirmed HTTP-layer, NOT on display |
| Weather320x240 | FUN_400f3a34→FUN_401f2fc0 | dynamically built (accessor-class); static frame partial; cb3 HH:MM row decoded |

Plus: **share-accept seam** `FUN_400f1620` (DROM `0x3f4736c8`, lit `0x400d1800`) — Phase 6 speaker hook; **ss_render** `0x400f6ff4` `(char* buf, uint w, uint h)` — Phase 5 screensaver.

## Method (fan-out)
One Ghidra extraction pass per concern (vtable trace, Clock force-resolve, missing-builder dumps), then **7 parallel agents** (one per screen) parsed the decompiled C into per-screen SPEC sections using the shared `LVGL_API_MAP.md` — text parsing, no Ghidra-lock contention. Sections merged into the unified `SPEC.md`.

## Deliverable + verification
- `hardware/recovered-assets/SPEC.md` (gitignored) — 12 sections, 55 widget rows, RE-02 verdict, API map, 6 screens + seam + ss_render + open-items.
- **RE-03 smoke:** `scripts/check_spec_screens.py` → **OK** (6 screens+addrs, Miner/Clock ≥5 rows, seam+ss_render addr+call-shape, LVGL 2-evidence, Confidence per table).
- **RE-04:** `git ls-files hardware/recovered-assets/` = 0; staged = 0; `SPEC.md` gitignored; committed scripts carry no `share accepted`/`%ldms` literal (grep guard passes).

## Committed artifacts (identifier-only, D-11)
`scripts/nmminer_str_xref.py`, `scripts/check_spec_screens.py`. All decompiled C, SPEC.md, .bin/.elf, Ghidra project stay gitignored / outside repo.

## Open items → later phases
Font human-names/sizes (Phase 2 descriptor extraction; addresses listed in SPEC); Clock exact green/red (`FUN_400f24ac` dumped, values tentative); Weather lazy-create accessors; SPrice chart internals.
