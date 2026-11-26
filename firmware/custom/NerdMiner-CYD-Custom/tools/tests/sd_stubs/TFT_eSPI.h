#pragma once
#include "SPI.h"
struct TFT_eSPI {
    static SPIClass& getSPIinstance() { return host_tft_bus; }
};
