#include "wifiBufferBudget.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

struct Config { int tx_buf_type, dynamic_tx_buf_num, dynamic_rx_buf_num, static_tx_buf_num, security; };
int main() {
    for (Config source : {Config{1,32,32,0,77}, Config{1,2,4,0,77},
                          Config{0,32,32,8,77}, Config{1,4,0,0,77}}) {
        Config c = source; boundWifiPacketBuffers(c);
        assert(c.tx_buf_type == source.tx_buf_type && c.static_tx_buf_num == source.static_tx_buf_num && c.security == 77);
        assert(c.dynamic_tx_buf_num == (source.tx_buf_type == 1 && source.dynamic_tx_buf_num > 4 ? 4 : source.dynamic_tx_buf_num));
        assert(c.dynamic_rx_buf_num == (source.dynamic_rx_buf_num == 0 || source.dynamic_rx_buf_num > 8 ? 8 : source.dynamic_rx_buf_num));
        Config once = c; boundWifiPacketBuffers(c);
        assert(c.dynamic_tx_buf_num == once.dynamic_tx_buf_num && c.dynamic_rx_buf_num == once.dynamic_rx_buf_num);
    }
    puts("PASS Wi-Fi packet budget: Arduino defaults, lower limits, static TX, unlimited RX and idempotence");
}
