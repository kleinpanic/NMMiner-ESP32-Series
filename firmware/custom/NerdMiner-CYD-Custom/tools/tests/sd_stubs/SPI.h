#pragma once
constexpr int VSPI=3;
struct SPIClass {
    explicit SPIClass(int = VSPI) {}
    bool running=false;
    int resets=0, stops=0;
    void begin(int=-1,int=-1,int=-1,int=-1) {
        if (!running) { running=true; ++resets; }
    }
    void end() { running=false; ++stops; }
};
inline SPIClass host_tft_bus;
