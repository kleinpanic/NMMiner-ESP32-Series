#include "sdstore.h"
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include "sdPath.h"

#if defined(ESP32_2432S028R) && defined(PANICMINER_UI)
#include <TFT_eSPI.h>
#include <esp32-hal-matrix.h>
#include <soc/gpio_sig_map.h>
// CYD has three wired SPI devices but only two usable hardware controllers.
// Touch owns HSPI. TFT_eSPI owns VSPI; a second SPIClass(VSPI) would reset its
// registers/pin routing and use a different transaction mutex. Share the TFT's
// actual SPIClass instead. Fan out clock/MOSI to both wired buses; separate CS
// pins select the device, while the SD's MISO is the only VSPI input.
#if defined(USE_HSPI_PORT) || !defined(TFT_MISO)
#error "CYD SD sharing requires TFT on VSPI with TFT_MISO=19"
#endif
static_assert(TFT_MISO == SD_MISO_PIN, "CYD VSPI input must be SD MISO");
static SPIClass& cardSPI() { return TFT_eSPI::getSPIinstance(); }
#else
static SPIClass vspi(VSPI);
static SPIClass& cardSPI() { return vspi; }
#endif
static SDStore* g_sdstore = nullptr;

SDStore::SDStore() : mounted_(false) {
  g_sdstore = this;
}

SDStore::~SDStore() {
  if (mounted_) {
    unmount();
  }
  g_sdstore = nullptr;
}

bool SDStore::mount() {
  if (mounted_) {
    return true;
  }

  Serial.println("[SD] Mounting VSPI SD card...");

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
#if defined(ESP32_2432S028R) && defined(PANICMINER_UI)
  // TFT is initialized first. Do not call begin() on another VSPI instance or
  // detach its pins. GPIO matrix outputs can drive both sets of clock/data
  // pins; both libraries then serialize on the same SPIClass transaction lock.
  pinMode(SD_CLK_PIN, OUTPUT);
  pinMode(SD_MOSI_PIN, OUTPUT);
  pinMatrixOutAttach(SD_CLK_PIN, VSPICLK_OUT_IDX, false, false);
  pinMatrixOutAttach(SD_MOSI_PIN, VSPID_OUT_IDX, false, false);
#else
  cardSPI().begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
#endif

  // Attempt to mount SD card with VSPI at specified frequency
  if (!SD.begin(SD_CS_PIN, cardSPI(), SD_FREQUENCY_MHZ * 1000000,
                "/sd", SD_MAX_OPEN_FILES, false)) {
    Serial.println("[SD] Failed to mount SD card on VSPI");
    mounted_ = false;
    return false;
  }

  // Verify filesystem is accessible
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("[SD] No SD card detected");
    SD.end();
    mounted_ = false;
    return false;
  }

  Serial.println("[SD] SD card mounted successfully");
  mounted_ = true;

  // Ensure /sd directory exists
  if (!SD.exists("/sd")) {
    if (!SD.mkdir("/sd")) {
      Serial.println("[SD] Warning: could not create /sd directory");
    }
  }

  // Ensure screensaver directory exists
  if (!SD.exists(SD_IMAGES_PATH)) {
    if (!SD.mkdir(SD_IMAGES_PATH)) {
      Serial.println("[SD] Warning: could not create " SD_IMAGES_PATH " directory");
    }
  }

  return true;
}

bool SDStore::unmount() {
  if (!mounted_) {
    return true;
  }

  Serial.println("[SD] Unmounting SD card");
  SD.end();
#if !defined(ESP32_2432S028R) || !defined(PANICMINER_UI)
  cardSPI().end();
#endif
  digitalWrite(SD_CS_PIN, HIGH);
  mounted_ = false;
  return true;
}

bool SDStore::isReady() const {
  return mounted_;
}

SDStatus SDStore::getStatus() {
  SDStatus status = {mounted_, 0, 0, 0};

  if (!mounted_) {
    return status;
  }

  // Get total and used space from SD card
  // ESP32 SD.totalBytes() and SD.usedBytes() return capacity info
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  uint64_t free = total - used;

  status.total_bytes = total;
  status.used_bytes = used;
  status.free_bytes = free;

  return status;
}

bool SDStore::writeConfig(const char* json_str) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot write config");
    return false;
  }

  if (!json_str) {
    return false;
  }

  // Remove existing config if present
  if (SD.exists(SD_CONFIG_PATH)) {
    SD.remove(SD_CONFIG_PATH);
  }

  // Write new config
  File configFile = SD.open(SD_CONFIG_PATH, FILE_WRITE);
  if (!configFile) {
    Serial.println("[SD] Failed to open config.json for writing");
    return false;
  }

  size_t written = configFile.print(json_str);
  configFile.close();

  if (written == 0) {
    Serial.println("[SD] Failed to write config.json");
    return false;
  }

  Serial.printf("[SD] Config written: %d bytes\n", written);
  return true;
}

bool SDStore::readConfig(char* buffer, size_t max_len) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot read config");
    return false;
  }

  if (!buffer || max_len == 0) {
    return false;
  }

  if (!SD.exists(SD_CONFIG_PATH)) {
    Serial.println("[SD] config.json not found");
    return false;
  }

  File configFile = SD.open(SD_CONFIG_PATH, FILE_READ);
  if (!configFile) {
    Serial.println("[SD] Failed to open config.json for reading");
    return false;
  }

  // Read up to max_len - 1 bytes to leave room for null terminator
  size_t read = configFile.readBytes(buffer, max_len - 1);
  configFile.close();

  // Null terminate
  if (read < max_len) {
    buffer[read] = '\0';
  } else {
    buffer[max_len - 1] = '\0';
  }

  Serial.printf("[SD] Config read: %d bytes\n", read);
  return (read > 0);
}

bool SDStore::rotateCSVIfNeeded() {
  if (!SD.exists(SD_CSV_PATH)) {
    return true;  // No file to rotate
  }

  size_t fileSize = getFileSize(SD_CSV_PATH);

  if (fileSize >= SD_CSV_MAX_SIZE) {
    // Rename current file to .csv.1
    String rotatedPath = String(SD_CSV_PATH) + ".1";

    // Remove old backup if exists
    if (SD.exists(rotatedPath.c_str())) {
      SD.remove(rotatedPath.c_str());
    }

    // Rename current to backup
    if (!SD.rename(SD_CSV_PATH, rotatedPath.c_str())) {
      Serial.println("[SD] CSV rotation failed");
      return false;
    }

    Serial.printf("[SD] CSV rotated: size was %d bytes\n", fileSize);
  }

  return true;
}

size_t SDStore::getFileSize(const char* path) {
  if (!SD.exists(path)) {
    return 0;
  }

  File file = SD.open(path, FILE_READ);
  if (!file) {
    return 0;
  }

  size_t size = file.size();
  file.close();
  return size;
}

bool SDStore::appendCSVLog(const char* csv_row) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot append CSV");
    return false;
  }

  if (!csv_row) {
    return false;
  }

  // Check if rotation is needed before appending
  if (!rotateCSVIfNeeded()) {
    return false;
  }

  // Open in append mode
  bool newFile = !SD.exists(SD_CSV_PATH);
  File csvFile = SD.open(SD_CSV_PATH, FILE_APPEND);
  if (!csvFile) {
    Serial.printf("[SD] Failed to open CSV for append\n");
    return false;
  }

  if (newFile) csvFile.println("uptime_s,khs_10s,khs_60s,accepted,rejected,best_diff,mining_s,heap_free_bytes,chip_temp_c");
  size_t written = csvFile.println(csv_row);
  csvFile.close();
  if (written != strlen(csv_row) + 2) return false;
  logged_rows_++;
  Serial.printf("[SD] Stats row %lu saved\n", (unsigned long)logged_rows_.load());
  return true;
}

bool SDStore::listFiles(const char* dir, std::vector<std::string>& filenames, bool include_dirs) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot list files");
    return false;
  }

  if (!dir) {
    return false;
  }

  // Sanitize directory path
  std::string safePath = sanitizePath(dir);
  if (safePath.empty()) {
    Serial.println("[SD] Invalid directory path");
    return false;
  }

  File dirFile = SD.open(safePath.c_str());
  if (!dirFile || !dirFile.isDirectory()) {
    Serial.printf("[SD] Failed to open directory: %s\n", safePath.c_str());
    dirFile.close();
    return false;
  }

  filenames.clear();

  File file = dirFile.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (include_dirs) {
        filenames.push_back(std::string(file.name()));
      }
    } else {
      filenames.push_back(std::string(file.name()));
    }
    file = dirFile.openNextFile();
  }

  dirFile.close();
  return true;
}

bool SDStore::deleteFile(const char* path) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot delete file");
    return false;
  }

  if (!path) {
    return false;
  }

  // Sanitize file path
  std::string safePath = sanitizePath(path);
  if (safePath.empty()) {
    Serial.println("[SD] Invalid file path for deletion");
    return false;
  }

  if (!SD.exists(safePath.c_str())) {
    Serial.printf("[SD] File does not exist: %s\n", safePath.c_str());
    return false;
  }

  if (!SD.remove(safePath.c_str())) {
    Serial.printf("[SD] Failed to delete file: %s\n", safePath.c_str());
    return false;
  }

  Serial.printf("[SD] File deleted: %s\n", safePath.c_str());
  return true;
}

bool SDStore::readFile(const char* path, uint8_t*& buffer, size_t& size) {
  if (!mounted_) {
    Serial.println("[SD] SD not mounted, cannot read file");
    buffer = nullptr;
    size = 0;
    return false;
  }

  if (!path) {
    buffer = nullptr;
    size = 0;
    return false;
  }

  // Sanitize file path
  std::string safePath = sanitizePath(path);
  if (safePath.empty()) {
    Serial.println("[SD] Invalid file path for reading");
    buffer = nullptr;
    size = 0;
    return false;
  }

  if (!SD.exists(safePath.c_str())) {
    Serial.printf("[SD] File does not exist: %s\n", safePath.c_str());
    buffer = nullptr;
    size = 0;
    return false;
  }

  File file = SD.open(safePath.c_str(), FILE_READ);
  if (!file) {
    Serial.printf("[SD] Failed to open file for reading: %s\n", safePath.c_str());
    buffer = nullptr;
    size = 0;
    return false;
  }

  size_t fileSize = file.size();

  // Enforce a download size limit (10 MB)
  const size_t MAX_DL_SIZE = 10 * 1024 * 1024;
  if (fileSize > MAX_DL_SIZE) {
    Serial.printf("[SD] File too large: %d bytes (max %d)\n", fileSize, MAX_DL_SIZE);
    file.close();
    buffer = nullptr;
    size = 0;
    return false;
  }

  // Allocate memory for the file
  buffer = (uint8_t*)malloc(fileSize);
  if (!buffer) {
    Serial.printf("[SD] Failed to allocate memory for file: %d bytes\n", fileSize);
    file.close();
    size = 0;
    return false;
  }

  // Read entire file into buffer
  size_t bytesRead = file.readBytes((char*)buffer, fileSize);
  file.close();

  if (bytesRead != fileSize) {
    Serial.printf("[SD] Failed to read entire file: %d of %d bytes\n", bytesRead, fileSize);
    free(buffer);
    buffer = nullptr;
    size = 0;
    return false;
  }

  size = fileSize;
  Serial.printf("[SD] File read: %s (%d bytes)\n", safePath.c_str(), size);
  return true;
}

std::string SDStore::sanitizePath(const char* path) {
  return normalizeSdPath(path);
}

fs::File SDStore::openRead(const char* path) {
  std::string safe = sanitizePath(path);
  return mounted_ && !safe.empty() ? SD.open(safe.c_str(), FILE_READ) : fs::File();
}
