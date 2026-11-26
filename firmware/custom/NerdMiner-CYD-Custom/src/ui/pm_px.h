// Pixel drawing for the panicminer pages: crisp 2:1 isometric cubes, the hex
// stream of hashes, and the difficulty-period dot grid. Each call runs inside
// an LV_EVENT_DRAW_MAIN handler, writes straight into the LVGL software draw
// buffer (lv_draw_ctx_t buf/buf_area), blends over what the screen background
// already put there, and touches only pixels inside ctx->clip_area. All
// coordinates are absolute screen pixels. Nothing is allocated per frame.
#ifndef PM_PX_H
#define PM_PX_H

#include "lvgl.h"

typedef struct {
    uint32_t face[3];   // fill of the top, left and right faces, 0xRRGGBB
    uint8_t tiles;      // tiles per face edge; 0 draws plain faces
    float scan;         // 0..1 scan line down the side faces, tiles above it lit; < 0 none
    uint8_t edge_opa;   // white outline: top rhombus and the front vertical edge
    uint8_t glow_opa;   // accent glow around the silhouette, 0 = none
    uint8_t opa;        // whole cube; 255 = opaque
} pm_cube_t;

// Top vertex at (cx, y0). The top face is a 2:1 rhombus 2w wide and w tall;
// the side faces drop h below it, so the cube spans [cx-w, cx+w] x [y0, y0+w+h].
void pm_px_cube(lv_draw_ctx_t* ctx, int cx, int y0, int w, int h, const pm_cube_t* c);

// Columns of hex digits drifting up between ytop and ybot: the hashes being
// tried. Column pitch 9 px, row pitch 12 px, faint hero colour with a rare lit
// accent digit, fading in over the top 26 px. ms is a free-running clock.
void pm_px_hex_stream(lv_draw_ctx_t* ctx, uint32_t ms, int cols, int x0, int ytop, int ybot);

// The 2016-block difficulty period as cols x rows dots (cols * rows == 2016)
// on a `pitch` grid, each dot pitch-1 square. Mined dots glow brighter toward
// the newest; the next block blinks orange with a halo; the rest are dim.
// since_block_ms >= 0 and < 1000 draws the ripple of a block just found.
void pm_px_period_grid(lv_draw_ctx_t* ctx, int x0, int y0, int cols, int rows, int pitch,
                       int mined, uint32_t ms, int32_t since_block_ms);

// Screen rectangle the next-block dot and its halo cover, for invalidating the blink.
void pm_px_period_next_area(int x0, int y0, int cols, int pitch, int mined, lv_area_t* out);

// A 1 px dashed vertical line, 3 on 3 off.
void pm_px_vdash(lv_draw_ctx_t* ctx, int x, int y0, int y1, uint32_t rgb, uint8_t opa);

// Linear mix of two 0xRRGGBB colours, t in 0..1.
uint32_t pm_px_mix(uint32_t a, uint32_t b, float t);

#endif
