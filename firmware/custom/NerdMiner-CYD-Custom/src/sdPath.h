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
        // FatFs strips trailing dots/spaces; reject aliases before a private
        // filename can reach a case-insensitive filesystem under another name.
        if (part.empty() || part == "." || part == ".." ||
            part.front() == ' ' || part.back() == ' ' || part.back() == '.') return "";
        if (end == std::string::npos) break;
        from = end + 1;
    } while (true);
    return "/sd/" + path;
}

inline bool sdPrivatePathPrefix(const std::string& path, const char* prefix) {
    size_t i = 0;
    for (; prefix[i]; ++i) {
        if (i == path.size()) return false;
        unsigned char a = path[i], b = prefix[i];
        if (a >= 'A' && a <= 'Z') a += 'a' - 'A';
        if (b >= 'A' && b <= 'Z') b += 'a' - 'A';
        if (a != b) return false;
    }
    return i == path.size() || path[i] == '/';
}

inline std::string publicSdPath(const char* raw, const char* configPath) {
    std::string path = normalizeSdPath(raw);
    // Do not expose FAT short-name aliases (for example CONFIG~1.JSO).
    if (path.find('~') != std::string::npos) return "";
    if (sdPrivatePathPrefix(path, configPath) ||
        sdPrivatePathPrefix(path, "/sd/secrets")) return "";
    return path;
}
