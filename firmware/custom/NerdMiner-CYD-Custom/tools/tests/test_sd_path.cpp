#include "sdPath.h"
#include <cassert>
#include <iostream>
#include <string>

int main() {
    unsigned cases = 0;
    struct Accepted { const char* input; const char* expected; };
    const Accepted accepted[] = {
        {"/sd", "/sd"}, {"sd", "/sd"}, {"/sd/log.csv", "/sd/log.csv"},
        {"sd/log.csv", "/sd/log.csv"}, {"log.csv", "/sd/log.csv"},
        {"folder/log.csv", "/sd/folder/log.csv"},
        {"/sd/folder/log.csv", "/sd/folder/log.csv"},
        {"plot..csv", "/sd/plot..csv"}, {"two words.csv", "/sd/two words.csv"}
    };
    for (const auto& item : accepted) {
        auto normalized = normalizeSdPath(item.input);
        assert(normalized == item.expected);
        assert(normalizeSdPath(normalized.c_str()) == normalized);
        ++cases;
    }
    const char* rejected[] = {
        nullptr, "", ".", "..", "../secret", "./log.csv", "folder/../secret",
        "folder/./log.csv", "/sd/../secret", "/sd/./log.csv", "sd/../../secret",
        "//sd/log.csv", "/sd//log.csv", "folder//log.csv", "\\sd\\log.csv",
        "folder\\log.csv", "a\nb", "a\rb", "a\tb", "a\x01" "b", "a\x7f" "b",
        "config.json.", "config.json ", "folder./log.csv", "folder /log.csv",
        " folder/log.csv", "/sd/secrets./item", "/sd/secrets /item"
    };
    for (const auto* path : rejected) {
        assert(normalizeSdPath(path).empty());
        ++cases;
    }
    const char* privatePaths[] = {
        "/sd/config.json", "config.json", "CONFIG.JSON", "/sd/CONFIG.JSON",
        "/sd/CoNfIg.JsOn", "/sd/config.json/child", "/sd/secrets",
        "/sd/SECRETS", "/sd/SeCrEtS/item", "secrets/item", "/sd/SECRETS/item",
        "/sd/CONFIG.JSON.", "/sd/config.json ", "/sd/secrets./item",
        "/sd/secrets /item", "/sd/ secrets/item",
        "/sd/CONFIG~1.JSO", "config~2.jso", "/sd/folder~1/item"
    };
    for (const char* path : privatePaths) {
        assert(publicSdPath(path, "/sd/config.json").empty());
        ++cases;
    }
    for (const auto& item : accepted) {
        assert(publicSdPath(item.input, "/sd/config.json") == item.expected);
        ++cases;
    }
    assert(publicSdPath("/sd/Plots/Log.CSV", "/sd/config.json") == "/sd/Plots/Log.CSV");
    assert(publicSdPath("/sd/secrets-public.csv", "/sd/config.json") == "/sd/secrets-public.csv");
    assert(publicSdPath("/sd/Other.JSON", "/sd/OTHER.json").empty());
    cases += 3;
    std::cout << "PASS SD paths: " << cases << " acceptance/idempotence/rejection scenarios\n";
}
