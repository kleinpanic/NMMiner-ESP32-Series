#ifndef _WMANAGER_H
#define _WMANAGER_H

void init_WifiManager();
void wifiManagerProcess();
void wifiRecoverConnection(); // called only by the Stratum task
void reset_configuration();
void request_configuration(); // open Setup once without erasing saved settings
String defaultAPName();   // setup AP name when the flasher set none

#endif // _WMANAGER_H
