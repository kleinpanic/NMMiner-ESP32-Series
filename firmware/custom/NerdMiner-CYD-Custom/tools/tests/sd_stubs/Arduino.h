#pragma once
#include "../stubs/Arduino.h"
#include <cstdlib>
#include <cstring>
#include <map>
constexpr int GPIO_NUM_5=5, GPIO_NUM_18=18, GPIO_NUM_19=19, GPIO_NUM_23=23;
constexpr int OUTPUT=1, HIGH=1;
inline std::map<int,int> host_pins;
inline void pinMode(int, int) {}
inline void digitalWrite(int pin,int value) { host_pins[pin]=value; }
