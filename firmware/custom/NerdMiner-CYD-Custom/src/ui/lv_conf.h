/* PanicMiner LVGL 8.3 config. Shared by the firmware and tools/uisim.
 * Anything not set here takes the default from lv_conf_internal.h. */
#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0          /* the firmware flush swaps bytes for SPI */

#define LV_MEM_CUSTOM 0
/* Leave room for the TLS record buffers and RSA certificate verification.
 * Reserving 56 KB here fragmented the remaining heap enough for RSA to fail
 * allocation, reported by mbedTLS as an untrusted certificate. The screens
 * use at most about 36 KB on ESP32; 40 KB retains animation/setup headroom
 * while leaving certificate-verification space when FAT is mounted.
 * The 64-bit renderer needs a larger pool for its larger LVGL structs. */
#include <stdint.h>
#if UINTPTR_MAX > 0xFFFFFFFFu
#define LV_MEM_SIZE (80U * 1024U)
#else
#define LV_MEM_SIZE (40U * 1024U)
#endif
#ifdef ARDUINO
/* A static pool this size overflows the ESP32's static DRAM segment, so the
 * firmware takes it from the heap once, in lv_init at boot. */
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC malloc
#endif

#define LV_DISP_DEF_REFR_PERIOD 33
#define LV_INDEV_DEF_READ_PERIOD 50
#ifdef ARDUINO
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#else
#define LV_TICK_CUSTOM 0            /* tools/uisim calls lv_tick_inc() */
#endif
#define LV_DPI_DEF 130

#define LV_USE_LOG 0
#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

#define LV_FONT_MONTSERRAT_14 0
#define LV_FONT_CUSTOM_DECLARE \
    LV_FONT_DECLARE(pm_hero_96) LV_FONT_DECLARE(pm_disp_40) \
    LV_FONT_DECLARE(pm_text_20) LV_FONT_DECLARE(pm_text_14) LV_FONT_DECLARE(pm_mono_12) \
    LV_FONT_DECLARE(pm_mono_22) LV_FONT_DECLARE(pm_wide_64) LV_FONT_DECLARE(pm_wide_18) \
    LV_FONT_DECLARE(pm_wide_32) LV_FONT_DECLARE(pm_wide_40) LV_FONT_DECLARE(pm_wide_14) \
    LV_FONT_DECLARE(pm_text_11)
#define LV_FONT_DEFAULT &pm_text_14

#define LV_USE_CHART 1
#define LV_USE_QRCODE 1
#define LV_USE_THEME_DEFAULT 0
#define LV_USE_THEME_BASIC 0

#endif
