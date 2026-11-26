#pragma once
#include "SPI.h"
#include "FS.h"
constexpr int CARD_NONE=0, CARD_SDHC=3;
struct HostSD {
    bool mount_ok=true;
    int type=CARD_SDHC, ends=0;
    SPIClass* last_bus=nullptr;
    bool begin(int,SPIClass& bus,unsigned) { last_bus=&bus;bus.begin();return mount_ok; }
    void end() { ++ends; }
    int cardType() const { return type; }
    uint64_t totalBytes() const { return 8192; }
    uint64_t usedBytes() const { return 2048; }
    bool exists(const char* p) const { return sdhost::files.count(p); }
    bool mkdir(const char* p) { sdhost::files[p].directory=true;return true; }
    bool remove(const char* p) { return sdhost::files.erase(p)>0; }
    bool rename(const char* a,const char* b) {
        auto it=sdhost::files.find(a);if(it==sdhost::files.end())return false;
        sdhost::files[b]=it->second;sdhost::files.erase(it);return true;
    }
    File open(const char* p,const char* mode=FILE_READ) {
        if(std::strcmp(mode,FILE_READ)==0 && !exists(p))return File();
        if(!exists(p))sdhost::files[p]={};
        return File(p);
    }
};
inline HostSD SD;
