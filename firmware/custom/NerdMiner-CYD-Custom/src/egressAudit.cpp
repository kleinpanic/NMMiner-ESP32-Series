#include "egressAudit.h"
#ifdef PANICMINER_UI
#include <Arduino.h>
#include <tcpip_adapter.h>
#include <lwip/netif.h>
#include <lwip/pbuf.h>
#include <lwip/tcpip.h>

namespace {
struct Entry { EgressEndpoint endpoint{}; uint32_t packets = 0; };
Entry entries[24];
size_t used = 0;
uint32_t packets = 0, arp = 0, ipv6 = 0, other = 0, overflow = 0;
netif* interface = nullptr;
netif_linkoutput_fn previous = nullptr;

err_t observe(netif* source, pbuf* packet) {
    uint8_t header[78];
    const size_t size = pbuf_copy_partial(packet, header, sizeof(header), 0);
    ++packets;
    EgressEndpoint endpoint{};
    if (egressEndpoint(header, size, endpoint)) {
        const size_t transport = 14 + (header[14] & 15) * 4;
        // Bucket replies from our HTTP server by destination address. Client
        // ephemeral ports would otherwise exhaust the bounded table during a
        // long API test. Port 0 in this TCP bucket means an HTTP response.
        if (endpoint.protocol == 6 && size >= transport + 4 &&
            header[transport] == 0 && header[transport + 1] == 80)
            endpoint.port = 0;
        size_t slot = 0;
        for (; slot < used; ++slot) {
            const auto& e = entries[slot].endpoint;
            if (e.address == endpoint.address && e.port == endpoint.port && e.protocol == endpoint.protocol) break;
        }
        if (slot < used) ++entries[slot].packets;
        else if (used < sizeof(entries) / sizeof(entries[0])) {
            entries[used].endpoint = endpoint;
            entries[used++].packets = 1;
        }
        else ++overflow;
    } else if (size >= 14 && header[12] == 0x08 && header[13] == 0x06) ++arp;
    else if (size >= 14 && header[12] == 0x86 && header[13] == 0xdd) ++ipv6;
    else ++other;
    // Forward the original pbuf unchanged. Never retain payloads or allocate.
    return previous(source, packet);
}

void install(void*) {
    if (interface) return;
    void* pointer = nullptr;
    if (tcpip_adapter_get_netif(TCPIP_ADAPTER_IF_STA, &pointer) != ESP_OK || !pointer) {
        Serial.println("[EGRESS] STA interface unavailable");
        return;
    }
    auto* candidate = static_cast<netif*>(pointer);
    if (!candidate->linkoutput) { Serial.println("[EGRESS] link output unavailable"); return; }
    interface = candidate;
    previous = interface->linkoutput;
    interface->linkoutput = observe;
    Serial.println("[EGRESS] STA header audit active; no payload retention");
}

void dump(void*) {
    Serial.printf("[EGRESS] packets %lu arp %lu ipv6 %lu other %lu overflow %lu active %d\n",
                  (unsigned long)packets, (unsigned long)arp, (unsigned long)ipv6,
                  (unsigned long)other, (unsigned long)overflow,
                  interface && interface->linkoutput == observe);
    for (size_t i = 0; i < used; ++i) {
        const auto& entry = entries[i];
        const auto& e = entry.endpoint;
        Serial.printf("[EGRESS] dst %u.%u.%u.%u port %u proto %u packets %lu\n",
                      unsigned(e.address >> 24), unsigned((e.address >> 16) & 255),
                      unsigned((e.address >> 8) & 255), unsigned(e.address & 255),
                      e.port, e.protocol, (unsigned long)entry.packets);
    }
    Serial.println("[EGRESS] END");
}
}

// Both pointer installation and reads run on lwIP's owner thread. The callback
// does not change routing, DNS, TLS, socket behavior or packet lifetime.
void egressAuditStart() {
    if (tcpip_callback(install, nullptr) != ERR_OK) Serial.println("[EGRESS] install queue failed");
}
void egressAuditDump() {
    if (tcpip_callback(dump, nullptr) != ERR_OK) Serial.println("[EGRESS] dump queue failed");
}
#else
void egressAuditStart() {}
void egressAuditDump() {}
#endif
