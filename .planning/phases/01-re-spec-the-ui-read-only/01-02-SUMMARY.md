# 01-02 SUMMARY — LVGL major-version verdict (RE-02)

**Status:** complete
**Requirements:** RE-02

## Verdict: LVGL **v8** (Confidence: probable→confirmed)

Settled BEFORE any geometry extraction (gates Plan 03's v8-specific recipe).

## Evidence (two independent items from the acceptable-evidence set)
- **Item #2 — decoder/widget call-shape:** widget create `FUN_4011e1d0(parent) → obj`; style setters take `(obj, value, selector=0)` matching v8 `lv_obj_set_style_*(obj, val, lv_style_selector_t)`.
- **Item #4 — constructor API signatures:** `FUN_40113390(obj, align, x_ofs, y_ofs)` is the 4-arg **v8** `lv_obj_align(obj, lv_align_t, x_ofs, y_ofs)`.
- **Item #5 (corroborating):** Oct-2023 build predates LVGL 9.0 (Jan-2024).
- **Item #1 (DROM strings) — INCONCLUSIVE and recorded as such:** the image is string-stripped; DROM scan for `lv_img_decoder`/`lv_image_decoder`/`LV_IMG_CF_*`/`LV_COLOR_FORMAT_*`/`lvgl` all returned **0**. Hence the verdict rests on the structural items #2/#4 (the acceptable-evidence design anticipated this).

## Recorded
- SPEC.md `## LVGL version` section + `## LVGL API map` (the `FUN_addr → lv_* function` table), both citing the two evidence items + Confidence. Captured in gitignored `hardware/recovered-assets/{SPEC.md,decompiled/LVGL_API_MAP.md}`.

## Decision-procedure note
No disagreement between the two structural items (both → v8), so no tiebreaker needed. The string-stripped reality means symbol-name evidence is permanently unavailable; structural evidence is authoritative.
