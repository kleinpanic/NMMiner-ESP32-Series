#include "webapi.h"

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>
#include <cctype>
#include <errno.h>
#include <lwip/sockets.h>

#include "speaker.h"
#include "sdstore.h"
#include "mining.h"
#include "monitor.h"
#include "chainFeed.h"
#include "sdPendingTransfer.h"
#include "sdTransferPolicy.h"
#include "sdPath.h"

// Mining globals (defined in mining.cpp) — the same ones logCSVStats() uses.
extern volatile uint32_t shares;
extern uint32_t Mhashes;
extern uint64_t upTime;

// Global SD store instance (defined in NerdMinerV2.ino.cpp).
extern SDStore sdStore;

static WebServer apiServer(80);
static bool apiStarted = false;

// --- helpers ---------------------------------------------------------------

// LAN-trust model: the dashboard talks to its own Flask proxy (server-side),
// which then calls this API — the browser never calls the miner directly. So we
// do NOT emit a wildcard Access-Control-Allow-Origin: that header is unnecessary
// for the proxy and would needlessly widen the cross-origin/CSRF surface. JSON
// bodies on POST endpoints also force a CORS preflight for any cross-site caller,
// which (absent ACAO) the browser blocks.
static void sendJson(int code, const String& body) {
  apiServer.send(code, "application/json", body);
}

// Whitelist a filename for the Content-Disposition header (prevent quote/CRLF
// breakout / header injection).
static std::string headerSafeName(const std::string& in) {
  std::string out;
  for (char c : in) {
    out += (isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-') ? c : '_';
  }
  return out.empty() ? "download" : out;
}

static void sendOk() { sendJson(200, "{\"ok\":true}"); }

static void sendError(int code, const char* msg) {
  StaticJsonDocument<128> doc;
  doc["ok"] = false;
  doc["error"] = msg;
  String out;
  serializeJson(doc, out);
  sendJson(code, out);
}

// Validate an SD path at the API boundary (defense in depth — sdstore also
// sanitizes). Returns sanitized path, or empty string if rejected.
static std::string apiSafePath(const String& raw) {
  if (raw.length() == 0) return std::string();
  return publicSdPath(raw.c_str(), SD_CONFIG_PATH);
}

// Keep the SDK's shared socket/file handles after WebServer releases its
// request. Only one 512-byte block is sent per Arduino loop pass; buttons,
// speaker updates, SD logging and other API requests keep running between them.
static fs::File sdTransferFile;
static WiFiClient sdTransferClient;
static size_t sdTransferLength = 0, sdTransferSent = 0;
static uint32_t sdTransferStarted = 0;
static bool sdTransferActive = false;
static SdPendingTransfer sdPending;

static void finishSdTransfer(bool complete) {
  sdTransferFile.close();
  sdTransferClient.stop();
  sdTransferActive = false;
  sdPending.reset();
  if (!complete)
    Serial.printf("[WEBAPI] SD transfer incomplete: %u/%u bytes; heap %u, largest %u\n",
                  (unsigned)sdTransferSent, (unsigned)sdTransferLength,
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

static void sendSdFile(fs::File& file, const char* contentType) {
  if (sdTransferActive) { sendError(503, "download busy"); return; }
  sdTransferLength = file.size();
  sdTransferSent = 0;
  sdTransferStarted = millis();
  apiServer.setContentLength(sdTransferLength);
  apiServer.send(200, contentType, "");
  sdTransferClient = apiServer.client();
  sdTransferClient.setTimeout(1);
  // Coalesce bulk data rather than allocating a packet for every small write.
  sdTransferClient.setNoDelay(false);
  sdPending.reset();
  sdTransferFile = file;
  // File::close closes the shared underlying file, so release the handler's
  // copy without closing it. The transfer now owns its lifetime.
  file = fs::File();
  sdTransferActive = true;
}

static void advanceSdTransfer() {
  if (!sdTransferActive) return;
  if (sdTransferSent == sdTransferLength) { finishSdTransfer(true); return; }
  // This SDK's connected() uses recv(..., 0) plus errno; another request can
  // leave a stale socket error and reject a healthy retained connection.
  // write() detects actual peer failure. Temporary low memory waits without
  // reading the file or queuing another packet, within the total deadline.
  const SdTransferStep step = sdTransferStep(sdTransferClient.fd() >= 0,
    uint32_t(millis() - sdTransferStarted), ESP.getFreeHeap(),
    heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
  if (step == SdTransferStep::Abort) { finishSdTransfer(false); return; }
  if (step == SdTransferStep::Wait) return;
  const size_t left = sdTransferLength - sdTransferSent;
  const SdWriteResult progress = sdPending.advance(left,
    [&](uint8_t* buffer, size_t n) { return sdTransferFile.read(buffer, n); },
    [&](const uint8_t* buffer, size_t n) {
      const int sent = ::send(sdTransferClient.fd(), buffer, n, MSG_DONTWAIT);
      if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK ||
                       errno == ENOMEM || errno == ENOBUFS)) return 0;
      return sent;
    });
  sdTransferSent += progress.sent;
  if (progress.state == SdWriteState::Failed) finishSdTransfer(false);
  else if (sdTransferSent == sdTransferLength) finishSdTransfer(true);
}

// --- handlers --------------------------------------------------------------

static void handleStatus() {
  StaticJsonDocument<1280> doc;
  JsonObject miner = doc.createNestedObject("miner");
  miner["sharesAccepted"] = minerAcceptedShares();
  miner["sharesRejected"] = minerRejectedShares();
  miner["poolAuthorized"] = minerIsSubscribed();
  miner["bestVerifiedDifficulty"] = minerBestDifficulty();
  miner["poolDifficulty"] = minerPoolDifficulty();
  hash_rate_stats_t rates = getHashRateStats();
  miner["hashrate10sKhs"] = rates.mean_10s;
  miner["hashrate60sKhs"] = rates.mean_60s;
  miner["heapFreeBytes"] = ESP.getFreeHeap();
  miner["heapTotalBytes"] = ESP.getHeapSize();
  miner["chipTempC"] = temperatureRead();
  miner["totalMHashes"] = minerTotalHashes() / 1000000.0;
  miner["uptimeSeconds"] = upTime;

  SDStatus st = sdStore.getStatus();
  JsonObject sd = doc.createNestedObject("sd");
  sd["mounted"] = st.mounted;
  sd["rowsSaved"] = sdStore.loggedRows();
  sd["total"] = st.total_bytes;
  sd["used"] = st.used_bytes;
  sd["free"] = st.free_bytes;

  JsonObject sp = doc.createNestedObject("speaker");
  sp["volume"] = Speaker::getVolume();
  sp["mute"] = Speaker::isMuted();

#ifdef PANICMINER_UI
  chain_snapshot_t snapshot{};
  chainFeedGet(&snapshot);
  JsonObject wallet = doc.createNestedObject("wallet");
  wallet["valid"] = snapshot.wallet_valid;
  if (snapshot.wallet_valid) {
    wallet["balanceSats"] = snapshot.wallet_sats;
    wallet["pendingSats"] = snapshot.wallet_pending_sats;
    wallet["ageSeconds"] = (millis() - snapshot.wallet_updated_ms) / 1000;
  }
#endif

  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleSpeakerGet() {
  StaticJsonDocument<128> doc;
  doc["volume"] = Speaker::getVolume();
  doc["mute"] = Speaker::isMuted();
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static int bodyInt(const char* key, int fallback) {
  if (!apiServer.hasArg("plain")) return fallback;
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, apiServer.arg("plain"))) return fallback;
  if (doc[key].isNull()) return fallback;
  return doc[key].as<int>();
}

static void handleSpeakerVolume() {
  // Dashboard contract: { "volume_percent": 0-100 }
  int v = bodyInt("volume_percent", -1);
  if (v < 0 || v > 100) { sendError(400, "volume_percent 0-100 required"); return; }
  Speaker::setVolume((uint8_t)v);
  sendOk();
}

static void handleSpeakerMute() {
  // Dashboard contract: { "muted": true|false }
  if (!apiServer.hasArg("plain")) { sendError(400, "muted required"); return; }
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, apiServer.arg("plain")) || doc["muted"].isNull()) {
    sendError(400, "muted required");
    return;
  }
  Speaker::setMute(doc["muted"].as<bool>());
  sendOk();
}

static void handleSpeakerTestTone() {
  // Dashboard contract: optional { "freq_hz", "duration_ms" }
  int freq = bodyInt("freq_hz", 0);
  int dur = bodyInt("duration_ms", 0);
  if (freq > 0 && dur > 0) {
    Speaker::playTone(freq, dur);
  } else {
    Speaker::playTestTone();
  }
  sendOk();
}

static void handleSdList() {
  std::string path = apiSafePath(apiServer.hasArg("path") ? apiServer.arg("path") : String("/sd"));
  if (path.empty()) { sendError(400, "invalid path"); return; }
  std::vector<std::string> names;
  if (!sdStore.listFiles(path.c_str(), names, true)) { sendError(503, "sd unavailable"); return; }
  DynamicJsonDocument doc(4096);
  doc["ok"] = true;
  doc["path"] = path.c_str();
  JsonArray arr = doc.createNestedArray("entries");
  for (const auto& n : names) arr.add(n.c_str());
  String out;
  serializeJson(doc, out);
  sendJson(200, out);
}

static void handleSdDownload() {
  if (!apiServer.hasArg("path")) { sendError(400, "path required"); return; }
  std::string path = apiSafePath(apiServer.arg("path"));
  if (path.empty()) { sendError(400, "invalid path"); return; }
  if (sdTransferActive) { sendError(503, "download busy"); return; }
  fs::File file = sdStore.openRead(path.c_str());
  if (!file || file.isDirectory()) { sendError(404, "not found"); return; }
  // Sanitized basename for the download filename.
  std::string base = path;
  size_t slash = base.find_last_of('/');
  if (slash != std::string::npos) base = base.substr(slash + 1);
  std::string safeBase = headerSafeName(base);
  apiServer.sendHeader("Content-Disposition", String("attachment; filename=\"") + safeBase.c_str() + "\"");
  apiServer.sendHeader("X-Content-Type-Options", "nosniff");
  sendSdFile(file, "application/octet-stream");
  file.close();
}

static void handleSdDelete() {
  if (!apiServer.hasArg("plain")) { sendError(400, "path required"); return; }
  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, apiServer.arg("plain")) || doc["path"].isNull()) {
    sendError(400, "path required");
    return;
  }
  std::string path = apiSafePath(String(doc["path"].as<const char*>()));
  if (path.empty()) { sendError(400, "invalid path"); return; }
  if (!sdStore.deleteFile(path.c_str())) { sendError(404, "delete failed"); return; }
  sendOk();
}

static void handleSdLog() {
  if (sdTransferActive) { sendError(503, "download busy"); return; }
  fs::File file = sdStore.openRead(SD_CSV_PATH);
  if (!file) { sendError(404, "no log"); return; }
  sendSdFile(file, "text/csv");
  file.close();
}

// --- lifecycle -------------------------------------------------------------

void webApiSetup() {
  if (apiStarted) return;

  apiServer.on("/api/status", HTTP_GET, handleStatus);
  apiServer.on("/api/system/info", HTTP_GET, handleStatus);  // dashboard alias

  apiServer.on("/api/speaker", HTTP_GET, handleSpeakerGet);
  apiServer.on("/api/speaker/volume", HTTP_POST, handleSpeakerVolume);
  apiServer.on("/api/speaker/mute", HTTP_POST, handleSpeakerMute);
  apiServer.on("/api/speaker/test-tone", HTTP_POST, handleSpeakerTestTone);

  apiServer.on("/api/sd/list", HTTP_GET, handleSdList);
  apiServer.on("/api/sd/download", HTTP_GET, handleSdDownload);
  apiServer.on("/api/sd/delete", HTTP_POST, handleSdDelete);
  apiServer.on("/api/sd/log", HTTP_GET, handleSdLog);

  apiServer.onNotFound([]() { sendError(404, "not found"); });

  apiServer.begin();
  apiStarted = true;
  Serial.println("[WEBAPI] Custom JSON API started on :80");
}

void webApiLoop() {
  if (apiStarted) {
    apiServer.handleClient();
    advanceSdTransfer();
  }
}
