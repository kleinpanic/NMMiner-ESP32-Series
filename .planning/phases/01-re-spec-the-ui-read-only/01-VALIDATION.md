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
| RE-01 | RE-01 | Ghidra 11.3.1 + dynacylabs v11.3.1 loads image; DROM `0x3f400020` / IROM `0x400d0020` mapped; FIDB attempted (applied OR documented loader-ROM/SVD + xref fallback sufficient to decompile the screen/render/share-accept functions); tool provenance + GUI/headless path in WORKBENCH.md; committed xref script carries NO verbatim firmware literal | manual + smoke | Launch Ghidra; verify Memory Map shows the two VMAs; spot-check symbol recovery; confirm WORKBENCH.md provenance (URL/tag/SHA256/path) + GUI-vs-headless path. Smoke: `python3 scripts/nmminer_str_xref.py hardware/recovered-assets/drom.bin hardware/recovered-assets/irom.bin Miner320x240 \| grep -q 0x400d1960 && test -z "$(grep -nE 'share accepted\|%ldms' scripts/nmminer_str_xref.py)" && echo OK` | ✗ (manual+smoke) | ⬜ pending |
| RE-02 | RE-02 | LVGL major version stated citing TWO `Item N:`-tagged items from the acceptable-evidence set + a Confidence label + the decoder/constructor address, all inside the §LVGL block (a one-item thin seed cannot pass) | manual + smoke | `python3 -c "import re; t=open('hardware/recovered-assets/SPEC.md').read(); assert 'LVGL' in t and ('v8' in t or 'v9' in t); assert 'Confidence' in t; m=re.search(r'(?is)#+\s*[^\n]*LVGL[^\n]*version.*?(?=\n#+\s\|\Z)', t); assert m; b=m.group(0); assert len(set(re.findall(r'Item\s*([1-5])\s*:', b)))>=2, 'fewer than 2 distinct evidence items'; assert re.search(r'0x4[0-9a-fA-F]{6,7}', b); print('OK')"` | ✗ (manual) | ⬜ pending |
| RE-03 | RE-03 | SPEC.md exists with all 6 screen entries (+addresses), Miner+Clock >=5-row geometry tables, seam/ss_render address+call-shape, LVGL two-label, and a PER-TABLE Confidence column on each required table | smoke | `python3 scripts/check_spec_screens.py` (strengthened: 6 classes + each screen has a `0x400...` addr + Miner/Clock >=5 widget rows + Share-Accept & ss_render rows carry addr+call-shape + §LVGL cites >=2 distinct `Item N:` markers + a `Confidence` column present PER-TABLE in §Screen Inventory, Miner geometry, Clock geometry, §Share-Accept Seam, §ss_render, §Font Inventory) | ❌ Wave 0 | ⬜ pending |
| RE-04 | RE-04 | SPEC.md + recovered assets NOT committed AND NOT staged; committed scripts carry no decompiled-body leakage AND no share-accept firmware format string | smoke | `test "$(git ls-files hardware/recovered-assets/ \| wc -l)" -eq 0 && test -z "$(git diff --cached --name-only \| grep '^hardware/recovered-assets/')" && test -z "$(grep -RnE 'FUN_400\|local_[0-9a-f]+ =\|undefined4 \|lv_obj_set_pos\(\|share accepted\|%ldms' scripts/check_spec_screens.py scripts/nmminer_str_xref.py \| grep -v '^[^:]*:[0-9]*: *#')" && echo OK` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] RE-03 strengthened smoke script (`scripts/check_spec_screens.py`) — created in Plan 03 Task 1; asserts class names + addresses + Miner/Clock widget-row minimums + seam/ss_render call-shape + LVGL >=2 `Item N:` markers + PER-TABLE Confidence column
- [ ] RE-04 gitignore + staged + leakage check — confirm `hardware/recovered-assets/` (incl. SPEC.md) is fully untracked AND unstaged, and committed scripts carry no decompiled-body leakage AND no share-accept firmware format string (`share accepted`/`%ldms`)

*Existing `.gitignore:25` already excludes `hardware/recovered-assets/` — RE-04 should pass by construction; the tracked/staged/leakage checks are the guard.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ghidra workbench loads the image at correct VMAs with symbol recovery | RE-01 | Requires interactive Ghidra GUI (or documented headless/X-forward path); no headless assertion captures "analyst can read it" | Open project, check Memory Map + Symbol Tree for FIDB/loader symbols; confirm WORKBENCH.md provenance + GUI/headless path + captured checkpoint artifacts |
| LVGL version verdict | RE-02 | Determined by reading decoder symbols/structure in Ghidra; verdict must cite two `Item N:`-tagged items from the acceptable-evidence set + Confidence | Grep DROM (item 1) + read decoder/constructor structure (item 2/3/4) in Ghidra; write the v8/v9 verdict + two `Item N:`-tagged items + Confidence + decoder/constructor address into SPEC.md §LVGL version; if items disagree, escalate per the decision procedure |
| Per-screen widget geometry accuracy | RE-03 | Geometry is read from decompiler output by a human/agent; correctness judged against `docs/research/nmminer-api-reference.md` field semantics; Pitfall-2 backtrack may be needed | Cross-check Miner/Clock field list + positions against the saved API reference; resolve literal->entry per the Pitfall-2 procedure; record Confidence per row in each table |

---

## Validation Sign-Off

- [ ] All tasks have an automated smoke check or a documented manual verification
- [ ] Sampling continuity: SPEC.md existence checked after each task once it exists
- [ ] Wave 0 covers the RE-03 strengthened smoke (per-table Confidence + >=2 evidence markers) + RE-04 tracked/staged/leakage check (incl. share-accept format string)
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter once plan tasks map to this table

**Approval:** pending
