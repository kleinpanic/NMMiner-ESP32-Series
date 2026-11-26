#define ESP_DRD_USE_SPIFFS true

// Include Libraries
//#include ".h"

#include <WiFi.h>

#include <WiFiManager.h>

#include "wManager.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "drivers/storage/nvMemory.h"
#include "drivers/storage/storage.h"
#include "mining.h"
#include "timeconst.h"
#include "speaker.h"
#ifdef PANICMINER_UI
#include "sdstore.h"
extern SDStore sdStore;
#endif

#include <ArduinoJson.h>
#include <esp_flash.h>
#include <esp_mac.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_phy_init.h>


// Flag for saving data
bool shouldSaveConfig = false;

// Variables to hold data from custom textboxes
TSettings Settings;

// Define WiFiManager Object
WiFiManager wm;
extern monitor_data mMonitor;

nvMemory nvMem;

#ifdef PANICMINER_UI
// Survives ESP.restart(), but is consumed before opening the portal. A normal
// power cycle never needs this flag: saved credentials are loaded as usual.
static RTC_NOINIT_ATTR uint32_t s_setup_request;
static constexpr uint32_t SETUP_REQUEST = 0x504D5345;
#endif

extern SDCard SDCrd;

String defaultAPName()
{
#ifdef PANICMINER_UI
    // panicminer-XXXX from the last two bytes of the station MAC, so two units
    // on one desk get different setup networks.
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char name[20];
    snprintf(name, sizeof(name), "panicminer-%02X%02X", mac[4], mac[5]);
    return String(name);
#else
    return String(DEFAULT_SSID);
#endif
}

String readCustomAPName() {
    Serial.println("DEBUG: Attempting to read custom AP name from flash at 0x3F0000...");
    
    // Leer directamente desde flash
    const size_t DATA_SIZE = 128;
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0, DATA_SIZE); // Clear buffer
    
    // Leer desde 0x3F0000
    esp_err_t result = esp_flash_read(NULL, buffer, 0x3F0000, DATA_SIZE);
    if (result != ESP_OK) {
        Serial.printf("DEBUG: Flash read error: %s\n", esp_err_to_name(result));
        return "";
    }
    
    Serial.println("DEBUG: Successfully read from flash");
    String data = String((char*)buffer);
    
    // Debug: show raw data read
    Serial.printf("DEBUG: Raw flash data: '%s'\n", data.c_str());
    
    if (data.startsWith("WEBFLASHER_CONFIG:")) {
        Serial.println("DEBUG: Found WEBFLASHER_CONFIG marker");
        String jsonPart = data.substring(18); // Después del marcador "WEBFLASHER_CONFIG:"
        
        Serial.printf("DEBUG: JSON part: '%s'\n", jsonPart.c_str());
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, jsonPart);
        
        if (error == DeserializationError::Ok) {
            Serial.println("DEBUG: JSON parsed successfully");
            
            if (doc.containsKey("apname")) {
                String customAP = doc["apname"].as<String>();
                customAP.trim();
                
                if (customAP.length() > 0 && customAP.length() < 32) {
                    Serial.printf("✅ Custom AP name from webflasher: %s\n", customAP.c_str());
                    return customAP;
                } else {
                    Serial.printf("DEBUG: AP name invalid length: %d\n", customAP.length());
                }
            } else {
                Serial.println("DEBUG: 'apname' key not found in JSON");
            }
        } else {
            Serial.printf("DEBUG: JSON parse error: %s\n", error.c_str());
        }
    } else {
        Serial.println("DEBUG: WEBFLASHER_CONFIG marker not found - no custom config");
    }
    
    Serial.println("DEBUG: Using default AP name");
    return "";
}

void saveConfigCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;    
    //wm.setConfigPortalBlocking(false);
}

/* void saveParamsCallback()
// Callback notifying us of the need to save configuration
{
    Serial.println("Should save config");
    shouldSaveConfig = true;
    nvMem.saveConfig(&Settings);
} */

void configModeCallback(WiFiManager* myWiFiManager)
// Called when config mode launched
{
    Serial.println("Entered Configuration Mode");
    drawSetupScreen();
    Serial.print("Config SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());

    Serial.print("Config IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void reset_configuration()
{
    Serial.println("Erasing Config, restarting");
    nvMem.deleteConfig();
    resetStat();
    wm.resetSettings();
    ESP.restart();
}

void request_configuration()
{
#ifdef PANICMINER_UI
    saveStat();
    s_setup_request = SETUP_REQUEST;
    Serial.println("[CFG] Opening Setup; saved settings preserved");
    ESP.restart();
#else
    reset_configuration();
#endif
}

// --- USB/serial provisioning -------------------------------------------------
// SD and serial provisioning share bounded, typed keys. Apply to a temporary
// copy so an invalid field never partly changes the running configuration.
static bool applyProvisionedConfig(JsonObjectConst json, TSettings* settings)
{
    if (json.isNull()) return false;
    const char* stringKeys[] = {"SSID", "WifiPW", "PoolUrl", "BtcWallet", "PoolPassword"};
    const size_t stringLimits[] = {32, 64, 80, sizeof(settings->BtcWallet) - 1,
                                  sizeof(settings->PoolPassword) - 1};
    for (unsigned i = 0; i < 5; i++) {
        if (!json.containsKey(stringKeys[i])) continue;
        if (!json[stringKeys[i]].is<const char*>()) return false;
        size_t length = strlen(json[stringKeys[i]].as<const char*>());
        if (length > stringLimits[i]) return false;
        if (!length && i != 1 && i != 4) return false;
    }
    if (json.containsKey("PoolPort") && (!json["PoolPort"].is<int>() ||
        json["PoolPort"].as<int>() < 1 || json["PoolPort"].as<int>() > 65535)) return false;
    if (json.containsKey("Timezone") && (!json["Timezone"].is<int>() ||
        json["Timezone"].as<int>() < -12 || json["Timezone"].as<int>() > 14)) return false;
    if (json.containsKey("Brightness") && (!json["Brightness"].is<int>() ||
        json["Brightness"].as<int>() < 0 || json["Brightness"].as<int>() > 255)) return false;
    const char* boolKeys[] = {"SaveStats", "saveStatsToNVS", "invertColors"};
    for (const char* key : boolKeys)
        if (json.containsKey(key) && !json[key].is<bool>()) return false;
    if (json.containsKey("SaveStats") && json.containsKey("saveStatsToNVS") &&
        json["SaveStats"].as<bool>() != json["saveStatsToNVS"].as<bool>()) return false;

    TSettings candidate = *settings;
    if (json.containsKey("SSID")) candidate.WifiSSID = json["SSID"].as<const char*>();
    if (json.containsKey("WifiPW")) candidate.WifiPW = json["WifiPW"].as<const char*>();
    if (json.containsKey("PoolUrl")) candidate.PoolAddress = json["PoolUrl"].as<const char*>();
    if (json.containsKey("BtcWallet")) snprintf(candidate.BtcWallet, sizeof(candidate.BtcWallet), "%s", json["BtcWallet"].as<const char*>());
    if (json.containsKey("PoolPassword")) snprintf(candidate.PoolPassword, sizeof(candidate.PoolPassword), "%s", json["PoolPassword"].as<const char*>());
    if (json.containsKey("PoolPort")) candidate.PoolPort = json["PoolPort"].as<int>();
    if (json.containsKey("Timezone")) candidate.Timezone = json["Timezone"].as<int>();
    if (json.containsKey("SaveStats")) candidate.saveStats = json["SaveStats"].as<bool>();
    if (json.containsKey("saveStatsToNVS")) candidate.saveStats = json["saveStatsToNVS"].as<bool>();
    if (json.containsKey("invertColors")) candidate.invertColors = json["invertColors"].as<bool>();
    if (json.containsKey("Brightness")) candidate.Brightness = json["Brightness"].as<int>();
    if (!candidate.WifiSSID.length()) return false;
    *settings = candidate;
    return true;
}

#ifdef PANICMINER_UI
static bool restoreConfigFromSD(TSettings* settings)
{
    char buffer[1024];
    if ((!sdStore.isReady() && !sdStore.mount()) || !sdStore.readConfig(buffer, sizeof(buffer))) return false;
    // readConfig is bounded; reject a full buffer rather than accepting a
    // potentially truncated file whose prefix happened to be valid JSON.
    if (strlen(buffer) == sizeof(buffer) - 1) {
        Serial.println("[CFG] SD config exceeds the supported size");
        return false;
    }
    StaticJsonDocument<1024> json;
    if (deserializeJson(json, buffer) || !json["SSID"].is<const char*>() ||
        !applyProvisionedConfig(json.as<JsonObjectConst>(), settings)) {
        Serial.println("[CFG] SD config invalid; opening Setup");
        return false;
    }
    if (!nvMem.saveConfig(settings)) {
        Serial.println("[CFG] Could not persist SD configuration");
        return false;
    }
    Serial.println("[CFG] SD configuration restored to internal storage");
    return true;
}
#endif

// Configure the unit over the USB cable instead of the WiFi portal. The host
// sends one line during the boot window:
//   NMCFG {"SSID":"..","WifiPW":"..","PoolUrl":"..","PoolPort":3333,
//          "BtcWallet":"..","Timezone":-4}
// Credentials are saved to NVS exactly like the portal path — nothing is
// hardcoded in the firmware. Returns true if a valid config was received.
static bool tryConfigOverSerial(TSettings* settings, uint32_t window_ms)
{
    Serial.println(F("[CFG] Send  NMCFG {json}  over USB serial to configure "
                     "(keys: SSID, WifiPW, PoolUrl, PoolPort, BtcWallet, PoolPassword, Timezone, saveStatsToNVS)."));
    Serial.printf("[CFG] Listening %lums for serial config, then the WiFi portal...\n",
                  (unsigned long)window_ms);

    String line;
    uint32_t start = millis();
    while (millis() - start < window_ms) {
        while (Serial.available()) {
            char c = (char)Serial.read();
            if (c == '\n' || c == '\r') {
                line.trim();
#ifdef PANICMINER_UI
                if (line == "PMRF") {
                    // Explicit USB diagnostic, accepted only in the boot window.
                    // Espressif's remedy deletes only RF calibration, preserving
                    // the app's Wi-Fi/pool/wallet config and statistics namespaces.
                    WiFi.mode(WIFI_OFF);
                    esp_err_t result = esp_phy_erase_cal_data_in_nvs();
                    Serial.printf("[WIFI] RF calibration reset: %s; settings preserved\n",
                                  esp_err_to_name(result));
                    if (result == ESP_OK) ESP.restart();
                    line = "";
                    continue;
                }
#endif
                if (line.startsWith("NMCFG")) {
                    int brace = line.indexOf('{');
                    if (brace < 0) { Serial.println(F("[CFG] no JSON object")); line = ""; continue; }
                    StaticJsonDocument<512> doc;
                    DeserializationError err = deserializeJson(doc, line.substring(brace));
                    if (err) { Serial.printf("[CFG] JSON error: %s\n", err.c_str()); line = ""; continue; }
                    if (!applyProvisionedConfig(doc.as<JsonObjectConst>(), settings)) {
                        Serial.println(F("[CFG] Invalid configuration fields")); line = ""; continue;
                    }
                    if (!nvMem.saveConfig(settings)) {
                        Serial.println(F("[CFG] Could not save configuration")); line = ""; continue;
                    }
#ifdef PANICMINER_UI
                    WiFi.persistent(false); // SPIFFS owns the saved credentials.
#else
                    WiFi.persistent(true);
#endif
                    // Optional "Bssid": join one access point of a multi-AP network
                    // for this attempt, to tell a bad AP from a bad password.
                    uint8_t bssid[6];
                    const char* pin = doc["Bssid"] | "";
                    if (sscanf(pin, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &bssid[0], &bssid[1], &bssid[2],
                               &bssid[3], &bssid[4], &bssid[5]) == 6) {
                        Serial.printf("[CFG] pinned to BSSID %s\n", pin);
                        int channel = doc["Channel"] | 0;
                        if (channel < 0 || channel > 14) channel = 0;
                        WiFi.begin(settings->WifiSSID.c_str(), settings->WifiPW.c_str(), channel, bssid);
                    } else {
                        WiFi.begin(settings->WifiSSID.c_str(), settings->WifiPW.c_str());
                    }
                    Serial.printf("[CFG] Saved config for SSID '%s' — connecting...\n",
                                  settings->WifiSSID.c_str());
                    // Wait here so we don't race WiFiManager's own begin() call.
                    uint32_t cstart = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - cstart < 25000)
                        delay(250);
                    if (WiFi.status() == WL_CONNECTED) {
                        Serial.printf("[CFG] Connected, IP %s\n", WiFi.localIP().toString().c_str());
                    } else {
                        Serial.printf("[CFG] not connected (status=%d). Scanning...\n", WiFi.status());
                        int n = WiFi.scanNetworks();
                        bool found = false;
                        for (int i = 0; i < n; i++) {
                            if (WiFi.SSID(i) == settings->WifiSSID) {
                                found = true;
                                Serial.printf("[CFG] SSID '%s' VISIBLE: %s ch %d RSSI %d, enc %d -> association failed\n",
                                              WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(), (int)WiFi.channel(i),
                                              WiFi.RSSI(i), (int)WiFi.encryptionType(i));
                            }
                        }
                        if (!found)
                            Serial.printf("[CFG] SSID '%s' NOT seen among %d networks (out of range / wrong name / 5GHz-only)\n",
                                          settings->WifiSSID.c_str(), n);
                    }
                    return true;
                }
                line = "";
            } else if (line.length() < 480) {
                line += c;
            }
        }
        delay(10);
    }
    Serial.println(F("[CFG] no serial config received"));
    return false;
}

void init_WifiManager()
{
#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED
    //Serial.setTxTimeoutMs(10);
    
    // Check for custom AP name from flasher config, otherwise use default
    String customAPName = readCustomAPName();
    if (customAPName.length() == 0) customAPName = defaultAPName();
    const char* apName = customAPName.c_str();

    //Init pin 15 to eneble 5V external power (LilyGo bug)
#ifdef PIN_ENABLE5V
    pinMode(PIN_ENABLE5V, OUTPUT);
    digitalWrite(PIN_ENABLE5V, HIGH);
#endif

    // Change to true when testing to force configuration every time we run
    bool forceConfig = false;
#ifdef PANICMINER_UI
    forceConfig = esp_reset_reason() == ESP_RST_SW && s_setup_request == SETUP_REQUEST;
    s_setup_request = 0;
    if (forceConfig) wm.setBreakAfterConfig(true);
#endif

#if defined(PIN_BUTTON_2)
    // Check if button2 is pressed to enter configMode with actual configuration
    if (!digitalRead(PIN_BUTTON_2)) {
        Serial.println(F("Button pressed to force start config mode"));
        forceConfig = true;
        wm.setBreakAfterConfig(true); //Set to detect config edition and save
    }
#endif
#ifdef PANICMINER_UI
    // The name the router lists this unit under; set before the STA starts.
    // Configure reconnect policy before STA starts, so the event task cannot
    // restart an old association while the application changes radio profiles.
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.setHostname(defaultAPName().c_str());
#endif
    // Explicitly set WiFi mode
    WiFi.mode(WIFI_STA);
#ifdef PANICMINER_UI
    // This is a USB-powered interactive miner: keep the radio awake for pool
    // jobs and the local API rather than waiting for modem-sleep wake windows.
    WiFi.setSleep(false);
#endif
#ifdef PM_WIFI_TXPOWER
    WiFi.setTxPower(PM_WIFI_TXPOWER);
    Serial.printf("[WIFI] TX power capped at %.2f dBm\n", WiFi.getTxPower() / 4.0f);
#endif
#ifdef PM_WIFI_11B
    Serial.printf("[WIFI] 802.11b only: %s\n",
                  esp_err_to_name(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B)));
#endif
    // Log the native reason: an authentication timeout alone does not establish
    // whether credentials, the AP, or radio traffic caused the failure.
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        const uint8_t* b = info.wifi_sta_disconnected.bssid;
        uint8_t r = info.wifi_sta_disconnected.reason;
        Serial.printf("[WIFI] disconnected from %02X:%02X:%02X:%02X:%02X:%02X, reason %u %s\n",
                      b[0], b[1], b[2], b[3], b[4], b[5], r,
                      WiFi.disconnectReasonName((wifi_err_reason_t)r));
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

    // Always offer USB/serial provisioning (reconfigure-anytime over the cable).
    // If a config line arrives it is saved to SPIFFS and we connect immediately.
    // Load first so a serial update can change one option without replacing
    // the saved network, wallet and pool with factory defaults.
    bool savedConfig = nvMem.loadConfig(&Settings);
    bool serialProvisioned = tryConfigOverSerial(&Settings, 6000);

    if (!serialProvisioned)
    {
        if (!savedConfig)
        {
            //No config on internal flash. Try the SD card, else the portal.
#ifdef PANICMINER_UI
            if (!restoreConfigFromSD(&Settings)) forceConfig = true;
#else
            if (SDCrd.loadConfigFile(&Settings))
                SDCrd.SD2nvMemory(&nvMem, &Settings); // reboot on success.
            else
                forceConfig = true;    // nothing anywhere -> WiFi config portal
#endif
        }
    }

    // Free the memory from SDCard class
#ifndef PANICMINER_UI
    SDCrd.terminate();
#endif
    
    // Reset settings (only for development)
    //wm.resetSettings();

#ifdef PANICMINER_UI
    wm.setTitle("panicminer");
    wm.setClass("invert"); // dark theme
#endif

    // Set config save notify callback
    wm.setSaveConfigCallback(saveConfigCallback);
    wm.setSaveParamsCallback(saveConfigCallback);

    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wm.setAPCallback(configModeCallback);    

    //Advanced settings
    wm.setConfigPortalBlocking(false); //Hacemos que el portal no bloquee el firmware
    wm.setConnectTimeout(40); // how long to try to connect for before continuing
    wm.setConfigPortalTimeout(180); // auto close configportal after n seconds
    // wm.setCaptivePortalEnable(false); // disable captive portal redirection
    // wm.setAPClientCheck(true); // avoid timeout if client connected to softap
    //wm.setTimeout(120);
    //wm.setConfigPortalTimeout(120); //seconds

    // Custom elements

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter pool_text_box("Poolurl", "Pool url", Settings.PoolAddress.c_str(), 80);

    // Need to convert numerical input to string to display the default value.
    char convertedValue[6];
    sprintf(convertedValue, "%d", Settings.PoolPort);

    // Text box (Number) - 7 characters maximum
    WiFiManagerParameter port_text_box_num("Poolport", "Pool port", convertedValue, 7);

    // Text box (String) - 80 characters maximum
    //WiFiManagerParameter password_text_box("Poolpassword", "Pool password (Optional)", Settings.PoolPassword, 80);

    // Text box (String) - 80 characters maximum
    WiFiManagerParameter addr_text_box("btcAddress", "Your BTC address", Settings.BtcWallet, 80);

  // Text box (Number) - 2 characters maximum
  char charZone[6];
  sprintf(charZone, "%d", Settings.Timezone);
  WiFiManagerParameter time_text_box_num("TimeZone", "TimeZone fromUTC (-12/+12)", charZone, 3);

  WiFiManagerParameter features_html("<hr><br><label style=\"font-weight: bold;margin-bottom: 25px;display: inline-block;\">Features</label>");

  char checkboxParams[24] = "type=\"checkbox\"";
  if (Settings.saveStats)
  {
    strcat(checkboxParams, " checked");
  }
  WiFiManagerParameter save_stats_to_nvs("SaveStatsToNVS", "Save mining statistics to flash memory.", "T", 2, checkboxParams, WFM_LABEL_AFTER);
  // Text box (String) - 80 characters maximum
  // WiFiManager drops a parameter whose ID holds anything but [A-Za-z0-9_].
  WiFiManagerParameter password_text_box("Poolpassword", "Pool password (optional)", Settings.PoolPassword, 80);

  // Add all defined parameters
  wm.addParameter(&pool_text_box);
  wm.addParameter(&port_text_box_num);
  wm.addParameter(&password_text_box);
  wm.addParameter(&addr_text_box);
  wm.addParameter(&time_text_box_num);
  wm.addParameter(&features_html);
  wm.addParameter(&save_stats_to_nvs);
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  char checkboxParams2[24] = "type=\"checkbox\"";
  if (Settings.invertColors)
  {
    strcat(checkboxParams2, " checked");
  }
  WiFiManagerParameter invertColors("inverColors", "Invert Display Colors (if the colors looks weird)", "T", 2, checkboxParams2, WFM_LABEL_AFTER);
  wm.addParameter(&invertColors);
  #endif
  #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
    char brightnessConvValue[4];
    snprintf(brightnessConvValue, sizeof(brightnessConvValue), "%d", Settings.Brightness);
    // Text box (Number) - 3 characters maximum
    WiFiManagerParameter brightness_text_box_num("Brightness", "Screen backlight Duty Cycle (0-255)", brightnessConvValue, 3);
    wm.addParameter(&brightness_text_box_num);
  #endif

    Serial.println("AllDone: ");
    if (forceConfig)    
    {
        // Run if we need a configuration
        //No configuramos timeout al modulo
        wm.setConfigPortalBlocking(true); //Hacemos que el portal SI bloquee el firmware
        drawSetupScreen();
        mMonitor.NerdStatus = NM_Connecting;
        wm.startConfigPortal(apName, DEFAULT_WIFIPW);

#ifdef PANICMINER_UI
        // Leaving or timing out of Setup without saving resumes the previous
        // configuration. The portal parameters still live on this stack.
        if (!shouldSaveConfig) ESP.restart();
#endif

        if (shouldSaveConfig)
        {
            // WiFiManager owns the newly saved STA credentials. Keep the app's
            // copy in sync so the next cold boot tries the new location first.
            Settings.WifiSSID = wm.getWiFiSSID(true);
            Settings.WifiPW = wm.getWiFiPass(true);
            //Could be break forced after edditing, so save new config
            Serial.println("failed to connect and hit timeout");
            Settings.PoolAddress = pool_text_box.getValue();
            Settings.PoolPort = atoi(port_text_box_num.getValue());
            strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
            strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
            Settings.Timezone = atoi(time_text_box_num.getValue());
            //Serial.println(save_stats_to_nvs.getValue());
            Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
            #endif
            #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
            #endif
            nvMem.saveConfig(&Settings);
            delay(3*SECOND_MS);
            //reset and try again, or maybe put it to deep sleep
            ESP.restart();            
        };
    }
    else
    {
        //Tratamos de conectar con la configuración inicial ya almacenada
        mMonitor.NerdStatus = NM_Connecting;
        // NerdMiner stores app config (incl. SSID/PW) in SPIFFS, but WiFiManager's
        // autoConnect uses its own (often empty) credential store. Connect directly
        // with the loaded credentials first; only fall back to the portal.
        if (WiFi.status() != WL_CONNECTED && Settings.WifiSSID.length() > 0) {
            Serial.printf("[WIFI] Connecting to '%s' from stored config...\n", Settings.WifiSSID.c_str());
#ifdef PANICMINER_UI
            WiFi.persistent(false);
            WiFi.setAutoReconnect(false);
            WiFi.disconnect(false, false);
            delay(120);
            // A repeated unpinned begin can select the same failing AP forever.
            // Capture each matching AP and its actual channel before joining.
            // Do not persist a BSSID: the miner must also work after moving.
            struct AccessPoint { uint8_t bssid[6]; int32_t channel; int32_t rssi; };
            AccessPoint accessPoints[4];
            unsigned accessPointCount = 0;
            int networks = WiFi.scanNetworks();
            for (int i = 0; i < networks && accessPointCount < 4; i++) {
                if (WiFi.SSID(i) != Settings.WifiSSID) continue;
                AccessPoint& ap = accessPoints[accessPointCount++];
                memcpy(ap.bssid, WiFi.BSSID(i), sizeof(ap.bssid));
                ap.channel = WiFi.channel(i);
                ap.rssi = WiFi.RSSI(i);
                Serial.printf("[WIFI] candidate %u: channel %ld, RSSI %ld dBm\n",
                    accessPointCount, (long)ap.channel, (long)ap.rssi);
            }
            WiFi.scanDelete();
            struct Profile { uint8_t protocols; wifi_power_t power; };
            const Profile profiles[] = {
                {WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N, WIFI_POWER_19_5dBm},
                {WIFI_PROTOCOL_11B, WIFI_POWER_19_5dBm},
                {WIFI_PROTOCOL_11B, WIFI_POWER_13dBm},
                {WIFI_PROTOCOL_11B, WIFI_POWER_8_5dBm}
            };
            for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); i++) {
                for (unsigned a = 0; a < (accessPointCount ? accessPointCount : 1); a++) {
                    WiFi.disconnect(false, false);
                    delay(120);
                    esp_err_t modeResult = esp_wifi_set_protocol(WIFI_IF_STA, profiles[i].protocols);
                    bool powerResult = WiFi.setTxPower(profiles[i].power);
                    Serial.printf("[WIFI] join profile %u, candidate %u: modes %u, power %.2f dBm, config %s/%s\n",
                        i + 1, a + 1, profiles[i].protocols, WiFi.getTxPower() / 4.0f,
                        esp_err_to_name(modeResult), powerResult ? "OK" : "FAILED");
                    if (accessPointCount) {
                        WiFi.begin(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str(),
                                   accessPoints[a].channel, accessPoints[a].bssid);
                    } else {
                        // Hidden SSIDs or a failed scan still get a normal join.
                        WiFi.begin(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str());
                    }
                    uint32_t started = millis();
                    while (WiFi.status() != WL_CONNECTED && millis() - started < 10000) delay(100);
                    if (WiFi.status() == WL_CONNECTED) break;
                }
                if (WiFi.status() == WL_CONNECTED) break;
            }
            WiFi.setAutoReconnect(true);
#else
            WiFi.persistent(true);
            WiFi.begin(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str());
            uint32_t cstart = millis();
            while (WiFi.status() != WL_CONNECTED && millis() - cstart < 25000) delay(250);
#endif
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf("[WIFI] Connected, IP %s\n", WiFi.localIP().toString().c_str());
            } else {
                Serial.printf("[WIFI] direct connect failed (status=%d), scanning...\n", WiFi.status());
                WiFi.disconnect(false, false);
                delay(120);
                int n = WiFi.scanNetworks(); bool found = false;
                for (int i = 0; i < n; i++) if (WiFi.SSID(i) == Settings.WifiSSID) { found = true;
                    Serial.printf("[WIFI] '%s' VISIBLE %s ch %d RSSI %d enc %d -> association failed\n",
                                  WiFi.SSID(i).c_str(), WiFi.BSSIDstr(i).c_str(), (int)WiFi.channel(i),
                                  WiFi.RSSI(i), (int)WiFi.encryptionType(i)); }
                if (!found) Serial.printf("[WIFI] '%s' NOT seen among %d networks\n", Settings.WifiSSID.c_str(), n);
            }
        }
        // disable captive portal redirection
        wm.setCaptivePortalEnable(true);
        wm.setConfigPortalBlocking(true);
        wm.setEnableConfigPortal(true);
        // if (!wm.autoConnect(Settings.WifiSSID.c_str(), Settings.WifiPW.c_str()))
        // Skip WiFiManager's connect if serial provisioning already joined us.
        if (WiFi.status() != WL_CONNECTED && !wm.autoConnect(apName, DEFAULT_WIFIPW))
        {
            Serial.println("Failed to connect to configured WIFI, and hit timeout");
            if (shouldSaveConfig) {
                Settings.WifiSSID = wm.getWiFiSSID(true);
                Settings.WifiPW = wm.getWiFiPass(true);
                // Save new config            
                Settings.PoolAddress = pool_text_box.getValue();
                Settings.PoolPort = atoi(port_text_box_num.getValue());
                strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));
                strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));
                Settings.Timezone = atoi(time_text_box_num.getValue());
                // Serial.println(save_stats_to_nvs.getValue());
                Settings.saveStats = (strncmp(save_stats_to_nvs.getValue(), "T", 1) == 0);
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
                #endif
                #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
                Settings.Brightness = atoi(brightness_text_box_num.getValue());
                #endif
                nvMem.saveConfig(&Settings);
                vTaskDelay(2000 / portTICK_PERIOD_MS);      
            }        
            ESP.restart();                            
        } 
    }
    
    //Conectado a la red Wifi
    if (WiFi.status() == WL_CONNECTED) {
#ifdef PANICMINER_UI
        // Serial provisioning and Setup can join without entering the saved-
        // network retry loop; every successful path must restore recovery.
        WiFi.setAutoReconnect(true);
#endif
        //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        Speaker::playStartupChime();


        // Lets deal with the user config values

        // Copy the string value
        Settings.PoolAddress = pool_text_box.getValue();
        //strncpy(Settings.PoolAddress, pool_text_box.getValue(), sizeof(Settings.PoolAddress));
        Serial.print("PoolString: ");
        Serial.println(Settings.PoolAddress);

        //Convert the number value
        Settings.PoolPort = atoi(port_text_box_num.getValue());
        Serial.print("portNumber: ");
        Serial.println(Settings.PoolPort);

        // Copy the string value
        strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));

        // Copy the string value
        strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));

        //Convert the number value
        Settings.Timezone = atoi(time_text_box_num.getValue());
        Serial.print("TimeZone fromUTC: ");
        Serial.println(Settings.Timezone);

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
        Serial.print("Invert Colors: ");
        Serial.println(Settings.invertColors);        
        #endif

        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        Settings.Brightness = atoi(brightness_text_box_num.getValue());
        Serial.print("Brightness: ");
        Serial.println(Settings.Brightness);
        #endif

    }

    // Lets deal with the user config values

    // Copy the string value
    Settings.PoolAddress = pool_text_box.getValue();
    //strncpy(Settings.PoolAddress, pool_text_box.getValue(), sizeof(Settings.PoolAddress));
    Serial.print("PoolString: ");
    Serial.println(Settings.PoolAddress);

    //Convert the number value
    Settings.PoolPort = atoi(port_text_box_num.getValue());
    Serial.print("portNumber: ");
    Serial.println(Settings.PoolPort);

    // Copy the string value
    strncpy(Settings.PoolPassword, password_text_box.getValue(), sizeof(Settings.PoolPassword));

    // Copy the string value
    strncpy(Settings.BtcWallet, addr_text_box.getValue(), sizeof(Settings.BtcWallet));

    //Convert the number value
    Settings.Timezone = atoi(time_text_box_num.getValue());
    Serial.print("TimeZone fromUTC: ");
    Serial.println(Settings.Timezone);

    #ifdef ESP32_2432S028R
    Settings.invertColors = (strncmp(invertColors.getValue(), "T", 1) == 0);
    Serial.print("Invert Colors: ");
    Serial.println(Settings.invertColors);
    #endif

    // Save the custom parameters to FS
    if (shouldSaveConfig)
    {
        Settings.WifiSSID = wm.getWiFiSSID(true);
        Settings.WifiPW = wm.getWiFiPass(true);
        nvMem.saveConfig(&Settings);
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
         if (Settings.invertColors) ESP.restart();                
        #endif
        #if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
        if (Settings.Brightness != 250) ESP.restart();
        #endif
    }
}

//----------------- MAIN PROCESS WIFI MANAGER --------------
int oldStatus = 0;

void wifiManagerProcess() {

    wm.process(); // avoid delays() in loop when non-blocking and other long running code

    int newStatus = WiFi.status();
    if (newStatus != oldStatus) {
        if (newStatus == WL_CONNECTED) {
            Serial.println("CONNECTED - Current ip: " + WiFi.localIP().toString());
        } else {
            Serial.print("[Error] - current status: ");
            Serial.println(newStatus);
        }
        oldStatus = newStatus;
    }
}
