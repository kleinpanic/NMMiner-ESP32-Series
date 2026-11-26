#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BUILD="$ROOT/tools/uisim/build-tests"
JSON="$ROOT/.pio/libdeps/ESP32-2432S028R/ArduinoJson/src"
CXX=${CXX:-g++}
mkdir -p "$BUILD"
if [[ ! -f "$JSON/ArduinoJson.h" ]]; then
  echo "ArduinoJson dependency missing. Build ESP32-2432S028R with PlatformIO first." >&2
  exit 1
fi
COMMON=(-std=c++17 -g -O1 -Wall -Wextra -Wno-format -fsanitize=address,undefined -fno-omit-frame-pointer)
"$CXX" "${COMMON[@]}" -I"$ROOT/src" "$ROOT/tools/tests/test_touch.cpp" -o "$BUILD/test_touch"
"$BUILD/test_touch"
for TEST in sd_path pool_watchdog mining_target mining_batches wifi_recovery wallet_stats tls_record_buffers chain_http; do
  "$CXX" "${COMMON[@]}" -I"$ROOT/src" "$ROOT/tools/tests/test_$TEST.cpp" -o "$BUILD/test_$TEST"
  "$BUILD/test_$TEST"
done
LVGL="$ROOT/.pio/libdeps/ESP32-2432S028R/lvgl"
TLSF_FLAGS=(-DLV_CONF_INCLUDE_SIMPLE -I"$ROOT/src/ui" -I"$LVGL")
"${CC:-gcc}" -std=c11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -ffunction-sections \
  "${TLSF_FLAGS[@]}" -c "$LVGL/src/misc/lv_tlsf.c" -o "$BUILD/lv_tlsf_test.o"
"$CXX" "${COMMON[@]}" -std=c++11 "${TLSF_FLAGS[@]}" -I"$ROOT/src" \
  "$ROOT/tools/tests/test_tls_scratch_buffer.cpp" "$BUILD/lv_tlsf_test.o" -Wl,--gc-sections -o "$BUILD/test_tls_scratch_buffer"
"$BUILD/test_tls_scratch_buffer"
"$CXX" "${COMMON[@]}" -DPANICMINER_UI -I"$ROOT/tools/tests/stubs" -I"$ROOT/src" -I"$JSON" \
  "$ROOT/src/stratum.cpp" "$ROOT/tools/tests/test_stratum.cpp" -o "$BUILD/test_stratum"
"$BUILD/test_stratum"
"$CXX" "${COMMON[@]}" -DPANICMINER_UI -I"$ROOT/tools/tests/stubs" -I"$ROOT/src" -I"$JSON" \
  "$ROOT/src/utils.cpp" "$ROOT/tools/tests/test_mining_data.cpp" -lcrypto -o "$BUILD/test_mining_data"
"$BUILD/test_mining_data"
"$CXX" "${COMMON[@]}" -DPANICMINER_UI -DESP32_2432S028R -DTFT_MISO=19 \
  -I"$ROOT/tools/tests/sd_stubs" -I"$ROOT/include" -I"$ROOT/src" \
  "$ROOT/src/sdstore.cpp" "$ROOT/tools/tests/test_sd_store.cpp" -o "$BUILD/test_sd_store"
"$BUILD/test_sd_store"
if [[ -f "$ROOT/tools/tests/run_speaker.sh" ]]; then
  bash "$ROOT/tools/tests/run_speaker.sh"
fi
