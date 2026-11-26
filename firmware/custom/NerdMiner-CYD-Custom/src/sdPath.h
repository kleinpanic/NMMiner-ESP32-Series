#pragma once
#include <string>
inline std::string normalizeSdPath(const char* raw) {
    if (!raw || !*raw) return "";
    std::string path(raw);
    for (unsigned char c : path) if (c < 32 || c == 127 || c == '\\') return "";
    if (path.find("//") != std::string::npos) return "";
    if (path[0] == '/') path.erase(0, 1);
    if (path == "sd" || path.empty()) return "/sd";
    if (path.compare(0, 3, "sd/") == 0) path.erase(0, 3);
    size_t from = 0;
    do {
        size_t end = path.find('/', from);
        std::string part = path.substr(from, end == std::string::npos ? end : end - from);
        if (part.empty() || part == "." || part == "..") return "";
        if (end == std::string::npos) break;
        from = end + 1;
    } while (true);
    return "/sd/" + path;
}
