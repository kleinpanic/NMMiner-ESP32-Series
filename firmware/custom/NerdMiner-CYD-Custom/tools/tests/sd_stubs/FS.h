#pragma once
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
namespace sdhost {
struct Entry { bool directory=false; std::string data; };
inline std::map<std::string,Entry> files;
inline bool short_write=false;
}
namespace fs {
class File {
    std::string path_;
    size_t position_=0;
public:
    File() = default;
    explicit File(const char* path) : path_(path) {}
    explicit operator bool() const { return !path_.empty() && sdhost::files.count(path_); }
    bool isDirectory() const { return *this && sdhost::files.at(path_).directory; }
    const char* name() const { return path_.c_str(); }
    size_t size() const { return *this ? sdhost::files.at(path_).data.size() : 0; }
    size_t print(const char* text) {
        size_t n=std::strlen(text);
        if(sdhost::short_write && n) --n;
        sdhost::files.at(path_).data.append(text,n); return n;
    }
    size_t println(const char* text) {
        std::string line=std::string(text)+"\r\n";return print(line.c_str());
    }
    size_t readBytes(char* buffer,size_t limit) {
        const auto& data=sdhost::files.at(path_).data;
        size_t n=std::min(limit,data.size()-position_);
        std::memcpy(buffer,data.data()+position_,n);position_+=n;return n;
    }
    File openNextFile() { return File(); }
    void close() { path_.clear(); }
};
}
using fs::File;
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"
