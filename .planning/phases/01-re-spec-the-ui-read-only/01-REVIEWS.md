---
phase: 1
reviewers: [gemini, codex, opencode]
reviewed_at: 2026-06-18T01:51:35Z
plans_reviewed: [01-01-PLAN.md, 01-02-PLAN.md, 01-03-PLAN.md]
self_skipped: claude (running inside Claude Code)
not_run: [coderabbit (no CLI; reviews code diffs not plan markdown — deferred to /gsd-code-review)]
---

# Cross-AI Plan Review — Phase 1: RE-Spec the UI (read-only)

## Gemini Review
# Phase 1: RE-Spec the UI (read-only) — Plan Review

## 1. Summary
The plan establishes a high-fidelity reverse-engineering environment using **Ghidra 11.3.1** and the **dynacylabs flash-loader** to turn the closed stock NMMiner v2.0.02 binary into a structured design specification (`SPEC.md`). It focuses on locating the 6 core screen constructors, authoritatively confirming the **LVGL version (v8 vs v9)**, and extracting pixel-perfect widget geometry for the primary Miner and Clock screens. The output is a read-only "design contract" that gates all subsequent native rebuilding on the NerdMiner fork, ensuring visual parity without redistributing proprietary code.

## 2. Strengths
- **Pragmatic Tool Selection:** Choosing **Ghidra 11.3.1** over 12.x to ensure compatibility with the last prebuilt **dynacylabs loader zip** is a senior-level move that avoids unnecessary build-system friction.
- **Fail-Safe FIDB Strategy:** Time-boxing the ESP-IDF v4.4.6 FIDB build and providing a clear fallback (loader symbols + xref pipeline) ensures the schedule isn't derailed by toolchain issues.
- **Rigorous Version Gating:** Plan 02 creates a hard dependency lock on the LVGL version verdict before Plan 03 begins geometry extraction, preventing wasted effort on incorrect extraction recipes.
- **Privacy/IP Hardening:** The plan strictly adheres to the no-redistribution constraint by keeping all binary artifacts and the resulting `SPEC.md` in a **gitignored** directory (`hardware/recovered-assets/`), with automated smoke tests to enforce this.

## 3. Concerns
- **[MEDIUM] Manual Extraction Fatigue:** Extracting full geometry (x/y/w/h, alignment, font, color, text) for the Miner and Clock screens manually from decompiler output is error-prone. 
- **[LOW] SPrice Indirect Xref:** The SPrice string xref is indirect (D-03). While identified as a pitfall, if the `0x400f37b0` candidate is actually a wrapper rather than the constructor, the analyst might miss top-level widget alignments.
- **[LOW] Stat Formatting Oracle:** Relying on `docs/research/nmminer-api-reference.md` is correct, but the API reference might not capture exact string truncations or rounding behaviors visible only in the UI code.

## 4. Suggestions
- **Automate Coordinate Capture:** In Plan 01, consider adding a snippet to `WORKBENCH.md` or a small Ghidra script to log all `lv_obj_set_pos` calls in a function to a text file. This reduces copy-paste errors during `SPEC.md` authoring.
- **Confirm SPrice Parent:** When investigating the SPrice candidate at `0x400f37b0`, verify if it calls `lv_obj_create(NULL)` (screen root) to confirm it is indeed the top-level constructor.
- **Record Font Pointers:** In the `SPEC.md` Font Inventory, record the actual memory addresses of the `lv_font_t` descriptors. This will make the Phase 2 extraction of font bitmaps significantly faster.

## 5. Overall Risk: LOW
The plan is technically sound, well-researched, and highly respectful of the project's IP boundaries. The primary risks are mechanical (misreading a coordinate) rather than architectural. The workbench setup and the xref pipeline have already been proven in the research phase, providing high confidence in the execution path.

**Approved for execution.**

---

## Codex Review

## Summary

The plans are technically coherent and generally aligned with the read-only RE goal: establish a Ghidra workbench, settle LVGL v8/v9 before extraction, and produce a local gitignored `SPEC.md`. The strongest parts are the explicit LVGL gate, the FIDB time-box, and the no-redistribution threat model.

The main risk is execution brittleness: too much depends on manual Ghidra interpretation, GUI checkpoints, and exact symbol/name availability, while the automated checks only verify shallow text presence. There are also scope and licensing tensions around committing helper scripts during a phase described as “markdown + recovered artifacts only.”

## Strengths

- Clear dependency ordering: workbench → LVGL verdict → geometry/spec extraction.
- Good correction of stale tooling choice: Ghidra `11.3.1` + dynacylabs loader `v11.3.1`, avoiding the no-asset `v12.0` path.
- FIDB is correctly treated as an accelerator, not a blocker.
- LVGL version is explicitly gated before v8-specific geometry extraction.
- RE-04 is considered throughout: recovered binaries, Ghidra projects, and `SPEC.md` stay gitignored.
- D-08 correction is important and well captured: do not use the live fork API as the stock oracle.
- The plan avoids flashing and firmware writes.

## Concerns

### HIGH: “Read-only phase” conflicts with committed script changes

The phase boundary says output is markdown plus recovered intermediate artifacts under `hardware/recovered-assets/`, but Plans 01 and 03 add committed files:

- `scripts/nmminer_str_xref.py`
- `scripts/check_spec_screens.py`

These are not firmware code, but they are still repo changes outside the gitignored recovery area. If “read-only” means “no firmware writes,” this is fine. If it means “no repo implementation artifacts except planning/spec,” this violates the stated phase boundary.

Clarify this explicitly before execution.

### HIGH: Automated verification is too shallow for the hardest deliverable

`check_spec_screens.py` only asserts section names and class names. It cannot detect:

- wrong function-entry boundaries,
- mistaking literal-pool references for constructors,
- incorrect widget geometry,
- copied stale/preconfirmed addresses,
- missing or fabricated color/font data,
- share-accept seam misidentification.

Given the phase’s core deliverable is a decompiled design contract, the smoke test is not sufficient evidence of RE-03 completion.

### HIGH: LVGL “two-signal” requirement may be internally inconsistent

Plan 02 says Signal 1 is DROM string family and Signal 2 is decoder symbol/call shape. But the plan also admits DROM grep may be inconclusive. If Signal 1 is inconclusive and Signal 2 is manual call-shape inference, that is not really two independent positive signals.

You need a fallback definition for “two-signal evidence,” for example:

- string family or absence/presence pattern,
- decoder call shape,
- `lv_img_dsc_t`/font descriptor layout,
- API call signatures in screen constructors,
- build-era/library strings.

Otherwise RE-02 can become a subjective checkpoint.

### HIGH: Manual Ghidra checkpoints are blocking but underspecified

The plans require a human to confirm memory map, decoder functions, geometry, seam call shape, and FIDB results. That is reasonable for RE work, but the acceptance criteria do not say exactly what artifacts must be captured from those manual checks.

`WORKBENCH.md` and `SPEC.md` should record enough evidence to make the checkpoint auditable later: addresses, screenshots optional but not committed, symbol names, call signatures, and uncertainty notes.

### MEDIUM: Stale references remain in canonical context

The context still references dynacylabs loader `v12.0` in places, while the plan correctly locks `v11.3.1`. This is a predictable footgun for downstream agents reading `STACK.md` or `SUMMARY.md`.

The plans mention the correction, but a future executor could still follow stale canonical refs. Require the summary/workbench to state “v12.0 intentionally not used.”

### MEDIUM: FIDB requirement wording may overpromise RE-01

RE-01 says stripped SDK/Arduino-core symbols recovered. Plan 01 allows “FIDB time-boxed out + fallback documented” to satisfy the requirement. That may be pragmatic, but it weakens the requirement unless “recovered” includes loader ROM/SVD annotations plus manual xrefs.

If fallback is acceptable, RE-01 should be phrased as “attempted and either applied or documented fallback sufficient for Plan 02/03.” Otherwise the plan marks a partially unmet requirement as complete.

### MEDIUM: Supply-chain verification is weak

The plan says use official sources, but does not require checksums, release asset names, pinned URLs, or recording SHA256s. For a RE workbench, tool reproducibility matters.

At minimum, `WORKBENCH.md` should record:

- download URL,
- release tag,
- asset filename,
- SHA256,
- install path.

### MEDIUM: `--all` embeds share-accept format string

The plan argues class names and one format string are functional inputs, not redistributed firmware content. That is probably low-risk, but the share-accept string is closer to literal firmware text than the class names.

If licensing/no-redistribution is strict, make `--all` load strings from a local gitignored config or from `SPEC.md` rather than embedding closed-binary strings in committed tooling.

### MEDIUM: Geometry extraction scope may exceed Phase 1’s realistic budget

Deep-specing Miner and Clock plus inventory/key geometry for four other screens, plus palette, fonts, stat formatting, `ss_render`, and share seam is substantial. The plan acknowledges prioritization, but still has broad acceptance criteria.

Risk: executor fills vague sections with low-confidence notes just to pass the section-presence checker.

### LOW: `git ls-files hardware/recovered-assets/ | wc -l == 0` is necessary but incomplete

It only proves recovered-assets files are not tracked. It does not detect accidental closed-firmware excerpts added elsewhere, such as committed scripts, docs, summaries, or copied decompiler snippets.

Add a review step for all staged files, especially `scripts/` and `.planning/*SUMMARY.md`.

### LOW: `esp32_image_parser create_elf` fallback may produce a different analysis shape than the flash-loader import

The plan treats both import paths as equivalent, but loader import and ELF import may differ in segment labels, symbols, relocation assumptions, and memory block permissions. This could affect Ghidra decompiler behavior.

`WORKBENCH.md` should record which import was authoritative for `SPEC.md`.

## Suggestions

1. Clarify phase boundary: explicitly allow or reject committed helper scripts during this “read-only” phase.

2. Strengthen RE-02 acceptance:
   - Define acceptable evidence combinations if DROM strings are inconclusive.
   - Require exact function address/name/call-shape notes in `SPEC.md`.

3. Make `SPEC.md` confidence-aware:
   - Add a `Confidence` column for screen addresses, widget rows, fonts, colors, and seams.
   - Mark `confirmed`, `probable`, or `tentative`.

4. Replace section-presence verification with stronger checks:
   - Assert each screen has an address-like `0x400...`.
   - Assert Miner and Clock tables contain minimum widget row counts.
   - Assert Share-Accept and `ss_render` include address plus call-shape fields.
   - Assert LVGL verdict includes both evidence labels.

5. Add a staged-file leakage check:
   - `git diff --cached --stat`
   - manual review of committed text for decompiled code, large string tables, or binary-derived assets.

6. Record tool provenance in `WORKBENCH.md`:
   - URLs, filenames, SHA256s, exact versions, Java version, import path, FIDB status.

7. Tighten FIDB fallback language:
   - If FIDB fails, say RE-01 is satisfied only insofar as downstream work has enough symbol recovery, not that SDK symbols were fully recovered.

8. Avoid embedding the share-accept format string in committed scripts if possible. Keep it in gitignored recovery notes/config.

## Overall Risk

**MEDIUM**

The plan is directionally sound and respects the no-flashing/no-firmware boundary. The risk is not the chosen approach; it is proof quality. The plans can produce a plausible-looking `SPEC.md` without enough hard evidence unless the manual Ghidra findings are captured more rigorously and the automated checks are strengthened.

---

## OpenCode Review (MiniMax-M2.7)

<think>
The user is asking me to review three implementation plans for Phase 1 of a project (RE-Spec the UI - read-only). This is a cross-AI plan review request. Let me read through the plans carefully and provide structured feedback.

The three plans are:
1. `01-01-PLAN.md` - Ghidra workbench setup, FIDB build, xref script
2. `01-02-PLAN.md` - LVGL v8 vs v9 verdict
3. `01-03-PLAN.md` - Decompile screens, write SPEC.md

Let me analyze each plan for:
- Missing edge cases
- Dependency ordering issues
- Scope creep/over-engineering
- No-redistribution/licensing constraint compliance
- Whether they achieve the read-only RE goal

I should provide:
1. Summary
2. Strengths
3. Concerns (tagged HIGH/MEDIUM/LOW)
4. Suggestions
5. Overall Risk (LOW/MEDIUM/HIGH)

Let me think through the plans carefully...

**Plan 01-01 Analysis:**
- Task 1: Create `nmminer_str_xref.py` script - looks good, reproducible
- Task 2: Human-verify Ghidra install - has `autonomous` portion but Task 2 is `checkpoint:human-verify` which means there's a human gate
- Task 3: FIDB build (time-boxed) - D-05 fallback is well documented

**Concerns for 01-01:**
1. The autonomous portion of Task 2 downloads Ghidra and dynacylabs from official sources, but there's no verification the downloads completed correctly or checksums match before the human checkpoint
2. The `esp32_image_parser create_elf` fallback is mentioned but not verified that it works with this specific binary


3. The Ghidra project location (`~/ghidra_projects/`) is outside the repo but the plan doesn't clarify whether the project itself stays on-disk only or gets cleaned up - important for RE-04 compliance since WORKBENCH.md is gitignored

**Plan 01-02 Analysis:**
- Task 1: Human-verify LVGL version with two signals
- D-07 requires two signals BEFORE any extractor runs, which is a proper gate
- Signal 1 (DROM grep) is automated, Signal 2 (Ghidra) is human-verify

**Concerns for 01-02:**
1. The automated Signal 1 grep could produce false positives/negatives if DROM has generic lvgl strings that don't definitively indicate version
2. There's a risk of confirmation bias - the plan acknowledges "working hypothesis is v8" but the human verifier might unconsciously favor confirming that hypothesis
3. If Signal 1 and Signal 2 disagree, there's no explicit decision procedure documented

**Plan 01-03 Analysis:**
- Task 1: Human-verify screen locations + geometry - this is the core RE work
- Task 2: Inventory remaining screens + finalize SPEC.md
- The v8-only assumption (Pattern 5 recipe) is a known constraint that gates this plan

**Concerns for 01-03:**
1. Pattern 5 geometry recipe assumes v8 throughout - if the binary turns out to be v9, this entire plan becomes invalid
2. The geometry extraction is described as "manual" but it's unclear how comprehensive the human verification actually is - are we checking every widget or sampling?
3. Pitfall 2 (literal addr != function entry) is mentioned but there's no explicit procedure for finding the enclosing function - the human just has to "confirm" it decompiles cleanly

**Dependency Chain:**
01-01 must complete before 01-02, which must complete before 01-03. The dependency chain is clear but Task 2 (FIDB) could block if it timeboxes, though the fallback is documented. However, there's a gap: if FIDB timeboxes in 01-01 Task 3, does 01-02's Signal 2 (Ghidra FIDB symbol names) still work, or does it fall back to manual reading? The 01-02 plan says "if FIDB was applied, read FIDB names; if D-05 fallback active, manually identify" - this is fine but the interaction between the timebox failure and downstream plans isn't explicitly called out as a contingency.

**RE-04 Compliance:**
The threat model identifies T-01-02 (WORKBENCH.md and Ghidra project, *.bin/*.elf stay gitignored) but there's no verification step ensuring these files actually remain untracked in git. The verification only checks `scripts/nmminer_str_xref.py` and `scripts/check_spec_screens.py` don't leak firmware content - it doesn't verify the recovered assets directory is truly gitignored.

**Scope Creep:**
Plan 01-03's D-03 mentions characterizing share-accept and ss_render without fully specifying them, but the line between "characterize" and "fully specify" is ambiguous. The plan also attempts to cross-check stat-field formatting against nmminer-api-reference.md, which is a good idea but the cross-check methodology isn't defined.

**Human Verify Checkpoints:**
The two human-verify checkpoints (Ghidra memory map in 01-01 Task 2, LVGL version in 01-02 Task 1) are appropriate for irreversible setup steps. However, the plan doesn't specify what happens if the human finds an issue - can they ask for rework and replan, or is there a retry mechanism?

**STRIDE Threats:**
T-01-01 (str_xref.py committed) and T-01-02 (WORKBENCH.md gitignored) are both mitigated. T-01-SC (tool downloads) is acknowledged but the mitigation ("official sources only") is incomplete.

**Ghidra CLI vs GUI:**
The plan installs Ghidra as a ZIP (not package manager) and the extension install requires GUI restart. For a headless environment, this could be problematic.

**Overall Risk Assessment:**
MEDIUM - the plans are well-structured with proper checkpoints, but there are several edge cases not fully addressed: what happens if the binary structure differs from expectations, what if the FIDB build completely fails and the D-05 fallback proves insufficient, and the RE-04 verification only covers committed scripts, not the gitignored directory.
</think>

## Cross-AI Plan Review: Phase 1 (01-01, 01-02, 01-03)

### 1. Summary

Phase 1 delivers a read-only design contract (`SPEC.md`) and a Ghidra workbench. Three sequential plans: (01-01) stand up Ghidra + FIDB + xref script, (01-02) confirm LVGL version, (01-03) decompile all 6 screens + write SPEC.md. The no-redistribution boundary is taken seriously; recovered assets are gitignored. The LVGL v8-vs-v9 gate is correctly placed before geometry extraction. Human checkpoints are appropriately used for irreversible setup steps.

### 2. Strengths

- **D-05 time-box on FIDB** prevents the highest-risk task from blocking the phase — screen funcs were already located without it.
- **Two-signal LVGL gate** (D-07) before any v8-specific geometry recipe runs prevents wasted work if binary is v9.
- **D-09 anti-pattern** explicitly documents that `lvgl_*.png`/jpg_* noise files must not seed the rebuild — prevents a plausible mistake.
- **D-08 correction** (use `nmminer-api-reference.md`, not live fork device) prevents cross-contamination from the running system.
- **STRIDE threat model** correctly identifies the closed-firmware → git boundary as the load-bearing control.
- **Sequential wave dependency** (01-01 → 01-02 → 01-03) is correctly enforced via `depends_on`.

### 3. Concerns

**[HIGH] 01-01 Task 2: No checksum verification on Ghidra/loader downloads**
The autonomous download step fetches from official sources but never verifies SHA256/PGP. An intermediate-compromise of the download mirror would defeat the T-01-SC mitigation entirely. Add: `sha256sum -c <known-hash>` or at minimum a size/version sanity check after unzip.

**[HIGH] 01-01: Ghidra headless vs. GUI assumption unstated**
The plan installs the ZIP and notes extension-install requires GUI restart, but never states whether the primary workflow (memory map verification, symbol inspection) is interactive GUI or headless batch. If running on a headless box, the extension-install step is impossible. Should explicitly pick one path and document it.

**[MEDIUM] 01-01: `esp32_image_parser create_elf` fallback not pre-verified**
Task 2 action says "run esp32_image_parser create_elf on app_factory.bin as fallback," but does not verify this actually produces a loadable ELF before the human checkpoint. If the fallback fails silently, the human arrives at the checkpoint with no fallback ready. Pre-verify the ELF output exists and is non-zero before the checkpoint.

**[MEDIUM] 01-02: Signal 1 (DROM grep) could be indeterminate**
The D-07 description acknowledges Signal 1 may be inconclusive ("only generic lvgl strings present in DROM"). If both signals are weak/indeterminate, the plan records "inconclusive" rather than escalating. A v8-vs-v9 inconclusive verdict leaves Plan 03 unblocked but running blind on the v8 recipe. Add a contingency: if both signals are weak, compile a representative v8.3 binary and compare call-shape fingerprints against the decoder function rather than relying on symbol names.

**[MEDIUM] 01-03: Pitfall 2 (literal ≠ function entry) resolution is underspecified**
The action says "find enclosing function entry in Ghidra per Pitfall 2" but gives no procedure. The human-verify checkpoint then says "confirm each of the 6 screen function entry points decompiles cleanly (entry prologue visible, not mid-literal-pool)." This requires the human to know what to do if they find a literal-address that isn't a function entry. Document the concrete steps: in Ghidra, navigate to the literal address, use "Find Next Code" or disassembly backward to locate the prologue. Without this, a human who hits Pitfall 2 has no path forward.

**[MEDIUM] 01-03: The "key geometry to the extent recoverable" standard is undefined**
D-02 says the 4 non-priority screens get "inventory entry + key geometry to the extent recoverable." The human-verify checkpoint for Task 2 doesn't define "key geometry" — is it 3 widgets? 10? The entire screen? This could result in widely varying SPEC.md completeness. Suggest: for the 4 non-priority screens, extract at minimum the screen's 3-5 primary content widgets (e.g., the main label, 1-2 data fields, the background object), not a full table.

**[MEDIUM] RE-04 smoke only checks committed scripts, not the gitignored directory**
The verification section says `git ls-files hardware/recovered-assets/ | wc -l` == 0, which correctly checks that no recovered assets are committed. However, if someone accidentally `git add`-s the directory in a future session before the check runs, the RE-04 violation would exist in staged changes. The check should run after any `git add` operations, not just at the end.

**[LOW] 01-03: SPrice `+1-byte` case handling is buried in the action text**
The SPrice `+1-byte` retry (D-03/Pitfall 3) is mentioned in the Task 1 action but not in the must-haves or success criteria. If the SPrice screen cannot be located via the standard xref, there's no documented fallback procedure or escalation path.

### 4. Suggestions

1. **Add pre-flight verification to 01-01 Task 2**: before the human checkpoint, run `python3 scripts/nmminer_str_xref.py --all` to re-confirm the 6 known addresses against the freshly-imported image. This proves the import worked before the human even opens Ghidra.

2. **Clarify the `scripts/check_spec_screens.py` assertion list**: the verifier asserts "all 6 class names" are present — confirm these exact 6 strings (`Loading320x240`, `Miner320x240`, `Clock320x240`, `SPrice320x240`, `Weather320x240`, `Config320x240`) are what Ghidra will show when you open the located functions. If the decompiler renames them or shows `FUN_xxx` instead, the verifier will false-fail even though the work is correct.

3. **Define "function entry" clearly**: specify `Entry Point` in Ghidra's Function Graph or `FindPrev` in the listing as the canonical method for resolving Pitfall 2.

4. **Add a SPEC.md section ordering note**: the RESEARCH.md §SPEC.md Structure lists 8 sections but doesn't mandate ordering. The verifier asserts presence of section names; if two sections have the same name or get merged during writing, the verifier might false-pass. Consider numbering sections explicitly in SPEC.md (e.g., `## 1. Metadata`, `## 2. LVGL Version`) and asserting sequential numbering in the verifier.

5. **Document what "share-accept seam characterized" means precisely**: the spec needs (address + call shape), but "call shape" could mean (return type, arg count, arg types) or just (arg count, calling convention). Specify: record the function address, its 4-6 lv_obj/screen-call predecessors, and whether share-count and elapsed-ms are passed as integers or pointers.

### 5. Overall Risk: **MEDIUM**

The plans are structurally sound and the critical path (LVGL gate before geometry extraction, FIDB time-box, gitignored recovered assets) is well-designed. The primary risks are:

1. The human checkpoints for Ghidra import and LVGL verdict are correctly placed but underspecified in procedure — a human at those checkpoints without clear next-step guidance when something goes wrong.
2. The FIDB build is the highest-effort task and is correctly time-boxed, but the fallback (dynacylabs loader ROM/SVD + string-xref) is untested as a complete substitute for SDK symbol recovery.
3. No automated regression check exists to ensure the gitignored boundary holds across future sessions.

**Recommend:** Address the [HIGH] items (checksum verification, headless/GUI clarity) before execution. The [MEDIUM] items can be resolved with small additions to the plan actions or a pre-flight checklist.

---

## Consensus Summary

**Risk verdicts:** Gemini LOW (approved) · Codex MEDIUM · OpenCode MEDIUM. Directionally sound (correct tool versions, LVGL-before-geometry gate, no-redistribution respected, no flashing) — but two reviewers flag that the plans can produce a *plausible-looking* SPEC.md without hard evidence unless proof quality is tightened.

### Agreed Strengths (2+ reviewers)
- Ghidra 11.3.1 + dynacylabs v11.3.1 version correction (avoids no-asset v12.0).
- FIDB correctly time-boxed as an accelerator, not a blocker (D-05).
- LVGL v8/v9 gate (D-07) placed before the v8-specific geometry recipe.
- No-redistribution boundary respected — recovered assets + SPEC.md gitignored (RE-04).
- D-08 correction (use saved API reference, not the live fork device) captured.

### Agreed Concerns → must clear before execution (HIGH)
- **H1 — Phase-boundary ambiguity on committed scripts** (Codex HIGH): `scripts/nmminer_str_xref.py` + `scripts/check_spec_screens.py` are repo changes outside the gitignored area. Resolve explicitly: read-only = "no firmware writes / no flashing"; identifier-only RE helper scripts ARE allowed and committed; they must contain ZERO verbatim closed-firmware strings.
- **H2 — Shallow automated verification** (Codex HIGH; OpenCode echoes): `check_spec_screens.py` only checks section/class-name presence. Strengthen: assert each screen has a `0x400…` address; Miner+Clock tables have a minimum widget-row count; Share-Accept + ss_render rows carry address + call-shape; LVGL verdict cites both evidence labels. Add a `Confidence` column (confirmed/probable/tentative) to SPEC.md.
- **H3 — LVGL "two-signal" may collapse to one** (Codex HIGH; OpenCode MEDIUM): Signal 1 (DROM strings) is admitted possibly-inconclusive; Signal 2 is manual. Define an explicit acceptable-evidence set so RE-02 isn't subjective: string family OR decoder call-shape OR lv_img_dsc_t/font-descriptor layout OR constructor API signatures OR build-era strings — and a decision procedure if signals disagree.
- **H4 — Manual Ghidra checkpoints underspecified** (Codex HIGH; OpenCode MEDIUM): capture concrete artifacts at each human gate (exact addresses, symbol names, call signatures, uncertainty notes) in WORKBENCH.md/SPEC.md so checkpoints are auditable; document the Pitfall-2 navigation procedure (literal addr → backtrack to function prologue).
- **H5 — Tool supply-chain / headless path** (OpenCode HIGH ×2; Codex MEDIUM): record download URL + release tag + asset filename + SHA256 + install path in WORKBENCH.md; and explicitly state the Ghidra GUI-vs-headless path (the box's display availability must be resolved — extension install + decompiler reading assume a GUI).

### Notable MEDIUM/LOW (fold into replan where cheap)
- Stale `v12.0` still in STACK.md/SUMMARY.md canonical refs — add a "v12.0 intentionally not used" note so downstream agents don't follow it.
- FIDB-vs-RE-01 wording: phrase RE-01 as "attempted; applied OR documented fallback sufficient for Plans 02/03," not "SDK symbols fully recovered."
- Define "key geometry" for the 4 non-priority screens (min 3-5 primary widgets).
- RE-04 leakage check should run against staged files (`git diff --cached`), not only tracked files.
- Automate `lv_obj_set_pos` capture via a Ghidra script to cut copy-paste error (Gemini); record `lv_font_t` descriptor addresses for Phase 2 (Gemini); confirm SPrice `0x400f37b0` calls `lv_obj_create(NULL)` (Gemini).

### Divergent Views
- **Gemini** rates overall LOW ("approved for execution") — sees risks as mechanical, not architectural. **Codex/OpenCode** rate MEDIUM, emphasizing proof quality. Resolution: adopt the Codex/OpenCode proof-hardening (it's cheap and strictly improves auditability) rather than ship on Gemini's lighter bar.

### Convergence verdict
HIGH concerns present (H1–H5) → **replan required** via `/gsd-plan-phase 1 --reviews` before execution. None are architectural; all are proof/spec-hardening fixes to the 3 existing plans.
