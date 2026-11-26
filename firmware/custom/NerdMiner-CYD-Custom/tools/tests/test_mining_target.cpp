#include "miningTarget.h"
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>

using Target = std::array<uint8_t, 32>;
static unsigned cases;

static void decodes(uint32_t compact, const Target& expected) {
    Target target;
    target.fill(0xa5); // successful decoding must initialize every byte
    assert(decodeCompactTarget(compact, target.data()));
    assert(target == expected);
    ++cases;
}

static void rejects(uint32_t compact) {
    Target target;
    target.fill(0xa5);
    assert(!decodeCompactTarget(compact, target.data()));
    ++cases;
}

static void meets(const Target& hash, const Target& target, bool expected) {
    assert(hashMeetsTarget(hash.data(), target.data()) == expected);
    ++cases;
}

int main() {
    Target genesis{};
    genesis[26] = 0xff;
    genesis[27] = 0xff;
    decodes(0x1d00ffff, genesis);

    Target current{};
    current[20] = 0x73;
    current[21] = 0x16;
    current[22] = 0x02;
    decodes(0x17021673, current);

    decodes(0x01010000, Target{0x01});
    decodes(0x01123456, Target{0x12}); // low mantissa bytes are discarded
    decodes(0x02008000, Target{0x80});
    decodes(0x02123456, Target{0x34, 0x12});
    decodes(0x03012345, Target{0x45, 0x23, 0x01});
    decodes(0x04012345, Target{0x00, 0x45, 0x23, 0x01});

    Target wide{};
    wide[29] = 0xff;
    wide[30] = 0xff;
    wide[31] = 0x7f;
    decodes(0x207fffff, wide);
    wide = {};
    wide[30] = 0x34;
    wide[31] = 0x12;
    decodes(0x21001234, wide); // exponent 33 can still fit 256 bits
    wide = {};
    wide[31] = 0x12;
    decodes(0x22000012, wide); // exponent 34 can fit a one-byte mantissa

    for (uint32_t compact : {
        0x00000000u, 0x1d000000u, 0x03000000u, // zero mantissa
        0x1d80ffffu, 0x01810000u, 0x17821673u, // signed/negative targets
        0x00012345u, 0x01000001u, 0x02000001u, // truncated to zero
        0x21010000u, 0x22000100u, 0x23000001u, // first overflowing bytes
        0xff000001u, 0xffffffffu              // extreme exponent/sign
    }) rejects(compact);

    meets(genesis, genesis, true);
    meets(current, current, true);

    Target below = current;
    // current has twenty trailing zero bytes in little-endian order.
    for (unsigned i = 0; i < 20; ++i) below[i] = 0xff;
    --below[20];
    meets(below, current, true);
    Target above = current;
    ++above[0];
    meets(above, current, false);

    const Target zero{};
    Target maximum;
    maximum.fill(0xff);
    meets(zero, zero, true);
    meets(zero, maximum, true);
    meets(maximum, maximum, true);
    meets(maximum, zero, false);

    // A lower most-significant byte wins over every lower byte being larger.
    Target hash = maximum;
    hash[31] = 0;
    Target target{};
    target[31] = 1;
    meets(hash, target, true);
    meets(target, hash, false);

    // Equal leading bytes must defer to the next differing high byte, not byte 0.
    hash = {};
    target = {};
    hash[30] = target[30] = 0x51;
    hash[5] = 3;
    hash[0] = 255;
    target[5] = 4;
    meets(hash, target, true);
    meets(target, hash, false);

    std::cout << "PASS mining targets: " << cases
              << " compact/overflow/little-endian threshold scenarios\n";
}
