#include <assert.h>
#include <stdio.h>
#include <string>
#include "pendingResponse.h"

struct Bytes {
    std::string data;
    static bool failAppend;
    size_t length() const { return data.size(); }
    const char* c_str() const { return data.c_str(); }
    bool concat(const char* value, size_t size) {
        if (failAppend) return false;
        data.append(value, size);
        return true;
    }
    Bytes& operator=(const char* value) {
        assert(value == nullptr);
        std::string().swap(data);
        return *this;
    }
};
bool Bytes::failAppend = false;

int main() {
    PendingResponse<Bytes> response;
    assert(response.advance([](const char*, size_t) { assert(false); return 0; }) == ResponseWriteState::Complete);
    const std::string header = "HTTP/1.1 200 OK\r\nContent-Length: 1500\r\n\r\n";
    std::string body;
    for (size_t i = 0; i < 1500; ++i) body += char('a' + i % 26);
    assert(response.append(header.data(), header.size()));
    assert(response.append(body.data(), body.size()));
    std::string received;
    unsigned calls = 0;
    ResponseWriteState state;
    do {
        const size_t before = response.sent();
        state = response.advance([&](const char* data, size_t size) {
            assert(size <= 512);
            ++calls;
            if (calls % 4 != 0) return 0; // Repeated global TCP backpressure.
            const size_t count = size < 137 ? size : 137;
            received.append(data, count);
            return int(count);
        });
        if (state == ResponseWriteState::Wait) assert(response.sent() == before);
        assert(calls < 100);
    } while (state != ResponseWriteState::Complete);
    assert(received == header + body);
    assert(response.sent() == response.size());
    response.reset();
    assert(response.size() == 0 && response.sent() == 0);

    std::string limit(PendingResponse<Bytes>::maxBytes, 'x');
    assert(response.append(limit.data(), limit.size()));
    assert(!response.append("x", 1));
    assert(response.size() == limit.size());
    assert(response.advance([](const char*, size_t) { return -1; }) == ResponseWriteState::Failed);
    assert(response.sent() == 0);
    assert(response.advance([](const char*, size_t n) { return int(n + 1); }) == ResponseWriteState::Failed);
    response.reset();
    Bytes::failAppend = true;
    assert(!response.append("x", 1) && response.size() == 0);
    Bytes::failAppend = false;
    assert(response.append("ok", 2));
    assert(response.advance([](const char* data, size_t size) {
        assert(std::string(data, size) == "ok"); return int(size);
    }) == ResponseWriteState::Complete);
    puts("PASS pending response: exact headers/body, repeated backpressure, partial sends, bounds, allocation failure, invalid writes and reset");
}
