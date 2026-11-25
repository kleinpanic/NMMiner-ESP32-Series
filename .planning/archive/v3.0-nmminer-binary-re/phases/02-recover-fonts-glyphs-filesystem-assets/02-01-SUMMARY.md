---
phase: 02-recover-fonts-glyphs-filesystem-assets
plan: 01
subsystem: assets/screensaver-decoder
tags: [littlefs, screensaver, ss-decode, rgb565, regression-test]
requires:
  - "Phase 1 RE foundation: littlefs already unpacked + ss_render decompilation"
provides:
  - "Byte-accurate host .ss decoder with bit-replication RGB565->RGB888"
  - "Committable all-17 regression test (tests/test_ss_decode.py)"
  - "LittleFS manifest (tooling + intact confirmation) [gitignored]"
  - "ss_render C-port algorithm spec for Phase 5 [gitignored]"
affects:
  - "Phase 5 firmware screensaver carousel (consumes SS-ALGORITHM-SPEC.md)"
tech-stack:
  added: []
  patterns:
    - "Pure module-level converter (rgb565_to_rgb888) extracted for unit testing"
    - "Loud-fail fixture assertion (test fails on != 17 fixtures, threat T-02-03)"
key-files:
  created:
    - tests/test_ss_decode.py
    - hardware/recovered-assets/LITTLEFS-MANIFEST.md
    - hardware/recovered-assets/SS-ALGORITHM-SPEC.md
  modified:
    - scripts/nmminer_ss_decode.py
decisions:
  - "D-26c implemented as bit-replication (r5<<3|r5>>2 etc.), not (v*255)//N scaling"
  - "Firmware C .ss decoder deferred to Phase 5; this plan only specs the algorithm"
  - "MANIFEST + SPEC stay gitignored under recovered-assets (D-21/D-25 boundary)"
metrics:
  duration: "~2m"
  completed: "2026-06-18"
  tasks: 2
  files: 4
requirements: [ASSET-02, ASSET-03]
---

# Phase 2 Plan 01: Recover Filesystem + Screensaver Assets Summary

Fixed RGB565->RGB888 to explicit bit-replication, proved all 17 stock `.ss`
screensavers decode to 320x240 via a committable pytest regression, and captured
the LittleFS tooling/intact-state plus the on-device `ss_render` algorithm as a
C-port spec for Phase 5.

## What was built

- **`scripts/nmminer_ss_decode.py`** — extracted the inline per-pixel conversion
  into a pure module-level `rgb565_to_rgb888(v) -> (r,g,b)` using bit-replication
  (`r5<<3 | r5>>2`, `g6<<2 | g6>>4`, `b5<<3 | b5>>2`). `decode_ss` now delegates to
  it. `hs_decode` / `rle_decode` untouched (already byte-accurate).
- **`tests/test_ss_decode.py`** (committable) — 4 tests: RGB endpoints, bit-
  replication (`0xF800` red == 255, not 248), mid-value (`1 -> 8`, not 32), and
  all-17 decode to 320x240. The all-17 test asserts exactly 17 fixtures and fails
  loudly on 0 (threat T-02-03).
- **`hardware/recovered-assets/LITTLEFS-MANIFEST.md`** (gitignored) — partition
  facts (`spiffs` label is LittleFS, magic at byte 8, block 4096 / page 256 /
  size 0x10000), explicit NOT-mkspiffs warning, mklittlefs / littlefs-python
  tooling, full content listing (web UI + 17 `.ss` with sizes), and 17/17 SN-magic
  intact confirmation.
- **`hardware/recovered-assets/SS-ALGORITHM-SPEC.md`** (gitignored) — full SN
  header / LZSS (10/4) / RLE (0x80) / RGB565 bit-replication pipeline transcribed
  from `hs_decode` + `rle_decode`, plus the `ss_render` = `FUN_400f6ff4` @
  `0x400f6ff4` render hook for Phase 5 (signature noted as inferred per SPEC.md).

## Tasks

| Task | Name | Status | Commit | Verify |
|------|------|--------|--------|--------|
| 1 (RED) | Failing regression test | done | `38bc1cc` | pytest fails on missing `rgb565_to_rgb888` (expected) |
| 1 (GREEN) | bit-replication + tests pass | done | `9274d82` | `pytest -x -q` -> 4 passed |
| 2 | LittleFS manifest + ss_render spec | done | (docs gitignored, not committed) | verify block -> `OK` |

## Verification results

- `.venv/bin/python -m pytest tests/test_ss_decode.py -x -q` -> **4 passed**
  (maps to V-ASSET-03).
- Task 2 verify block (`test -f` both docs + `grep` NOT-mkspiffs + `FUN_400f6ff4`
  + bit-replication) -> **OK** (maps to V-ASSET-02, V-ASSET-03-spec).
- 17/17 `.ss` confirmed carrying `SN` magic; all decode to 320x240.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Restored test runtime (broken/relocated `.venv`)**
- **Found during:** pre-flight environment check
- **Issue:** The project was moved from `codeWS/Devices/...` to
  `codeWS/Projects/Devices/...`; the `.venv` was created at the old path so its
  `pip` shebang was stale AND `Pillow` was absent — `import PIL` failed, so the
  plan's verify command (`.venv/bin/python -m pytest`) could not run.
- **Fix:** Installed `Pillow` into the existing venv via
  `.venv/bin/python -m pip install Pillow pytest` (pytest already present). `python
  -m pip` resolves correctly despite the stale shebang. Pillow is the canonical
  PIL provider the already-committed decoder requires (`from PIL import Image`,
  line 11) — restoring a pre-existing dependency, not introducing a new one, so
  not a package-legitimacy checkpoint.
- **Files modified:** none (venv site-packages only)
- **Commit:** n/a (environment fix)

### Note on the discriminating RGB test

The plan framed Test 2 against a hypothetical bare `<<3` (which yields 248). The
*prior* code used `(r*255)//31`, which already produced 255 at max — so the
observable endpoint did not change for pure red. The two formulas diverge at
mid-range values (e.g. 5-bit 16: replication=132 vs scaling=131). The plan
mandates the bit-replication form for parity with the on-device path, which is
what was implemented; all specified tests pass.

## Threat surface scan

No new network endpoints, auth paths, or trust-boundary surface introduced. The
two docs remain gitignored (T-02-01 mitigated); the committable test references
gitignored fixtures by path, never embedding their bytes (D-21 boundary upheld).
The all-17 loud-fail assertion mitigates T-02-03.

## Known Stubs

None.

## Out-of-scope / left untouched

Pre-existing untracked / modified files NOT part of this plan and deliberately
left alone: `.coverage`, `build_fidb_examples.py`, `factory_out.bin`,
`ghidra_scripts/`, `scripts/build_fidb_examples.py`, the `firmware/custom`
submodule pointer, and all `.planning/` files (orchestrator-owned).

## Self-Check: PASSED

All created files exist on disk; both per-task commits (`38bc1cc`, `9274d82`)
present in git history. Task 2 docs are intentionally gitignored (no commit).
