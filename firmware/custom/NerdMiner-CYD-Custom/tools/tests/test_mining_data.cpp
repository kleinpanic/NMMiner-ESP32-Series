#include <Arduino.h>
#include "utils.h"
#include <cassert>
#include <cstring>
#include <iostream>

static std::string hex_bytes(const uint8_t* bytes, size_t length) {
    std::string out;
    for (size_t i = 0; i < length; ++i) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", bytes[i]);
        out += hex;
    }
    return out;
}

int main() {
    {
        uint8_t bytes[4] = {0xcc, 0xcc, 0xcc, 0xcc};
        const char raw[] = {'1', '2', 'a', 'B'};
        assert(to_byte_array(raw, sizeof(raw), bytes) == 2);
        assert(bytes[0] == 0x12 && bytes[1] == 0xab && bytes[2] == 0xcc);
        assert(to_byte_array("abcdef", 2, bytes) == 1 && bytes[0] == 0xab);
        bytes[1] = 0xcc;
        assert(to_byte_array("123", 3, bytes) == 1 && bytes[0] == 0x12 && bytes[1] == 0xcc);
        assert(to_byte_array("", 10, bytes) == 0);
        assert(to_byte_array("1", 2, bytes) == 0);
        assert(to_byte_array(raw, 0, bytes) == 0);
        assert(to_byte_array(raw, 1, bytes) == 0);
        assert(to_byte_array(nullptr, 4, bytes) == 0);
        assert(to_byte_array(raw, 4, nullptr) == 0);
    }
    // Independent hashlib SHA256d fixtures include coinbases larger than the
    // former 512-character buffer and complete 13/32-level merkle paths.
    struct Vector { size_t prefix_bytes; unsigned branches; const char* merkle; };
    const Vector vectors[] = {
        {1, 0, "58c4ef6270b1eb861d906881ba97158e4ed5eeca72068a4c8c243cae56a18be7"},
        {200, 0, "fff209daac00551d140219ffa3f3e7150a87525791fd3310b2bc193c3e4b00fb"},
        {256, 0, "921bd308d499d619bad73dfb0fbb4f4b499721ff6ed3969be7043a3e0fdbe324"},
        {1100, 0, "be1284358f7a60a45a6f2066cde380be368da9bf268727bf046d05dc98e8f023"},
        {140, 13, "c013723a26a5e5b9ce4a5231bebac68e13b07c1ee2cdb6804ab9be6874768af1"},
        {140, 32, "a5517f76fb11fb209cf4a2146afc4a7e25d6dcba3f073a6f303700444e034591"}
    };
    unsigned cases = 0;
    for (const auto& vector : vectors) {
        mining_subscribe worker{};
        worker.extranonce1 = "aabbccdd";
        worker.extranonce2_size = 4;
        mining_job job{};
        for (size_t i = 0; i < vector.prefix_bytes; ++i) job.coinb1 += "ab";
        for (unsigned i = 0; i < 50; ++i) job.coinb2 += "cd";
        StaticJsonDocument<4096> doc;
        job.merkle_branch = doc.to<JsonArray>();
        for (unsigned i = 0; i < vector.branches; ++i) {
            uint8_t bytes[32];
            memset(bytes, i, sizeof(bytes));
            job.merkle_branch.add(hex_bytes(bytes, sizeof(bytes)));
        }
        job.version = "20000000";
        job.prev_block_hash = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
        job.ntime = "12345678";
        job.nbits = "1d00ffff";
        Serial.output.clear();
        auto data = calculateMiningData(worker, job);
        assert(hex_bytes(data.merkle_result, 32) == vector.merkle);
        std::string header = "0000002003020100070605040b0a09080f0e0d0c13121110171615141b1a19181f1e1d1c";
        header += vector.merkle;
        header += "78563412ffff001d00000000";
        assert(hex_bytes(data.bytearray_blockheader, 80) == header);
        assert(worker.extranonce2 == "00000001");
        assert(Serial.output.empty()); // no blocking job payload on release builds
        ++cases;
    }
    std::cout << "PASS mining data: " << cases << " coinbase/merkle/header vectors and 9 bounded hex decoding cases\n";
}
