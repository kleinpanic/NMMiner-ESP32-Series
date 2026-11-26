// Temporary CYD isolation test. No mining, TFT/touch SPI, SD, TLS or audio.
// Reads the existing SPIFFS config; never formats or writes it, and never logs
// network credentials, wallet, pool address or device addresses.
#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <atomic>

static StaticJsonDocument<2048> config;
static uint32_t last_status, connected_since;
static std::atomic<unsigned> disconnects{0}, stage{0};

void setup() {
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW); // backlight off: isolate radio/power from display load
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH); // SD chip deselected; no card access
    Serial.begin(115200);
    delay(800);
    Serial.println("[RADIO] isolation test: display/SD/mining/audio off; config read-only");
    if (!SPIFFS.begin(false)) {
        Serial.println("[RADIO] FAIL: config filesystem unavailable; no format attempted");
        return;
    }
    File file = SPIFFS.open("/config.json", "r");
    if (!file || deserializeJson(config, file) || !config["SSID"].is<const char*>() ||
        !config["WifiPW"].is<const char*>()) {
        Serial.println("[RADIO] FAIL: config unavailable; no changes made");
        return;
    }
    file.close();
    SPIFFS.end();
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(false);
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        ++disconnects;
        Serial.printf("[RADIO] disconnect reason %u, stage %u\n",
                      info.wifi_sta_disconnected.reason, stage.load());
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    struct Profile { uint8_t protocols; wifi_power_t power; };
    const Profile profiles[] = {
        {WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N, WIFI_POWER_19_5dBm},
        {WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N, WIFI_POWER_13dBm},
        {WIFI_PROTOCOL_11B, WIFI_POWER_13dBm},
        {WIFI_PROTOCOL_11B, WIFI_POWER_8_5dBm}
    };
    for (unsigned i = 0; i < 4; ++i) {
        WiFi.disconnect(false, false);
        delay(250);
        esp_wifi_set_protocol(WIFI_IF_STA, profiles[i].protocols);
        WiFi.setTxPower(profiles[i].power);
        Serial.printf("[RADIO] profile %u: modes %u power %.2f dBm\n",
                      i + 1, profiles[i].protocols, WiFi.getTxPower() / 4.0f);
        WiFi.begin(config["SSID"].as<const char*>(), config["WifiPW"].as<const char*>());
        uint32_t started = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - started < 25000) delay(50);
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[RADIO] JOINED profile %u; DHCP ready; RSSI %d\n", i + 1, WiFi.RSSI());
            connected_since = millis();
            break;
        }
    }
    WiFi.setAutoReconnect(true);
    if (!connected_since) Serial.println("[RADIO] FAIL: all profiles failed with display/SD/mining/audio off");
}

void loop() {
    if (millis() - last_status >= 5000) {
        last_status = millis();
        Serial.printf("[RADIO] status %u disconnects %u stage %u heap %u\n",
                      WiFi.status(), disconnects.load(), stage.load(), ESP.getFreeHeap());
    }
    // If the radio joins, test backlight load in two explicit stages while all
    // display buses and mining remain inactive. No saved brightness is changed.
    if (connected_since && millis() - connected_since >= (stage + 1) * 60000UL && stage < 2) {
        ++stage;
        if (stage == 1) { ledcSetup(0, 5000, 8); ledcAttachPin(21, 0); }
        ledcWrite(0, stage == 1 ? 64 : 255);
        Serial.printf("[RADIO] backlight test level %u\n", stage == 1 ? 64 : 255);
    }
    delay(20);
}
