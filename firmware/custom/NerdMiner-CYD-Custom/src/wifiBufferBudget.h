#pragma once

// Arduino 2.x changes SDK defaults to 32 dynamic TX and RX buffers. Their
// burst allocation competes with mining and TLS on a no-PSRAM CYD.
template<class Config>
void boundWifiPacketBuffers(Config& config) {
    if (config.tx_buf_type == 1 && config.dynamic_tx_buf_num > 4)
        config.dynamic_tx_buf_num = 4;
    if (config.dynamic_rx_buf_num == 0 || config.dynamic_rx_buf_num > 8)
        config.dynamic_rx_buf_num = 8;
}
