#pragma once
#include <ArduinoJson.h>
#include <cstring>
#include <cctype>
#include "drivers/storage/storage.h"

inline bool applyProvisionedConfig(JsonObjectConst json, TSettings* settings)
{
    if (json.isNull()) return false;
    const char* stringKeys[] = {"SSID", "WifiPW", "PoolUrl", "BtcWallet", "PoolPassword",
                              JSON_KEY_FALLBACKURL, JSON_KEY_FALLBACKPASS};
    const size_t stringLimits[] = {32, 64, 80, sizeof(settings->BtcWallet) - 1,
                                  sizeof(settings->PoolPassword) - 1, 80,
                                  sizeof(settings->FallbackPoolPassword) - 1};
    for (unsigned i = 0; i < 7; i++) {
        if (!json.containsKey(stringKeys[i])) continue;
        if (!json[stringKeys[i]].is<const char*>()) return false;
        size_t length = strlen(json[stringKeys[i]].as<const char*>());
        if (length > stringLimits[i]) return false;
        if (!length && i != 1 && i != 4 && i != 5 && i != 6) return false;
        if (i == 2 || i == 5) {
            for (const char* c = json[stringKeys[i]].as<const char*>(); *c; ++c)
                if (!(isalnum(static_cast<unsigned char>(*c)) || *c == '.' || *c == '-')) return false;
        }
    }
    if (json.containsKey("PoolPort") && (!json["PoolPort"].is<int>() ||
        json["PoolPort"].as<int>() < 1 || json["PoolPort"].as<int>() > 65535)) return false;
    if (json.containsKey(JSON_KEY_FALLBACKPORT) && (!json[JSON_KEY_FALLBACKPORT].is<int>() ||
        json[JSON_KEY_FALLBACKPORT].as<int>() < 1 || json[JSON_KEY_FALLBACKPORT].as<int>() > 65535)) return false;
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
    if (json.containsKey(JSON_KEY_FALLBACKURL)) candidate.FallbackPoolAddress = json[JSON_KEY_FALLBACKURL].as<const char*>();
    if (json.containsKey(JSON_KEY_FALLBACKPORT)) candidate.FallbackPoolPort = json[JSON_KEY_FALLBACKPORT].as<int>();
    if (json.containsKey(JSON_KEY_FALLBACKPASS)) snprintf(candidate.FallbackPoolPassword, sizeof(candidate.FallbackPoolPassword), "%s", json[JSON_KEY_FALLBACKPASS].as<const char*>());
    if (json.containsKey("Timezone")) candidate.Timezone = json["Timezone"].as<int>();
    if (json.containsKey("SaveStats")) candidate.saveStats = json["SaveStats"].as<bool>();
    if (json.containsKey("saveStatsToNVS")) candidate.saveStats = json["saveStatsToNVS"].as<bool>();
    if (json.containsKey("invertColors")) candidate.invertColors = json["invertColors"].as<bool>();
    if (json.containsKey("Brightness")) candidate.Brightness = json["Brightness"].as<int>();
    if (!candidate.WifiSSID.length()) return false;
    *settings = candidate;
    return true;
}

