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
| RE-01 | RE-01 | Ghidra 11.3.1 + dynacylabs v11.3.1 + FIDB loads image; DROM `0x3f400020` / IROM `0x400d0020` mapped; SDK symbols recovered | manual | Launch Ghidra; verify Memory Map shows the two VMAs; spot-check FIDB-named functions | ✗ (manual) | ⬜ pending |
| RE-02 | RE-02 | LVGL major version stated with evidence | manual | In Ghidra w/ FIDB: presence of `lv_img_decoder_*`/`LV_IMG_CF_*` (v8) vs `lv_image_*`/`LV_COLOR_FORMAT_*` (v9); record verdict in SPEC.md §LVGL version | ✗ (manual) | ⬜ pending |
| RE-03 | RE-03 | SPEC.md exists with all 6 screen entries + Miner+Clock full geometry | smoke | `python3 -c "t=open('hardware/recovered-assets/SPEC.md').read(); assert all(s in t for s in ['Miner320x240','Clock320x240','Loading320x240','SPrice320x240','Weather320x240','Config320x240'])"` | ❌ Wave 0 | ⬜ pending |
| RE-04 | RE-04 | SPEC.md + recovered assets NOT committed to git | smoke | `test "$(git ls-files hardware/recovered-assets/ \| wc -l)" -eq 0 && echo OK` | ❌ Wave 0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] RE-03 smoke script — run after SPEC.md is written (asserts all 6 screen classes present)
- [ ] RE-04 gitignore check — confirm `hardware/recovered-assets/` (incl. SPEC.md) is fully untracked

*Existing `.gitignore:25` already excludes `hardware/recovered-assets/` — RE-04 should pass by construction; the check is a guard.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Ghidra workbench loads the image at correct VMAs with FIDB symbols | RE-01 | Requires interactive Ghidra GUI; no headless assertion captures "analyst can read it" | Open project, check Memory Map + Symbol Tree for FIDB-named SDK functions |
| LVGL version verdict | RE-02 | Determined by reading decoder symbols/strings in Ghidra after FIDB applied | Grep DROM + check FIDB symbol names; write the v8/v9 verdict + evidence into SPEC.md |
| Per-screen widget geometry accuracy | RE-03 | Geometry is read from decompiler output by a human/agent; correctness judged against `docs/research/nmminer-api-reference.md` field semantics | Cross-check Miner/Clock field list + positions against the saved API reference |

---

## Validation Sign-Off

- [ ] All tasks have an automated smoke check or a documented manual verification
- [ ] Sampling continuity: SPEC.md existence checked after each task once it exists
- [ ] Wave 0 covers RE-03 + RE-04 smoke scripts
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter once plan tasks map to this table

**Approval:** pending
