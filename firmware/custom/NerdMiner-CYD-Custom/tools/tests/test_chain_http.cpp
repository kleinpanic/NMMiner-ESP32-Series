#include "chainHttp.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

struct Tls { unsigned stops = 0; void stop() { ++stops; } };
struct Http {
    bool begin_ok = true;
    unsigned begins = 0, gets = 0, ends = 0;
    std::vector<int> codes;
    std::string response = "{\"price\":123}";
    bool begin(Tls&, const char*) { ++begins; return begin_ok; }
    int GET() { return codes.at(gets++); }
    std::string getString() { return response; }
    void end() { ++ends; }
};

int main() {
    unsigned scenarios = 0;
    for (const std::vector<int>& codes : {std::vector<int>{200}, {-1, 200}, {-11, -1}, {404}, {429}, {503}}) {
        Http http; http.codes = codes;
        Tls tls; unsigned waits = 0, errors = 0; std::string body = "stale";
        bool ok = chainGetBody(http, tls, "https://test.invalid/feed", body,
            [&](int code, unsigned attempt) { assert(code != 200); assert(attempt <= 2); ++errors; },
            [&] { assert(tls.stops == 1); ++waits; });
        bool expected = codes.back() == 200;
        assert(ok == expected);
        assert(http.gets == codes.size() && http.ends == codes.size());
        assert(waits == (codes.size() == 2 ? 1u : 0u));
        assert(errors == codes.size() - (expected ? 1u : 0u));
        assert(tls.stops == errors);
        assert(body == (expected ? http.response : ""));
        ++scenarios;
    }
    {
        Http http; http.begin_ok = false;
        Tls tls; std::string body = "stale";
        assert(!chainGetBody(http, tls, "invalid", body, [](int, unsigned) { assert(false); }, [] { assert(false); }));
        assert(http.gets == 0 && http.ends == 1 && tls.stops == 1 && body.empty());
        ++scenarios;
    }
    {
        Http http; http.codes = {200}; http.response.clear();
        Tls tls; std::string body;
        assert(!chainGetBody(http, tls, "https://test.invalid/feed", body, [](int, unsigned) { assert(false); }, [] { assert(false); }));
        assert(http.gets == 1 && http.ends == 1);
        ++scenarios;
    }
    {
        // Multiple snapshots must keep the authenticated connection usable.
        // The caller owns its lifetime; a successful GET never closes TLS.
        Http http; http.codes.assign(18, 200);
        Tls tls; std::string body;
        for (unsigned request = 0; request < 18; ++request) {
            assert(chainGetBody(http, tls, "https://test.invalid/feed", body,
                [](int, unsigned) { assert(false); }, [] { assert(false); }));
            assert(body == http.response && tls.stops == 0);
        }
        assert(http.gets == 18 && http.ends == 18);
        ++scenarios;
    }
    std::cout << "PASS Chain HTTP: " << scenarios << " transport retry, cleanup and error scenarios\n";
}
