---
phase: 1
slug: re-spec-the-ui-read-only
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-18
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
> This phase produces a markdown **spec** (read-only RE), not runnable firmware,
> so most verification is manual review + a few smoke scripts against SPEC.md.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | None — phase produces `hardware/recovered-assets/SPEC.md` (markdown) + recovered RE artifacts, not code. |
| **Config file** | none |
| **Quick run command** | `test -f hardware/recovered-assets/SPEC.md && echo OK` |
| **Full suite command** | SPEC.md smoke checks (below) + manual review checklist against RE-01..RE-04 |
| **Estimated runtime** | ~2 seconds (smoke) |

---

## Sampling Rate

- **After every task commit:** `test -f hardware/recovered-assets/SPEC.md && echo OK` (once SPEC.md exists)
- **After every plan wave:** run both smoke scripts (RE-03, RE-04)
- **Before `/gsd-verify-work`:** both smoke scripts green + manual review of LVGL-version evidence (RE-02) and Ghidra workbench load (RE-01)
- **Max feedback latency:** ~5 seconds

---

## Per-Task Verification Map

| Task ID | Req | Behavior | Test Type | Automated Command | File Exists | Status |
|---------|-----|----------|-----------|-------------------|-------------|--------|
| RE-01 | RE-01 | Ghidra 11.3.1 + dynacylabs v11.3.1 + FIDB loads image; DROM `0x3f400020` / IROM `0x400d0020` mapped; FIDB attempted (applied OR documented fallback sufficient for Plans 02/03); tool provenance + GUI/headless path recorded in WORKBENCH.md | manual | Launch Ghidra; verify Memory Map shows the two VMAs; spot-check symbol recovery; confirm WORKBENCH.md provenance (URL/tag/SHA256/path) + GUI-vs-headless path | ✗ (manual) | ⬜ pending |
| RE-02 | RE-02 | LVGL major version stated citing TWO items from the acceptable-evidence set + a Confidence label + the decoder/constructor address | manual + smoke | `python3 -c "t=open('hardware/recovered-assets/SPEC.md').read(); import re; assert 'LVGL' in t and ('v8' in t or 'v9' in t); assert 'Confidence' in t; assert len(re.findall(r'0x4[0-9a-fA-F]{6,7}', t))>=1; print('OK')"` | ✗ (manual) | ⬜ pending |
| RE-03 | RE-03 | SPEC.md exists with all 6 screen entries (+addresses), Miner+Clock >=5-row geometry tables, seam/ss_render address+call-shape, LVGL two-label, Confidence column on every table | smoke | `python3 scripts/check_spec_screens.py` (strengthened: 6 classes + each screen has a `0x400...` addr + Miner/Clock >=5 widget rows + Share-Accept & ss_render rows carry addr+call-shape + §LVGL cites both evidence labels + `Confidence` column present) | ❌ Wave 0 | ⬜ pending |
| RE-04 | RE-04 | SPEC.md + recovered assets NOT committed AND NOT staged; committed scripts carry no decompiled-body leakage | smoke | `test "$(git ls-files hardware/recovered-assets/ \| wc -l)" -eq 0 && test -z "$(git diff --cached --name-only \| grep '^hardware/recovered-assets/')" && test -z "$(grep -RnE 'FUN_400\|local_[0-9a-f]+ =\|undefined4 \|lv_obj_set_pos\(' scripts/check_spec_screens.py scripts/nmminer_str_xref.py \| grep -v '^[^:]*:[0-9]*: *#')" && echo OK` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] RE-03 strengthened smoke script (`scripts/check_spec_screens.py`) — created in Plan 03 Task 1; asserts class names + addresses + Miner/Clock widget-row minimums + seam/ss_render call-shape + LVGL two-label + Confidence column
- [ ] RE-04 gitignore + staged + leakage check — confirm `hardware/recovered-assets/` (incl. SPEC.md) is fully untracked AND unstaged, and committed scripts carry no decompiled-body leakage

*Existing `.gitignore:25` already excludes `hardware/recovered-assets/` — RE-04 should pass by construction; the tracked/staged/leakage checks are the guard.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ghidra workbench loads the image at correct VMAs with symbol recovery | RE-01 | Requires interactive Ghidra GUI (or documented headless/X-forward path); no headless assertion captures "analyst can read it" | Open project, check Memory Map + Symbol Tree for FIDB/loader symbols; confirm WORKBENCH.md provenance + GUI/headless path + captured checkpoint artifacts |
| LVGL version verdict | RE-02 | Determined by reading decoder symbols/structure in Ghidra; verdict must cite two items from the acceptable-evidence set + Confidence | Grep DROM (item 1) + read decoder/constructor structure (item 2/3/4) in Ghidra; write the v8/v9 verdict + two cited items + Confidence + decoder/constructor address into SPEC.md; if items disagree, escalate per the decision procedure |
| Per-screen widget geometry accuracy | RE-03 | Geometry is read from decompiler output by a human/agent; correctness judged against `docs/research/nmminer-api-reference.md` field semantics; Pitfall-2 backtrack may be needed | Cross-check Miner/Clock field list + positions against the saved API reference; resolve literal->entry per the Pitfall-2 procedure; record Confidence per row |

---

## Validation Sign-Off

- [ ] All tasks have an automated smoke check or a documented manual verification
- [ ] Sampling continuity: SPEC.md existence checked after each task once it exists
- [ ] Wave 0 covers the RE-03 strengthened smoke + RE-04 tracked/staged/leakage check
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter once plan tasks map to this table

**Approval:** pending
