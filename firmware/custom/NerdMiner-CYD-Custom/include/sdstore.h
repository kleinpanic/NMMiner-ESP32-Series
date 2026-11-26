#ifndef SDSTORE_H_
#define SDSTORE_H_

#include <Arduino.h>
#include <vector>
#include <string>
#include <cstdint>
#include <atomic>
#include <FS.h>

// VSPI SD Card pins for CYD ESP32-2432S028R
#define SD_CS_PIN   GPIO_NUM_5
#define SD_CLK_PIN  GPIO_NUM_18
#define SD_MISO_PIN GPIO_NUM_19
#define SD_MOSI_PIN GPIO_NUM_23
#define SD_FREQUENCY_MHZ 20

// CSV rotation settings
#define SD_CSV_MAX_SIZE (1024 * 1024)  // 1 MB before rotation
#define SD_CSV_PATH "/sd/mining_stats_v2.csv"
#define SD_CONFIG_PATH "/sd/config.json"
#define SD_IMAGES_PATH "/sd/screensaver"

struct SDStatus {
  bool mounted;
  uint64_t total_bytes;
  uint64_t used_bytes;
  uint64_t free_bytes;
};

class SDStore {
public:
  SDStore();
  ~SDStore();

  // Lifecycle
  bool mount();
  bool unmount();
  bool isReady() const;
  uint32_t loggedRows() const { return logged_rows_.load(); }

  // Status and capacity
  SDStatus getStatus();

  // Config file I/O (config.json)
  bool writeConfig(const char* json_str);
  bool readConfig(char* buffer, size_t max_len);

  // CSV logging with rotation
  bool appendCSVLog(const char* csv_row);

  // File operations (with path sanitization)
  bool listFiles(const char* dir, std::vector<std::string>& filenames, bool include_dirs = false);
  bool deleteFile(const char* path);
  bool readFile(const char* path, uint8_t*& buffer, size_t& size);
  fs::File openRead(const char* path);

  // Path sanitization (static utility)
  static std::string sanitizePath(const char* path);

private:
  bool mounted_;
  std::atomic<uint32_t> logged_rows_{0};

  // Helper: check and rotate CSV if needed
  bool rotateCSVIfNeeded();

  // Helper: get file size
  size_t getFileSize(const char* path);
};

#endif // SDSTORE_H_
