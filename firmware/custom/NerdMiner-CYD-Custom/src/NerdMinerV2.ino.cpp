
#include <Wire.h>

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <OneButton.h>

#include "mbedtls/md.h"
#include "wManager.h"
#include "version.h"
#include "mining.h"
#include "monitor.h"
#include "drivers/displays/display.h"
#include "drivers/storage/SDCard.h"
#include "ShaTests/nerdSHA_HWTest.h"
#include "timeconst.h"
#include "speaker.h"
#include "sdstore.h"
#include "webapi.h"
#ifdef PANICMINER_UI
#include "systemHealth.h"
#endif

#ifdef TOUCH_ENABLE
#include "TouchHandler.h"
#endif

#include <soc/soc_caps.h>
//#define HW_SHA256_TEST

//3 seconds WDT
#define WDT_TIMEOUT 3
//15 minutes WDT for miner task
#define WDT_MINER_TIMEOUT 900

#ifdef PIN_BUTTON_1
  OneButton button1(PIN_BUTTON_1);
#endif

#ifdef PIN_BUTTON_2
  OneButton button2(PIN_BUTTON_2);
#endif

#ifdef TOUCH_ENABLE
extern TouchHandler touchHandler;
#endif

extern monitor_data mMonitor;

#ifdef SD_ID
  SDCard SDCrd = SDCard(SD_ID);
#else
  SDCard SDCrd = SDCard();
#endif

// Custom SD store for stats logging and config persistence (VSPI)
SDStore sdStore;

/**********************⚡ GLOBAL Vars *******************************/

unsigned long start = millis();
const char* ntpServer = "pool.ntp.org";

//void runMonitor(void *name);


/********* INIT *****/
void setup()
{
      //Init pin 15 to eneble 5V external power (LilyGo bug)
  #ifdef PIN_ENABLE5V
      pinMode(PIN_ENABLE5V, OUTPUT);
      digitalWrite(PIN_ENABLE5V, HIGH);
  #endif

#ifdef MONITOR_SPEED
    Serial.begin(MONITOR_SPEED);
#else
    Serial.begin(115200);
#endif //MONITOR_SPEED

  Serial.setTimeout(0);
  delay(SECOND_MS/10);

#ifdef SHA_BENCH
  // SHA engine bench instead of the miner (src/sha_bench.cpp); never returns.
  extern void sha_bench_run();
  sha_bench_run();
#endif

  esp_task_wdt_init(WDT_MINER_TIMEOUT, true);
  // Idle task that would reset WDT never runs, because core 0 gets fully utilized
  disableCore0WDT();
  //disableCore1WDT();

#ifdef HW_SHA256_TEST
  while (1) HwShaTest();
#endif

  // Setup the buttons
  #if defined(PIN_BUTTON_1) && !defined(PIN_BUTTON_2) //One button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(switchToNextScreen);
    button1.attachDoubleClick(alternateScreenRotation);
#ifdef PANICMINER_UI
    button1.attachLongPressStart(request_configuration);
#else
    button1.attachLongPressStart(reset_configuration);
#endif
    button1.attachMultiClick(alternateScreenState);
  #endif

  #if defined(PIN_BUTTON_1) && defined(PIN_BUTTON_2) //Button 1 of two button device
    button1.setPressMs(5*SECOND_MS);
    button1.attachClick(alternateScreenState);
    button1.attachDoubleClick(alternateScreenRotation);
  #endif

  #if defined(PIN_BUTTON_2) //Button 2 of two button device
    button2.setPressMs(5*SECOND_MS);
    button2.attachClick(switchToNextScreen);
#ifdef PANICMINER_UI
    button2.attachLongPressStart(request_configuration);
#else
    button2.attachLongPressStart(reset_configuration);
#endif
  #endif

  /******** INIT NERDMINER ************/
#ifdef PANICMINER_UI
  Serial.println("panicminer " CURRENT_VERSION " starting");
#else
  Serial.println("NerdMiner v2 starting......");
#endif

  /******** INIT DISPLAY ************/
  initDisplay();
  
  /******** PRINT INIT SCREEN *****/
  drawLoadingScreen();
  delay(2*SECOND_MS);

  /******** SHOW LED INIT STATUS (devices without screen) *****/
  mMonitor.NerdStatus = NM_waitingConfig;
  doLedStuff(0);

#ifdef SDMMC_1BIT_FIX
  SDCrd.initSDcard();
#endif

  /******** INIT SD STORE (VSPI for stats/config) *****/
  // Shared controller ownership now keeps SD initialization from resetting
  // the display. Probe before Wi-Fi so card failures are visible over USB
  // even when the saved access point is out of range.
  if (sdStore.mount()) {
    Serial.println("[Main] SD store mounted successfully");
  } else {
    Serial.println("[Main] Warning: SD store mount failed (SD is optional)");
  }

  /******** INIT WIFI ************/
  init_WifiManager();
#ifdef PANICMINER_UI
  systemHealthStart();
#endif

  /******** CREATE TASK TO PRINT SCREEN *****/
  //tft.pushImage(0, 0, MinerWidth, MinerHeight, MinerScreen);
  // Higher prio monitor task
  Serial.println("");
  Serial.println("Initiating tasks...");
  static const char monitor_name[] = "(Monitor)";
  #if defined(CONFIG_IDF_TARGET_ESP32)
  // Increased stack for ESP32 classic due to NVS operations  
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 9500, (void*)monitor_name, 5, NULL, APP_TASK_CORE);
  #else
  BaseType_t res1 = xTaskCreatePinnedToCore(runMonitor, "Monitor", 10000, (void*)monitor_name, 5, NULL, APP_TASK_CORE);
  #endif

  /******** CREATE STRATUM TASK *****/
  static const char stratum_name[] = "(Stratum)";
 #if defined(CONFIG_IDF_TARGET_ESP32) && !defined(ESP32_2432S028R) && !defined(ESP32_2432S028_2USB)
  // Reduced stack for ESP32 classic to save memory
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 12000, (void*)stratum_name, 4, NULL, APP_TASK_CORE);
 #elif defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  // Free a little bit of the heap to the screen
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 13500, (void*)stratum_name, 4, NULL, APP_TASK_CORE);
 #else
  BaseType_t res2 = xTaskCreatePinnedToCore(runStratumWorker, "Stratum", 15000, (void*)stratum_name, 4, NULL, APP_TASK_CORE);
 #endif

  /******** CREATE MINER TASKS *****/
  //for (size_t i = 0; i < THREADS; i++) {
  //  char *name = (char*) malloc(32);
  //  sprintf(name, "(%d)", i);

  // Start mining tasks
  //BaseType_t res = xTaskCreate(runWorker, name, 35000, (void*)name, 1, NULL);
  TaskHandle_t minerTask1, minerTask2 = NULL;
  #ifdef HARDWARE_SHA265
    #if defined(HW_MINER_OWNS_CORE1)
    // Core 1 alone, at a priority above everything else that might land
    // there: the pipelined loop counts engine cycles and yields only between
    // chunks. Monitor, Stratum, ChainFeed, the SW miner, Arduino's loopTask
    // and arduino_events (ARDUINO_*_RUNNING_CORE=0 in platformio.ini) run on
    // core 0 with the Wi-Fi and lwIP tasks. Core 1's idle task starves; its
    // task-watchdog check is off in the Arduino sdkconfig, and this task
    // feeds the miner watchdog once per chunk.
    xTaskCreatePinnedToCore(minerWorkerHw, "MinerHw-0", 3584, (void*)0, 19, &minerTask1, 1);
    #elif defined(CONFIG_IDF_TARGET_ESP32)
    xTaskCreate(minerWorkerHw, "MinerHw-0", 3584, (void*)0, 3, &minerTask1); // Reduced for ESP32 classic
    //xTaskCreate(minerWorkerSw, "MinerSw-0", 5000, (void*)0, 1, &minerTask1); // Reduced for ESP32 classic
    #else
    xTaskCreate(minerWorkerHw, "MinerHw-0", 4096, (void*)0, 3, &minerTask1);
    #endif
  #else
    #if defined(CONFIG_IDF_TARGET_ESP32)
    xTaskCreate(minerWorkerSw, "MinerSw-0", 5000, (void*)0, 1, &minerTask1); // Reduced for ESP32 classic
    #else
    xTaskCreate(minerWorkerSw, "MinerSw-0", 6000, (void*)0, 1, &minerTask1);
    #endif
  #endif
  esp_task_wdt_add(minerTask1);

#if (SOC_CPU_CORES_NUM >= 2)
  #if defined(HW_MINER_OWNS_CORE1)
  xTaskCreatePinnedToCore(minerWorkerSw, "MinerSw-1", 5000, (void*)1, 1, &minerTask2, 0);
  #elif defined(CONFIG_IDF_TARGET_ESP32)
  xTaskCreate(minerWorkerSw, "MinerSw-1", 5000, (void*)1, 1, &minerTask2); // Reduced for ESP32 classic
  #else
  xTaskCreate(minerWorkerSw, "MinerSw-1", 6000, (void*)1, 1, &minerTask2);
  #endif
  esp_task_wdt_add(minerTask2);
#endif

  vTaskPrioritySet(NULL, 4);

  /******** MONITOR SETUP *****/
  setup_monitor();

  /******** SPEAKER SETUP *****/
  Speaker::init();

  /******** CUSTOM WEB API *****/
  webApiSetup();
}

void app_error_fault_handler(void *arg) {
  // Get stack errors
  char *stack = (char *)arg;

  // Print the stack errors in the console
  esp_log_write(ESP_LOG_ERROR, "APP_ERROR", "Error Stack Code:\n%s", stack);

  // restart ESP32
  esp_restart();
}

// Periodic CSV logging (every ~60 seconds)
static unsigned long lastCSVLog = 0;
static const unsigned long CSV_LOG_INTERVAL = 60000;  // 60 seconds

void logCSVStats() {
  extern uint32_t shares;
  extern uint32_t hashes;
  extern uint32_t Mhashes;
  extern uint64_t upTime;

  if (!sdStore.isReady()) {
    return;
  }

  unsigned long now = millis();
  if (now - lastCSVLog < CSV_LOG_INTERVAL) {
    return;
  }
  lastCSVLog = now;

  // Log measured rates, not the old raw hash accumulator. No credentials.
  hash_rate_stats_t rates = getHashRateStats();
  char csvRow[192];
  snprintf(csvRow, sizeof(csvRow), "%lu,%.1f,%.1f,%lu,%lu,%.6g,%llu,%u,%.1f",
           now / 1000, rates.mean_10s, rates.mean_60s,
           (unsigned long)minerAcceptedShares(), (unsigned long)minerRejectedShares(),
           minerBestDifficulty(), upTime, ESP.getFreeHeap(), temperatureRead());

  sdStore.appendCSVLog(csvRow);
}

void loop() {
  // keep watching the push buttons:
  #ifdef PIN_BUTTON_1
    button1.tick();
  #endif

  #ifdef PIN_BUTTON_2
    button2.tick();
  #endif

#ifdef TOUCH_ENABLE
  touchHandler.isTouched();
#endif
  wifiManagerProcess(); // avoid delays() in loop when non-blocking and other long running code

  Speaker::update();
  webApiLoop();
  // Periodic SD CSV logging
  logCSVStats();

  vTaskDelay(50 / portTICK_PERIOD_MS);
}
