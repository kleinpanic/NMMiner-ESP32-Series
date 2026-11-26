#pragma once
#include <stdint.h>
uint32_t millis();
struct SerialStub { void println(const char*) {} };
extern SerialStub Serial;
