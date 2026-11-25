---
phase: 07-on-device-validation
reviewed: 2026-06-19T04:46:42Z
depth: deep
files_reviewed: 8
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/utils.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/stratum.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
  - firmware/custom/NerdMiner-CYD-Custom/include/webapi.h
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.h
findings:
  critical: 3
  warning: 2
  info: 0
  total: 5
status: superseded_fixed_later
superseded_by: ".planning/phases/07-on-device-validation/07-VALIDATION.md"
supersession_note: "The 5 review findings were later fixed, built, flashed, and read back in the review-05/webfix2 live app chain. Keep this report as historical review evidence, not current open blocker state."
---

# Phase 7: Code Review Report

**Reviewed:** 2026-06-19T04:46:42Z
**Depth:** deep
**Files Reviewed:** 8
**Status:** issues_found

> Supersession note, 2026-06-19: this report is historical. Its five findings
> were later fixed in source and included in the later live app chain recorded in
> `07-VALIDATION.md`. Do not treat `issues_found` below as the current blocker
> state; runtime mining UAT remains open, but these specific code-review issues
> are not the active route.

## Summary

Reviewed the requested post-forensics firmware files with cross-file tracing through mining, stratum parsing, config persistence, WiFiManager import paths, and the custom web API gate. The API sensitive-route gating and stratum authorize/submit serial redaction are materially improved, but there are still mining/runtime blockers before Wi-Fi/pool UAT can be trusted.

Supporting dependency files were inspected only to validate call chains (`src/drivers/storage/storage.h` for pool parsing and `sdstore` for API SD path behavior). No firmware source files were modified.

## Narrative Findings (AI reviewer)

The findings below are from direct deep review of the requested firmware files and their immediate call-chain dependencies.

## Critical Issues

### CR-01 [BLOCKER]: Share Validation Reads The Wrong Target And Can Run Out Of Bounds

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/utils.cpp:128`

**Issue:** `checkValid()` copies from `&target` instead of `target`, so it compares a share hash against bytes from the pointer variable and adjacent stack memory, not against `mMiner.bytearray_target`. The reverse loop uses `uint8_t i` with `i >= 0`, which underflows after `0` and can read outside both 32-byte arrays. This path is reachable from `mining.cpp:709` whenever a 32-bit share is submitted, so accepted-share/runtime UAT can crash or misclassify valid shares.

**Fix:**

```cpp
bool checkValid(const unsigned char* hash, const unsigned char* target) {
  if (!hash || !target) return false;

  unsigned char diff_target[32];
  memcpy(diff_target, target, sizeof(diff_target));
  reverse_bytes(diff_target, sizeof(diff_target));

  for (int i = 31; i >= 0; --i) {
    if (hash[i] > diff_target[i]) return false;
    if (hash[i] < diff_target[i]) return true;
  }
  return true;
}
```

Also update the declaration in `src/utils.h` to take `const unsigned char*`.

### CR-02 [BLOCKER]: Malformed Stratum `nbits` Can Write Outside The Target Buffer

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/utils.cpp:197`

**Issue:** `calculateMiningData()` derives `zeros` from pool-controlled `mJob.nbits` and immediately writes `mJob.nbits.length() - 2` bytes to `target + zeros - 2` without validating length, hex content, or offset bounds. A malformed or hostile stratum notify can make the destination negative or beyond the 64-byte target buffer before mining starts.

**Fix:**

```cpp
static bool isHexString(const String& s) {
  for (size_t i = 0; i < s.length(); ++i) {
    if (!isxdigit((unsigned char)s[i])) return false;
  }
  return true;
}

if (mJob.nbits.length() != 8 || !isHexString(mJob.nbits)) {
  Serial.println(F("[WORKER] invalid nbits"));
  return mMiner;
}

int zeros = (int)strtol(mJob.nbits.substring(0, 2).c_str(), nullptr, 16) - 3;
int offset = zeros - 2;
size_t copyLen = mJob.nbits.length() - 2;
if (offset < 0 || (size_t)offset + copyLen > TARGET_BUFFER_SIZE) {
  Serial.println(F("[WORKER] nbits target out of range"));
  return mMiner;
}
memcpy(target + offset, mJob.nbits.substring(2).c_str(), copyLen);
```

### CR-03 [BLOCKER]: Invalid URL Ports Are Persisted As Pool Hostnames

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:107`, `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp:529`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/storage/nvMemory.cpp:15`

**Issue:** All config/import paths delegate pool parsing to `nm_pool_parse_endpoint()`, but callers do not get a hard failure when a URL includes an invalid port. The helper only updates `port` when the suffix is valid; otherwise it leaves the `host:bad-port` suffix in the host and returns success. That bad hostname is saved through NVS/API/SD paths and later fails DNS in `mining.cpp:139`, leaving the miner in connecting/no-mining state.

**Fix:** Make parsing transactional and reject any colon-suffixed endpoint with a non-numeric or out-of-range port, then have reviewed call sites keep the previous settings on failure.

```cpp
static bool applyPoolUrl(const char* raw, String& host, int& port) {
  String parsedHost = host;
  int parsedPort = port;
  if (!nm_pool_parse_endpoint(raw, parsedHost, parsedPort)) {
    Serial.println(F("[CFG] Ignoring invalid pool endpoint"));
    return false;
  }
  host = parsedHost;
  port = parsedPort;
  return true;
}
```

The shared parser should treat `host:abc`, `host:0`, `host:70000`, and trailing `host:` as invalid instead of accepting them as hostnames.

## Warnings

### WR-01 [WARNING]: Saved Mining Stats Are Not Reliably Persisted Or Restored

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp:1277`

**Issue:** `restoreStat()` ignores the `nvs_open()` and `nvs_get_*()` return values, then uses local values such as `nv_shares`, `nv_valids`, and `nv_crc` even when the keys were not read. `saveStat()` writes NVS keys at `mining.cpp:1315` but never calls `nvs_commit()`, so the stats feature can silently fail to persist or restore corrupted in-memory values.

**Fix:** Initialize locals, check every NVS return, reset to zero on missing/corrupt state, and commit writes.

```cpp
esp_err_t ret = nvs_open("state", NVS_READWRITE, &stat_handle);
if (ret != ESP_OK) {
  resetStat();
  return;
}

uint32_t nv_shares = 0, nv_valids = 0, nv_crc = 0;
if (nvs_get_u32(stat_handle, "shares", &nv_shares) != ESP_OK ||
    nvs_get_u32(stat_handle, "valids", &nv_valids) != ESP_OK ||
    nvs_get_u32(stat_handle, "crc32", &nv_crc) != ESP_OK) {
  resetStat();
  return;
}

// after nvs_set_* in saveStat()
nvs_commit(stat_handle);
```

### WR-02 [WARNING]: Pool Passwords Are Truncated Before Stratum Authorization

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp:420`

**Issue:** Runtime settings allow longer pool passwords (`Settings.PoolPassword` is 80 bytes and fallback password is a `String`), but `mining_subscribe::wPass` is only 20 bytes in `stratum.h:24`. `strlcpy()` silently truncates before `tx_mining_auth()`, so any pool password longer than 19 characters will be saved correctly but authorization will fail at runtime.

**Fix:** Match the worker buffer to the configured field size or keep credentials as `String`, and reject/log truncation before attempting to authorize.

```cpp
typedef struct {
  String sub_details;
  String extranonce1;
  String extranonce2;
  int extranonce2_size;
  char wName[96];
  char wPass[80];
} mining_subscribe;
```

---

_Reviewed: 2026-06-19T04:46:42Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
