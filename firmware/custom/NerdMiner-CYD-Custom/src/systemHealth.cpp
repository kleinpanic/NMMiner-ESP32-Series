#ifdef PANICMINER_UI
#include <Arduino.h>
#include <esp_freertos_hooks.h>
#include "systemHealth.h"
static TaskHandle_t s_idle[2];
static volatile uint32_t s_ticks[2], s_busy[2];
static bool s_started;
static void IRAM_ATTR tick0() {
    s_ticks[0]++;
    if (xTaskGetCurrentTaskHandleForCPU(0) != s_idle[0]) s_busy[0]++;
}
static void IRAM_ATTR tick1() {
    s_ticks[1]++;
    if (xTaskGetCurrentTaskHandleForCPU(1) != s_idle[1]) s_busy[1]++;
}
void systemHealthStart() {
    if (s_started) return;
    s_idle[0] = xTaskGetIdleTaskHandleForCPU(0);
    s_idle[1] = xTaskGetIdleTaskHandleForCPU(1);
    esp_err_t a = esp_register_freertos_tick_hook_for_cpu(tick0, 0);
    esp_err_t b = esp_register_freertos_tick_hook_for_cpu(tick1, 1);
    s_started = a == ESP_OK && b == ESP_OK;
}
bool systemCpuUsage(float out[2]) {
    static uint32_t previous_ticks[2], previous_busy[2];
    bool valid = s_started;
    for (int i = 0; i < 2; i++) {
        uint32_t ticks = s_ticks[i], busy = s_busy[i];
        uint32_t dt = ticks - previous_ticks[i], db = busy - previous_busy[i];
        out[i] = dt ? fminf(100.0f, 100.0f * db / dt) : 0;
        previous_ticks[i] = ticks;
        previous_busy[i] = busy;
        valid = valid && dt > 0;
    }
    return valid;
}
#endif
