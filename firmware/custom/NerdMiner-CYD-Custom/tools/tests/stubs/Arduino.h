#pragma once
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdint>
#include <string>
using std::isfinite;
using byte = uint8_t;
#define unlikely(x) __builtin_expect(!!(x), 0)

// Host substitutes only the Arduino transport/runtime boundary. Tests compile
// the real firmware parser and use the same ArduinoJson headers as PlatformIO.
class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const char* value) : std::string(value ? value : "") {}
    String(const std::string& value) : std::string(value) {}
    bool isEmpty() const { return empty(); }
    void trim() {
        auto begin = find_first_not_of(" \t\r\n");
        if (begin == npos) { clear(); return; }
        auto finish = find_last_not_of(" \t\r\n");
        *this = substr(begin, finish - begin + 1);
    }
};

struct HostSerial {
    std::string output;
    template<class T> void print(const T& value) {
        append(value);
    }
    template<class T> void println(const T& value) {
        append(value); output += '\n';
    }
    void println() { output += '\n'; }
    void println(double value, int precision) {
        char buffer[96];
        snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
        output += buffer; output += '\n';
    }
    void printf(const char* format, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        output += buffer;
    }
private:
    void append(const char* value) { output += value ? value : ""; }
    void append(const std::string& value) { output += value; }
    void append(const String& value) { output += value; }
    template<class T> void append(T value) { output += std::to_string(value); }
};
inline HostSerial Serial;
inline uint32_t host_millis = 0;
inline uint32_t millis() { return host_millis; }
inline void vTaskDelay(unsigned long ticks) { host_millis += ticks; }
inline void delay(unsigned long ms) { host_millis += ms; }
#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) (ms)
