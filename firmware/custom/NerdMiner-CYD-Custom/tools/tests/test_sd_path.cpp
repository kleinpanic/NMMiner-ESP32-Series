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
        "folder\\log.csv", "a\nb", "a\rb", "a\tb", "a\x01" "b", "a\x7f" "b"
    };
    for (const auto* path : rejected) {
        assert(normalizeSdPath(path).empty());
        ++cases;
    }
    std::cout << "PASS SD paths: " << cases << " acceptance/idempotence/rejection scenarios\n";
}
