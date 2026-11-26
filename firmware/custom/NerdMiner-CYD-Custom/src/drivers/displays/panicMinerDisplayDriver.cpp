// PanicMiner display driver for the ESP32-2432S028R: runs src/ui (LVGL) on
// the TFT. LVGL is touched only from the Monitor task (initDisplay and the
// two setup-time screens run before that task exists). The BOOT button runs on
// loopTask and only moves current_cyclic_screen; animate() follows it.
#include "displayDriver.h"
#include "display.h"

#if defined(ESP32_2432S028R) && defined(PANICMINER_UI)

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <TFT_eTouch.h>
#include <WiFi.h>
#include <time.h>

#include "chainFeed.h"
#include "lvgl.h"
#include "mining.h"
#include "monitor.h"
#include "rotation.h"
#include "ui.h"
#include "version.h"
#include "touchPager.h"
#include "systemHealth.h"
#include "sdstore.h"
#include "wManager.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"

extern TSettings Settings;
extern nvMemory nvMem;
extern bool invertColors;
extern uint32_t templates;
extern uint32_t shares;
extern uint32_t valids;
extern double best_diff;
extern uint64_t upTime;
extern SDStore sdStore;
String readCustomAPName();
String defaultAPName();

static TFT_eSPI tft = TFT_eSPI();
// The CYD touch controller has its own bus, exactly as in esp23_2432s028r.cpp.
static SPIClass s_touch_spi(HSPI);
static TFT_eTouch<TFT_eSPI> s_touch(tft, ETOUCH_CS, 0xFF, s_touch_spi);
static TouchPager s_touch_pager;
static uint32_t s_page_at;
static bool s_page_hold = true; // pages stay put; PMAUTO is a diagnostic opt-in
static const uint32_t PAGE_INTERVAL_MS = 20000;
static uint32_t s_touch_events;
static bool s_touch_enabled = true;
static lv_disp_draw_buf_t s_draw_buf;
// Ten rows keep 6.4 KB available for verified TLS while SD is mounted.
static lv_color_t s_lines[320 * 4];
static lv_disp_drv_t s_disp;
static ui_model_t s_model;
static bool s_backlight_on = true;
static uint32_t s_unlinked_since = 0;
static uint32_t s_last_templates, s_last_shares;
static double s_last_best;
static bool s_was_linked;

/* ---- serial framebuffer dump (SEE-02) --------------------------------------
 * Send "PMDUMP\n" on the serial port. The driver redraws the whole screen and
 * prints "PMFB BEGIN w h", one "PMFB y x w <hex>" line per row segment (4 hex
 * digits per RGB565 pixel), then "PMFB END". scripts/fbdump.py turns it into a PNG.
 * "PMNEXT\n" moves to the next page, as a BOOT press does.
 * "PMFEED\n" fetches chain data now (src/chainFeed.cpp), and "PMLOCK\n"
 * toggles holding every SHA engine per job: the SHA collision test. */
static bool s_dumping = false;
static char s_cmd[16];
static uint8_t s_cmd_len = 0;

static void dump_rows(const lv_area_t* a, const lv_color_t* px)
{
    static const char hex[] = "0123456789abcdef";
    static char row[320 * 4 + 1];
    int w = a->x2 - a->x1 + 1;
    for (int y = a->y1; y <= a->y2; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t v = px->full;
            row[x * 4 + 0] = hex[v >> 12];
            row[x * 4 + 1] = hex[(v >> 8) & 15];
            row[x * 4 + 2] = hex[(v >> 4) & 15];
            row[x * 4 + 3] = hex[v & 15];
            px++;
        }
        row[w * 4] = '\0';
        Serial.printf("PMFB %d %d %d ", y, a->x1, w);
        Serial.write(row, w * 4);
        Serial.write('\n');
    }
}

static void poll_serial_command(void)
{
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c != '\n') {
            if (s_cmd_len < sizeof(s_cmd) - 1) s_cmd[s_cmd_len++] = c;
            continue;
        }
        s_cmd[s_cmd_len] = '\0';
        s_cmd_len = 0;
        if (strcmp(s_cmd, "PMDUMP") == 0) {
            Serial.printf("PMFB BEGIN %d %d\n", (int)s_disp.hor_res, (int)s_disp.ver_res);
            s_dumping = true;
            lv_obj_invalidate(lv_scr_act());
            lv_refr_now(NULL);
            s_dumping = false;
            Serial.println("PMFB END");
        } else if (strcmp(s_cmd, "PMNEXT") == 0) {
            switchToNextScreen();   // same path as a BOOT press
        } else if (strcmp(s_cmd, "PMFEED") == 0) {
            chainFeedPoke();
        } else if (strcmp(s_cmd, "PMFEEDPAUSED") == 0) {
            chainFeedPokePaused();
        } else if (strcmp(s_cmd, "PMMINE") == 0) {
            s_page_hold = true;
            resetToFirstScreen();
            ui_set_page(UI_PAGE_MINE);
            Serial.println("[UI] page Mine");
        } else if (strcmp(s_cmd, "PMAUTO") == 0) {
            s_page_hold = false;
            s_page_at = millis();
            Serial.println("[UI] automatic pages enabled");
        } else if (strcmp(s_cmd, "PMTOUCH") == 0) {
            int16_t x = -1, y = -1;
            bool pressed = s_touch.getXY(x, y);
            Serial.printf("[TOUCH] irq %d pressed %d x %d y %d events %lu\n",
                digitalRead(TOUCH_IRQ), pressed, x, y, (unsigned long)s_touch_events);
        } else if (strcmp(s_cmd, "PMTOUCHOFF") == 0) {
            s_touch_enabled = !s_touch_enabled;
            Serial.printf("[TOUCH] polling %s\n", s_touch_enabled ? "on" : "off");
        } else if (strcmp(s_cmd, "PMNET") == 0) {
            Serial.printf("[NET] wifi %d RSSI %d channel %d sleep %d tx %d\n",
                WiFi.status(), WiFi.RSSI(), WiFi.channel(), WiFi.getSleep(), WiFi.getTxPower());
        } else if (strcmp(s_cmd, "PMWIFIDROP") == 0) {
            Serial.println("[WIFI] controlled disconnect test; saved settings preserved");
            WiFi.disconnect(false, false);
        } else if (strcmp(s_cmd, "PMSETUP") == 0) {
            request_configuration(); // same non-destructive path as BOOT hold
        } else if (strcmp(s_cmd, "PMSAVE") == 0) {
            saveStat();
        } else if (strcmp(s_cmd, "PMSTATE") == 0) {
            Serial.printf("[STATE] pool %d diff %.6g accepted %lu rejected %lu best %.6g sd %d rows %lu persist %d\n",
                minerIsSubscribed(), minerPoolDifficulty(), (unsigned long)minerAcceptedShares(),
                (unsigned long)minerRejectedShares(), minerBestDifficulty(), sdStore.isReady(),
                (unsigned long)sdStore.loggedRows(), Settings.saveStats);
        } else if (strcmp(s_cmd, "PMMEM") == 0) {
            lv_mem_monitor_t mon;
            lv_mem_monitor(&mon);
            Serial.printf("[UI] pool used %u/%u, free block %u; heap free %u, largest %u\n",
                (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size,
                (unsigned)mon.free_biggest_size, (unsigned)ESP.getFreeHeap(),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
        } else if (strcmp(s_cmd, "PMLOCK") == 0) {
            minerShaLockAll(!minerShaLockAllOn());
            Serial.printf("[SHA] lock %s\n", minerShaLockAllOn() ? "all" : "sha256");
        }
    }
}

/* ---- LVGL glue -------------------------------------------------------------- */

static void flush(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* px)
{
    uint32_t w = a->x2 - a->x1 + 1;
    uint32_t h = a->y2 - a->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(a->x1, a->y1, w, h);
    tft.pushColors((uint16_t*)px, w * h, true);   // panel wants big-endian RGB565
    tft.endWrite();
    if (s_dumping) dump_rows(a, px);
    lv_disp_flush_ready(drv);
}

static void boot_log(const char* fmt, ...)
{
    char line[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    ui_boot_log(line);
    lv_refr_now(NULL);
}

static void pm_init(void)
{
    // Keep the SD deselected before any shared VSPI display traffic.
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    // Panel bring-up is the stock CYD sequence, unchanged.
    tft.init();
    if (nvMem.loadConfig(&Settings)) invertColors = Settings.invertColors;
#ifdef TFT_INVERSION_ON
    // The panel needs INVON for true colours, so the owner's "invert colours"
    // setting flips it from there instead of from INVOFF.
    tft.invertDisplay(!invertColors);
#else
    tft.invertDisplay(invertColors);
#endif
    tft.setRotation(1);
    if (invertColors) {
        tft.writecommand(ILI9341_GAMMASET);
        tft.writedata(2);
        delay(120);
        tft.writecommand(ILI9341_GAMMASET);
        tft.writedata(1);
    }
    tft.fillScreen(TFT_BLACK);

    s_touch_spi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, ETOUCH_CS);
    s_touch.init();
    s_touch.setCalibration({233, 3785, 3731, 120, 2});
    pinMode(TOUCH_IRQ, INPUT); // GPIO36 has no internal pull-up
    Serial.println("[TOUCH] CYD HSPI ready: CLK25 MISO39 MOSI32 CS33 IRQ36");

    ledcSetup(0, 5000, 8);
    ledcAttachPin(TFT_BL, 0);
    ledcWrite(0, Settings.Brightness);

    // The RGB LED is active low; keep it dark.
    pinMode(LED_PIN, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(LED_PIN_G, HIGH);
    digitalWrite(LED_PIN_B, HIGH);

    lv_init();
    lv_disp_draw_buf_init(&s_draw_buf, s_lines, NULL, sizeof(s_lines) / sizeof(s_lines[0]));
    lv_disp_drv_init(&s_disp);
    s_disp.hor_res = 320;
    s_disp.ver_res = 240;
    s_disp.flush_cb = flush;
    s_disp.draw_buf = &s_draw_buf;
    lv_disp_drv_register(&s_disp);
    ui_init();

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    Serial.printf("[UI] lvgl pool %u used of %u, heap free %u, largest block %u\n",
                  (unsigned)(mon.total_size - mon.free_size), (unsigned)mon.total_size,
                  (unsigned)ESP.getFreeHeap(), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

static void pm_loading(void)
{
    ui_show_boot();
    // 12 px mono fits 41 characters on a line.
    boot_log("panicminer %s", CURRENT_VERSION);
    boot_log("%s rev %d  %d MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getCpuFreqMHz());
    boot_log("flash %u KB  heap %u KB free", (unsigned)(ESP.getFlashChipSize() / 1024),
             (unsigned)(ESP.getFreeHeap() / 1024));
    boot_log("display 320x240  invert %s", invertColors ? "on" : "off");
    boot_log("pool %s:%d", Settings.PoolAddress.c_str(), Settings.PoolPort);
    boot_log("wifi joining %s", Settings.WifiSSID.c_str());
}

static void pm_setup(void)
{
    String ssid = WiFi.softAPSSID();
    if (ssid.length() == 0) {
        ssid = readCustomAPName();
        if (ssid.length() == 0) ssid = defaultAPName();
    }
    ui_show_setup(ssid.c_str(), DEFAULT_WIFIPW, "192.168.4.1");
    lv_refr_now(NULL);
}

static void pm_toggle_backlight(void)
{
    s_backlight_on = !s_backlight_on;
    ledcWrite(0, s_backlight_on ? Settings.Brightness : 0);
}

static void pm_toggle_rotation(void)
{
    tft.setRotation(flipRotation(tft.getRotation()));
    lv_obj_invalidate(lv_scr_act());
}

/* ---- data ------------------------------------------------------------------- */

static void set_log(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_model.log_line, sizeof(s_model.log_line), fmt, ap);
    va_end(ap);
}

static void refresh(unsigned long mElapsed)
{
    ui_model_t& m = s_model;
    // getCurrentHashRate keeps a 10 s running average; call it once per tick.
    m.hashrate_khs = getCurrentHashRate(mElapsed).toFloat();
    m.shares = minerAcceptedShares();
    m.blocks = valids;
    m.jobs = templates;
    m.best_diff = minerBestDifficulty();
    m.uptime_s = (uint32_t)upTime;
    m.temp_c = temperatureRead();
    m.rssi_dbm = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
    hash_rate_stats_t rates = getHashRateStats();
    m.hashrate_60s_khs = rates.mean_60s;
    m.hashrate_session_khs = rates.mean_session;
    m.hashrate_min_khs = rates.min_60s;
    m.hashrate_max_khs = rates.max_60s;
    m.total_mhash = rates.total_mhash;
    m.rejected = minerRejectedShares();
    minerShaStats(&m.sha_checked, &m.sha_mismatched);
    m.heap_free = ESP.getFreeHeap();
    m.heap_total = ESP.getHeapSize();
    m.cpu_valid = systemCpuUsage(m.cpu_pct);
    extern SDStore sdStore;
    m.sd_ready = sdStore.isReady();
    m.sd_rows = sdStore.loggedRows();
    m.pool_diff = minerPoolDifficulty();
    m.wallet_set = Settings.BtcWallet[0] != '\0';
    snprintf(m.pool, sizeof(m.pool), "%s:%d", Settings.PoolAddress.c_str(), Settings.PoolPort);

    bool linked = minerIsSubscribed();
    uint32_t now = millis();
    if (linked) {
        m.link = UI_LINK_MINING;
        s_unlinked_since = 0;
    } else {
        if (s_unlinked_since == 0) s_unlinked_since = now ? now : 1;
        m.link = now - s_unlinked_since > 20000 ? UI_LINK_NO_POOL : UI_LINK_CONNECTING;
    }

    if (linked != s_was_linked) {
        if (linked) set_log("subscribed to %s", Settings.PoolAddress.c_str());
        else set_log("pool link down, reconnecting");
    } else if (m.best_diff > s_last_best) {
        char d[16];
        ui_fmt_si(m.best_diff, d, sizeof(d));
        set_log("new best share %s", d);
    } else if (m.shares != s_last_shares) {
        set_log("share %lu accepted", (unsigned long)m.shares);
    } else if (templates != s_last_templates) {
        set_log("job %lu from pool", (unsigned long)templates);
    }
    s_was_linked = linked;
    s_last_best = m.best_diff;
    s_last_shares = m.shares;
    s_last_templates = templates;

    unsigned long epoch = getEpochLocal();
    m.time_valid = epoch > 1700000000UL;
    if (m.time_valid) {
        time_t t = (time_t)epoch;
        struct tm tm;
        gmtime_r(&t, &tm);   // the epoch already carries the timezone offset
        m.hour = tm.tm_hour;
        m.minute = tm.tm_min;
        m.second = tm.tm_sec;
        m.year = tm.tm_year + 1900;
        m.month = tm.tm_mon + 1;
        m.day = tm.tm_mday;
        m.weekday = tm.tm_wday;
    }

    m.best_hash_valid = minerBestHash(m.best_hash);
    m.live_valid = minerLiveHash(m.live_hash);

    m.chain_enabled = true;
    if (WiFi.status() == WL_CONNECTED) chainFeedStart();
    chain_snapshot_t c{};
    m.chain_valid = chainFeedGet(&c);
    m.wallet_valid = c.wallet_valid;
    m.wallet_attempted = c.wallet_attempted;
    m.wallet_fresh = c.wallet_valid && millis() - c.wallet_updated_ms < 300000;
    m.wallet_sats = c.wallet_sats;
    m.wallet_pending_sats = c.wallet_pending_sats;
    if (m.chain_valid) {
        m.block_height = c.height;
        m.price_usd = c.price_usd;
        m.net_difficulty = c.difficulty;
        m.net_hashrate = c.hashrate;
        m.retarget_progress = c.retarget_progress;
        m.retarget_change_pct = c.retarget_change_pct;
        m.retarget_blocks = c.retarget_blocks;
        m.fee_fast = c.fee_fast;
        m.fee_hour = c.fee_hour;
        m.fee_eco = c.fee_eco;
    }

    ui_update(&m);
}

static void follow_page(void)
{
    ui_page_t want = (ui_page_t)currentDisplayDriver->current_cyclic_screen;
    if (lv_scr_act() != NULL && want != ui_current_page()) {
        ui_set_page(want);
        s_page_at = millis();
        Serial.printf("[UI] page %u\n", (unsigned)want);
    }
}

static void poll_touch(void)
{
    if (!s_touch_enabled) return;
    // Keep the original driver's polling path (some CYDs have an unreliable
    // IRQ line), but sample every Monitor frame instead of every 500 ms.
    int16_t x = -1, y = -1;
    bool pressed = s_touch.getXY(x, y);
    int direction = s_touch_pager.sample(millis(), pressed, x, y);
    if (!direction) return;
    s_touch_events++;
    s_page_hold = true;
    s_page_at = millis();
    if (!s_backlight_on) {
        pm_toggle_backlight(); // first tap wakes the display
    } else {
        int n = currentDisplayDriver->num_cyclic_screens;
        int delta = direction < 0 ? n - 1 : 1;
        currentDisplayDriver->current_cyclic_screen =
            (currentDisplayDriver->current_cyclic_screen + delta) % n;
    }
    Serial.printf("[TOUCH] tap x %d y %d page %d\n", x, y,
                  currentDisplayDriver->current_cyclic_screen);
}

static void pm_tick(unsigned long mElapsed)
{
    refresh(mElapsed);
    follow_page();
    Serial.printf(">>> Completed %lu share(s), avg. hashrate %.1f KH/s\n",
                  (unsigned long)shares, s_model.hashrate_khs);
}

static bool s_first_page_shown = false;
static uint32_t s_live_at;
static uint8_t s_live[32];

static void pm_animate(unsigned long frame)
{
    if (!s_first_page_shown) {
        // The Mine page replaces the boot log on the first tick after start.
        s_first_page_shown = true;
        ui_set_page((ui_page_t)currentDisplayDriver->current_cyclic_screen);
        s_page_at = millis();
    }
    poll_serial_command();
    poll_touch();
    follow_page();
    if (!s_page_hold && millis() - s_page_at >= PAGE_INTERVAL_MS) {
        switchToNextScreen();
        follow_page();
    }
    // Prefilter hits arrive about 15 times a second: tick the live line at 8 Hz.
    if (millis() - s_live_at >= 125) {
        uint8_t h[32];
        s_live_at = millis();
        if (minerLiveHash(h) && memcmp(h, s_live, sizeof(h)) != 0) {
            memcpy(s_live, h, sizeof(h));
            if (ui_current_page() == UI_PAGE_HASH) ui_live_hash(h);
        }
    }
    lv_timer_handler();
}

static void pm_leds(unsigned long frame) {}

// One per page, in ui_page_t order: Mine, Hash, Chain, Clock, Stats.
static CyclicScreenFunction s_screens[] = {pm_tick, pm_tick, pm_tick, pm_tick, pm_tick};

DisplayDriver panicMinerDisplayDriver = {
    pm_init,
    pm_toggle_backlight,
    pm_toggle_rotation,
    pm_loading,
    pm_setup,
    s_screens,
    pm_animate,
    pm_leds,
    SCREENS_ARRAY_SIZE(s_screens),
    0,
    320,
    240,
};

#endif
