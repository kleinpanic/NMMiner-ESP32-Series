# Phase 1 — Artifacts Produced

> What this phase outputs and where it lives. Most outputs are gitignored
> (no-redistribution, RE-04); only two small helper scripts are committed.

## Committed (the only code that leaves this phase)

| Artifact | Plan | Purpose | No-redistribution note |
|----------|------|---------|------------------------|
| `scripts/nmminer_str_xref.py` | 01-01 | Reusable string→l32r literal→IROM-address xref CLI (reproduces the 6 screen addresses, share-accept seam, ss_render) | Contains only search strings + the xref algorithm — NO verbatim closed-firmware content |
| `scripts/check_spec_screens.py` | 01-03 | Commit-safe SPEC.md section/class presence verifier (RE-03 smoke) | Asserts only section/class identifiers — NO decompiled geometry/code |

## Gitignored (under `hardware/recovered-assets/`, `.gitignore:25` — NEVER committed)

| Artifact | Plan | Contents |
|----------|------|----------|
| `hardware/recovered-assets/SPEC.md` | 01-02 (seeds §Metadata + §LVGL version), 01-03 (all remaining sections) | The phase deliverable — the read-only UI design contract |
| `hardware/recovered-assets/WORKBENCH.md` | 01-01 | Setup log: Ghidra/loader/JDK versions, import method, memory-map verification, FIDB outcome |
| `~/ghidra_projects/nmminer-re/*` (outside repo) | 01-01 | Ghidra project + imported program (decompiled C bodies) |
| `hardware/recovered-assets/nmminer-app.elf` | 01-01 | create_elf fallback import artifact (if used) |

## The LVGL-version verdict (RE-02)

Recorded in `SPEC.md §LVGL version` with two-signal evidence:
- Signal 1: DROM grep result (`lv_img_decoder` v8 vs `lv_image_decoder` v9)
- Signal 2 (authoritative): in-decompiler decoder symbol name (FIDB-named or manual xref)

Working hypothesis v8; the verdict cites actual in-decompiler evidence, not the hypothesis.
If the verdict is v9, Plan 03's v8-specific geometry recipe must be replanned.

## SPEC.md required sections (per 01-RESEARCH.md §SPEC.md Structure)

1. `## Metadata` — tool versions, LVGL verdict, 320×240 RGB565 (Plan 02 seeds)
2. `## LVGL version` — v8/v9 verdict + two-signal evidence (Plan 02)
3. `## Screen Inventory` — all 6 addresses (Plan 03 Task 1)
4. `## Per-Screen Widget Geometry` — Miner+Clock full, other 4 key geometry (Plan 03)
5. `## Stat Field Formatting` — cross-checked vs nmminer-api-reference.md (Plan 03 Task 2)
6. `## Share-Accept Seam` — address + call shape, not full spec (Plan 03 Task 1)
7. `## ss_render Characterization` — address + entry signature, not full spec (Plan 03 Task 1)
8. `## Color Palette` + `## Font Inventory` (Plan 03 Task 2)

## Requirement coverage

| Req | Plan | Where satisfied |
|-----|------|-----------------|
| RE-01 | 01-01 | Ghidra workbench + xref script + FIDB/fallback |
| RE-02 | 01-02 | LVGL verdict with two signals |
| RE-03 | 01-03 | SPEC.md screen inventory + geometry + seam + ss_render |
| RE-04 | 01-03 | SPEC.md markdown-only, gitignored (RE-04 smoke) |
