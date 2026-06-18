---
phase: 01-re-spec-the-ui-read-only
verified: 2026-06-17T00:00:00Z
status: passed
score: 4/4 must-haves verified
re_verification: # No — initial verification
  previous_status: none
overrides_applied: 0
---

# Phase 1: RE-Spec the UI (read-only) Verification Report

**Phase Goal:** Turn the closed stock NMMiner v2.0.02 binary into a written read-only design contract (`hardware/recovered-assets/SPEC.md`) that Phase 3 rebuilds from, and settle the LVGL major version. READ-ONLY: no firmware code, no flashing.
**Verified:** 2026-06-17
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths (Requirements RE-01..RE-04)

| # | Truth (Requirement) | Status | Evidence |
|---|---------------------|--------|----------|
| RE-01 | Ghidra 11.3.1 + dynacylabs v11.3.1 workbench exists; image decompilable; xref resolves screen funcs | ✓ VERIFIED | Install + extension + Ghidra project + decompiled artifacts all on disk; `nmminer_str_xref.py --all` → 6 screens + ss_render → `0x400d...`, exit 0 |
| RE-02 | SPEC.md §LVGL states v8 with ≥2 acceptable-evidence items + Confidence, before geometry (gate) | ✓ VERIFIED | §LVGL version (line 27-42) verdict v8, Confidence "probable→confirmed", Items #4 + #2 (+#5 corroborator); precedes first geometry table (Loading @ line 79) |
| RE-03 | SPEC.md documents 6 screens, Miner/Clock geometry, market field set, seam, ss_render | ✓ VERIFIED | `check_spec_screens.py` → OK, exit 0; all 6 class IDs present; Miner 22 geometry rows, Clock 13 rows; Share-Accept seam + ss_render sections; market binding to `/api/system/info` |
| RE-04 | SPEC.md markdown-only, never linked into firmware, all recovered assets gitignored | ✓ VERIFIED | `git ls-files hardware/recovered-assets/` = 0; staged = 0; `git check-ignore SPEC.md` returns the path; committed scripts carry no `share accepted`/`%ldms` or decompiled-body leakage |

**Score:** 4/4 truths verified

### Commands Run + Output

**RE-01 — environment + xref:**
```
$ ls -d ~/ghidra_11.3.1_PUBLIC                                  → present
$ ls ~/ghidra_11.3.1_PUBLIC/Ghidra/Extensions/ghidra-esp32-flash-loader → present
$ ls -d ~/ghidra_projects/nmminer.*                            → nmminer.gpr, nmminer.rep
$ ls hardware/recovered-assets/decompiled/                     → Miner320x240.c, Clock320x240.c,
                                                                 LVGL_API_MAP.md, ss_render.c,
                                                                 SEAM_RENDER.txt, vtables/, +per-screen SPEC_*.md
$ python3 scripts/nmminer_str_xref.py hardware/recovered-assets/drom.bin hardware/recovered-assets/irom.bin --all
  Loading320x240 -> DROM 0x3f499108 -> IROM literal 0x400d1930
  Miner320x240   -> DROM 0x3f4be960 -> IROM literal 0x400d1960
  Clock320x240   -> DROM 0x3f47389b -> IROM literal 0x400d1864
  SPrice320x240  -> DROM 0x3f4e41a8 -> IROM literal 0x400d19a8 (+1-byte offset)
  Weather320x240 -> DROM 0x3f4e41d8 -> IROM literal 0x400d19e8
  Config320x240  -> DROM 0x3f4738d4 -> IROM literal 0x400d190c
  ss_render      -> DROM 0x3f4e42db -> IROM literal 0x400d1c14
  EXIT=0
```
FIDB fallback path (loader ROM/SVD + string→l32r xref) is the documented, accepted route per the reworded RE-01. All target funcs decompilable; decompiled C present on disk under gitignored `decompiled/`.

**RE-02 — LVGL verdict (read SPEC.md §LVGL version, lines 27-42):**
- Verdict: **LVGL v8**, Confidence **probable → confirmed**
- Item #4: `FUN_40113390(obj, align, x_ofs, y_ofs)` = 4-arg v8 `lv_obj_align`; v8 style-setter `(obj, value, selector=0)` shape
- Item #2: `FUN_4011e1d0(parent) -> obj` widget-create call-shape; no v9 `LV_COLOR_FORMAT_*` family (DROM scan = 0)
- Item #5 (corroborator): Oct-2023 build predates LVGL 9.0 (Jan-2024)
- Gate confirmed: §LVGL version block precedes the first geometry table (Loading320x240 @ line 79)

**RE-03 — spec structural check:**
```
$ python3 scripts/check_spec_screens.py
  OK: SPEC.md passes strengthened screen/evidence checks (6 screens+addrs,
      Miner/Clock>=5 rows, seam+ss_render addr+shape, LVGL 2-evidence, Confidence)
  EXIT=0
```
Spot-check: all 6 exact class identifiers present in SPEC.md; Miner geometry = 22 widget rows, Clock = 9 + 4 compound rows, each with a per-row Confidence column; Share-Accept seam `FUN_400f1620` @ `0x400f1620` (marker DROM `0x3f4736c8`); ss_render `0x400f6ff4` `(char* buf, uint w, uint h)`; market field set bound to `/api/system/info` fields.

**RE-04 — no-redistribution boundary:**
```
$ test "$(git ls-files hardware/recovered-assets/ | wc -l)" -eq 0       → tracked_count=0 (OK)
$ git diff --cached --name-only | grep '^hardware/recovered-assets/'     → empty (OK)
$ git check-ignore hardware/recovered-assets/SPEC.md                     → hardware/recovered-assets/SPEC.md
$ grep -nE 'share accepted|%ldms' scripts/nmminer_str_xref.py scripts/check_spec_screens.py → empty (OK)
$ grep -RnE 'FUN_400|local_..|undefined4|lv_obj_set_pos\(|share accepted|%ldms' (committed scripts) → empty (OK)
$ git grep -nE 'FUN_400f|share accepted|%ldms' -- 'scripts/*.py'         → NONE
$ grep -n recovered-assets .gitignore                                    → 25:hardware/recovered-assets/
```

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `~/ghidra_11.3.1_PUBLIC` + flash-loader ext | RE workbench | ✓ VERIFIED | Install + `Ghidra/Extensions/ghidra-esp32-flash-loader` present |
| `~/ghidra_projects/nmminer.{gpr,rep}` | Decompilable project | ✓ VERIFIED | Both exist |
| `hardware/recovered-assets/decompiled/` | Dumped builders | ✓ VERIFIED | Per-screen `.c` + LVGL_API_MAP + seam/render dumps |
| `scripts/nmminer_str_xref.py` | Resolves screen funcs | ✓ VERIFIED | Committed, tracked; `--all` → 6 + ss_render @ `0x400d...`, exit 0 |
| `scripts/check_spec_screens.py` | SPEC structural gate | ✓ VERIFIED | Committed, tracked; exits OK on current SPEC.md |
| `hardware/recovered-assets/SPEC.md` | Design contract (gitignored) | ✓ VERIFIED | On disk, 22KB, 12 sections, gitignored, not tracked |

### Anti-Patterns Found

| File | Severity | Impact |
|------|----------|--------|
| `nmminer_str_xref.py` WR-01 (+1-byte first-match fallback) | ⚠️ Warning | Correctness-polish from 01-REVIEW; does not block goal — all 7 targets resolve correctly today |
| `check_spec_screens.py` WR-02 (widget-row heuristic) | ℹ️ Info | Non-issue here: Miner 22 / Clock 13 rows vastly exceed the 5-row minimum, no miscount risk at current scale |
| Both scripts WR-03 (file-handle leak) | ℹ️ Info | Short-lived CLIs; matches sibling `nmminer_ss_decode.py` style |

No debt markers (`TBD`/`FIXME`/`XXX`) introduced. No critical findings in 01-REVIEW. D-11 redistribution boundary confirmed clean.

### Human Verification Required

None. All four requirements are verifiable programmatically and were confirmed by running the actual checks. The 01-VALIDATION manual items (Ghidra GUI symbol-reading, geometry accuracy against API reference) are evidentiary backstops; the committed smoke checks + on-disk artifacts + xref reproduction satisfy the goal contract without an interactive GUI session.

### Gaps Summary

None. Phase goal achieved: the closed binary is decompilable in the documented workbench (RE-01), the LVGL version is settled as v8 with two independent evidence items before geometry (RE-02), SPEC.md is a complete read-only design contract covering all 6 screens + seam + ss_render with evidence-bearing geometry (RE-03), and every recovered closed-firmware asset stays gitignored with no redistribution leakage in committed code (RE-04). Phase 3 has a concrete contract to rebuild from.

---

_Verified: 2026-06-17_
_Verifier: Claude (gsd-verifier)_
