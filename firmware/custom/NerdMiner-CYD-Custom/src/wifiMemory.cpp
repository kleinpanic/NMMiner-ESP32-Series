#if defined(ESP32_2432S028R) && defined(PANICMINER_UI)
#include <Arduino.h>
#include <esp_wifi.h>
#include "wifiBufferBudget.h"

extern "C" esp_err_t __real_esp_wifi_init(const wifi_init_config_t*);
extern "C" esp_err_t __wrap_esp_wifi_init(const wifi_init_config_t* config) {
    if (!config) return __real_esp_wifi_init(config);
    wifi_init_config_t bounded = *config;
    boundWifiPacketBuffers(bounded);
    Serial.printf("[WIFI] packet buffers: dynamic TX %d, RX %d\n",
                  bounded.dynamic_tx_buf_num, bounded.dynamic_rx_buf_num);
    return __real_esp_wifi_init(&bounded);
}
#endif
