#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p tools/uisim/build-tests
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Werror -DPANICMINER_UI \
  -Itools/tests/speaker_stubs -Iinclude \
  tools/tests/test_speaker.cpp src/speaker.cpp -o tools/uisim/build-tests/test_speaker
./tools/uisim/build-tests/test_speaker
