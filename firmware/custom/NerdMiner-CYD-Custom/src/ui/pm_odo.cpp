// Motion widgets for the panicminer pages. See pm_odo.h.
#include "pm_odo.h"

#include <stdint.h>
#include <string.h>

#define ODO_MAX 12
#define ROLL_MS 600
#define TOAST_MS 2400
#define TOAST_IN_MS 240      // 0-10 %: fade in, slide down 8 px
#define TOAST_OUT_MS 1920    // 80 %: start fading out, drift up 4 px
#define TOAST_PAD_X 9

typedef struct {
    const lv_font_t* font;
    lv_obj_t* suffix;
    uint8_t n;
    char cur[ODO_MAX + 1];
    // A digit's entry is its reel label (the slot is the reel's parent);
    // any other character's entry is a plain label in the row.
    lv_obj_t* cell[ODO_MAX];
} odo_t;

static const char REEL[] = "0\n1\n2\n3\n4\n5\n6\n7\n8\n9";

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static lv_coord_t digit_w(const lv_font_t* font, int d)
{
    return lv_font_get_glyph_width(font, (uint32_t)('0' + d), 0);
}

/* ---- rolling digits ----------------------------------------------------- */

static void odo_deleted(lv_event_t* e)
{
    lv_obj_t* odo = lv_event_get_target(e);
    lv_mem_free(lv_obj_get_user_data(odo));
    lv_obj_set_user_data(odo, NULL);
}

lv_obj_t* pm_odo_create(lv_obj_t* parent, const lv_font_t* font, lv_color_t color)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(o, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(o, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(o, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START);
    // Inherited by every reel and static character, so they carry no local styles.
    lv_obj_set_style_text_font(o, font, 0);
    lv_obj_set_style_text_color(o, color, 0);
    lv_obj_set_style_text_align(o, LV_TEXT_ALIGN_CENTER, 0);

    odo_t* s = (odo_t*)lv_mem_alloc(sizeof(odo_t));
    LV_ASSERT_MALLOC(s);
    memset(s, 0, sizeof(odo_t));
    s->font = font;
    lv_obj_set_user_data(o, s);
    lv_obj_add_event_cb(o, odo_deleted, LV_EVENT_DELETE, NULL);
    return o;
}

static void odo_build(lv_obj_t* odo, odo_t* s, const char* text)
{
    for (int i = 0; i < s->n; i++)
        lv_obj_del(is_digit(s->cur[i]) ? lv_obj_get_parent(s->cell[i]) : s->cell[i]);

    lv_coord_t lh = lv_font_get_line_height(s->font);
    int n = 0;
    for (; text[n]; n++) {
        char c = text[n];
        if (is_digit(c)) {
            int d = c - '0';
            lv_obj_t* slot = lv_obj_create(odo);
            lv_obj_remove_style_all(slot);
            lv_obj_clear_flag(slot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
            lv_obj_set_size(slot, digit_w(s->font, d), lh);
            lv_obj_t* reel = lv_label_create(slot);
            lv_label_set_text_static(reel, REEL);
            // The reel is wider than the slot (its widest digit); centring
            // it centres every digit's advance in the slot.
            lv_obj_set_align(reel, LV_ALIGN_TOP_MID);
            lv_obj_set_style_translate_y(reel, -d * lh, 0);
            s->cell[n] = reel;
        } else {
            char one[2] = {c, '\0'};
            lv_obj_t* l = lv_label_create(odo);
            lv_label_set_text(l, one);
            s->cell[n] = l;
        }
    }
    s->n = (uint8_t)n;
    if (s->suffix) lv_obj_move_foreground(s->suffix);
}

static void roll_exec(void* reel, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t*)reel, (lv_coord_t)v, 0);
}

// The reel's resting offset says which digit it shows; the slot narrows to it.
static void roll_done(lv_anim_t* a)
{
    lv_obj_t* reel = (lv_obj_t*)a->var;
    const lv_font_t* font = lv_obj_get_style_text_font(reel, 0);
    int d = -lv_obj_get_style_translate_y(reel, 0) / lv_font_get_line_height(font);
    lv_obj_set_width(lv_obj_get_parent(reel), digit_w(font, d));
}

static void roll(odo_t* s, int i, int d, bool animate)
{
    lv_obj_t* reel = s->cell[i];
    lv_obj_t* slot = lv_obj_get_parent(reel);
    lv_coord_t to = -d * lv_font_get_line_height(s->font);
    lv_coord_t w = digit_w(s->font, d);
    lv_anim_del(reel, roll_exec);
    if (!animate) {
        lv_obj_set_style_translate_y(reel, to, 0);
        lv_obj_set_width(slot, w);
        return;
    }
    // Interrupted rolls leave the slot at its widest; never narrow mid-roll.
    if (w > lv_obj_get_style_width(slot, 0)) lv_obj_set_width(slot, w);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, reel);
    lv_anim_set_exec_cb(&a, roll_exec);
    lv_anim_set_values(&a, lv_obj_get_style_translate_y(reel, 0), to);
    lv_anim_set_time(&a, ROLL_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_ready_cb(&a, roll_done);
    lv_anim_start(&a);
}

// Same length, and the same non-digits in the same places.
static bool same_shape(const char* a, const char* b)
{
    for (; *a && *b; a++, b++) {
        if (is_digit(*a) != is_digit(*b)) return false;
        if (!is_digit(*a) && *a != *b) return false;
    }
    return *a == *b;
}

void pm_odo_set(lv_obj_t* odo, const char* text, bool animate)
{
    odo_t* s = (odo_t*)lv_obj_get_user_data(odo);
    char next[ODO_MAX + 1];
    strncpy(next, text, ODO_MAX);
    next[ODO_MAX] = '\0';
    if (strcmp(next, s->cur) == 0) return;

    if (!same_shape(s->cur, next)) {
        odo_build(odo, s, next);
    } else {
        for (int i = 0; i < s->n; i++)
            if (is_digit(next[i]) && next[i] != s->cur[i]) roll(s, i, next[i] - '0', animate);
    }
    memcpy(s->cur, next, sizeof(next));
}

void pm_odo_set_color(lv_obj_t* odo, lv_color_t color)
{
    // Called every second with the same colour; setting it would invalidate.
    if (lv_obj_get_style_text_color(odo, 0).full == color.full) return;
    lv_obj_set_style_text_color(odo, color, 0);
}

lv_obj_t* pm_odo_add_suffix(lv_obj_t* odo, const lv_font_t* font, lv_color_t color, int gap)
{
    odo_t* s = (odo_t*)lv_obj_get_user_data(odo);
    lv_obj_t* l = lv_label_create(odo);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_style_pad_left(l, (lv_coord_t)gap, 0);
    // The row aligns bottoms (flex cross END); base_line is measured up from
    // the bottom of the line, so lift the suffix by the difference.
    lv_obj_set_style_translate_y(l, (lv_coord_t)(font->base_line - s->font->base_line), 0);
    s->suffix = l;
    return l;
}

/* ---- toast -------------------------------------------------------------- */

// CSS `ease` on 0..1024.
static int32_t ease(int32_t t)
{
    return (int32_t)lv_bezier3((uint32_t)t, 0, 102, 1024, 1024);
}

// Fades with bg_opa and text_opa, not the `opa` style: `opa` below cover
// renders the object through a layer buffer allocated from the LVGL pool.
static void toast_exec(void* obj, int32_t ms)
{
    lv_obj_t* t = (lv_obj_t*)obj;
    int32_t opa = LV_OPA_COVER, y = 0;
    if (ms < TOAST_IN_MS) {
        int32_t e = ease(ms * 1024 / TOAST_IN_MS);
        opa = LV_OPA_COVER * e / 1024;
        y = -8 + 8 * e / 1024;
    } else if (ms >= TOAST_OUT_MS) {
        int32_t e = ease((ms - TOAST_OUT_MS) * 1024 / (TOAST_MS - TOAST_OUT_MS));
        opa = LV_OPA_COVER - LV_OPA_COVER * e / 1024;
        y = -4 * e / 1024;
    } else if (lv_obj_get_style_bg_opa(t, 0) == LV_OPA_COVER && lv_obj_get_style_translate_y(t, 0) == 0) {
        return;  // holding: leave it untouched so nothing is invalidated
    }
    lv_obj_set_style_bg_opa(t, (lv_opa_t)opa, 0);
    lv_obj_set_style_text_opa(t, (lv_opa_t)opa, 0);
    lv_obj_set_style_translate_y(t, (lv_coord_t)y, 0);
}

static void toast_done(lv_anim_t* a)
{
    lv_obj_add_flag((lv_obj_t*)a->var, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t* pm_toast_create(lv_obj_t* parent, int x_center, int y, lv_color_t bg, lv_color_t fg)
{
    lv_obj_t* t = lv_label_create(parent);
    lv_obj_set_style_text_font(t, &pm_text_14, 0);
    lv_obj_set_style_text_color(t, fg, 0);
    lv_obj_set_style_bg_color(t, bg, 0);
    lv_obj_set_style_radius(t, 9, 0);
    lv_obj_set_style_pad_hor(t, TOAST_PAD_X, 0);
    lv_obj_set_style_pad_ver(t, 2, 0);
    lv_obj_set_y(t, (lv_coord_t)y);
    lv_obj_set_user_data(t, (void*)(intptr_t)x_center);
    lv_obj_add_flag(t, LV_OBJ_FLAG_HIDDEN);
    return t;
}

void pm_toast_show(lv_obj_t* toast, const char* text)
{
    lv_label_set_text(toast, text);
    const lv_font_t* font = lv_obj_get_style_text_font(toast, 0);
    lv_coord_t w = lv_txt_get_width(text, (uint32_t)strlen(text), font, 0, LV_TEXT_FLAG_NONE) + 2 * TOAST_PAD_X;
    lv_obj_set_x(toast, (lv_coord_t)((intptr_t)lv_obj_get_user_data(toast) - w / 2));
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(toast);

    lv_anim_del(toast, toast_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, toast);
    lv_anim_set_exec_cb(&a, toast_exec);
    lv_anim_set_values(&a, 0, TOAST_MS);
    lv_anim_set_time(&a, TOAST_MS);
    lv_anim_set_ready_cb(&a, toast_done);
    lv_anim_start(&a);
}

/* ---- flash -------------------------------------------------------------- */

// The two colours ride in the anim's user_data, which holds 32 bits on ESP32.
#if LV_COLOR_DEPTH != 16
#error "pm_flash packs two 16-bit colours into a pointer"
#endif

static void flash_exec(void* obj, int32_t v)
{
    lv_anim_t* a = lv_anim_get(obj, flash_exec);
    uint32_t packed = (uint32_t)(uintptr_t)a->user_data;
    lv_color_t from, to;
    from.full = (uint16_t)(packed >> 16);
    to.full = (uint16_t)(packed & 0xFFFF);
    lv_obj_set_style_text_color((lv_obj_t*)obj, lv_color_mix(to, from, (uint8_t)v), 0);
}

void pm_flash(lv_obj_t* label, lv_color_t from, lv_color_t to, uint32_t ms)
{
    lv_anim_del(label, flash_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, label);
    lv_anim_set_exec_cb(&a, flash_exec);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_time(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_user_data(&a, (void*)(uintptr_t)(((uint32_t)from.full << 16) | to.full));
    lv_anim_start(&a);
}
