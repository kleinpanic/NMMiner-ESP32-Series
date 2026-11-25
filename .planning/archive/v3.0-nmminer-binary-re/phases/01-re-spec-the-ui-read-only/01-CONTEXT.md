# Phase 1: RE-Spec the UI (read-only) - Context

**Gathered:** 2026-06-18
**Status:** Ready for planning

> Supersession, 2026-06-19: Phase 1 remains useful historical RE context, but
> the active route is Phase 7 official `v2.0.02` ST7789 binary
> decompilation/custom-edit. Use
> `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md` before this
> file. Do not follow any stale fork-primary, loader `v12.0`, old LittleFS
> geometry, or live-oracle assumptions without checking the Phase 7 authority
> docs first.

> Gathered in autonomous (`--auto`) mode per the milestone goal's standing
> authorization. Most technical approach is already locked by
> `.planning/research/` (STACK/ARCHITECTURE/PITFALLS/SUMMARY); this captures the
> remaining implementation decisions the researcher/planner need. No decision
> here required live user input — all are recommended defaults derived from the
> research, reversible, and within the locked phase boundary.

<domain>
## Phase Boundary

Turn the closed stock NMMiner v2.0.02 binary into a **written, read-only design
contract** (`hardware/recovered-assets/SPEC.md`) that all later UI-rebuild phases
are driven from, and settle the LVGL major-version question (v8 vs v9)
authoritatively. This phase stands up the decompiler workbench (Ghidra 11.x +
dynacylabs flash-loader v12.0 + a self-built ESP-IDF v4.4.6 FIDB), decompiles the
located screen-construction functions and the render / share-accept / screensaver
code paths, and records the recovered geometry/fields/colors/behavior.

**No flashing. No firmware code written. No assets committed.** Output is markdown
+ recovered intermediate artifacts, all under the gitignored
`hardware/recovered-assets/`.
</domain>

<decisions>
## Implementation Decisions

### SPEC.md coverage breadth
- **D-01:** Locate all **6** screen-construction functions (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) via the proven string→`l32r`-literal→function xref method (the same that found `Clock320x240`@`0x400d1864` and `Miner320x240`@`0x400d1960`). Record each function's address in SPEC.md even if not all are deeply decompiled this phase.
- **D-02:** **Depth-prioritize `Miner320x240` and `Clock320x240`** (the most-used screens, both already located) for full per-widget geometry. The other four get an inventory entry + key geometry to the extent recoverable; remaining detail can be back-filled when their phases need it.
- **D-03:** Identify and record the **share-accept seam** (where an accepted share is signalled) and the **screensaver render path** (`ss_render`) as function addresses + call shape in SPEC.md — these feed Phase 6 (speaker hook) and Phase 5 (screensaver). Do NOT fully spec them here; just locate + characterize.

### FIDB build effort + fallback
- **D-04:** Attempt the self-built ESP-IDF **v4.4.6** Function-ID database (Tarlogic method: compile representative IDF example projects at the optimization levels the stock image likely used, populate the FidDb). This is the highest-leverage symbol-recovery step (research STACK.md).
- **D-05:** **Time-box the FIDB build.** If it stalls (toolchain/version friction), fall back to the already-working symbol sources: the dynacylabs loader's ROM/SVD annotations + the project's existing objdump/string→xref pipeline. The screen funcs were already located WITHOUT a FIDB, so the FIDB is an accelerator, not a hard dependency — do not let it block SPEC.md.

### Tooling install
- **D-06:** Install **Ghidra 11.3.1** from the official NSA GitHub release (prebuilt zip; JDK 21 already present at `21.0.11`), then add the **dynacylabs/ghidra-esp32-flash-loader v11.3.1** extension (prebuilt zip dated 2025-02-20). Import via the loader (auto-maps DROM `0x3f400020` / IROM `0x400d0020`) OR via `tenable/esp32_image_parser create_elf` (already cloned under `hardware/recovered-assets/esp32_image_parser`) as a fallback import path. Mechanical, low-risk.
  - **CORRECTION (RESEARCH.md, 2026-06-18):** the original "Ghidra 11.4.x + loader v12.0" was stale — the dynacylabs **v12.0** tag ships **no prebuilt asset** (would require building from source). Ghidra **11.3.1 + dynacylabs v11.3.1** is the zero-friction prebuilt pairing and is the locked choice. Do NOT install the obsolete `yath/ghidra-xtensa` processor module (native Xtensa is built into Ghidra ≥11.0).

### LVGL version confirmation
- **D-07:** Confirm v8 vs v9 with **two independent signals before writing any extractor**: (a) FIDB/loader-named LVGL decoder symbols (`lv_img_decoder_*` v8 vs `lv_image_*` v9), and (b) the string family present in DROM (`LV_IMG_CF_*` = v8 vs `LV_COLOR_FORMAT_*` = v9). Working hypothesis is **v8** (Arduino-ESP32 on IDF v4.4.6, Oct-2023 build predates LVGL 9.0) — but state it with evidence, do not assume.

### Validation cross-check
- **D-08:** Cross-check recovered **stat-field formatting** (units/abbreviations like "92.67T", "8.00K") and screen content against the saved **`docs/research/nmminer-api-reference.md`**. Geometry comes from the disasm; the API reference is the formatting/semantics oracle.
  - **CORRECTION (RESEARCH.md, 2026-06-18):** the live device at `192.168.1.136` is running the **fork**, not stock NMMiner (its API returns fork-only `sd`/`speaker` fields), so it CANNOT serve as the stock stat-formatting oracle. Use the saved `docs/research/nmminer-api-reference.md` instead. On-device photos, if the user supplies any, are a secondary cross-check.

### Don't-do (anti-patterns, from research)
- **D-09:** The `hardware/recovered-assets/lvgl_*.png` and `jpg_*` files are **mis-carved noise**, NOT real screens (screens are runtime LVGL widget code, not stored bitmaps). Do not treat them as recovered art or seed the rebuild from them.
- **D-10:** Do **not** run a v9 `lv_img_dsc_t` magic-byte scan and conclude "no images" — for v8 the descriptor is a 12-byte packed bitfield with no magic. Fonts are the extractable LVGL C-arrays (Phase 2 work); screens are code (this phase).

### No-redistribution boundary (D-11 — user decision 2026-06-18)
- **D-11:** Boundary = **script-only strict + fair-use analysis docs**. Committed **scripts** (`scripts/*.py`) MUST contain ZERO verbatim closed-firmware literals (grep-enforced: no `share accepted`/`%ldms`/decompiled-body markers); they carry only addresses, our own class-name-derived identifiers, and structural logic. Private `.planning/` analysis docs MAY quote **short individual** firmware literals as RE evidence (fair-use, not asset redistribution). ALL bulk assets — binaries, fonts, images, decompiled function bodies, full string tables — stay under gitignored `hardware/recovered-assets/`. No history rewrite. This resolves the cross-AI "literal leakage" concern as a **documented exception**, not a violation.

### Read-only invariant scope (D-12 — clarified cycle 4)
- **D-12:** "Read-only" = **no NMMiner/custom/device firmware build and no flashing/device write**. Building local **ESP-IDF example projects to populate the FIDB** is explicitly ALLOWED (those ELFs are throwaway tooling under gitignored paths, not the target firmware). The invariant forbids touching the device and building/altering the NMMiner or fork firmware images — not local RE-tooling compiles.

### Claude's Discretion
- Exact Ghidra script vs interactive workflow for bulk symbol import (headless export for symbols; interactive/assisted reading for the novel per-screen geometry extraction).
- Which specific IDF example projects to compile for the FIDB.
- SPEC.md internal section ordering.
</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Phase strategy & RE pipeline
- `.planning/research/SUMMARY.md` — recommended approach (B* Asset-Informed Reimplement), Phase 1 definition, confidence/gaps, LVGL v8-vs-v9 gap.
- `.planning/research/STACK.md` — Ghidra 11.x + dynacylabs loader v12.0 + self-built ESP-IDF v4.4.6 FIDB; LittleFS tooling correction; tool versions.
- `.planning/research/ARCHITECTURE.md` — RE-Spec → Custom-Firmware one-way pipeline; SPEC.md as read-only contract; share-accept seam concept.
- `.planning/research/PITFALLS.md` — pitfalls 1 (LVGL header scan), 2 (image SHA), 3 (dual-SPI), 4 (licensing), 5 (RF/flash) sequenced for the roadmap.
- `.planning/research/FEATURES.md` — confirmed 6 screen classes, per-screen content, 21-endpoint API, `.ss`/`ss_render`, screensaver carousel, swarm/findme — the parity target SPEC.md must capture.
- `docs/nmminer-re-pipeline.md` — the working RE pipeline already established (image map, objcopy→ELF→objdump, string→function xref method, the two located screen funcs).

### Requirements / scope
- `.planning/REQUIREMENTS.md` — RE-01..RE-04 (this phase's requirements) + the no-redistribution constraint.
- `.planning/ROADMAP.md` §"Phase 1" — goal + 4 success criteria.

### Recovered artifacts (all gitignored under `hardware/recovered-assets/`)
- `hardware/recovered-assets/{app_factory,drom,irom}.bin`, `irom.asm` (476k lines), `irom_text.elf` — the disasm pipeline outputs.
- `hardware/recovered-assets/esp32_image_parser/` — tenable parser (alternate ELF-from-flash import for Ghidra).
- `hardware/recovered-assets/littlefs/`, `spiffs.bin` — unpacked filesystem (Phase 2's input, not this phase's).
- `hardware/backups/cyd-backup-20260617-172106.bin` — the 4 MB source image (gitignored).

### Live oracle
- `http://192.168.1.136/api/system/info` — live stat-field formatting reference (device on Homebase).
- `docs/research/nmminer-api-reference.md` — the 21-endpoint API field semantics.
</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- **`scripts/nmminer_ss_decode.py`** — working `.ss` decoder; relevant only to characterize `ss_render` here (full reuse is Phase 2/5).
- **`docs/nmminer-re-pipeline.md` pipeline** — objcopy→ELF→objdump + string→`l32r` xref already reproduces function locations; the FIDB just accelerates naming.
- **`hardware/recovered-assets/irom.asm`** — already-disassembled 476k-line linear disasm for fast cross-checks against Ghidra's analysis.

### Established Patterns
- **String→function xref:** find a DROM string address (`grep` in `drom.bin`, add base `0x3f400020`), then find that 4-byte LE literal in `irom.bin`'s literal pool → the referencing function. Proven on both located screen funcs.

### Integration Points
- This phase writes NO firmware. Its only output `SPEC.md` is consumed by Phase 2 (fonts/assets) and Phase 3 (screen rebuild) as a read-only contract; it is never `#include`d or linked.
</code_context>

<specifics>
## Specific Ideas

- SPEC.md output path is fixed: `hardware/recovered-assets/SPEC.md` (gitignored dir — `.gitignore:25`). It is NOT committed; downstream phases read it locally. This satisfies RE-04 / the no-redistribution constraint without losing the deliverable.
- Decompiled verbatim C bodies, the Ghidra project, and all `.bin`/`.elf` stay under `hardware/recovered-assets/` (gitignored). Only project-level *analysis notes* that contain no verbatim closed code may surface in committed docs.
</specifics>

<deferred>
## Deferred Ideas

- **Font/glyph C-array extraction + LittleFS unpack + `.ss` decoder reuse** → Phase 2 (ASSET-01..03). SPEC.md only needs to *name* which fonts matter.
- **Actual screen rebuild on the fork** → Phase 3 (UI-*). This phase produces the spec, not the LVGL code.
- **Full `ss_render` byte-layout + idle→screensaver state machine** → Phase 5 (research-recommended there). Phase 1 only locates/characterizes.
- **Path-A binary-patch integrity recon** (`esptool image_info` + `espefuse summary`) → Phase 8 contingency only; not needed unless RE surfaces an unreproducible behavior.

None of the above are scope creep — they are downstream phases already in the roadmap.
</deferred>

---

*Phase: 1-RE-Spec the UI (read-only)*
*Context gathered: 2026-06-18*
