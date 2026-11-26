// Motion widgets for the panicminer pages: rolling digits, toasts and a
// colour flash. Plain LVGL 8.3 objects and lv_anim; no per-frame drawing.
#ifndef PM_ODO_H
#define PM_ODO_H

#include "lvgl.h"

// Rolling digits. The odometer is a flex row: every digit is a reel of 0-9 in
// a clipped slot that rolls to its new value (600 ms, ease out); other
// characters are static. A slot is as wide as its digit; while a reel rolls
// the slot is as wide as the wider of the old and new digit, and it settles
// to the new digit's width when the roll ends.
lv_obj_t* pm_odo_create(lv_obj_t* parent, const lv_font_t* font, lv_color_t color);
// Rolls the digits that changed. A different shape (length, or where the
// non-digits sit) rebuilds the slots without rolling. animate=false jumps.
void pm_odo_set(lv_obj_t* odo, const char* text, bool animate);
void pm_odo_set_color(lv_obj_t* odo, lv_color_t color);
// A label that follows the digits in the same row, `gap` px after them, its
// baseline on the digits' baseline. Returns the label.
lv_obj_t* pm_odo_add_suffix(lv_obj_t* odo, const lv_font_t* font, lv_color_t color, int gap);

// A pill centred on x_center at y: slides down 8 px and fades in, holds, fades
// out (2.4 s in all). Showing it again restarts it.
lv_obj_t* pm_toast_create(lv_obj_t* parent, int x_center, int y, lv_color_t bg, lv_color_t fg);
void pm_toast_show(lv_obj_t* toast, const char* text);

// Text colour goes to `from` at once and eases back to `to` over ms.
void pm_flash(lv_obj_t* label, lv_color_t from, lv_color_t to, uint32_t ms);

#endif
