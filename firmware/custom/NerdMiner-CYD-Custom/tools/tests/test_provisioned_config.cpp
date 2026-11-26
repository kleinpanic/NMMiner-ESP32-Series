#include "provisionedConfig.h"
#include <cassert>
#include <cstdio>
#include <string>

static bool applyConfigJson(const std::string& text, TSettings& settings) {
    StaticJsonDocument<2048> doc;
    assert(!deserializeJson(doc, text));
    return applyProvisionedConfig(doc.as<JsonObjectConst>(), &settings);
}
int main() {
    TSettings settings;
    settings.WifiSSID = "fixture-network";
    settings.PoolAddress = "primary.example";
    settings.PoolPort = 1234;
    assert(applyConfigJson(R"({"FallbackPoolUrl":"fallback.example","FallbackPoolPort":5678,"FallbackPoolPassword":"fixture"})", settings));
    assert(settings.FallbackPoolAddress == "fallback.example" && settings.FallbackPoolPort == 5678);
    assert(std::string(settings.FallbackPoolPassword) == "fixture");
    assert(settings.PoolAddress == "primary.example" && settings.PoolPort == 1234);
    for (const char* invalid : {
        R"({"PoolUrl":"changed.example","FallbackPoolPort":0})",
        R"({"FallbackPoolPort":65536})", R"({"FallbackPoolPort":"3333"})",
        R"({"FallbackPoolUrl":"https://bad.example"})", R"({"PoolUrl":"bad.example/path"})",
        R"({"FallbackPoolUrl":null})", R"({"FallbackPoolPassword":false})"}) {
        assert(!applyConfigJson(invalid, settings));
        assert(settings.PoolAddress == "primary.example" && settings.PoolPort == 1234);
        assert(settings.FallbackPoolAddress == "fallback.example" && settings.FallbackPoolPort == 5678);
    }
    assert(!applyConfigJson("{\"FallbackPoolPassword\":\"" + std::string(80, 'x') + "\"}", settings));
    assert(applyConfigJson("{\"FallbackPoolPassword\":\"" + std::string(79, 'x') + "\"}", settings));
    assert(settings.FallbackPoolPassword[79] == '\0');
    assert(applyConfigJson(R"({"FallbackPoolUrl":"","FallbackPoolPassword":""})", settings));
    assert(settings.FallbackPoolAddress.empty());
    assert(applyConfigJson(R"({"PoolPort":65535,"FallbackPoolPort":1})", settings));
    std::puts("configuration: fallback fields, legacy partial updates, typed limits and atomic rejection PASS");
}
