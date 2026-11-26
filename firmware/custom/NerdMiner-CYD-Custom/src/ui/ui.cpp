// PanicMiner UI, the Afterglow Blockspace pages. Mine draws the block being
// hashed as an isometric cube with the hash stream behind it; the other pages
// share its compact columns and type. Hash keeps the best proof and live hash; Chain shows the
// price and the difficulty period as 2016 dots; Clock shows the time. Boot and
// setup have their own screens. Pixels come from pm_px, motion from pm_odo and
// one timer that redraws only the moving parts of the page on screen.
// See docs/design/panicminer-design.md.
#include "ui.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "lvgl.h"
#include "pm_odo.h"
#include "pm_px.h"

#define W 320
#define H 240
#define M 12                       // outer margin
// Puts a pm_disp_40 label on the pm_hero_96 baseline when aligned to its
// bottom edge: the fonts' base_line values are 13 and 8.
#define BASELINE_OFS (-5)
#define LOG_LINES 6

#define C_INK    lv_color_hex(0x000000)
#define C_PAPER  lv_color_hex(0xECEEEF)
#define C_ALARM  lv_color_hex(0xFF3B2F)

#define SPARK_POINTS 60
#define PRICE_POINTS 60            /* one per feed update: 2 hours at 2 min */

#define FRAME_MS 66                /* the motion timer, about 15 fps */
#define SHARE_GLOW_MS 1400
#define STRIP_MS 900
#define RIPPLE_MS 1000
#define TOAST_Y 27

static const char* const WEEKDAY[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
                                      "Thursday", "Friday", "Saturday"};
static const char* const MONTH[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

/* Two looks, picked at build time:
 * 1 Afterglow: rose light over a plum-to-black field, wide geometric numerals.
 * 2 Hazard: signal yellow on black under a strip of warning tape. */
#ifndef PM_THEME
#define PM_THEME 1
#endif
#if PM_THEME == 1
#define T_BG_TOP   lv_color_hex(0x230B30)
#define T_ACCENT   lv_color_hex(0xFF3D7F)
#define T_ACCENT2  lv_color_hex(0xFFA24A)
#define T_HERO     lv_color_hex(0xF7F3F8)
#define T_DIM      lv_color_hex(0x8C8293)
#define T_FAINT    lv_color_hex(0x3B3342)
#define T_VALUE    T_HERO
#define T_ACCENT_HEX  "FF3D7F"
#define T_ACCENT2_HEX "FFA24A"
#define T_HERO_HEX    "F7F3F8"
#define T_DIM_RGB     0x8C8293
#define T_TOAST_FG    lv_color_hex(0x1D0612)
#define T_TOAST2_FG   lv_color_hex(0x2A1300)
#define CUBE_FACES    {0x3A1430, 0x2A0E24, 0x1C0918}
#define PEND_FACES    {0xFF3D7F, 0xB02A5A, 0x7A1D40}
#define DONE_FACES    {0x5B4A66, 0x3D3046, 0x2B2232}
#define HERO_FONT  (&pm_wide_64)
#define UNIT_FONT  (&pm_wide_18)
#define HERO_TOP   30
#define CHART_Y    94
#define HEADER_Y   10
#else
#define T_BG_TOP   lv_color_hex(0x000000)
#define T_ACCENT   lv_color_hex(0xFFD21F)
#define T_ACCENT2  lv_color_hex(0xFFFFFF)
#define T_HERO     lv_color_hex(0xFFD21F)
#define T_DIM      lv_color_hex(0x8A8D90)
#define T_FAINT    lv_color_hex(0x34363A)
#define T_VALUE    C_PAPER
#define T_ACCENT_HEX  "FFD21F"
#define T_ACCENT2_HEX "FFFFFF"
#define T_HERO_HEX    "FFFFFF"
#define T_DIM_RGB     0x8A8D90
#define T_TOAST_FG    lv_color_hex(0x000000)
#define T_TOAST2_FG   lv_color_hex(0x000000)
#define CUBE_FACES    {0x2A2A2A, 0x1E1E1E, 0x141414}
#define PEND_FACES    {0xFFD21F, 0xB39317, 0x735E0F}
#define DONE_FACES    {0x4A4C50, 0x34363A, 0x26282B}
#define HERO_FONT  (&pm_hero_96)
#define UNIT_FONT  (&pm_disp_40)
#define HERO_TOP   20
#define CHART_Y    108
#define HEADER_Y   14
#endif

/* Mine page geometry */
#define BLOCK_CX 80
#define BLOCK_Y  34
#define BLOCK_W  56
#define BLOCK_H  62
#define RX       170               /* the right-hand column */
#define SCOPE_Y  88
#define SCOPE_H  22
#define BAR_Y    134
#define BAR_W    140
#define STRIP_Y  174

/* Chain page: the 2016 blocks of a difficulty period, 96 x 21 dots */
#define GRID_X     16
#define GRID_Y     124
#define GRID_COLS  96
#define GRID_ROWS  21
#define GRID_PITCH 3

/* Hash page */
#define HASH_NIBBLES 22
#define HASH_X M
#define HASH_Y 160
#define NIBBLE_W (211.0f / 16.0f)   /* pm_mono_22 advance, 1/16 px units */
#define LIVE_HEX 36

typedef struct {
    lv_obj_t* value;
    lv_obj_t* label;
} stat_t;

typedef struct {
    lv_obj_t* left;
    lv_obj_t* state;
    lv_obj_t* dot;
} header_t;

static struct {
    lv_obj_t* scr;
    header_t head;
    lv_obj_t* block;
    lv_obj_t* hashing;
    lv_obj_t* rate;
    lv_obj_t* rate_unit;
    lv_obj_t* chart;
    lv_chart_series_t* series;
    lv_obj_t* head_dot;
    lv_obj_t* best;
    lv_obj_t* bar;
    lv_obj_t* cell_val[4];
    lv_obj_t* cell_cap[4];
    lv_obj_t* strip;
    lv_obj_t* toast;
} mine;

static struct {
    lv_obj_t* scr;
    header_t head;
    lv_obj_t* stream;
    lv_obj_t* hero;
    lv_obj_t* unit;
    lv_obj_t* proof;
    lv_obj_t* proof_unit;
    lv_obj_t* proof_bar;
    lv_obj_t* chart;
    lv_chart_series_t* series;
    lv_obj_t* hash_score;
    lv_obj_t* glow;
    lv_obj_t* hash;
    lv_obj_t* block_track;
    lv_obj_t* block_fill;
    lv_obj_t* block_mark;
    lv_obj_t* live;
    stat_t shares, best, blocks, uptime;
    lv_obj_t* toast;
} hashp;

static struct {
    lv_obj_t* scr;
    header_t head;
    lv_obj_t* dollar;
    lv_obj_t* price;
    lv_obj_t* price_unit;
    lv_obj_t* change;
    lv_obj_t* change_foot;
    lv_obj_t* chart;
    lv_chart_series_t* series;
    lv_obj_t* period_key;
    lv_obj_t* period_val;
    lv_obj_t* grid;
    stat_t difficulty, hashrate, fee_fast, fee_slow;
    lv_obj_t* toast;
    double last_price;
} chain;

static struct {
    lv_obj_t* scr;
    header_t head;
    lv_obj_t* stream;
    lv_obj_t* hero;
    lv_obj_t* seconds;
    lv_obj_t* minute_fill;
    lv_obj_t* row_key[2];
    lv_obj_t* row_val[2];
    stat_t hashrate, shares, best, uptime;
} clock_;

static struct {
    lv_obj_t* scr;
    header_t head;
    lv_obj_t* rate[2];
    lv_obj_t* value[10];
} stats;

static struct {
    lv_obj_t* scr;
    lv_obj_t* lines[LOG_LINES];
    lv_obj_t* cube;
    lv_obj_t* progress;
    header_t head;
    int count;
    unsigned steps;
} boot;

static struct {
    lv_obj_t* scr;
} setup;

// What the draw callbacks and the motion timer read; ui_update fills it.
static struct {
    float khs;
    bool mining;
    bool chain_valid;
    uint32_t height;           // newest block, 0 until the feed answers
    int fee[3];                // fast, hour, eco sat/vB
    int mined;                 // blocks into the difficulty period
    int zeros, need;           // best hash zero nibbles; the nibbles a block needs
    uint32_t shares;
    bool have_shares;          // seen once, so the first update fires no toast
    uint32_t share_at;         // lv_tick of the newest accepted share, 0 = none
    uint32_t block_at;         // lv_tick of the newest block, 0 = none
    float scan;                // 0..1 through a sweep of 2^22 nonces
    // lv_tick at the start of this frame. Every draw callback reads it, because a
    // frame renders in 20-row chunks and a fresh tick per chunk tears moving parts.
    uint32_t now;
    uint32_t last_ms;
    uint32_t frame;
} st;

static ui_page_t s_page = UI_PAGE_MINE;
static int16_t s_spark[SPARK_POINTS];
static int s_spark_n = 0;

/* ---- building blocks -------------------------------------------------- */

static lv_obj_t* screen(lv_color_t bg)
{
    lv_obj_t* s = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s, bg, 0);
    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
    return s;
}

static lv_obj_t* rect(lv_obj_t* parent, int x, int y, int w, int h, lv_color_t bg)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_bg_color(o, bg, 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return o;
}

// An empty object whose LV_EVENT_DRAW_MAIN handler draws pixels with pm_px.
static lv_obj_t* px_layer(lv_obj_t* parent, int x, int y, int w, int h, lv_event_cb_t draw)
{
    lv_obj_t* o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_clear_flag(o, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(o, draw, LV_EVENT_DRAW_MAIN, NULL);
    return o;
}

static lv_obj_t* text(lv_obj_t* parent, const lv_font_t* font, lv_color_t color,
                      int x, int y, const char* s)
{
    lv_obj_t* l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, s);
    return l;
}

static lv_obj_t* text_right(lv_obj_t* parent, const lv_font_t* font, lv_color_t color,
                            int right_x, int y, const char* s)
{
    lv_obj_t* l = text(parent, font, color, 0, y, s);
    lv_obj_align(l, LV_ALIGN_TOP_RIGHT, right_x - W, y);
    return l;
}

static void set_text(lv_obj_t* l, const char* s)
{
    // Skip identical text so LVGL does not invalidate the area every second.
    if (strcmp(lv_label_get_text(l), s) != 0)
        lv_label_set_text(l, s);
}

// Setting a style re-renders the object even when the value is unchanged.
static void set_color(lv_obj_t* o, lv_color_t c)
{
    if (lv_obj_get_style_text_color(o, 0).full != c.full)
        lv_obj_set_style_text_color(o, c, 0);
}

static void set_series_color(lv_obj_t* chart, lv_chart_series_t* s, lv_color_t c)
{
    if (s->color.full != c.full) lv_chart_set_series_color(chart, s, c);
}

static int text_w(const lv_font_t* font, const char* s)
{
    lv_point_t p;
    lv_txt_get_size(&p, s, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return p.x;
}

static uint32_t stamp(void)
{
    uint32_t t = lv_tick_get();
    return t ? t : 1;
}

// Milliseconds from a stamp to this frame. A stamp taken after the frame began
// counts as 0, so an effect starts from its first state. motion_cb clears
// stamps after a minute, so the signed difference never wraps.
static uint32_t since(uint32_t at)
{
    if (!at) return UINT32_MAX;
    int32_t d = (int32_t)(st.now - at);
    return d < 0 ? 0 : (uint32_t)d;
}

static float ease_out3(float t)
{
    t = 1.0f - (t < 0 ? 0 : t > 1 ? 1 : t);
    return 1.0f - t * t * t;
}

/* ---- formatting --------------------------------------------------------- */

void ui_fmt_si(double v, char* out, int len)
{
    static const char* const suffix[] = {"", "K", "M", "G", "T", "P", "E", "Z"};
    if (!(v >= 0)) { snprintf(out, len, "-"); return; }
    if (v < 1.0) { snprintf(out, len, "%.2g", v); return; }
    int i = 0;
    while (v >= 999.5 && i < 7) { v /= 1000.0; i++; }
    if (v >= 99.95)      snprintf(out, len, "%.0f%s", v, suffix[i]);
    else if (v >= 9.995) snprintf(out, len, "%.1f%s", v, suffix[i]);
    else                 snprintf(out, len, "%.2f%s", v, suffix[i]);
}

void ui_fmt_commas(uint32_t v, char* out, int len)
{
    char raw[16];
    snprintf(raw, sizeof(raw), "%lu", (unsigned long)v);
    int n = (int)strlen(raw), o = 0;
    for (int i = 0; i < n && o < len - 1; i++) {
        if (i > 0 && (n - i) % 3 == 0 && o < len - 1) out[o++] = ',';
        if (o < len - 1) out[o++] = raw[i];
    }
    out[o] = '\0';
}

static void fmt_hashrate(float khs, char* num, int nlen, const char** unit)
{
    if (khs >= 1000.0f) { snprintf(num, nlen, "%.2f", khs / 1000.0f); *unit = "MH/s"; }
    else if (khs >= 100.0f) { snprintf(num, nlen, "%.0f", khs); *unit = "kH/s"; }
    else { snprintf(num, nlen, "%.1f", khs); *unit = "kH/s"; }
}

static void fmt_uptime(uint32_t s, char* out, int len)
{
    uint32_t d = s / 86400, h = (s / 3600) % 24, m = (s / 60) % 60;
    if (d) snprintf(out, len, "%lud %luh", (unsigned long)d, (unsigned long)h);
    else if (h) snprintf(out, len, "%luh %lum", (unsigned long)h, (unsigned long)m);
    else snprintf(out, len, "%lum", (unsigned long)m);
}

static void hash_hex(const uint8_t* h, char* out, int nibbles)
{
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < nibbles; i++)
        out[i] = hex[(h[i / 2] >> (i % 2 ? 0 : 4)) & 15];
    out[nibbles] = '\0';
}

static int zero_nibbles(const char* hex)
{
    int z = 0;
    while (hex[z] == '0') z++;
    return z;
}

// "#RRGGBB 0000#rest": LVGL label recolor, leading zeros in `color`.
static void lit_zeros(char* out, int len, const char* hex, const char* color)
{
    int z = zero_nibbles(hex);
    if (z == 0) snprintf(out, len, "%s", hex);
    else snprintf(out, len, "#%s %.*s#%s", color, z, hex, hex + z);
}

/* ---- shared frame ------------------------------------------------------- */

static void chart_area_cb(lv_event_t* e)
{
    // LVGL 8.3 lv_example_chart_2: mask under each segment, fade to the bottom.
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_draw_part_dsc_t* dsc = lv_event_get_draw_part_dsc(e);
    if (dsc->part != LV_PART_ITEMS || !dsc->p1 || !dsc->p2) return;

    lv_draw_mask_line_param_t line;
    lv_draw_mask_line_points_init(&line, dsc->p1->x, dsc->p1->y, dsc->p2->x, dsc->p2->y,
                                  LV_DRAW_MASK_LINE_SIDE_BOTTOM);
    int16_t line_id = lv_draw_mask_add(&line, NULL);
    lv_draw_mask_fade_param_t fade;
    lv_draw_mask_fade_init(&fade, &obj->coords, LV_OPA_COVER, obj->coords.y1, LV_OPA_TRANSP,
                           obj->coords.y2);
    int16_t fade_id = lv_draw_mask_add(&fade, NULL);

    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.bg_opa = LV_OPA_40;
    r.bg_color = dsc->line_dsc->color;
    lv_area_t a;
    a.x1 = dsc->p1->x;
    a.x2 = dsc->p2->x - 1;
    a.y1 = LV_MIN(dsc->p1->y, dsc->p2->y);
    a.y2 = obj->coords.y2;
    lv_draw_rect(dsc->draw_ctx, &r, &a);

    lv_draw_mask_free_param(&line);
    lv_draw_mask_free_param(&fade);
    lv_draw_mask_remove_id(line_id);
    lv_draw_mask_remove_id(fade_id);
}

#if PM_THEME == 2
/* 16x16 diagonal stripes, tiled across the tape. */
static uint16_t s_tape_px[16 * 16];
static lv_img_dsc_t s_tape_img;
static lv_color_t s_tape_color;

static void tape_paint(lv_color_t c)
{
    s_tape_color = c;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
            s_tape_px[y * 16 + x] = ((x + y) % 16) < 8 ? c.full : 0;
    s_tape_img.header.cf = LV_IMG_CF_TRUE_COLOR;
    s_tape_img.header.w = 16;
    s_tape_img.header.h = 16;
    s_tape_img.data_size = sizeof(s_tape_px);
    s_tape_img.data = (const uint8_t*)s_tape_px;
    lv_img_cache_invalidate_src(&s_tape_img);
}
#endif

static lv_obj_t* themed_screen(void)
{
    lv_obj_t* scr = screen(C_INK);
    lv_obj_set_style_bg_color(scr, T_BG_TOP, 0);
    lv_obj_set_style_bg_grad_color(scr, C_INK, 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_main_stop(scr, 0, 0);
    lv_obj_set_style_bg_grad_stop(scr, 150, 0);
#if PM_THEME == 2
    lv_obj_t* tape = rect(scr, 0, 0, W, 8, C_INK);
    lv_obj_set_style_bg_img_src(tape, &s_tape_img, 0);
    lv_obj_set_style_bg_img_tiled(tape, true, 0);
#endif
    return scr;
}

static void glow(lv_obj_t* o, int width, lv_opa_t opa)
{
    lv_obj_set_style_shadow_color(o, T_ACCENT, 0);
    lv_obj_set_style_shadow_width(o, width, 0);
    lv_obj_set_style_shadow_opa(o, opa, 0);
}

// Every page shares the header: context on the left, a status dot and the
// miner state on the right.
static header_t header(lv_obj_t* scr)
{
    header_t h;
    h.left = text(scr, &pm_text_14, T_DIM, M, HEADER_Y, "");
    h.state = text_right(scr, &pm_text_14, PM_THEME == 1 ? T_HERO : C_PAPER, W - M, HEADER_Y, "connecting");
    h.dot = rect(scr, 0, 0, 7, 7, T_ACCENT);
    lv_obj_set_style_radius(h.dot, LV_RADIUS_CIRCLE, 0);
    glow(h.dot, 12, LV_OPA_80);
    return h;
}

static void header_update(header_t* h, const ui_model_t* m)
{
    bool alarm = m->link == UI_LINK_NO_POOL;
    lv_color_t signal = alarm ? C_ALARM : T_ACCENT;
    set_text(h->state, m->link == UI_LINK_MINING ? "mining" : alarm ? "no pool" : "connecting");
    lv_color_t dot = m->link == UI_LINK_CONNECTING ? T_DIM : signal;
    if (lv_obj_get_style_bg_color(h->dot, 0).full != dot.full) {
        lv_obj_set_style_bg_color(h->dot, dot, 0);
        lv_obj_set_style_shadow_color(h->dot, signal, 0);
    }
    lv_obj_align_to(h->dot, h->state, LV_ALIGN_OUT_LEFT_MID, -7, 1);
}

static lv_obj_t* area_chart(lv_obj_t* parent, int x, int y, int w, int h, int points,
                            lv_chart_series_t** series)
{
    lv_obj_t* c = lv_chart_create(parent);
    lv_obj_remove_style_all(c);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_chart_set_type(c, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(c, points);
    lv_chart_set_div_line_count(c, 0, 0);
    lv_chart_set_update_mode(c, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_line_width(c, 2, LV_PART_ITEMS);
    lv_obj_set_style_size(c, 0, LV_PART_INDICATOR);
    *series = lv_chart_add_series(c, T_ACCENT, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(c, *series, LV_CHART_POINT_NONE);
    lv_obj_add_event_cb(c, chart_area_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    return c;
}

// A 3 px track with a glowing fill; returns the fill.
static lv_obj_t* track(lv_obj_t* parent, int y)
{
    lv_obj_t* t = rect(parent, M, y, W - 2 * M, 3, T_FAINT);
    lv_obj_set_style_radius(t, 2, 0);
    lv_obj_t* f = rect(parent, M, y, 1, 3, T_ACCENT);
    lv_obj_set_style_radius(f, 2, 0);
    glow(f, 10, LV_OPA_70);
    return f;
}

static void track_set(lv_obj_t* fill, float fraction)
{
    fraction = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
    lv_obj_set_width(fill, 1 + (int)((W - 2 * M - 1) * fraction));
}

static stat_t small_stat(lv_obj_t* parent, int x, const char* caption)
{
    stat_t s;
    s.value = text(parent, &pm_text_20, T_VALUE, x, 200, "-");
    s.label = text(parent, &pm_text_14, T_DIM, x, 221, caption);
    return s;
}

// A key on the left, its value on the right.
static void kv_row(lv_obj_t* parent, int y, lv_obj_t** key, lv_obj_t** val)
{
    *key = text(parent, &pm_text_14, T_DIM, M, y, "");
    *val = text_right(parent, &pm_text_14, T_VALUE, W - M, y, "");
    lv_label_set_recolor(*val, true);
}

// Measurements appear immediately. Rolling through zero looked like missing
// or invented data when a page was first opened.
static void odo_show(lv_obj_t* odo, const char* s)
{
    pm_odo_set(odo, s, false);
}

// The faint hex stream drifting behind a page's hero.
static void draw_stream_hero(lv_event_t* e)
{
    lv_obj_t* o = lv_event_get_target(e);
    pm_px_hex_stream(lv_event_get_draw_ctx(e), st.now, 35, 4, o->coords.y1, o->coords.y2);
}

/* ---- mine: the block being hashed ---------------------------------------- */

static void draw_mine_block(lv_event_t* e)
{
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    pm_px_hex_stream(ctx, st.now, 17, 4, 24, 168);
    uint32_t g = since(st.share_at);
    pm_cube_t c = {CUBE_FACES, 7, st.mining ? st.scan : -1.0f, 46,
                   (uint8_t)(g < SHARE_GLOW_MS ? 255 - 255 * g / SHARE_GLOW_MS : 0), 255};
    pm_px_cube(ctx, BLOCK_CX, BLOCK_Y, BLOCK_W, BLOCK_H, &c);
}

// The best hash's zeros against the zeros a block needs, one segment each.
static void draw_bar(lv_event_t* e)
{
    lv_obj_t* o = lv_event_get_target(e);
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    int need = st.need > 0 ? st.need : 20;
    int pitch = BAR_W / need < 3 ? 3 : BAR_W / need;
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    for (int i = 0; i < need; i++) {
        d.bg_color = i < st.zeros ? T_ACCENT : T_FAINT;
        lv_area_t a = {(lv_coord_t)(o->coords.x1 + i * pitch), o->coords.y1,
                       (lv_coord_t)(o->coords.x1 + i * pitch + pitch - 3), o->coords.y2};
        lv_draw_rect(ctx, &d, &a);
    }
}

static void strip_cube(lv_draw_ctx_t* ctx, float x, float t, float alpha, const char* label)
{
    static const uint32_t pend[3] = PEND_FACES, done[3] = DONE_FACES;
    pm_cube_t c;
    for (int i = 0; i < 3; i++) c.face[i] = pm_px_mix(pend[i], done[i], t);
    c.tiles = 0;
    c.scan = -1.0f;
    c.edge_opa = 26;
    c.glow_opa = 0;
    c.opa = (uint8_t)(255.0f * (alpha < 0 ? 0 : alpha > 1 ? 1 : alpha));
    int cx = (int)lroundf(x);
    pm_px_cube(ctx, cx, STRIP_Y + 8, 14, 15, &c);

    lv_draw_label_dsc_t d;
    lv_draw_label_dsc_init(&d);
    d.font = &pm_text_11;
    d.color = t > 0.5f ? T_DIM : T_HERO;
    d.opa = c.opa;
    d.align = LV_TEXT_ALIGN_CENTER;
    lv_area_t a = {(lv_coord_t)(cx - 22), STRIP_Y + 42, (lv_coord_t)(cx + 21), STRIP_Y + 56};
    lv_draw_label(ctx, &d, &a, label, NULL);
}

// Three pending blocks by fee, the divider, then the newest blocks mined. A
// new block slides across the divider and pushes the others right.
static void draw_strip(lv_event_t* e)
{
    lv_draw_ctx_t* ctx = lv_event_get_draw_ctx(e);
    char fee[3][20], blk[4][12];
    // Each label gets the 44 px between cubes: "99 sat/vB" is 41 px, "100 sat/vB" 46.
    for (int i = 0; i < 3; i++) {
        int f = st.fee[i];
        if (!st.chain_valid) snprintf(fee[i], sizeof(fee[i]), "-");
        else if (f < 100) snprintf(fee[i], sizeof(fee[i]), "%d sat/vB", f);
        else if (f < 10000) snprintf(fee[i], sizeof(fee[i]), "%d sat", f);
        else snprintf(fee[i], sizeof(fee[i]), "%dk sat", f / 1000);
    }
    for (int i = 0; i < 4; i++) {
        if (st.chain_valid && st.height >= (uint32_t)i) ui_fmt_commas(st.height - i, blk[i], sizeof(blk[i]));
        else snprintf(blk[i], sizeof(blk[i]), "-");
    }
    pm_px_vdash(ctx, 149, STRIP_Y + 2, STRIP_Y + 58, T_DIM_RGB, 150);

    uint32_t el = since(st.block_at);
    if (el >= STRIP_MS) {
        strip_cube(ctx, 30, 0, 1, fee[2]);
        strip_cube(ctx, 74, 0, 1, fee[1]);
        strip_cube(ctx, 118, 0, 1, fee[0]);
        for (int i = 0; i < 3; i++) strip_cube(ctx, 178 + 44 * i, 1, 1, blk[i]);
        return;
    }
    float p = ease_out3(el / (float)STRIP_MS);
    strip_cube(ctx, 178 + 44 * p, 1, 1, blk[1]);
    strip_cube(ctx, 222 + 44 * p, 1, 1, blk[2]);
    strip_cube(ctx, 266 + 44 * p, 1, 1 - p, blk[3]);
    strip_cube(ctx, 118 + 60 * p, p, 1, p > 0.5f ? blk[0] : fee[0]);
    strip_cube(ctx, 74 + 44 * p, 0, 1, fee[0]);
    strip_cube(ctx, 30 + 44 * p, 0, 1, fee[1]);
    // The new pending block slides in from the edge so its label never overlaps.
    strip_cube(ctx, 30 - 44 * (1 - p), 0, p, fee[2]);
}

static void build_mine(void)
{
    static const char* const caps[4] = {"shares", "blocks", "best", "up"};
    mine.scr = themed_screen();
    mine.block = px_layer(mine.scr, 0, 22, 162, 150, draw_mine_block);
    mine.head = header(mine.scr);
    mine.hashing = text(mine.scr, &pm_mono_12, T_DIM, 0, 156, "");
    lv_obj_set_width(mine.hashing, 160);
    lv_obj_set_style_text_align(mine.hashing, LV_TEXT_ALIGN_CENTER, 0);

    text(mine.scr, &pm_text_14, T_DIM, RX, 28, "hashrate");
    mine.rate = pm_odo_create(mine.scr, &pm_wide_40, T_HERO);
    lv_obj_set_pos(mine.rate, RX - 2, 44);
    mine.rate_unit = pm_odo_add_suffix(mine.rate, &pm_wide_14, T_ACCENT, 5);

    mine.chart = area_chart(mine.scr, RX, SCOPE_Y, W - M - RX, SCOPE_H, SPARK_POINTS, &mine.series);
    mine.head_dot = rect(mine.scr, 0, 0, 5, 5, T_HERO);
    lv_obj_set_style_radius(mine.head_dot, LV_RADIUS_CIRCLE, 0);
    glow(mine.head_dot, 10, LV_OPA_80);
    lv_obj_add_flag(mine.head_dot, LV_OBJ_FLAG_HIDDEN);

    text(mine.scr, &pm_text_14, T_DIM, RX, 114, "best hash");
    mine.best = text_right(mine.scr, &pm_text_14, T_DIM, W - M, 114, "");
    lv_label_set_recolor(mine.best, true);
    mine.bar = px_layer(mine.scr, RX, BAR_Y, BAR_W, 9, draw_bar);

    for (int i = 0; i < 4; i++) {
        mine.cell_val[i] = text(mine.scr, &pm_text_14, T_VALUE, RX, 147 + (i / 2) * 17, "-");
        mine.cell_cap[i] = text(mine.scr, &pm_text_14, T_DIM, RX, 147 + (i / 2) * 17, caps[i]);
    }
    mine.strip = px_layer(mine.scr, 0, STRIP_Y, W, 60, draw_strip);
    mine.toast = pm_toast_create(mine.scr, W / 2, TOAST_Y, T_ACCENT, T_TOAST_FG);
}

static void spark_push(float khs)
{
    // 2 MH/s is twice what the board can do. The cap keeps hi + pad and
    // lo - 3 * pad inside lv_coord_t when a glitched reading spikes the window.
    int16_t v = (int16_t)fminf(khs * 10.0f, 20000.0f);
    if (s_spark_n < SPARK_POINTS) s_spark[s_spark_n++] = v;
    else { memmove(s_spark, s_spark + 1, sizeof(s_spark[0]) * (SPARK_POINTS - 1)); s_spark[SPARK_POINTS - 1] = v; }

    int16_t lo = INT16_MAX, hi = 0;
    for (int i = 0; i < s_spark_n; i++) { if (s_spark[i] < lo) lo = s_spark[i]; if (s_spark[i] > hi) hi = s_spark[i]; }
    // Both rate scopes use the same scale and compact column geometry.
    lv_chart_set_range(hashp.chart, LV_CHART_AXIS_PRIMARY_Y, lo - 30, hi + 30);
    lv_chart_set_next_value(hashp.chart, hashp.series, v);

    // The Mine scope spans the window with 3 kH/s either side; the dot marks now.
    int lo2 = lo - 30, hi2 = hi + 30;
    lv_chart_set_range(mine.chart, LV_CHART_AXIS_PRIMARY_Y, lo2, hi2);
    lv_chart_set_next_value(mine.chart, mine.series, v);
    int y = SCOPE_Y + SCOPE_H - (v - lo2) * SCOPE_H / (hi2 - lo2);
    lv_obj_set_pos(mine.head_dot, W - M - 3, y - 2);
    lv_obj_clear_flag(mine.head_dot, LV_OBJ_FLAG_HIDDEN);
}

static void update_mine(const ui_model_t* m)
{
    static const char* const caps[4] = {"shares", "blocks", "best", "up"};
    char num[16], buf[64], val[4][16];
    const char* unit = "kH/s";
    bool alarm = m->link == UI_LINK_NO_POOL;
    lv_color_t signal = alarm ? C_ALARM : T_ACCENT;

    set_text(mine.head.left, m->pool);
    header_update(&mine.head, m);

    fmt_hashrate(m->hashrate_khs, num, sizeof(num), &unit);
    odo_show(mine.rate, num);
    set_text(mine.rate_unit, unit);
    pm_odo_set_color(mine.rate, alarm ? T_DIM : T_HERO);
    set_color(mine.rate_unit, signal);
    set_series_color(mine.chart, mine.series, signal);

    if (m->link != UI_LINK_MINING) {
        set_text(mine.hashing, alarm ? "waiting for the pool" : "connecting");
    } else if (m->chain_valid) {
        ui_fmt_commas(m->block_height + 1, num, sizeof(num));
        snprintf(buf, sizeof(buf), "hashing %s", num);
        set_text(mine.hashing, buf);
    } else {
        set_text(mine.hashing, "hashing the next block");
    }

    // The right column leaves 72 px beside the caption, so the empty state keeps
    // the same short shape.
    if (m->best_hash_valid)
        snprintf(buf, sizeof(buf), "#" T_HERO_HEX " %d# of #" T_ACCENT2_HEX " %d# zeros", st.zeros, st.need);
    else
        snprintf(buf, sizeof(buf), "#" T_HERO_HEX " -# of #" T_ACCENT2_HEX " %d# zeros", st.need);
    set_text(mine.best, buf);

    ui_fmt_commas(m->shares, val[0], sizeof(val[0]));
    snprintf(val[1], sizeof(val[1]), "%lu", (unsigned long)m->blocks);
    ui_fmt_si(m->best_diff, val[2], sizeof(val[2]));
    fmt_uptime(m->uptime_s, val[3], sizeof(val[3]));
    // Two columns; the second moves right when a value in the first grows long.
    int wide = 0;
    for (int i = 0; i < 4; i += 2) {
        int w = text_w(&pm_text_14, val[i]) + 4 + text_w(&pm_text_14, caps[i]);
        if (w > wide) wide = w;
    }
    int x1 = RX + (wide + 10 > 74 ? wide + 10 : 74);
    for (int i = 0; i < 4; i++) {
        int x = i % 2 ? x1 : RX, y = 147 + (i / 2) * 17;
        set_text(mine.cell_val[i], val[i]);
        lv_obj_set_pos(mine.cell_val[i], x, y);
        lv_obj_set_pos(mine.cell_cap[i], x + text_w(&pm_text_14, val[i]) + 4, y);
    }
}

/* ---- hash: the hashrate, the best hash and the live hash --------------- */

static void build_hash(void)
{
    hashp.scr = themed_screen();
    hashp.chart = area_chart(hashp.scr, RX, SCOPE_Y, W - M - RX, SCOPE_H, SPARK_POINTS, &hashp.series);
    hashp.stream = px_layer(hashp.scr, 0, 22, 160, 114, draw_stream_hero);
    hashp.head = header(hashp.scr);

    text(hashp.scr, &pm_text_14, T_DIM, M, 28, "best proof");
    hashp.proof = pm_odo_create(hashp.scr, &pm_wide_40, T_HERO);
    lv_obj_set_pos(hashp.proof, M - 2, 44);
    hashp.proof_unit = pm_odo_add_suffix(hashp.proof, &pm_text_20, T_ACCENT2, 6);
    text(hashp.scr, &pm_text_14, T_DIM, M, 88, "leading zero digits");
    hashp.proof_bar = px_layer(hashp.scr, M, 111, BAR_W, 9, draw_bar);
    text(hashp.scr, &pm_text_11, T_DIM, M, 125, "verified in software");

    text(hashp.scr, &pm_text_14, T_DIM, RX, 28, "hashrate");
    hashp.hero = pm_odo_create(hashp.scr, &pm_wide_40, T_HERO);
    lv_obj_set_pos(hashp.hero, RX - 2, 44);
    hashp.unit = pm_odo_add_suffix(hashp.hero, &pm_wide_14, T_ACCENT, 5);
    text(hashp.scr, &pm_text_14, T_DIM, RX, 116, "last 60 seconds");

    // The best hash so far, leading zeros lit, against the zeros a block needs.
    text(hashp.scr, &pm_text_14, T_DIM, M, 140, "best hash");
    hashp.hash_score = text_right(hashp.scr, &pm_text_14, T_DIM, W - M, 140, "");
    lv_label_set_recolor(hashp.hash_score, true);
    hashp.glow = rect(hashp.scr, HASH_X - 3, HASH_Y - 3, 1, 22, T_ACCENT);
    lv_obj_set_style_bg_opa(hashp.glow, LV_OPA_20, 0);
    lv_obj_set_style_radius(hashp.glow, 4, 0);
    glow(hashp.glow, 22, LV_OPA_60);
    hashp.hash = text(hashp.scr, &pm_mono_22, T_FAINT, HASH_X, HASH_Y, "");
    lv_label_set_recolor(hashp.hash, true);
    // Under the row: a rule out to where a block's zeros end, lit as far as
    // the best hash reaches, with a tick at the block line.
    hashp.block_track = rect(hashp.scr, HASH_X, HASH_Y + 19, 1, 1, T_FAINT);
    hashp.block_fill = rect(hashp.scr, HASH_X, HASH_Y + 18, 0, 2, T_ACCENT);
    glow(hashp.block_fill, 8, LV_OPA_70);
    hashp.block_mark = rect(hashp.scr, 0, HASH_Y + 16, 2, 6, T_ACCENT2);

    hashp.live = text(hashp.scr, &pm_mono_12, T_FAINT, M, 182, "");
    lv_label_set_recolor(hashp.live, true);

    hashp.shares = small_stat(hashp.scr, M, "shares");
    hashp.best = small_stat(hashp.scr, 92, "best share");
    hashp.blocks = small_stat(hashp.scr, 176, "blocks");
    hashp.uptime = small_stat(hashp.scr, 248, "uptime");
    hashp.toast = pm_toast_create(hashp.scr, W / 2, TOAST_Y, T_ACCENT, T_TOAST_FG);
}

void ui_live_hash(const uint8_t* h)
{
    char hex[LIVE_HEX + 1], buf[64];
    hash_hex(h, hex, LIVE_HEX);
    lit_zeros(buf, sizeof(buf), hex, T_ACCENT2_HEX);
    set_text(hashp.live, buf);
}

static void update_hash(const ui_model_t* m)
{
    char num[16], buf[96], hex[HASH_NIBBLES + 1];
    const char* unit;
    bool alarm = m->link == UI_LINK_NO_POOL;
    lv_color_t signal = alarm ? C_ALARM : T_ACCENT;

    set_text(hashp.head.left, m->pool);
    header_update(&hashp.head, m);

    fmt_hashrate(m->hashrate_khs, num, sizeof(num), &unit);
    odo_show(hashp.hero, num);
    set_text(hashp.unit, unit);
    pm_odo_set_color(hashp.hero, alarm ? T_DIM : T_HERO);
    if (PM_THEME == 1) set_color(hashp.unit, signal);
    set_series_color(hashp.chart, hashp.series, signal);
    snprintf(num, sizeof(num), "%d", st.zeros);
    pm_odo_set(hashp.proof, m->best_hash_valid ? num : "-", true);
    snprintf(buf, sizeof(buf), "/ %d", st.need);
    set_text(hashp.proof_unit, buf);

    int need_w = (int)(st.need * NIBBLE_W);
    lv_obj_set_width(hashp.block_track, need_w);
    lv_obj_set_x(hashp.block_mark, HASH_X + need_w - 2);

    if (m->best_hash_valid) {
        hash_hex(m->best_hash, hex, HASH_NIBBLES);
        lit_zeros(buf, sizeof(buf), hex, T_ACCENT_HEX);
        set_text(hashp.hash, buf);
        lv_obj_set_width(hashp.glow, st.zeros ? (int)(st.zeros * NIBBLE_W) + 6 : 0);
        lv_obj_set_width(hashp.block_fill, (int)(st.zeros * NIBBLE_W));
        snprintf(buf, sizeof(buf), "#" T_HERO_HEX " %d# zeros, a block needs #" T_ACCENT2_HEX " %d#",
                 st.zeros, st.need);
    } else {
        set_text(hashp.hash, "----------------------");
        lv_obj_set_width(hashp.glow, 0);
        lv_obj_set_width(hashp.block_fill, 0);
        snprintf(buf, sizeof(buf), "a block needs #" T_ACCENT2_HEX " %d# zeros", st.need);
    }
    set_text(hashp.hash_score, buf);

    if (m->live_valid) ui_live_hash(m->live_hash);
    else set_text(hashp.live, "waiting for the first job");

    ui_fmt_commas(m->shares, buf, sizeof(buf));
    set_text(hashp.shares.value, buf);
    ui_fmt_si(m->best_diff, buf, sizeof(buf));
    set_text(hashp.best.value, buf);
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)m->blocks);
    set_text(hashp.blocks.value, buf);
    fmt_uptime(m->uptime_s, buf, sizeof(buf));
    set_text(hashp.uptime.value, buf);
}

/* ---- chain: price and the difficulty period ----------------------------- */

static void draw_grid(lv_event_t* e)
{
    uint32_t el = since(st.block_at);
    pm_px_period_grid(lv_event_get_draw_ctx(e), GRID_X, GRID_Y, GRID_COLS, GRID_ROWS, GRID_PITCH,
                      st.mined, st.now, el < RIPPLE_MS ? (int32_t)el : -1);
}

// The block ripple spreads up to 26 px past the grid's own box.
static void grid_ext(lv_event_t* e)
{
    lv_event_set_ext_draw_size(e, 26);
}

static void build_chain(void)
{
    chain.scr = themed_screen();
    chain.chart = area_chart(chain.scr, RX, 82, W - M - RX, 17, PRICE_POINTS, &chain.series);
    chain.head = header(chain.scr);
    text(chain.scr, &pm_text_14, T_DIM, M, 28, "next retarget");
    chain.dollar = text(chain.scr, &pm_text_14, T_DIM, RX, 28, "bitcoin price");
    chain.price = pm_odo_create(chain.scr, &pm_wide_40, T_HERO);
    lv_obj_set_pos(chain.price, RX - 2, 44);
    chain.price_unit = pm_odo_add_suffix(chain.price, &pm_text_11, T_ACCENT, 4);
    chain.change = text(chain.scr, &pm_disp_40, T_ACCENT, M, 42, "");
    chain.change_foot = text(chain.scr, &pm_text_14, T_DIM, M, 84, "");

    kv_row(chain.scr, 104, &chain.period_key, &chain.period_val);
    lv_obj_set_style_text_color(chain.period_val, T_DIM, 0);
    chain.grid = px_layer(chain.scr, 0, 120, W, 76, draw_grid);
    lv_obj_add_event_cb(chain.grid, grid_ext, LV_EVENT_REFR_EXT_DRAW_SIZE, NULL);
    lv_obj_refresh_ext_draw_size(chain.grid);

    chain.difficulty = small_stat(chain.scr, M, "difficulty");
    chain.hashrate = small_stat(chain.scr, 92, "network H/s");
    chain.fee_fast = small_stat(chain.scr, 176, "sat/vB fast");
    chain.fee_slow = small_stat(chain.scr, 248, "sat/vB slow");
    chain.toast = pm_toast_create(chain.scr, W / 2, TOAST_Y, T_ACCENT2, T_TOAST2_FG);
}

static void price_push(double usd)
{
    static int32_t pts[PRICE_POINTS];
    static int n = 0;
    int32_t v = (int32_t)usd;
    if (n < PRICE_POINTS) pts[n++] = v;
    else { memmove(pts, pts + 1, sizeof(pts[0]) * (PRICE_POINTS - 1)); pts[PRICE_POINTS - 1] = v; }
    int32_t lo = INT32_MAX, hi = 0;
    for (int i = 0; i < n; i++) { if (pts[i] < lo) lo = pts[i]; if (pts[i] > hi) hi = pts[i]; }
    // Chart values are 16-bit: plot dollars above the window's low.
    int32_t span = hi - lo + 1;
    int32_t scale = span > 30000 ? span / 30000 + 1 : 1;
    lv_chart_set_range(chain.chart, LV_CHART_AXIS_PRIMARY_Y, -(span / scale) / 4 - 10, span / scale + 10);
    for (int i = 0; i < n; i++)
        lv_chart_set_value_by_id(chain.chart, chain.series, PRICE_POINTS - n + i, (lv_coord_t)((pts[i] - lo) / scale));
    lv_chart_refresh(chain.chart);
}

static void update_chain(const ui_model_t* m)
{
    char buf[96], num[24];
    header_update(&chain.head, m);
    set_series_color(chain.chart, chain.series, m->link == UI_LINK_NO_POOL ? C_ALARM : T_ACCENT);
    if (!m->chain_valid) {
        set_text(chain.head.left, "bitcoin");
        pm_odo_set(chain.price, "-", false);
        set_text(chain.price_unit, "");
        set_text(chain.change, "...");
        set_text(chain.change_foot, "fetching secure feed");
        set_text(chain.period_key, m->chain_enabled ? "waiting for mempool.space" : "chain data is off");
        set_text(chain.period_val, m->chain_enabled ? "" : "turn it on in setup");
        lv_obj_add_flag(chain.grid, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(chain.chart, LV_OBJ_FLAG_HIDDEN);
        set_text(chain.difficulty.value, "-");
        set_text(chain.hashrate.value, "-");
        set_text(chain.fee_fast.value, "-");
        set_text(chain.fee_slow.value, "-");
        return;
    }
    lv_obj_clear_flag(chain.grid, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(chain.chart, LV_OBJ_FLAG_HIDDEN);
    ui_fmt_commas(m->block_height, num, sizeof(num));
    snprintf(buf, sizeof(buf), "bitcoin at block %s", num);
    set_text(chain.head.left, buf);

    // Keep the price in the same compact column as Mine's hashrate.
    if (m->price_usd >= 999500.0) {
        snprintf(num, sizeof(num), "%.2f", m->price_usd / 1000000.0);
        set_text(chain.price_unit, "M USD");
    } else if (m->price_usd >= 1000.0) {
        snprintf(num, sizeof(num), m->price_usd >= 99950.0 ? "%.0f" : "%.1f", m->price_usd / 1000.0);
        set_text(chain.price_unit, "k USD");
    } else {
        ui_fmt_commas((uint32_t)(m->price_usd + 0.5), num, sizeof(num));
        set_text(chain.price_unit, "USD");
    }
    odo_show(chain.price, num);
    if (m->price_usd != chain.last_price) {
        chain.last_price = m->price_usd;
        price_push(m->price_usd);
    }

    snprintf(buf, sizeof(buf), "%+.1f %%", m->retarget_change_pct);
    set_text(chain.change, buf);
    set_color(chain.change, m->retarget_change_pct >= 0 ? T_ACCENT : T_ACCENT2);
    ui_fmt_commas(m->retarget_blocks, num, sizeof(num));
    snprintf(buf, sizeof(buf), "in %s blocks", num);
    set_text(chain.change_foot, buf);

    snprintf(buf, sizeof(buf), "period %.0f %%", m->retarget_progress);
    set_text(chain.period_key, buf);
    ui_fmt_commas((uint32_t)st.mined, num, sizeof(num));
    snprintf(buf, sizeof(buf), "#" T_HERO_HEX " %s# of 2,016 blocks", num);
    set_text(chain.period_val, buf);

    ui_fmt_si(m->net_difficulty, buf, sizeof(buf));
    set_text(chain.difficulty.value, buf);
    ui_fmt_si(m->net_hashrate, buf, sizeof(buf));
    set_text(chain.hashrate.value, buf);
    snprintf(buf, sizeof(buf), "%d", m->fee_fast);
    set_text(chain.fee_fast.value, buf);
    snprintf(buf, sizeof(buf), "%d", m->fee_eco);
    set_text(chain.fee_slow.value, buf);
}

/* ---- clock -------------------------------------------------------------- */

static void build_clock(void)
{
    clock_.scr = themed_screen();
    clock_.stream = px_layer(clock_.scr, 0, 22, 160, 78, draw_stream_hero);
    clock_.head = header(clock_.scr);
    text(clock_.scr, &pm_text_14, T_DIM, M, 28, "local time");
    text(clock_.scr, &pm_text_14, T_DIM, RX, 28, "seconds");
    // Four wide zeros are the largest HH:MM. At 32 px they keep the gutter
    // clear even at midnight, while the seconds retain Mine's 40 px size.
    clock_.hero = pm_odo_create(clock_.scr, &pm_wide_32, T_HERO);
    lv_obj_set_pos(clock_.hero, M - 2, 50);
    clock_.seconds = pm_odo_create(clock_.scr, &pm_wide_40, T_ACCENT);
    lv_obj_set_pos(clock_.seconds, RX - 2, 44);
    text(clock_.scr, &pm_text_14, T_DIM, M, 92, "this minute");
    clock_.minute_fill = track(clock_.scr, 112);

    kv_row(clock_.scr, 146, &clock_.row_key[0], &clock_.row_val[0]);
    kv_row(clock_.scr, 168, &clock_.row_key[1], &clock_.row_val[1]);
    set_text(clock_.row_key[0], "pool");
    set_text(clock_.row_key[1], "wifi and chip");

    clock_.hashrate = small_stat(clock_.scr, M, "kH/s");
    clock_.shares = small_stat(clock_.scr, 92, "shares");
    clock_.best = small_stat(clock_.scr, 176, "best share");
    clock_.uptime = small_stat(clock_.scr, 248, "uptime");
}

static void update_clock(const ui_model_t* m)
{
    char buf[96];
    header_update(&clock_.head, m);
    if (m->time_valid) {
        snprintf(buf, sizeof(buf), "%02d:%02d", m->hour, m->minute);
        odo_show(clock_.hero, buf);
        snprintf(buf, sizeof(buf), "%02d", m->second);
        pm_odo_set(clock_.seconds, buf, false);
        track_set(clock_.minute_fill, m->second / 59.0f);
        snprintf(buf, sizeof(buf), "%s %d %s", WEEKDAY[m->weekday % 7], m->day,
                 MONTH[(m->month + 11) % 12]);
        set_text(clock_.head.left, buf);
    } else {
        pm_odo_set(clock_.hero, "--:--", false);
        pm_odo_set(clock_.seconds, "--", false);
        track_set(clock_.minute_fill, 0);
        set_text(clock_.head.left, "waiting for network time");
    }

    set_text(clock_.row_val[0], m->pool);
    snprintf(buf, sizeof(buf), "%d dBm   %.0f \xC2\xB0" "C", m->rssi_dbm, m->temp_c);
    set_text(clock_.row_val[1], buf);

    snprintf(buf, sizeof(buf), "%.0f", m->hashrate_khs);
    set_text(clock_.hashrate.value, buf);
    ui_fmt_commas(m->shares, buf, sizeof(buf));
    set_text(clock_.shares.value, buf);
    ui_fmt_si(m->best_diff, buf, sizeof(buf));
    set_text(clock_.best.value, buf);
    fmt_uptime(m->uptime_s, buf, sizeof(buf));
    set_text(clock_.uptime.value, buf);
}

/* ---- boot and setup ----------------------------------------------------- */

#define BOOT_STEPS 6               /* the lines pm_loading prints */

static void draw_boot_cube(lv_event_t* e)
{
    float done = boot.steps / (float)BOOT_STEPS;
    pm_cube_t c = {CUBE_FACES, 4, done > 1 ? 1.0f : done, 40, 0, 255};
    pm_px_cube(lv_event_get_draw_ctx(e), BLOCK_CX, BLOCK_Y, 42, 45, &c);
}

static void build_boot(void)
{
    boot.scr = themed_screen();
    boot.head = header(boot.scr);
    set_text(boot.head.left, "starting up");
    set_text(boot.head.state, "booting");
    lv_obj_align_to(boot.head.dot, boot.head.state, LV_ALIGN_OUT_LEFT_MID, -7, 1);
    text(boot.scr, &pm_text_14, T_DIM, RX, 28, "owned firmware");
    text(boot.scr, &pm_text_20, T_HERO, RX, 49, "panicminer");
    rect(boot.scr, RX, SCOPE_Y, BAR_W, 3, T_FAINT);
    boot.progress = rect(boot.scr, RX, SCOPE_Y, 1, 3, T_ACCENT);
    glow(boot.progress, 10, LV_OPA_70);
    text(boot.scr, &pm_text_11, T_DIM, RX, 100, "bringing the block online");
    for (int i = 0; i < LOG_LINES; i++)
        boot.lines[i] = text(boot.scr, &pm_mono_12, T_FAINT, M, 138 + 13 * i, "");
    text(boot.scr, &pm_text_14, T_DIM, M, 220, "hold BOOT for setup");
    boot.cube = px_layer(boot.scr, 0, 22, 160, 112, draw_boot_cube);
}

void ui_boot_log(const char* line)
{
    if (boot.count == LOG_LINES) {
        for (int i = 0; i < LOG_LINES - 1; i++)
            lv_label_set_text(boot.lines[i], lv_label_get_text(boot.lines[i + 1]));
        boot.count--;
    }
    if (boot.count > 0)
        lv_obj_set_style_text_color(boot.lines[boot.count - 1], T_DIM, 0);
    lv_label_set_text(boot.lines[boot.count], line);
    lv_obj_set_style_text_color(boot.lines[boot.count], PM_THEME == 1 ? T_HERO : C_PAPER, 0);
    boot.count++;
    if (boot.steps < BOOT_STEPS) boot.steps++;
    lv_obj_set_width(boot.progress, BAR_W * boot.steps / BOOT_STEPS);
    lv_obj_invalidate(boot.cube);
}

void ui_show_boot(void)
{
    lv_scr_load(boot.scr);
}

void ui_show_setup(const char* ap_name, const char* ap_password, const char* ap_ip)
{
    // LVGL must have a live active screen while the replacement is built.
    // The portal can reopen after a failed Wi-Fi join.
    lv_obj_t* previous = setup.scr;
    if (previous) {
        lv_scr_load(boot.scr);
        lv_obj_del(previous);
    }
    setup.scr = themed_screen();
    header_t h = header(setup.scr);
    set_text(h.left, "set up this miner");
    set_text(h.state, "setup");
    lv_obj_align_to(h.dot, h.state, LV_ALIGN_OUT_LEFT_MID, -7, 1);

    // The code sits on an accent tile that glows; the tile is its quiet zone.
    lv_obj_t* tile = rect(setup.scr, M, 40, 136, 136, T_ACCENT);
    lv_obj_set_style_radius(tile, 8, 0);
    glow(tile, 24, LV_OPA_50);
    lv_obj_t* qr = lv_qrcode_create(tile, 120, C_INK, T_ACCENT);
    lv_obj_set_pos(qr, 8, 8);
    char payload[128];
    snprintf(payload, sizeof(payload), "WIFI:T:WPA;S:%s;P:%s;;", ap_name, ap_password);
    lv_qrcode_update(qr, payload, strlen(payload));
    text(setup.scr, &pm_text_11, T_DIM, M + 37, 181, "scan to join");

    const int x = 164;
    lv_color_t value = PM_THEME == 1 ? T_HERO : C_PAPER;
    text(setup.scr, &pm_text_14, T_DIM, x, 40, "1  join this Wi-Fi");
    text(setup.scr, &pm_text_20, value, x, 56, ap_name);
    text(setup.scr, &pm_text_14, T_DIM, x, 90, "2  password");
    text(setup.scr, &pm_text_20, value, x, 106, ap_password);
    text(setup.scr, &pm_text_14, T_DIM, x, 140, "3  open");
    text(setup.scr, &pm_text_20, T_ACCENT, x, 156, ap_ip);
    rect(setup.scr, M, 195, W - 2 * M, 1, T_FAINT);
    text(setup.scr, &pm_text_14, T_DIM, M, 200, "Scan the code with a phone camera, then pick");
    text(setup.scr, &pm_text_14, T_DIM, M, 218, "your home network and wallet on that page.");
    lv_scr_load(setup.scr);
}

/* ---- stats: measured work, rate windows and hardware health -------------- */

static void build_stats(void)
{
    static const char* caps[] = {"10s / 60s kH/s", "accepted / rejected",
        "best / pool difficulty", "CPU 0 / CPU 1", "RAM used", "chip temp / Wi-Fi",
        "SD statistics", "SHA checked / bad", "pending wallet sats", "wallet / pool"};
    stats.scr = themed_screen();
    stats.head = header(stats.scr);
    for (int i = 0; i < 2; i++) {
        int x = i ? RX : M;
        text(stats.scr, &pm_text_14, T_DIM, x, 31, i ? "wallet USD value" : "wallet BTC balance");
        stats.rate[i] = text(stats.scr, &pm_mono_22, T_HERO, x, 51, "loading");
    }
    rect(stats.scr, M, 77, W - 2 * M, 1, T_FAINT);
    for (int i = 0; i < 10; i++) {
        int x = i % 2 ? RX : M, y = 82 + (i / 2) * 30;
        text(stats.scr, &pm_text_11, T_DIM, x, y, caps[i]);
        stats.value[i] = text(stats.scr, &pm_text_14, T_VALUE, x, y + 12, "-");
    }
}

static void update_stats(const ui_model_t* m)
{
    char buf[64], a[16], b[16];
    set_text(stats.head.left, "miner statistics  5/5");
    header_update(&stats.head, m);
    if (m->wallet_valid) snprintf(buf, sizeof(buf), "%lld.%08lld",
        (long long)(m->wallet_sats / 100000000), (long long)(m->wallet_sats % 100000000));
    else snprintf(buf, sizeof(buf), "%s", !m->wallet_set ? "not set" : m->wallet_attempted ? "unavailable" : "loading");
    set_text(stats.rate[0], buf);
    if (m->wallet_valid && m->chain_valid) snprintf(buf, sizeof(buf), "%.2f", m->wallet_sats / 100000000.0 * m->price_usd);
    else snprintf(buf, sizeof(buf), "%s", m->wallet_attempted ? "unavailable" : "loading");
    set_text(stats.rate[1], buf);
    snprintf(buf, sizeof(buf), "%.0f / %.0f", m->hashrate_khs, m->hashrate_60s_khs);
    set_text(stats.value[0], buf);
    snprintf(buf, sizeof(buf), "%lu / %lu", (unsigned long)m->shares, (unsigned long)m->rejected);
    set_text(stats.value[1], buf);
    ui_fmt_si(m->best_diff, a, sizeof(a));
    ui_fmt_si(m->pool_diff, b, sizeof(b));
    snprintf(buf, sizeof(buf), "%s / %s", a, b);
    set_text(stats.value[2], buf);
    if (m->cpu_valid) snprintf(buf, sizeof(buf), "%.0f%% / %.0f%%", m->cpu_pct[0], m->cpu_pct[1]);
    else snprintf(buf, sizeof(buf), "measuring");
    set_text(stats.value[3], buf);
    uint32_t used = m->heap_total > m->heap_free ? m->heap_total - m->heap_free : 0;
    if (m->heap_total) snprintf(buf, sizeof(buf), "%.0f%%  %lu/%lu KB", 100.0f * used / m->heap_total,
        (unsigned long)(used / 1024), (unsigned long)(m->heap_total / 1024));
    else snprintf(buf, sizeof(buf), "measuring");
    set_text(stats.value[4], buf);
    snprintf(buf, sizeof(buf), "%.0f C / %d dBm", m->temp_c, m->rssi_dbm);
    set_text(stats.value[5], buf);
    if (m->sd_ready) snprintf(buf, sizeof(buf), "%lu rows saved", (unsigned long)m->sd_rows);
    else snprintf(buf, sizeof(buf), "unavailable");
    set_text(stats.value[6], buf);
    ui_fmt_si(m->sha_checked, a, sizeof(a));
    ui_fmt_si(m->sha_mismatched, b, sizeof(b));
    snprintf(buf, sizeof(buf), "%s / %s", a, b);
    set_text(stats.value[7], buf);
    set_color(stats.value[7], m->sha_mismatched ? C_ALARM : T_VALUE);
    if (m->wallet_valid) snprintf(buf, sizeof(buf), "%+lld", (long long)m->wallet_pending_sats);
    else snprintf(buf, sizeof(buf), "loading");
    set_text(stats.value[8], buf);
    snprintf(buf, sizeof(buf), "%s", !m->wallet_set ? "not configured" :
        m->wallet_valid && !m->wallet_fresh ? "stale balance" :
        m->link == UI_LINK_MINING ? "set / authorized" : "set / connecting");
    set_text(stats.value[9], buf);
}

/* ---- motion and pages --------------------------------------------------- */

// Redraws only what moves on the page on screen: the cube and hash stream
// every frame on Mine, the streams every other frame on Hash and Clock, the
// next-block blink on Chain, and the block slide and ripple while they run.
static void motion_cb(lv_timer_t* t)
{
    uint32_t now = lv_tick_get();
    uint32_t dt = now - st.last_ms;
    st.last_ms = now;
    st.now = now;
    // Every effect is over within a minute; an old stamp is forgotten so
    // since() never sees a difference past 2^31 ms.
    if (since(st.share_at) > 60000) st.share_at = 0;
    if (since(st.block_at) > 60000) st.block_at = 0;
    if (dt > 500) dt = 0;
    // kH/s is hashes per ms; one sweep of the scan line is 2^22 nonces.
    st.scan += dt * st.khs / 4194304.0f;
    st.scan -= floorf(st.scan);
    st.frame++;

    switch (s_page) {
        case UI_PAGE_MINE:
            lv_obj_invalidate(mine.block);
            if (since(st.block_at) < STRIP_MS + 2 * FRAME_MS) lv_obj_invalidate(mine.strip);
            break;
        case UI_PAGE_HASH:
            if (st.frame % 2 == 0) lv_obj_invalidate(hashp.stream);
            break;
        case UI_PAGE_CHAIN:
            if (since(st.block_at) < RIPPLE_MS + 2 * FRAME_MS) {
                lv_obj_invalidate(chain.grid);
            } else {
                lv_area_t a;
                pm_px_period_next_area(GRID_X, GRID_Y, GRID_COLS, GRID_PITCH, st.mined, &a);
                lv_obj_invalidate_area(chain.grid, &a);
            }
            break;
        case UI_PAGE_CLOCK:
            if (st.frame % 2 == 0) lv_obj_invalidate(clock_.stream);
            break;
        default:
            break;
    }
}

static ui_model_t s_latest_model;
static bool s_have_model;

static void update_page(ui_page_t p, const ui_model_t* m)
{
    switch (p) {
        case UI_PAGE_HASH: update_hash(m); break;
        case UI_PAGE_CHAIN: update_chain(m); break;
        case UI_PAGE_CLOCK: update_clock(m); break;
        case UI_PAGE_STATS: update_stats(m); break;
        default: update_mine(m); break;
    }
}

static lv_obj_t* page_screen(ui_page_t p)
{
    switch (p) {
        case UI_PAGE_HASH: return hashp.scr;
        case UI_PAGE_CHAIN: return chain.scr;
        case UI_PAGE_CLOCK: return clock_.scr;
        case UI_PAGE_STATS: return stats.scr;
        default: return mine.scr;
    }
}

void ui_set_page(ui_page_t page)
{
    s_page = page;
    if (s_have_model) update_page(page, &s_latest_model);
    lv_scr_load(page_screen(page));
}

void ui_next_page(void)
{
    ui_set_page((ui_page_t)((s_page + 1) % UI_PAGE_COUNT));
}

ui_page_t ui_current_page(void)
{
    return s_page;
}

void ui_init(void)
{
#if PM_THEME == 2
    tape_paint(T_ACCENT);
#endif
    st.need = 20;
    build_boot();
    build_mine();
    build_hash();
    build_chain();
    build_clock();
    build_stats();
    st.now = st.last_ms = lv_tick_get();
    lv_timer_create(motion_cb, FRAME_MS, NULL);
}

// Shares and blocks that arrived since the last update: toasts, the cube's
// glow, the strip slide and the ripple.
static void moments(const ui_model_t* m)
{
    char buf[40], num[16];
    if (st.have_shares && m->shares > st.shares) {
        uint32_t n = m->shares - st.shares;
        st.share_at = stamp();
        if (n == 1) snprintf(buf, sizeof(buf), "+1 share accepted");
        else snprintf(buf, sizeof(buf), "+%lu shares accepted", (unsigned long)n);
        pm_toast_show(mine.toast, buf);
        pm_toast_show(hashp.toast, buf);
        pm_flash(mine.cell_val[0], T_ACCENT, T_VALUE, 700);
        pm_flash(hashp.shares.value, T_ACCENT, T_VALUE, 700);
        pm_flash(clock_.shares.value, T_ACCENT, T_VALUE, 700);
    }
    st.shares = m->shares;
    st.have_shares = true;

    // Heights only move forward: a lagging feed backend can answer one block
    // behind, and taking that would replay the newest block's toast and slide.
    if (m->chain_valid && m->block_height > st.height) {
        if (st.height) {
            st.block_at = stamp();
            ui_fmt_commas(m->block_height, num, sizeof(num));
            snprintf(buf, sizeof(buf), "new block %s", num);
            pm_toast_show(chain.toast, buf);
            pm_flash(chain.head.left, T_ACCENT2, T_DIM, 900);
        }
        st.height = m->block_height;
        lv_obj_invalidate(mine.strip);
    }
}

void ui_update(const ui_model_t* m)
{
    s_latest_model = *m;
    s_have_model = true;
    moments(m);
    st.khs = m->link == UI_LINK_MINING ? m->hashrate_khs : 0.0f;
    st.mining = m->link == UI_LINK_MINING;
    // The strip redraws only on a slide or here, when the labels it shows change.
    if (m->chain_valid != st.chain_valid || m->fee_fast != st.fee[0] ||
        m->fee_hour != st.fee[1] || m->fee_eco != st.fee[2])
        lv_obj_invalidate(mine.strip);
    st.chain_valid = m->chain_valid;
    st.fee[0] = m->fee_fast;
    st.fee[1] = m->fee_hour;
    st.fee[2] = m->fee_eco;

    // Leading zero bits of a hash at difficulty d: 32 + log2(d).
    double net = m->chain_valid && m->net_difficulty > 1 ? m->net_difficulty : 1e14;
    int need = (int)ceil((32.0 + log2(net)) / 4.0);
    int zeros = 0;
    if (m->best_hash_valid) {
        char hex[HASH_NIBBLES + 1];
        hash_hex(m->best_hash, hex, HASH_NIBBLES);
        zeros = zero_nibbles(hex);
    }
    if (need != st.need || zeros != st.zeros) {
        st.need = need;
        st.zeros = zeros;
        lv_obj_invalidate(mine.bar);
        lv_obj_invalidate(hashp.proof_bar);
    }
    int mined = m->chain_valid ? 2016 - (int)m->retarget_blocks : 0;
    mined = mined < 0 ? 0 : mined > 2015 ? 2015 : mined;
    if (mined != st.mined) {
        st.mined = mined;
        lv_obj_invalidate(chain.grid);
    }

    spark_push(m->hashrate_khs);
    // Keep measurements/history for every page, but only format and animate
    // labels on the visible one. Page changes use the cached current model.
    update_page(s_page, m);
#if PM_THEME == 2
    lv_color_t signal = m->link == UI_LINK_NO_POOL ? C_ALARM : T_ACCENT;
    if (s_tape_color.full != signal.full) {
        tape_paint(signal);
        lv_obj_invalidate(lv_scr_act());
    }
#endif
}
