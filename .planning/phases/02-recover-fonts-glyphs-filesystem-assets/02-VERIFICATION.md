---
phase: 02-recover-fonts-glyphs-filesystem-assets
verified: 2026-06-17T00:00:00Z
status: passed
score: 11/11 must-haves verified
overrides_applied: 0
deferred:
  - truth: "Clock BTC price-down red color is exact-resolved"
    addressed_in: "Phase 3"
    evidence: "Phase 3 SC #4: 'Clock screen renders ... BTC price with green-up / red-down coloring'. nmminer_palette.h marks NMMINER_COLOR_PRICE_DOWN as `tentative — confirm in Phase 3 (FUN_400f24ac disasm)`. Phase-2 ASSET-01 only requires the palette captured, which it is."
---

# Phase 2: Recover Fonts/Glyphs + Filesystem Assets Verification Report

**Phase Goal:** Recover reusable visual assets (fonts, palette, LittleFS contents, byte-accurate `.ss` decoder) cleanly and ready to feed the Phase 3 rebuild.
**Verified:** 2026-06-17
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Stock fonts/glyphs extracted as reusable LVGL C-arrays + palette (ASSET-01) | ✓ VERIFIED | `fa_nmminer_subset.c` (158 lines, real `lv_font_t` with glyph_bitmap/glyph_dsc/cmaps), `recovered_fonts.h`, `nmminer_palette.h` (24 `NMMINER_COLOR_*`) all on disk + committed in fork `fcd6fee` |
| 2 | lv_conf.h enables resolved Montserrat sizes 12/14/16/22/24/26 | ✓ VERIFIED | `lv_conf.h:359-366` — all six `LV_FONT_MONTSERRAT_NN 1` present |
| 3 | D-21 boundary: fonts open-upstream, NOT byte-copied closed glyphs | ✓ VERIFIED | FONT-IDENTIFICATION.md traces Montserrat→OFL built-in + FA→FA6 Free Solid 6.5.1 (SHA-256 `fbbf06d7...`, lv_font_conv@1.5.3). fa_nmminer_subset.c carries clean provenance comment, no `/home/` path leak. Descriptor-referencing FONT-IDENTIFICATION.md is gitignored |
| 4 | LittleFS partition documented with littlefs tooling, NOT mkspiffs (ASSET-02) | ✓ VERIFIED | LITTLEFS-MANIFEST.md `## NOT mkspiffs` section + mklittlefs/littlefs-python tooling table |
| 5 | 17 `.ss` + web UI present and intact | ✓ VERIFIED | 17 `ss_saver_320_240_*.ss` on disk; index.html(.gz) + apidoc.html(.gz) present |
| 6 | `.ss` decoder byte-accurate, all 17 decode cleanly (ASSET-03) | ✓ VERIFIED | `pytest tests/test_ss_decode.py -q` → 4 passed; all-17 test asserts exactly 17 fixtures + (320,240) each |
| 7 | RGB565→RGB888 bit-replication fix (0xF800 → R=255, not 248) | ✓ VERIFIED | `rgb565_to_rgb888` uses `(r5<<3)|(r5>>2)` etc.; test asserts `0xF800[0]==255`, `0x0800→8` |
| 8 | on-device ss_render captured as C-port spec for Phase 5 | ✓ VERIFIED | SS-ALGORITHM-SPEC.md references `FUN_400f6ff4` + bit-replication pipeline |
| 9 | FA subset covers U+F1EB/F019/F57D/F080/F382 | ✓ VERIFIED | cmaps `list_length=5`; UTF-8 symbol macros byte-correct (U+F1EB→EF 87 AB, U+F382→EF 8E 82) |
| 10 | Palette captured as bit-replicated #hex | ✓ VERIFIED | `nmminer_palette.h` orange `0xebe6`, price-up `0x86c6`, corrected #hex annotations |
| 11 | Fork carries the recovered fonts (commit on disk) | ✓ VERIFIED | fork `git log -1` = `fcd6fee`; `--stat` shows 4 files +310 (fa_nmminer_subset.c, recovered_fonts.h, nmminer_palette.h, lv_conf.h) |

**Score:** 11/11 truths verified

### Deferred Items

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | Clock BTC price-down red exact value | Phase 3 | Phase 3 SC #4 (red-down coloring); palette marks `PRICE_DOWN` tentative with `FUN_400f24ac disasm` resolution path. ASSET-01 only requires palette captured — satisfied |

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `firmware/custom/NerdMiner-CYD-Custom/recovered/fonts/fa_nmminer_subset.c` | LVGL v8 FA subset C-array | ✓ VERIFIED | 158 lines, real lv_font_t (glyph_bitmap/glyph_dsc/cmaps), committed fcd6fee |
| `.../recovered/fonts/recovered_fonts.h` | FA extern + symbol macros + Montserrat map | ✓ VERIFIED | `extern const lv_font_t fa_nmminer_subset;` + 5 NMMINER_SYMBOL_* macros |
| `.../recovered/nmminer_palette.h` | RGB565 + bit-replicated #hex | ✓ VERIFIED | 24 NMMINER_COLOR_* constants |
| `.../lib/hansolov2/lv_conf.h` | Montserrat 12/14/16/22/24/26 enabled | ✓ VERIFIED | lines 359-366 all =1 |
| `hardware/recovered-assets/LITTLEFS-MANIFEST.md` | littlefs tooling + NOT-mkspiffs + intact | ✓ VERIFIED | NOT-mkspiffs section + tooling + 17 .ss listed |
| `hardware/recovered-assets/SS-ALGORITHM-SPEC.md` | C-port spec FUN_400f6ff4 | ✓ VERIFIED | references FUN_400f6ff4 + pipeline |
| `tests/test_ss_decode.py` | all-17 + bit-replication regression | ✓ VERIFIED | 4 tests pass, loud-fail on != 17 |
| `scripts/nmminer_ss_decode.py` | rgb565_to_rgb888 bit-replication | ✓ VERIFIED | function present, decode_ss delegates (line 95) |
| `hardware/recovered-assets/FONT-IDENTIFICATION.md` | descriptor→family map (gitignored) | ✓ VERIFIED | exists, `git check-ignore` confirms ignored, not tracked in fork |

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|----|--------|---------|
| test_ss_decode.py | nmminer_ss_decode.py | import decode_ss/rgb565_to_rgb888 | ✓ WIRED | line 16 `from nmminer_ss_decode import decode_ss, rgb565_to_rgb888` |
| test_ss_decode.py | littlefs/*.ss | glob 17 fixtures | ✓ WIRED | SS_GLOB asserts 17 + decode each |
| recovered_fonts.h | fa_nmminer_subset.c | extern lv_font_t | ✓ WIRED | line 33 extern decl |
| lv_conf.h | LVGL built-in montserrat | LV_FONT_MONTSERRAT_NN enable | ✓ WIRED | 6 sizes enabled |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|----------|---------|--------|--------|
| All 17 .ss decode to 320x240 + bit-replication | `.venv/bin/python -m pytest tests/test_ss_decode.py -q` | 4 passed (exit 0) | ✓ PASS |
| RGB565 max-red → 255 | test_rgb565_bit_replication_not_bare_shift | pass | ✓ PASS |
| UTF-8 FA macro bytes | `python3` chr().encode | U+F1EB→EF 87 AB, U+F382→EF 8E 82 match macros | ✓ PASS |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| ASSET-01 | 02-02 | Fonts/glyphs as LVGL C-arrays + palette | ✓ SATISFIED | Truths 1-3,9-11 |
| ASSET-02 | 02-01 | LittleFS unpacked (NOT mkspiffs) + 17 .ss + web UI | ✓ SATISFIED | Truths 4-5 |
| ASSET-03 | 02-01 | Byte-accurate .ss decoder all 17 + reusable | ✓ SATISFIED | Truths 6-8 |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| (none) | - | - | - | No TBD/FIXME/XXX/TODO/placeholder in any phase-modified file |

The `tentative` annotation on `NMMINER_COLOR_PRICE_DOWN` is a documented deferral with a formal Phase-3 resolution path (FUN_400f24ac disasm), not an unreferenced debt marker — not a blocker.

### Human Verification Required

None. All criteria verified programmatically (file checks + test execution + commit log). Pixel-level font parity (D-23 "visually indistinguishable") is explicitly deferred to Phase-3 pixel compare per plan, not a Phase-2 gate.

### Gaps Summary

No gaps. All 11 must-haves verified against the codebase: fonts/palette committed to the fork (fcd6fee) from open upstreams with the D-21 no-redistribution boundary intact (descriptor-referencing analysis gitignored), LittleFS documented with correct tooling and 17 intact screensavers + web UI, and the `.ss` decoder regression (4 tests) green with the bit-replication fix confirmed (0xF800 → R=255). The single tentative item (Clock price-down red) is correctly deferred to Phase 3 and does not block Phase 2.

---

_Verified: 2026-06-17_
_Verifier: Claude (gsd-verifier)_
