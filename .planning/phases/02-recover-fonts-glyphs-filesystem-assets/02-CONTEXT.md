# Phase 2: Recover Fonts/Glyphs + Filesystem Assets - Context

**Gathered:** 2026-06-18 (auto mode — standing autonomy; decisions derived from Phase 1 RE findings)
**Status:** Ready for planning

<domain>
## Phase Boundary

Recover the reusable visual assets the screen rebuild (Phase 3) needs: the **fonts/glyphs** the stock screens use (as LVGL C-arrays usable by the fork), the **color palette**, the **LittleFS contents** (web UI + 17 screensavers — already unpacked in the RE foundation), and a **byte-accurate `.ss` decoder** reusable by the firmware screensaver carousel. No firmware code written here beyond committable font C-arrays + the host-side decoder; no flashing.
</domain>

<decisions>
## Implementation Decisions

### Fonts — regenerate from open upstreams, do NOT redistribute closed bitmaps (D-21)
- **D-21 (KEY, no-redistribution):** The stock `lv_font_t` descriptors are located (e.g. `0x3f4454a0`, `0x3f44046c`, shared-callback heads `24a91140 e4a81140`; full list in SPEC.md "Open items"). Rather than byte-extracting the closed firmware's glyph bitmaps (which would be redistributing closed assets — forbidden by D-11), **IDENTIFY** each font (family + point size + glyph subset) from the descriptor metrics, then **REGENERATE** pixel-equivalent LVGL v8 C-array fonts from the ORIGINAL open-source font files. NMMiner's UI fonts are almost certainly **Montserrat** (LVGL's built-in family, OFL) at several point sizes + a **FontAwesome** glyph subset (the Miner screen uses FA codepoints U+F1EB wifi, U+F019, U+F57D, U+F080, U+F382). Montserrat (OFL) + FontAwesome Free are committable; the regenerated C-arrays are our own build artifacts, NOT closed-firmware data.
- **D-22:** Determine each used point size from the descriptor `line_height`/glyph metrics (Phase 1 surfaced line heights 24/26/11/23/15 on SPrice; similar on others). Generate matching `lv_font_montserrat_NN` (or `lv_font_conv`-built) sizes + a FA subset font. Where LVGL's built-in montserrat size already matches, just enable it (zero new bytes).
- **D-23:** Verify the regenerated fonts render the screens pixel-close by comparing against the decoded `.ss` screenshots + (optional) on-device photos. Exact-match is the bar; "visually indistinguishable at the used sizes" is acceptable given Montserrat is the actual source family.

### Palette (D-24)
- **D-24:** Capture the stock color constants already decoded in Phase 1 SPEC (RGB565 → #hex: orange `0xebe6`/#EE7D31, grays `0x8410`/#838183, `0xb596`/#B4B2B4, `0xad55`, white `0xffffffff`, etc.) into a committed `palette` header/section for the fork to use. These are short numeric constants (facts), committable.

### LittleFS (D-25)
- **D-25:** LittleFS is ALREADY unpacked in the RE foundation (`hardware/recovered-assets/littlefs/`: `index.html(.gz)`, `apidoc.html(.gz)`, 17 `ss_saver_320_240_*.ss` + decoded `.png`). Tooling = **littlefs-python / mklittlefs** (NOT mkspiffs — the partition labelled "spiffs" is LittleFS; block 4096). Phase 2 formalizes: document contents + tooling + confirm the 17 screensavers + web UI are intact. All stay gitignored under recovered-assets.

### `.ss` decoder (D-26)
- **D-26:** The host-side decoder `scripts/nmminer_ss_decode.py` ALREADY decodes all 17 `.ss` cleanly (SN magic + RGB565→RLE→LZSS). ASSET-03 = (a) confirm byte-accuracy across all 17 (regression check, committable test), (b) the **firmware** screensaver carousel (Phase 5) needs a C decoder — Phase 2 captures the on-device algorithm from the Phase 1 `ss_render` decompilation (`FUN_400f6ff4`, 1527 instrs) as a spec the C port follows. Apply proper RGB565→RGB888 **bit-replication** (the user noted decoded images look over-saturated — left-shift without low-bit replication oversaturates; fix in the decoder/spec).

### Claude's Discretion
- Exact font-gen toolchain (`lv_font_conv` vs LVGL built-in montserrat enable).
- Whether to commit the regenerated font C-arrays into the fork now (Phase 2) or in Phase 3 — lean Phase 2 so Phase 3 just `#include`s them.
</decisions>

<canonical_refs>
## Canonical References
- `hardware/recovered-assets/SPEC.md` (gitignored) — the font descriptor addresses + line-heights + color constants per screen ("Open items" section).
- `hardware/recovered-assets/decompiled/vtables/ss_render*` + `Clock_*`/`Miner_*` etc. — font-pointer usage + the ss_render algorithm.
- `.planning/research/STACK.md` — LittleFS tooling correction (NOT mkspiffs); font extraction approach.
- `.planning/research/PITFALLS.md` — pitfall 1 (fonts ARE the extractable LVGL C-arrays; screens are not).
- `scripts/nmminer_ss_decode.py` — the working `.ss` decoder to confirm/extend.
- `.planning/REQUIREMENTS.md` — ASSET-01, ASSET-02, ASSET-03.
- `firmware/custom/NerdMiner-CYD-Custom/` — the fork (LVGL config; where regenerated fonts will be consumed in Phase 3).
</canonical_refs>

<code_context>
## Existing Code Insights
- `scripts/nmminer_ss_decode.py` — RGB565→RGB888 conversion already present (the bit-replication fix lives here per D-26).
- The fork already bundles LVGL v8 with its font set; Phase 3 swaps/adds the recovered montserrat sizes + FA subset.
</code_context>

<specifics>
## Specific Ideas
- User flagged decoded images look "weirdly saturated" → make RGB565→RGB888 use bit-replication (`r = (v>>11)&0x1f; r = (r<<3)|(r>>2)` etc.), not bare left-shift. Bake into the decoder + the palette → #hex conversions in SPEC.
</specifics>

<deferred>
## Deferred Ideas
- On-device C `.ss` decoder implementation → Phase 5 (screensaver carousel). Phase 2 only specs the algorithm + confirms host-side byte-accuracy.
- Actually wiring fonts into the screens → Phase 3.
</deferred>

---
*Phase: 2-recover-fonts-glyphs-filesystem-assets*
*Context gathered: 2026-06-18*
