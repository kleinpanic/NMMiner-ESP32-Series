// Host renderer for the PanicMiner UI: runs src/ui on LVGL 8.3 with a
// 320x240 RGB565 framebuffer and writes one PNG per screen state.
// Usage: uisim <out_dir>
#include <png.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lvgl.h"
#include "ui.h"

#define W 320
#define H 240

static lv_color_t fb[W * H];

static void flush(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* px)
{
    for (int y = a->y1; y <= a->y2; y++) {
        memcpy(&fb[y * W + a->x1], px, (size_t)(a->x2 - a->x1 + 1) * sizeof(lv_color_t));
        px += a->x2 - a->x1 + 1;
    }
    lv_disp_flush_ready(drv);
}

// Advance LVGL by ms of simulated time so animations finish.
static void run(int ms)
{
    for (int t = 0; t < ms; t += 5) {
        lv_tick_inc(5);
        lv_timer_handler();
    }
    lv_refr_now(NULL);
}

static void save(const char* dir, const char* name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s.png", dir, name);
    FILE* f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, f);
    png_set_IHDR(png, info, W, H, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    static uint8_t row[W * 3];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            lv_color_t c = fb[y * W + x];
            // Expand 5/6/5 to 8 bits the way the panel does: replicate high bits.
            row[x * 3 + 0] = (uint8_t)((c.ch.red << 3) | (c.ch.red >> 2));
            row[x * 3 + 1] = (uint8_t)((c.ch.green << 2) | (c.ch.green >> 4));
            row[x * 3 + 2] = (uint8_t)((c.ch.blue << 3) | (c.ch.blue >> 2));
        }
        png_write_row(png, row);
    }
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(f);
    printf("%s\n", path);
}

static ui_model_t fixture(void)
{
    ui_model_t m;
    memset(&m, 0, sizeof(m));
    m.link = UI_LINK_MINING;
    m.hashrate_khs = 968.3f;  // the pipelined HW loop's expected rate
    m.hashrate_60s_khs = 972.6f;
    m.hashrate_session_khs = 970.1f;
    m.hashrate_min_khs = 950.2f;
    m.hashrate_max_khs = 1003.4f;
    m.total_mhash = 436820.6;
    m.sha_checked = 17603;
    m.heap_free = 76500;
    m.heap_total = 300000;
    m.cpu_valid = true;
    m.cpu_pct[0] = 74.2f;
    m.cpu_pct[1] = 99.8f;
    m.pool_diff = 0.00015f;
    m.wallet_set = true;
    m.sd_ready = true;
    m.sd_rows = 48;
    m.shares = 1284;
    m.blocks = 0;
    m.jobs = 212;
    m.best_diff = 0.0347;
    m.uptime_s = 3 * 86400 + 7 * 3600 + 12 * 60;
    m.temp_c = 52.0f;
    m.rssi_dbm = -61;
    snprintf(m.pool, sizeof(m.pool), "public-pool.io:3333");
    snprintf(m.log_line, sizeof(m.log_line), "job 1a2f  diff 0.0012  nonce 3fa81c02");

    // Display order, most significant byte first: 6 zero nibbles, then a 1.
    static const uint8_t best[32] = {0x00, 0x00, 0x00, 0x1c, 0x8e, 0x7f, 0x3a, 0x90, 0xb2, 0xd4, 0xe1, 0xf7,
                                     0x5c, 0x02, 0x9b, 0x6a, 0x44, 0x81, 0xcd, 0x3e, 0x27, 0x90, 0xaa, 0x15};
    static const uint8_t live[32] = {0x00, 0x00, 0xa9, 0x1f, 0x3c, 0x27, 0xe0, 0xb1, 0xd4, 0x5f, 0x9a, 0x2c,
                                     0x71, 0x08, 0xee, 0x43, 0x9d, 0xb6, 0x0f, 0x52};
    m.best_hash_valid = true;
    memcpy(m.best_hash, best, sizeof(best));
    m.live_valid = true;
    memcpy(m.live_hash, live, sizeof(live));

    m.time_valid = true;
    m.hour = 23; m.minute = 47; m.second = 12;
    m.year = 2026; m.month = 11; m.day = 24; m.weekday = 2;

    m.chain_enabled = true;
    m.chain_valid = true;
    m.block_height = 977564;
    m.net_difficulty = 1.348e14;
    m.net_hashrate = 1.04e21;
    m.price_usd = 59855.0;
    m.retarget_change_pct = 8.7f;
    m.retarget_blocks = 196;
    m.retarget_progress = 90.3f;
    m.fee_fast = 6; m.fee_hour = 4; m.fee_eco = 2;
    return m;
}

int main(int argc, char** argv)
{
    const char* out = argc > 1 ? argv[1] : ".";
    lv_init();

    static lv_disp_draw_buf_t buf;
    static lv_color_t lines[W * 24];
    lv_disp_draw_buf_init(&buf, lines, NULL, W * 24);
    static lv_disp_drv_t drv;
    lv_disp_drv_init(&drv);
    drv.hor_res = W;
    drv.ver_res = H;
    drv.flush_cb = flush;
    drv.draw_buf = &buf;
    lv_disp_drv_register(&drv);

    ui_init();

    ui_show_boot();
    // The lines the firmware's pm_loading prints.
    const char* boot[] = {"panicminer 0.1.0",
                          "ESP32-D0WD-V3 rev 3  240 MHz",
                          "flash 4096 KB  heap 199 KB free",
                          "display 320x240  invert on",
                          "pool public-pool.io:21496",
                          "wifi joining Homebase"};
    for (unsigned i = 0; i < sizeof(boot) / sizeof(boot[0]); i++) ui_boot_log(boot[i]);
    run(100);
    save(out, "boot");

    // Sixty updates of history so the sparkline and the price chart have a shape.
    ui_model_t m = fixture();
    float base = m.hashrate_khs;
    double price = m.price_usd;
    for (int i = 0; i < 60; i++) {
        m.hashrate_khs = base + 3.0f * ((i * 37) % 11 - 5) / 5.0f - (i > 40 && i < 46 ? 9.0f : 0.0f);
        m.price_usd = price - 9.0 * (59 - i) + 260.0 * sin(i / 7.0) + 40.0 * ((i * 17) % 5);
        ui_update(&m);
        run(5);
    }
    m.hashrate_khs = base;
    m.price_usd = price;
    ui_update(&m);

    ui_set_page(UI_PAGE_MINE);
    run(150);
    save(out, "mine-enter");     // mid-roll: the hashrate rolling up from zeros
    run(900);
    save(out, "mine");

    // A share: toast, the cube's glow, the shares value flashing.
    m.shares++;
    ui_update(&m);
    run(300);
    save(out, "mine-share");
    run(2600);

    // A new block: the strip slides the block across the divider.
    m.block_height++;
    m.retarget_blocks--;
    ui_update(&m);
    run(350);
    save(out, "mine-block");
    run(900);

    ui_set_page(UI_PAGE_HASH);
    run(200);
    save(out, "hash");

    ui_set_page(UI_PAGE_CHAIN);
    run(200);
    save(out, "chain");
    m.block_height++;
    m.retarget_blocks--;
    ui_update(&m);
    run(300);
    save(out, "chain-block");    // ripple on the new dot, toast
    run(2400);

    ui_set_page(UI_PAGE_CLOCK);
    run(200);
    save(out, "clock");
    m.minute++;
    m.second = 0;
    ui_update(&m);
    run(200);
    save(out, "clock-roll");     // the minute digit mid-roll
    run(800);

    // Past 1000 kH/s the hashrate switches to MH/s.
    m.hashrate_khs = 1012.4f;
    ui_update(&m);
    ui_set_page(UI_PAGE_MINE);
    run(900);
    save(out, "mine-mhs");
    ui_set_page(UI_PAGE_HASH);
    run(100);
    save(out, "hash-mhs");
    m.hashrate_khs = base;

    m.price_usd = 123456;
    m.retarget_change_pct = -12.4f;
    ui_update(&m);
    ui_set_page(UI_PAGE_CHAIN);
    run(3000);
    save(out, "chain-six-figures");
    m.price_usd = 999999;
    ui_update(&m);
    run(900);
    save(out, "chain-million");

    m.hour = 0; m.minute = 0; m.second = 0;
    ui_update(&m);
    ui_set_page(UI_PAGE_CLOCK);
    run(900);
    save(out, "clock-midnight");

    m.link = UI_LINK_NO_POOL;
    snprintf(m.log_line, sizeof(m.log_line), "pool closed the connection, retry in 30 s");
    ui_update(&m);
    ui_set_page(UI_PAGE_MINE);
    run(100);
    save(out, "mine-alarm");
    ui_set_page(UI_PAGE_HASH);
    run(100);
    save(out, "hash-alarm");

    ui_model_t cold;
    memset(&cold, 0, sizeof(cold));
    snprintf(cold.pool, sizeof(cold.pool), "public-pool.io:3333");
    ui_update(&cold);
    ui_set_page(UI_PAGE_CHAIN);
    run(100);
    save(out, "chain-cold");
    cold.chain_enabled = true;
    ui_update(&cold);
    run(100);
    save(out, "chain-waiting");

    ui_set_page(UI_PAGE_HASH);
    run(900);
    save(out, "hash-cold");

    ui_set_page(UI_PAGE_CLOCK);
    run(100);
    save(out, "clock-cold");

    ui_set_page(UI_PAGE_STATS);
    ui_update(&m);
    run(900);
    save(out, "stats-cold");
    m = fixture();
    ui_update(&m);
    run(900);
    save(out, "stats");
    m.sha_mismatched = 7;
    m.rejected = 42;
    ui_update(&m);
    run(200);
    save(out, "stats-errors");

    // Fresh boot, before the first best hash.
    ui_set_page(UI_PAGE_MINE);
    run(100);
    save(out, "mine-cold");

    // Mining, then the feed answers while Mine is on screen, at the height
    // already seen: the strip must pick it up with no new block and no page
    // change. The fees cover all three label lengths.
    m.link = UI_LINK_MINING;
    m.chain_valid = false;
    ui_update(&m);
    ui_set_page(UI_PAGE_CHAIN);
    ui_set_page(UI_PAGE_MINE);
    run(2600);
    m.chain_valid = true;
    m.fee_fast = 12400; m.fee_hour = 152; m.fee_eco = 4;
    ui_update(&m);
    run(100);
    save(out, "mine-feed");

    ui_show_setup("panicminer-DE0C", "panicminer", "192.168.4.1");
    run(100);
    save(out, "setup");
    ui_show_setup("panicminer-DE0C", "panicminer", "192.168.4.1");
    run(100);
    save(out, "setup-reopened");

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    printf("lvgl pool %u used of %u, peak %u, frag %u %%\n", (unsigned)(mon.total_size - mon.free_size),
           (unsigned)mon.total_size, (unsigned)mon.max_used, (unsigned)mon.frag_pct);
    return 0;
}
