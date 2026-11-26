#include "statusLed.h"
#include <cassert>
#include <cstdio>

static void expect(StatusLedColor c, bool r, bool g, bool b) {
    assert(c.red == r && c.green == g && c.blue == b);
}
int main() {
    StatusLed state;
    expect(state.sample(0, true, true, 3), false, true, false); // persisted count is not a new event
    expect(state.sample(10, true, true, 4), false, true, true);
    expect(state.sample(809, true, true, 4), false, true, true);
    expect(state.sample(810, true, true, 4), false, true, false);
    expect(state.sample(1000, false, false, 4), true, false, false);
    expect(state.sample(2000, false, false, 4), false, false, false);
    expect(state.sample(2001, true, false, 4), true, true, false);
    expect(state.sample(2002, true, true, 0), false, true, false); // reset is not a share
    StatusLed wrap;
    wrap.sample(UINT32_MAX - 500, true, true, 0);
    expect(wrap.sample(UINT32_MAX - 400, true, true, 1), false, true, true);
    expect(wrap.sample(400, true, true, 1), false, true, false);
    std::puts("status LEDs: mining, new share, offline, waiting, reset and wrap PASS");
}
