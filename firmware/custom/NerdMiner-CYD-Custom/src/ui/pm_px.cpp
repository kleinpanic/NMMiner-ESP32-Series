// Pixel drawing for the panicminer pages, see pm_px.h. Every shape is a pure
// function of the pixel position, so LVGL can render the screen in strips of
// any height and the strips join without seams. Pixels belong to a shape when
// their centre lies inside it (half-open), which makes 2:1 edges step exactly
// 2 px per row and lets faces and tiles meet without gaps or double blends.
#include "pm_px.h"

#include <math.h>

#define C_PINK   0xFF3D7F
#define C_ORANGE 0xFFA24A
#define C_SCAN   0xFFD3A6
#define C_HERO   0xF7F3F8
#define C_UNMINED 0x2A2230

#define HEX_PITCH_X 9
#define HEX_PITCH_Y 12
#define HEX_FADE 26
#define GLYPH_W 8
#define GLYPH_H 13

LV_FONT_DECLARE(pm_mono_12)

/* ---- buffer access ---------------------------------------------------- */

static void hspan(lv_draw_ctx_t* ctx, int x1, int x2, int y, lv_color_t c, lv_opa_t opa)
{
    const lv_area_t* clip = ctx->clip_area;
    if (opa == 0 || y < clip->y1 || y > clip->y2) return;
    if (x1 < clip->x1) x1 = clip->x1;
    if (x2 > clip->x2) x2 = clip->x2;
    if (x1 > x2) return;
    const lv_area_t* ba = ctx->buf_area;
    lv_color_t* p = (lv_color_t*)ctx->buf + (y - ba->y1) * lv_area_get_width(ba) + (x1 - ba->x1);
    // lv_color_mix rounds opa to 1/32 steps, so >= LV_OPA_MAX is already a plain store.
    if (opa >= LV_OPA_MAX) {
        for (int n = x2 - x1; n >= 0; n--) *p++ = c;
        return;
    }
    for (int n = x2 - x1; n >= 0; n--, p++) *p = lv_color_mix(c, *p, opa);
}

static void fill_rect(lv_draw_ctx_t* ctx, int x1, int y1, int x2, int y2, lv_color_t c, lv_opa_t opa)
{
    if (y1 < ctx->clip_area->y1) y1 = ctx->clip_area->y1;
    if (y2 > ctx->clip_area->y2) y2 = ctx->clip_area->y2;
    for (int y = y1; y <= y2; y++) hspan(ctx, x1, x2, y, c, opa);
}

static lv_opa_t opa_f(float a)
{
    if (a <= 0.0f) return 0;
    if (a >= 1.0f) return 255;
    return (lv_opa_t)(a * 255.0f + 0.5f);
}

// ceilf is a libcall on the ESP32 and runs twice per span row.
static inline int iceil(float v)
{
    const int i = (int)v;
    return i + (v > (float)i);
}

static uint32_t hx(uint32_t a, uint32_t b)
{
    uint32_t h = a * 374761393u + b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* ---- convex polygons by row span ------------------------------------- */

// k[e] is the edge's dx/dy, set by the caller: every edge of a cube is parallel
// to one of two face axes, so the slopes are computed once per face, not per tile.
typedef struct {
    float x[6], y[6], k[6];
    int n;
} poly_t;

static void poly_slopes(poly_t* p)
{
    for (int e = 0; e < p->n; e++) {
        int f = e + 1 == p->n ? 0 : e + 1;
        float dy = p->y[f] - p->y[e];
        p->k[e] = dy != 0.0f ? (p->x[f] - p->x[e]) / dy : 0.0f;
    }
}

static void poly_rows(const poly_t* p, int* y1, int* y2)
{
    float lo = p->y[0], hi = p->y[0];
    for (int e = 1; e < p->n; e++) {
        if (p->y[e] < lo) lo = p->y[e];
        if (p->y[e] > hi) hi = p->y[e];
    }
    *y1 = iceil(lo - 0.5f);
    *y2 = iceil(hi - 0.5f) - 1;
}

// Pixels of row y whose centre lies inside; false when none do.
static bool poly_span(const poly_t* p, int y, int* xl, int* xr)
{
    float yc = y + 0.5f, lo = 1e9f, hi = -1e9f;
    for (int e = 0; e < p->n; e++) {
        int f = e + 1 == p->n ? 0 : e + 1;
        if ((yc >= p->y[e]) == (yc >= p->y[f])) continue;
        float x = p->x[e] + (yc - p->y[e]) * p->k[e];
        if (x < lo) lo = x;
        if (x > hi) hi = x;
    }
    if (lo > hi) return false;
    *xl = iceil(lo - 0.5f);
    *xr = iceil(hi - 0.5f) - 1;
    return *xl <= *xr;
}

static void poly_fill(lv_draw_ctx_t* ctx, const poly_t* p, lv_color_t c, lv_opa_t opa)
{
    int y1, y2, xl, xr;
    if (opa == 0) return;
    poly_rows(p, &y1, &y2);
    if (y1 < ctx->clip_area->y1) y1 = ctx->clip_area->y1;
    if (y2 > ctx->clip_area->y2) y2 = ctx->clip_area->y2;
    for (int y = y1; y <= y2; y++)
        if (poly_span(p, y, &xl, &xr)) hspan(ctx, xl, xr, y, c, opa);
}

// Span of the shape grown by d px in every direction (Chebyshev), from the
// rows within d of y. Convex shapes only.
static bool grown_span(const poly_t* p, int y, int d, int* xl, int* xr)
{
    int lo = 1 << 20, hi = -(1 << 20), a, b;
    for (int k = -d; k <= d; k++) {
        if (!poly_span(p, y + k, &a, &b)) continue;
        if (a < lo) lo = a;
        if (b > hi) hi = b;
    }
    if (lo > hi) return false;
    *xl = lo - d;
    *xr = hi + d;
    return true;
}

// Ring d of a convex shape: d = 0 is its inner 4-connected boundary, d > 0
// the pixels the shape gains when grown from d-1 to d.
static void poly_ring(lv_draw_ctx_t* ctx, const poly_t* p, int d, lv_color_t c, lv_opa_t opa)
{
    int y1, y2;
    if (opa == 0) return;
    poly_rows(p, &y1, &y2);
    y1 -= d;
    y2 += d;
    if (y1 < ctx->clip_area->y1) y1 = ctx->clip_area->y1;
    if (y2 > ctx->clip_area->y2) y2 = ctx->clip_area->y2;
    for (int y = y1; y <= y2; y++) {
        int al, ar, bl, br;
        if (d == 0) {
            int ul, ur, dl, dr;
            if (!poly_span(p, y, &al, &ar)) continue;
            if (!poly_span(p, y - 1, &ul, &ur) || !poly_span(p, y + 1, &dl, &dr)) {
                hspan(ctx, al, ar, y, c, opa);
                continue;
            }
            bl = (ul > dl ? ul : dl);
            br = (ur < dr ? ur : dr);
            if (bl < al + 1) bl = al + 1;
            if (br > ar - 1) br = ar - 1;
        } else {
            if (!grown_span(p, y, d, &al, &ar)) continue;
            if (!grown_span(p, y, d - 1, &bl, &br)) {
                hspan(ctx, al, ar, y, c, opa);
                continue;
            }
        }
        if (bl > br) {
            hspan(ctx, al, ar, y, c, opa);
        } else {
            hspan(ctx, al, bl - 1, y, c, opa);
            hspan(ctx, br + 1, ar, y, c, opa);
        }
    }
}

/* ---- cube ------------------------------------------------------------- */

static void quad(poly_t* p, float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy)
{
    p->n = 4;
    p->x[0] = ax; p->y[0] = ay;
    p->x[1] = bx; p->y[1] = by;
    p->x[2] = cx; p->y[2] = cy;
    p->x[3] = dx; p->y[3] = dy;
}

// First row of the side faces in column x: the top rhombus ends just above it.
static int side_top_row(int cx, int by, int x)
{
    float xc = x + 0.5f - cx;
    if (xc < 0.0f) xc = -xc;
    return iceil(by - 0.5f * xc - 0.5f);
}

void pm_px_cube(lv_draw_ctx_t* ctx, int cx, int y0, int w, int h, const pm_cube_t* c)
{
    const lv_area_t* clip = ctx->clip_area;
    if (c->opa == 0 || w <= 0 || h <= 0) return;
    if (cx + w + 4 < clip->x1 || cx - w - 4 > clip->x2) return;
    if (y0 + w + h + 4 < clip->y1 || y0 - 4 > clip->y2) return;

    const float A = c->opa / 255.0f;
    const float Tx = cx, Ty = y0, Rx = cx + w, Ry = y0 + 0.5f * w, Bx = cx, By = y0 + w;
    const float Lx = cx - w, Ly = Ry;
    poly_t top, left, right;
    quad(&top, Tx, Ty, Rx, Ry, Bx, By, Lx, Ly);
    quad(&left, Lx, Ly, Bx, By, Bx, By + h, Lx, Ly + h);
    quad(&right, Bx, By, Rx, Ry, Rx, Ry + h, Bx, By + h);
    poly_slopes(&top);
    poly_slopes(&left);
    poly_slopes(&right);
    const lv_color_t face[3] = {lv_color_hex(c->face[0]), lv_color_hex(c->face[1]), lv_color_hex(c->face[2])};
    poly_fill(ctx, &left, face[1], c->opa);
    poly_fill(ctx, &right, face[2], c->opa);
    poly_fill(ctx, &top, face[0], c->opa);

    const int n = c->tiles;
    if (n > 0) {
        // Face f(u, v) = origin + u*U + v*V; kU, kV are the axes' dx/dy.
        const float ox[3] = {Tx, Lx, Bx}, oy[3] = {Ty, Ly, By};
        const float ux[3] = {Rx - Tx, Bx - Lx, Rx - Bx}, uy[3] = {Ry - Ty, By - Ly, Ry - By};
        const float vx[3] = {Lx - Tx, 0.0f, 0.0f}, vy[3] = {Ly - Ty, (float)h, (float)h};
        const float shade[3] = {1.0f, 0.72f, 0.5f};
        const float kin = 0.14f, inv = 1.0f / n;
        const lv_color_t pink = lv_color_hex(C_PINK), orange = lv_color_hex(C_ORANGE);
        for (int fi = 0; fi < 3; fi++) {
            const float kU = ux[fi] / uy[fi], kV = vx[fi] / vy[fi];
            for (int j = 0; j < n; j++) {
                const float v0 = (j + kin) * inv, v1 = (j + 1 - kin) * inv;
                // Rows only grow with v on every face, so whole tile rows can be skipped.
                const float rowtop = oy[fi] + v0 * vy[fi] + (uy[fi] < 0 ? uy[fi] : 0.0f);
                const float rowbot = oy[fi] + v1 * vy[fi] + (uy[fi] > 0 ? uy[fi] : 0.0f);
                if (rowbot < clip->y1 || rowtop > clip->y2 + 1) continue;
                const bool scanned = fi > 0 && (j + 0.5f) * inv < c->scan;
                for (int i = 0; i < n; i++) {
                    const float u0 = (i + kin) * inv, u1 = (i + 1 - kin) * inv;
                    const uint32_t r = hx(fi * n * n + i * n + j, 7) % 1000;
                    const uint32_t r2 = hx(fi * 97 + i * 13 + j * 7, 7) % 1000;
                    float a = (0.18f + 0.0005f * r2) * shade[fi] * (scanned ? 1.55f : (fi > 0 ? 0.55f : 1.0f));
                    if (a > 1.0f) a = 1.0f;
                    poly_t t;
                    quad(&t, ox[fi] + u0 * ux[fi] + v0 * vx[fi], oy[fi] + u0 * uy[fi] + v0 * vy[fi],
                         ox[fi] + u1 * ux[fi] + v0 * vx[fi], oy[fi] + u1 * uy[fi] + v0 * vy[fi],
                         ox[fi] + u1 * ux[fi] + v1 * vx[fi], oy[fi] + u1 * uy[fi] + v1 * vy[fi],
                         ox[fi] + u0 * ux[fi] + v1 * vx[fi], oy[fi] + u0 * uy[fi] + v1 * vy[fi]);
                    t.k[0] = kU;
                    t.k[1] = kV;
                    t.k[2] = kU;
                    t.k[3] = kV;
                    const lv_color_t tc = r < 620 ? pink : orange;
                    // An opaque cube's face pixels are exactly the face colour, so
                    // the tile can be pre-mixed once and stored instead of blended.
                    if (c->opa == 255) poly_fill(ctx, &t, lv_color_mix(tc, face[fi], opa_f(a)), LV_OPA_COVER);
                    else poly_fill(ctx, &t, tc, opa_f(a * A));
                }
            }
        }

        const int sy = (int)(c->scan * h + 0.5f);
        if (c->scan >= 0.0f && sy < h) {
            // The line is the side faces' top boundary moved sy rows down, so it
            // stays parallel to the tile rows; the halo stays on the side faces.
            const lv_color_t core = lv_color_hex(C_SCAN);
            const lv_opa_t oc = opa_f(A), o1 = opa_f(0.55f * A), o2 = opa_f(0.22f * A);
            int x1 = cx - w, x2 = cx + w - 1;
            if (x1 < clip->x1) x1 = clip->x1;
            if (x2 > clip->x2) x2 = clip->x2;
            for (int x = x1; x <= x2; x++) {
                const int top_row = side_top_row(cx, (int)By, x), y = top_row + sy;
                for (int d = -2; d <= 2; d++) {
                    const int yy = y + d;
                    if (yy < top_row || yy >= top_row + h) continue;
                    if (d == 0) hspan(ctx, x, x, yy, core, oc);
                    else hspan(ctx, x, x, yy, orange, d == 1 || d == -1 ? o1 : o2);
                }
            }
        }
    }

    const lv_opa_t eo = opa_f(c->edge_opa / 255.0f * A);
    if (eo) {
        const lv_color_t white = lv_color_hex(0xFFFFFF);
        const int fy = side_top_row(cx, (int)By, cx);
        poly_ring(ctx, &top, 0, white, eo);
        fill_rect(ctx, cx, fy, cx, fy + h - 1, white, eo);
    }

    const float g = c->glow_opa / 255.0f * A;
    if (g > 0.0f) {
        poly_t sil;
        sil.n = 6;
        sil.x[0] = Tx; sil.y[0] = Ty;
        sil.x[1] = Rx; sil.y[1] = Ry;
        sil.x[2] = Rx; sil.y[2] = Ry + h;
        sil.x[3] = Bx; sil.y[3] = By + h;
        sil.x[4] = Lx; sil.y[4] = Ly + h;
        sil.x[5] = Lx; sil.y[5] = Ly;
        poly_slopes(&sil);
        const lv_color_t pink = lv_color_hex(C_PINK);
        const float fall[4] = {1.0f, 0.62f, 0.3f, 0.12f};
        for (int d = 0; d < 4; d++) poly_ring(ctx, &sil, d, pink, opa_f(g * fall[d]));
    }
}

/* ---- hex stream ------------------------------------------------------- */

static struct {
    bool ready;
    int8_t dx[16], dy[16], w[16], h[16];
    uint8_t a[16][GLYPH_W * GLYPH_H];
} s_glyph;

// 4 bpp, rows packed back to back with no byte alignment (lv_font_fmt_txt, uncompressed).
static void glyph_cache(void)
{
    static const char HEX[] = "0123456789abcdef";
    const lv_font_t* f = &pm_mono_12;
    for (int g = 0; g < 16; g++) {
        lv_font_glyph_dsc_t d;
        s_glyph.w[g] = 0;
        if (!lv_font_get_glyph_dsc(f, &d, HEX[g], 0) || d.bpp != 4) continue;
        const uint8_t* bm = lv_font_get_glyph_bitmap(f, HEX[g]);
        if (!bm) continue;
        const int bw = d.box_w, bh = d.box_h;
        s_glyph.dx[g] = (int8_t)d.ofs_x;
        s_glyph.dy[g] = (int8_t)(f->line_height - f->base_line - d.box_h - d.ofs_y);
        s_glyph.w[g] = (int8_t)(bw < GLYPH_W ? bw : GLYPH_W);
        s_glyph.h[g] = (int8_t)(bh < GLYPH_H ? bh : GLYPH_H);
        for (int y = 0; y < s_glyph.h[g]; y++)
            for (int x = 0; x < s_glyph.w[g]; x++) {
                const int bit = (y * bw + x) * 4;
                const uint8_t v = (bm[bit >> 3] >> (4 - (bit & 7))) & 0x0F;
                s_glyph.a[g][y * GLYPH_W + x] = (uint8_t)(v * 17);
            }
    }
    s_glyph.ready = true;
}

static void glyph_draw(lv_draw_ctx_t* ctx, int g, int x, int y, int ytop, int ybot, lv_color_t c, uint32_t opa)
{
    const lv_area_t* clip = ctx->clip_area;
    const lv_area_t* ba = ctx->buf_area;
    const int bw = lv_area_get_width(ba);
    x += s_glyph.dx[g];
    y += s_glyph.dy[g];
    int r1 = 0, r2 = s_glyph.h[g] - 1, c1 = 0, c2 = s_glyph.w[g] - 1;
    const int lo = ytop > clip->y1 ? ytop : clip->y1, hi = ybot < clip->y2 ? ybot : clip->y2;
    if (y + r1 < lo) r1 = lo - y;
    if (y + r2 > hi) r2 = hi - y;
    if (x + c1 < clip->x1) c1 = clip->x1 - x;
    if (x + c2 > clip->x2) c2 = clip->x2 - x;
    for (int r = r1; r <= r2; r++) {
        const uint8_t* a = &s_glyph.a[g][r * GLYPH_W];
        lv_color_t* p = (lv_color_t*)ctx->buf + (y + r - ba->y1) * bw + (x - ba->x1);
        for (int k = c1; k <= c2; k++)
            if (a[k]) p[k] = lv_color_mix(c, p[k], (lv_opa_t)((a[k] * opa) >> 8));
    }
}

void pm_px_hex_stream(lv_draw_ctx_t* ctx, uint32_t ms, int cols, int x0, int ytop, int ybot)
{
    const lv_area_t* clip = ctx->clip_area;
    if (ybot < clip->y1 || ytop > clip->y2) return;
    if (!s_glyph.ready) glyph_cache();
    const lv_color_t hero = lv_color_hex(C_HERO), lit_c = lv_color_hex(C_PINK);
    const int rows = (ybot - ytop + HEX_PITCH_Y - 1) / HEX_PITCH_Y + 1;
    // Scroll offset in milli-pixels, ms * speed, split so it never leaves 32 bits.
    const uint32_t q = ms / (HEX_PITCH_Y * 1000), rm = ms % (HEX_PITCH_Y * 1000);
    for (int col = 0; col < cols; col++) {
        const int x = x0 + col * HEX_PITCH_X;
        if (x + GLYPH_W < clip->x1 || x > clip->x2) continue;
        const uint32_t sp = 5 + (col * 37 % 7), part = rm * sp;
        const uint32_t base = q * sp + part / (HEX_PITCH_Y * 1000);
        const int frac = (int)(part % (HEX_PITCH_Y * 1000)) / 1000;
        for (int row = 0; row < rows; row++) {
            const int yy = ybot + HEX_PITCH_Y - frac - row * HEX_PITCH_Y;
            if (yy < ytop || yy > ybot) continue;
            if (yy > clip->y2 || yy + GLYPH_H < clip->y1) continue;
            const uint32_t hh = hx(col, base - row);
            const bool lit = hh % 61 == 0;
            int fade = (yy - ytop) * 256 / HEX_FADE;
            if (fade > 256) fade = 256;
            const uint32_t opa = (uint32_t)((lit ? 141 : 28) * fade) >> 8;
            if (opa) glyph_draw(ctx, hh & 15, x, yy, ytop, ybot, lit ? lit_c : hero, opa);
        }
    }
}

/* ---- difficulty period ------------------------------------------------ */

void pm_px_period_next_area(int x0, int y0, int cols, int pitch, int mined, lv_area_t* out)
{
    if (mined < 0) mined = 0;
    const int x = x0 + (mined % cols) * pitch, y = y0 + (mined / cols) * pitch;
    out->x1 = x - 2;
    out->y1 = y - 2;
    out->x2 = x + pitch - 1 + 1;
    out->y2 = y + pitch - 1 + 1;
}

void pm_px_period_grid(lv_draw_ctx_t* ctx, int x0, int y0, int cols, int rows, int pitch,
                       int mined, uint32_t ms, int32_t since_block_ms)
{
    const lv_area_t* clip = ctx->clip_area;
    const int s = pitch - 1, total = cols * rows;
    const lv_color_t pink = lv_color_hex(C_PINK), orange = lv_color_hex(C_ORANGE);
    const lv_color_t dim = lv_color_hex(C_UNMINED);
    const float inv = mined > 0 ? 1.0f / mined : 0.0f;
    int r1 = (clip->y1 - y0 - 2) / pitch, r2 = clip->y2 < y0 - 2 ? -1 : (clip->y2 - y0 + 2) / pitch;
    if (r1 < 0) r1 = 0;
    if (r2 > rows - 1) r2 = rows - 1;
    for (int r = r1; r <= r2; r++) {
        const int y = y0 + r * pitch;
        for (int col = 0; col < cols; col++) {
            const int i = r * cols + col, x = x0 + col * pitch;
            if (i < mined) {
                const float t = i * inv;
                fill_rect(ctx, x, y, x + s - 1, y + s - 1, pink, opa_f(0.22f + 0.78f * t * t * t));
            } else if (i == mined) {
                // 2*pi*220*1000 ms, nearly whole, keeps the phase in float range.
                const float a = 0.55f + 0.45f * sinf((float)(ms % 1382301u) / 220.0f);
                fill_rect(ctx, x - 2, y - 2, x + s + 1, y + s + 1, orange, opa_f(a));
                fill_rect(ctx, x, y, x + s - 1, y + s - 1, orange, LV_OPA_COVER);
            } else {
                fill_rect(ctx, x, y, x + s - 1, y + s - 1, dim, LV_OPA_COVER);
            }
        }
    }

    if (since_block_ms < 0 || since_block_ms >= 1000 || mined < 1 || mined > total) return;
    const float p = since_block_ms / 1000.0f, e = 1.0f - (1.0f - p) * (1.0f - p) * (1.0f - p);
    const float rad = 3.0f + 22.0f * e, ro = rad + 0.75f, ri = rad - 0.75f;
    const float ccx = x0 + ((mined - 1) % cols) * pitch + 0.5f * s;
    const float ccy = y0 + ((mined - 1) / cols) * pitch + 0.5f * s;
    const lv_opa_t opa = opa_f(1.0f - p);
    int ya = iceil(ccy - ro) - 1, yb = iceil(ccy + ro);
    if (ya < clip->y1) ya = clip->y1;
    if (yb > clip->y2) yb = clip->y2;
    for (int y = ya; y <= yb; y++) {
        const float dy = y + 0.5f - ccy, d2 = dy * dy;
        if (d2 >= ro * ro) continue;
        const float xo = sqrtf(ro * ro - d2);
        const int ol = iceil(ccx - xo - 0.5f), orr = iceil(ccx + xo - 0.5f) - 1;
        if (d2 >= ri * ri) {
            hspan(ctx, ol, orr, y, orange, opa);
            continue;
        }
        const float xi = sqrtf(ri * ri - d2);
        const int il = iceil(ccx - xi - 0.5f), ir = iceil(ccx + xi - 0.5f) - 1;
        hspan(ctx, ol, il - 1, y, orange, opa);
        hspan(ctx, ir + 1, orr, y, orange, opa);
    }
}

/* ---- small parts ------------------------------------------------------ */

void pm_px_vdash(lv_draw_ctx_t* ctx, int x, int y0, int y1, uint32_t rgb, uint8_t opa)
{
    const lv_color_t c = lv_color_hex(rgb);
    for (int y = y0; y <= y1; y++)
        if ((y - y0) % 6 < 3) hspan(ctx, x, x, y, c, opa);
}

uint32_t pm_px_mix(uint32_t a, uint32_t b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    uint32_t out = 0;
    for (int sh = 0; sh <= 16; sh += 8) {
        const float ca = (a >> sh) & 0xFF, cb = (b >> sh) & 0xFF;
        out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << sh;
    }
    return out;
}
