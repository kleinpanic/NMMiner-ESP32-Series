#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#include <cstdio>
#include <inttypes.h>

#include "nmminer_recovered/official_v2002_st7789.h"

namespace rec = nmminer::recovered::v2002_st7789;

namespace {

constexpr int kHorRes = 320;
constexpr int kVerRes = 240;
constexpr uint8_t kDisplayRotation = 3;
constexpr int kDrawBufLines = 20;
constexpr int kBacklightChannel = 0;
constexpr int kBacklightFrequencyHz = 5000;
constexpr int kBacklightResolutionBits = 8;
constexpr int kBacklightDuty = 210;

TFT_eSPI tft;
lv_disp_draw_buf_t draw_buf;
lv_disp_drv_t disp_drv;
lv_color_t draw_buf_1[kHorRes * kDrawBufLines];

static_assert(LV_COLOR_DEPTH == 16, "ST7789 baseline flush expects RGB565");

void displayFlush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    const uint32_t width = static_cast<uint32_t>(area->x2 - area->x1 + 1);
    const uint32_t height = static_cast<uint32_t>(area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, width, height);
    tft.pushColors(reinterpret_cast<uint16_t *>(&color_p->full), width * height, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

lv_obj_t *makeLabel(lv_obj_t *parent,
                    const char *text,
                    const lv_font_t *font,
                    lv_color_t color,
                    lv_align_t align,
                    lv_coord_t x,
                    lv_coord_t y)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_label_set_text(label, text);
    lv_obj_align(label, align, x, y);
    return label;
}

void initDisplay()
{
    tft.init();
    tft.setRotation(kDisplayRotation);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    ledcSetup(kBacklightChannel, kBacklightFrequencyHz, kBacklightResolutionBits);
    ledcAttachPin(TFT_BL, kBacklightChannel);
    ledcWrite(kBacklightChannel, kBacklightDuty);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, draw_buf_1, nullptr, kHorRes * kDrawBufLines);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = kHorRes;
    disp_drv.ver_res = kVerRes;
    disp_drv.flush_cb = displayFlush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

void buildRecoveredBaselineScreen()
{
    const rec::OfficialAppModelSummary summary = rec::summarizeOfficialAppModel();
    const rec::LifecycleValidationSummary lifecycle = rec::summarizeLifecycleValidation();
    const rec::SPriceBehaviorSummary sprice = rec::summarizeSPriceBehaviorRecovery();
    const rec::SwarmBehaviorSummary swarm = rec::summarizeSwarmBehaviorRecovery();

    lv_obj_t *screen = lv_obj_create(nullptr);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    makeLabel(screen, "NMMiner", &lv_font_montserrat_28,
              lv_color_hex(0xEF7D31), LV_ALIGN_TOP_MID, 0, 10);
    makeLabel(screen, "v2.0.02 ST7789 recovered baseline", &lv_font_montserrat_16,
              lv_color_hex(0xF7F3F7), LV_ALIGN_TOP_MID, 0, 45);

    char line[96];
    snprintf(line, sizeof(line), "entry 0x%08" PRIx32 "  IDF %s",
             rec::kOfficialAppEntryVm,
             rec::kEspIdf.data());
    makeLabel(screen, line, &lv_font_montserrat_14,
              lv_color_hex(0xB5B2B5), LV_ALIGN_TOP_LEFT, 12, 78);

    snprintf(line, sizeof(line), "startup %u  lifecycle %u/%u  gates %u/%u",
             static_cast<unsigned>(rec::startupSteps().size()),
             static_cast<unsigned>(lifecycle.passed),
             static_cast<unsigned>(lifecycle.total),
             static_cast<unsigned>(summary.closed_gates),
             static_cast<unsigned>(summary.total_gates));
    makeLabel(screen, line, &lv_font_montserrat_14,
              lv_color_white(), LV_ALIGN_TOP_LEFT, 12, 104);

    snprintf(line, sizeof(line), "scan %c swarm %c stratum %c miners %c flash %s",
             summary.has_scan_task ? 'Y' : 'N',
             summary.has_swarm_task ? 'Y' : 'N',
             summary.has_stratum_task ? 'Y' : 'N',
             summary.has_miner_tasks ? 'Y' : 'N',
             lifecycle.flashable ? "ready" : "locked");
    makeLabel(screen, line, &lv_font_montserrat_14,
              lv_color_hex(0xB5B2B5), LV_ALIGN_TOP_LEFT, 12, 130);

    snprintf(line, sizeof(line), "shares +0x%03" PRIx32 "/+0x%03" PRIx32
             "  hashrate +0x%03" PRIx32 "/+0x%03" PRIx32,
             rec::kMinerAcceptedShareCountOffset,
             rec::kMinerRejectedShareCountOffset,
             rec::kMinerCombinedHashrateLowOffset,
             rec::kMinerCombinedHashrateHighOffset);
    makeLabel(screen, line, &lv_font_montserrat_14,
              lv_color_hex(0xB5B2B5), LV_ALIGN_TOP_LEFT, 12, 156);

    snprintf(line, sizeof(line), "SPrice %u/%u render %u/%u  swarm %u/%u",
             static_cast<unsigned>(sprice.default_watchcoins),
             static_cast<unsigned>(rec::kWatchCoinsMaxSymbols),
             static_cast<unsigned>(sprice.ordered_rows),
             static_cast<unsigned>(sprice.rendered_row_limit),
             static_cast<unsigned>(swarm.writer_records),
             static_cast<unsigned>(swarm.publisher_records));
    makeLabel(screen, line, &lv_font_montserrat_14,
              lv_color_hex(0xB5B2B5), LV_ALIGN_TOP_LEFT, 12, 182);

    makeLabel(screen, "compile-only harness: do not flash until gate passes",
              &lv_font_montserrat_14, lv_color_hex(0xFF0000),
              LV_ALIGN_BOTTOM_MID, 0, -18);

    lv_scr_load(screen);
}

void printRecoveredEvidence()
{
    const rec::OfficialAppModelSummary summary = rec::summarizeOfficialAppModel();
    const rec::LifecycleValidationSummary lifecycle = rec::summarizeLifecycleValidation();
    const rec::SPriceBehaviorSummary sprice = rec::summarizeSPriceBehaviorRecovery();
    const rec::SwarmBehaviorSummary swarm = rec::summarizeSwarmBehaviorRecovery();

    Serial.println();
    Serial.println("NMMiner official v2.0.02 ST7789 recovered baseline harness");
    Serial.printf("target=%s idf=%s entry=0x%08" PRIx32 "\n",
                  rec::kTarget.data(), rec::kEspIdf.data(), rec::kOfficialAppEntryVm);
    Serial.printf("segments=%u startup_steps=%u startup_literals=%u widgets=%u publishers=%u tasks=%u\n",
                  static_cast<unsigned>(rec::kSegments.size()),
                  static_cast<unsigned>(rec::startupSteps().size()),
                  static_cast<unsigned>(rec::startupLiterals().size()),
                  static_cast<unsigned>(rec::widgetSpecs().size()),
                  static_cast<unsigned>(rec::publisherSpecs().size()),
                  static_cast<unsigned>(rec::taskRegistrationEvidence().size()));
    Serial.printf("bringup_steps=%u runtime_slots=%u gates=%u/%u flash_gate=%s\n",
                  static_cast<unsigned>(summary.bringup_steps),
                  static_cast<unsigned>(summary.runtime_slots),
                  static_cast<unsigned>(summary.closed_gates),
                  static_cast<unsigned>(summary.total_gates),
                  summary.flash_gate_closed ? "locked" : "open");
    Serial.printf("lifecycle_checks=%u/%u startup=%u tasks=%u runtime=%u flashable=%u\n",
                  static_cast<unsigned>(lifecycle.passed),
                  static_cast<unsigned>(lifecycle.total),
                  lifecycle.startup_route_recovered ? 1u : 0u,
                  lifecycle.task_route_recovered ? 1u : 0u,
                  lifecycle.runtime_slots_recovered ? 1u : 0u,
                  lifecycle.flashable ? 1u : 0u);
    Serial.printf("required_tasks scan=%u swarm=%u stratum=%u miners=%u\n",
                  summary.has_scan_task ? 1u : 0u,
                  summary.has_swarm_task ? 1u : 0u,
                  summary.has_stratum_task ? 1u : 0u,
                  summary.has_miner_tasks ? 1u : 0u);
    Serial.printf("sprice_watchcoins default=%u/%u cap_sample=%u truncate=%u"
                  " rows=%u ordered=%u/%u recovered=%u/%u/%u\n",
                  static_cast<unsigned>(sprice.default_watchcoins),
                  static_cast<unsigned>(rec::kWatchCoinsMaxSymbols),
                  static_cast<unsigned>(sprice.sample_cap_count),
                  static_cast<unsigned>(sprice.sample_truncate_at),
                  static_cast<unsigned>(sprice.adapted_rows),
                  static_cast<unsigned>(sprice.ordered_rows),
                  static_cast<unsigned>(sprice.rendered_row_limit),
                  sprice.watchcoins_cap_recovered ? 1u : 0u,
                  sprice.row_adapter_recovered ? 1u : 0u,
                  sprice.row_ordering_key_recovered ? 1u : 0u);
    Serial.printf("swarm writer_records=%u publisher_records=%u layout=%u"
                  " writer_quartet=%u publisher_pairing=%u count=%u"
                  " metric0c_meaning=%u metric10_meaning=%u metric14_publisher=%u\n",
                  static_cast<unsigned>(swarm.writer_records),
                  static_cast<unsigned>(swarm.publisher_records),
                  swarm.layout_offsets_recovered ? 1u : 0u,
                  swarm.writer_quartet_recovered ? 1u : 0u,
                  swarm.publisher_pairing_recovered ? 1u : 0u,
                  swarm.probe_count_recovered ? 1u : 0u,
                  swarm.aggregate_float_meaning_recovered ? 1u : 0u,
                  swarm.display_float_meaning_recovered ? 1u : 0u,
                  swarm.auxiliary_direct_publisher_recovered ? 1u : 0u);
    Serial.println("No upload/flash is performed by this firmware build target.");
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(100);
    printRecoveredEvidence();
    initDisplay();
    buildRecoveredBaselineScreen();
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
