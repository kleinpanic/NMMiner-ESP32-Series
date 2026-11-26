#pragma once
#include <Arduino.h>
#include <vector>

class WiFiClient {
public:
    std::string transmitted;
    String incoming;
    bool socket_connected = true;
    struct Chunk { uint32_t at; std::string bytes; };
    std::vector<Chunk> chunks;
    size_t next_chunk = 0;
    void schedule(const std::vector<Chunk>& input) {
        incoming.clear(); chunks = input; next_chunk = 0;
    }
    int available() {
        while (next_chunk < chunks.size()
            && static_cast<int32_t>(millis() - chunks[next_chunk].at) >= 0) {
            incoming += chunks[next_chunk].bytes;
            next_chunk++;
        }
        return static_cast<int>(incoming.size());
    }
    int read() {
        if (!available()) return -1;
        unsigned char c = incoming[0];
        incoming.erase(0, 1);
        return c;
    }
    bool connected() const { return socket_connected; }
    void stop() { socket_connected = false; }
    size_t print(const char* value) {
        transmitted += value;
        return std::char_traits<char>::length(value);
    }
    String readStringUntil(char) {
        String result = incoming;
        incoming.clear();
        return result;
    }
};
