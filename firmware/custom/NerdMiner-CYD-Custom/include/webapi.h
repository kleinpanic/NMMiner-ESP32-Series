#ifndef WEBAPI_H
#define WEBAPI_H

// Custom JSON HTTP API for the NerdMiner-CYD-Custom fork.
// Exposes mining status plus the SD and speaker features the local dashboard
// (apps/nmminer-control) drives. Started after WiFi connects; serviced
// non-blocking from the main loop so the mining hot path is never stalled.

void webApiSetup();   // create routes + server.begin() (call once, after WiFi up)
void webApiLoop();    // server.handleClient() (call every loop iteration)

#endif // WEBAPI_H
