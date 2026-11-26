#include "stratum.h"
#include <cassert>
#include <cstring>
#include <iostream>

static unsigned cases;
static const char* subscription_response = R"({"id":1,"result":[[["mining.notify","test"]],"aabbccdd",4],"error":null})";
static void method(const char* json, stratum_method expected) {
    assert(parse_mining_method(String(json)) == expected);
    ++cases;
}
static String job_with_bits(const char* bits) {
    return std::string(R"({"method":"mining.notify","params":["job-target","0000000000000000000000000000000000000000000000000000000000000000","00","00",[],"20000000",")")
        + bits + R"(","6b123456",true]})";
}

int main() {
    method(R"({"id":2,"result":true,"error":null})", STRATUM_SUCCESS);
    method(R"({"id":2,"result":false,"error":null})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"result":null,"error":[24,"Unauthorized worker",null]})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"result":true,"error":[23,"Low difficulty",null]})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"result":"true","error":null})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"result":1,"error":null})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"result":null,"error":null})", STRATUM_PARSE_ERROR);
    method(R"({"id":2,"error":null})", STRATUM_PARSE_ERROR);
    method("", STRATUM_PARSE_ERROR);
    method("   \r\n", STRATUM_PARSE_ERROR);
    method("{broken", STRATUM_PARSE_ERROR);
    method(R"({"method":null,"params":[]})", STRATUM_PARSE_ERROR);
    method(R"({"method":123,"params":[]})", STRATUM_PARSE_ERROR);
    method(R"({"id":null,"method":"mining.notify","params":[]})", MINING_NOTIFY);
    method(R"({"id":null,"method":"mining.set_difficulty","params":[0.00015]})", MINING_SET_DIFFICULTY);
    method(R"({"id":null,"method":"mining.unknown","params":[]})", STRATUM_UNKNOWN);

    WiFiClient client;
    mining_subscribe subscription = init_mining_subscribe();
    client.incoming = String(subscription_response) + "\n";
    assert(tx_mining_subscribe(client, subscription));
    assert(subscription.extranonce1 == "aabbccdd");
    assert(subscription.extranonce2_size == 4);
    ++cases;

    Serial.output.clear();
    client.transmitted.clear();
    assert(tx_mining_auth(client, "TEST_WORKER_NOT_A_WALLET", "TEST_PASSWORD"));
    auto auth_id = stratumAuthRequestId();
    assert(auth_id == 2);
    StaticJsonDocument<1024> transmitted;
    assert(!deserializeJson(transmitted, client.transmitted));
    assert(transmitted["id"].as<unsigned long>() == auth_id);
    assert(std::strcmp(transmitted["method"], "mining.authorize") == 0);
    assert(std::strcmp(transmitted["params"][0], "TEST_WORKER_NOT_A_WALLET") == 0);
    assert(std::strcmp(transmitted["params"][1], "TEST_PASSWORD") == 0);
    assert(Serial.output.find("TEST_WORKER_NOT_A_WALLET") == std::string::npos);
    assert(Serial.output.find("TEST_PASSWORD") == std::string::npos);
    ++cases;

    assert(tx_suggest_difficulty(client, 0.00015));
    assert(stratumAuthRequestId() == auth_id); // later request IDs cannot replace auth ID
    assert(parse_extract_id(R"({"id":2,"result":true,"error":null})") == auth_id);
    assert(parse_extract_id("{broken") == 0);
    assert(parse_extract_id(R"({"result":true})") == 0);
    ++cases;

    mining_job job;
    Serial.output.clear();
    assert(parse_mining_notify(R"({"method":"mining.notify","params":["job-123","0000000000000000000000000000000000000000000000000000000000000000","00","00",[],"20000000","17012345","6b123456",true]})", job));
    assert(job.job_id == "job-123");
    assert(job.nbits == "17012345");
    assert(job.ntime == "6b123456");
    assert(job.clean_jobs);
    assert(Serial.output.empty());
    assert(parse_mining_method(job_with_bits("1d00ffff")) == MINING_NOTIFY);
    assert(Serial.output.empty());
    ++cases;
    assert(!parse_mining_notify(R"({"method":"mining.notify","params":[]})", job));
    assert(!parse_mining_notify(R"({"method":"mining.notify","params":["job-123","0000000000000000000000000000000000000000000000000000000000000000","00","00","not-an-array","20000000","17012345","6b123456",true]})", job));
    ++cases;

    mining_job target_job;
    for (const char* bits : {"1d00ffff", "17021673"}) {
        assert(parse_mining_notify(job_with_bits(bits), target_job));
        assert(target_job.nbits == bits);
        ++cases;
    }
    const String last_valid_bits = target_job.nbits;
    for (const char* bits : {
        "00000000", "1d000000", "1d80ffff", "17821673",
        "01000001", "02000001", "21010000", "22000100",
        "23000001", "ff000001", "1701234z", "1701234 "
    }) {
        assert(!parse_mining_notify(job_with_bits(bits), target_job));
        assert(target_job.nbits == last_valid_bits);
        ++cases;
    }

    double difficulty = 9;
    assert(parse_mining_set_difficulty(R"({"method":"mining.set_difficulty","params":[0.00015]})", difficulty));
    assert(std::fabs(difficulty - 0.00015) < 1e-10);
    const double last_valid_difficulty = difficulty;
    ++cases;
    assert(!parse_mining_set_difficulty(R"({"method":"mining.set_difficulty","params":[]})", difficulty));
    assert(!parse_mining_set_difficulty(R"({"method":"mining.set_difficulty","params":["not-a-number"]})", difficulty));
    assert(!parse_mining_set_difficulty(R"({"method":"mining.set_difficulty","params":[0]})", difficulty));
    assert(!parse_mining_set_difficulty(R"({"method":"mining.set_difficulty","params":[-1]})", difficulty));
    assert(difficulty == last_valid_difficulty); // invalid input preserves last valid value
    ++cases;

    subscription.extranonce2 = "00000001";
    std::strcpy(subscription.wName, "TEST_WORKER_NOT_A_WALLET");
    client.transmitted.clear();
    unsigned long submission_id = 0;
    assert(tx_mining_submit(client, subscription, job, 0x123, submission_id));
    assert(!deserializeJson(transmitted, client.transmitted));
    assert(std::strcmp(transmitted["params"][4], "00000123") == 0);
    assert(transmitted["id"].as<unsigned long>() == submission_id);
    assert(stratumAuthRequestId() == auth_id);
    ++cases;

    // TCP fragments are not JSON messages; only newline-complete id=1 counts.
    {
        host_millis = 0;
        WiFiClient fragmented;
        std::string response = std::string(subscription_response) + "\n";
        fragmented.schedule({
            {0, "{\"method\":\"mining.set_difficulty\",\"params\":[0.00015]}\n"},
            {100, response.substr(0, 19)},
            {400, response.substr(19, 27)},
            {700, response.substr(46)}
        });
        auto pending = init_mining_subscribe();
        assert(tx_mining_subscribe(fragmented, pending));
        assert(pending.extranonce1 == "aabbccdd");
        assert(host_millis >= 700 && host_millis <= 1100);
        ++cases;
    }
    {
        host_millis = 0;
        WiFiClient wrong_id;
        wrong_id.schedule({
            {0, "{\"id\":99,\"result\":true,\"error\":null}\n"},
            {300, std::string(subscription_response) + "\n"}
        });
        auto pending = init_mining_subscribe();
        assert(tx_mining_subscribe(wrong_id, pending));
        assert(pending.extranonce2_size == 4);
        ++cases;
    }
    {
        host_millis = 0;
        WiFiClient truncated;
        truncated.schedule({{0, "{\"id\":1,\"result\":["}});
        auto pending = init_mining_subscribe();
        assert(!tx_mining_subscribe(truncated, pending));
        assert(host_millis >= 5000 && host_millis <= 5500);
        assert(pending.extranonce1.empty());
        ++cases;
    }
    {
        host_millis = 0;
        WiFiClient delayed;
        delayed.schedule({{4800, std::string(subscription_response) + "\n"}});
        auto pending = init_mining_subscribe();
        assert(tx_mining_subscribe(delayed, pending));
        assert(host_millis >= 4800 && host_millis <= 5100);
        ++cases;
    }
    {
        host_millis = 0;
        WiFiClient disconnected;
        disconnected.socket_connected = false;
        auto pending = init_mining_subscribe();
        assert(!tx_mining_subscribe(disconnected, pending));
        assert(host_millis <= 5000); // even a dead socket has a bounded wait
        ++cases;
    }
    std::cout << "PASS stratum: " << cases << " regression scenarios\n";
}
