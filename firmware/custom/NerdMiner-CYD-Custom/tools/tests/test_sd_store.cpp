#include "sdstore.h"
#include <SD.h>
#include <TFT_eSPI.h>
#include <esp32-hal-matrix.h>
#include <soc/gpio_sig_map.h>
#include <cassert>
#include <iostream>

int main() {
    host_tft_bus.begin(14,19,13,15);
    host_routes[14]=VSPICLK_OUT_IDX;host_routes[13]=VSPID_OUT_IDX;
    {
        SDStore store;
        assert(store.mount());
        assert(SD.max_files==2 && !SD.format_requested);
        assert(SD.last_bus==&host_tft_bus && host_tft_bus.resets==1);
        assert(host_routes.at(14)==VSPICLK_OUT_IDX && host_routes.at(13)==VSPID_OUT_IDX);
        assert(host_routes.at(18)==VSPICLK_OUT_IDX && host_routes.at(23)==VSPID_OUT_IDX);
        assert(host_pins.at(5)==HIGH);
        assert(store.mount() && host_tft_bus.resets==1);
        assert(store.unmount() && !store.isReady());
        assert(host_tft_bus.running && host_tft_bus.stops==0);
    }
    {
        SDStore store;
        SD.mount_ok=false;
        assert(!store.mount() && !store.isReady());
        assert(host_tft_bus.running && host_tft_bus.stops==0);
        SD.mount_ok=true;
        SD.type=CARD_NONE;
        assert(!store.mount() && !store.isReady());
        assert(host_tft_bus.running && host_tft_bus.stops==0);
        SD.type=CARD_SDHC;
        assert(store.mount());
        auto st=store.getStatus();
        assert(st.mounted && st.total_bytes==8192 && st.used_bytes==2048 && st.free_bytes==6144);
        assert(store.appendCSVLog("1,980,975,0,0,0.1,1,72000,72"));
        assert(store.appendCSVLog("2,981,976,0,0,0.1,2,72000,72"));
        assert(store.loggedRows()==2);
        const auto& csv=sdhost::files.at(SD_CSV_PATH).data;
        assert(csv.find("uptime_s,khs_10s,khs_60s,")==0);
        assert(csv.find("uptime_s",1)==std::string::npos);
        assert(csv.find("1,980,975,")!=std::string::npos);
        assert(csv.find("2,981,976,")!=std::string::npos);
        sdhost::short_write=true;
        assert(!store.appendCSVLog("3,980,975,0,0,0.1,3,72000,72"));
        assert(store.loggedRows()==2);
        sdhost::short_write=false;
    }
    assert(host_tft_bus.running && host_tft_bus.stops==0);
    std::cout<<"PASS SD store: shared display bus, retained pin routes, mount retries, non-disruptive unmount, capacity, CSV and failed-write accounting\n";
}
