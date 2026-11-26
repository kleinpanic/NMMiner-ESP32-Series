# Firmware provenance and source boundary

PanicMiner is an application built from the source in
[`firmware/custom/NerdMiner-CYD-Custom`](../firmware/custom/NerdMiner-CYD-Custom/).
It replaces the stock NMMiner application. Its mining, pool protocol, display,
touch, storage and sound behavior can be inspected and changed in that checkout.
It still uses Espressif's SDK, binary radio components and the ESP32's ROM and
hardware peripherals. This project does **not** claim a completely source-built
ESP32 platform or a complete reverse engineering of the stock firmware or chip.

This inventory was checked against the CYD source, installed package metadata
and a local `ESP32-2432S028R` linker map on 2026-11-25. It describes build inputs;
the [validation record](panicminer-validation-2026-11-25.md) identifies which
binary was actually tested on hardware.

## Application source map

Paths below are relative to the firmware checkout.

| Behavior | Source to inspect |
| --- | --- |
| Startup and task creation | `src/NerdMinerV2.ino.cpp` |
| Work scheduling, hardware/software mining and independent software verification of submitted candidates | `src/mining.cpp`, `src/miningBatches.h`, `src/miningJobQueue.h`, `src/ShaTests/nerdSHA256plus.h` |
| Classic ESP32 SHA256d assembly loop and self-test support | `src/pipelined_hw_sha_classic_v2.S`, `src/sha_bench_variants.inc`, `src/sha_bench*.cpp` |
| Bitcoin target decoding and comparison | `src/miningTarget.h`, `src/utils.cpp` |
| Stratum subscription, authorization, difficulty and share submission | `src/stratum.cpp`, `src/stratum.h`, `src/poolWatchdog.h` |
| Screens, fonts and animation | `src/ui/`, `src/drivers/displays/panicMinerDisplayDriver.cpp` |
| Touch bus, calibration and page debounce | `src/drivers/displays/panicMinerDisplayDriver.cpp`, `src/touchPager.h`; the source dependency is TFT_eTouch |
| Chain/price/address-balance data and TLS allocation | `src/chainFeed.cpp`, `src/chainTlsDiagnostics.cpp`, `src/walletStats.h`, `src/tlsRecordBuffers.h`, `src/tlsScratchBuffer.h` |
| Wi-Fi setup, runtime recovery, packet memory and saved configuration | `src/wManager.cpp`, `src/wifiRecovery.h`, `src/wifiMemory.cpp`, `src/wifiBufferBudget.h`, `src/drivers/storage/nvMemory.cpp` |
| SD logging and path validation | `src/sdstore.cpp`, `include/sdstore.h`, `src/sdPath.h` |
| Sound events, volume and mute | `src/speaker.cpp`, `include/speaker.h` |
| Local status/control API, bounded SD transfers and resource measurements | `src/webapi.cpp`, `src/sdPendingTransfer.h`, `src/sdTransferPolicy.h`, `src/pendingResponse.h`, `src/socketErrorState.h`, `src/monitor.cpp`, `src/systemHealth.cpp` |

The application originates from [BitMaker's NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2).
The pipelined classic ESP32 mining loop was ported from
[HenrysCat/NerdMiner-Modded](https://github.com/HenrysCat/NerdMiner-Modded), recorded
in the fork's [NOTICE](../firmware/custom/NerdMiner-CYD-Custom/NOTICE) as upstream
commit `522764d`. The fork retains the
[MIT license and attribution](../firmware/custom/NerdMiner-CYD-Custom/LICENSE).

The active PlatformIO environment compiles this checkout's application sources.
Its source filter and build hooks do not link a stock NMMiner application image,
execute its application, or call an NMMiner activation/license service. Historical
stock tooling, vendor documentation, binary investigations and the separately
reconstructed baseline in this parent workspace are separate from that build.
Locally retained full-flash recovery backups are device data, not source inputs.
The [stock NMMiner project](https://github.com/NMminer1024/NMMiner) is a separate
upstream; our MIT application license does not grant rights to its binaries.

## SDK and silicon boundary

The checked package is PlatformIO `espressif32` **6.6.0**, with
`framework-arduinoespressif32` **3.20014.231204**. Its headers identify Arduino
ESP32 **2.0.14** and ESP-IDF **4.4.6**. Arduino core and many IDF components have
published source, but this build consumes the SDK's prebuilt archives rather
than rebuilding the whole SDK from source.

The inspected linker map includes Espressif radio archives such as
`libnet80211.a`, `libpp.a` and `libphy.a`, along with SDK networking, crypto and
controller/coexistence support, including `libbtdm_app.a`. An archive appearing
in a map does not mean every function in it is retained or every radio feature
is enabled. The build also uses SDK ROM linker definitions such as
`esp32.rom.ld`. This project has neither replaced nor completely decoded those
binary components or the immutable ROM.

The mining assembly directly programs the ESP32 SHA peripheral. That driver
code and the software verification are visible; the implementation of the
silicon accelerator is not supplied as editable hardware source here. Using
the accelerator does not imply that a hidden stock NMMiner application runs
under PanicMiner.

Upstream source entry points are
[Arduino ESP32](https://github.com/espressif/arduino-esp32),
[ESP-IDF](https://github.com/espressif/esp-idf) and
[PlatformIO's Espressif platform](https://github.com/platformio/platform-espressif32).
Their component licenses apply independently. The installed Arduino package
declares `LGPL-2.1-or-later`; Espressif binary components have their own terms.
The fork's MIT license does not relicense them.

## Dependencies and reproducibility

The relevant configuration is the `[env:ESP32-2432S028R]` section of
[`platformio.ini`](../firmware/custom/NerdMiner-CYD-Custom/platformio.ini), not
the upstream multi-board default list. It selects `esp32dev`, Arduino,
`huge_app.csv`, `PANICMINER_UI`, pipelined assembly and the recorded CYD pins.
Both application event handling and Arduino's loop run on core 0; the hardware
miner uses core 1. Pre/post hooks derive version labels and merge generated
images; they do not import a vendor application. Factory packaging consumes
PlatformIO's resolved upload manifest, including the SDK boot-app image, and
rejects missing, overlapping or out-of-range inputs.

| Dependency | Declared selection | Inspected resolution / license |
| --- | --- | --- |
| Espressif platform | `espressif32@6.6.0` | Exact platform release; SDK boundary described above |
| LVGL | `8.3.11` | 8.3.11, MIT |
| ArduinoJson | `^6.21.5` | 6.21.6, MIT |
| WiFiManager | Git tag `v2.0.17` | 2.0.17, MIT |
| OneButton | `^2.6.1` | 2.6.2, BSD-3-Clause |
| NTPClient | `^3.2.1` | 3.2.1, MIT |
| TFT_eSPI | `^2.5.43`; a copy also exists under `lib/TFT_eSPI` | Both inspected copies identify 2.5.43; retain its license and embedded font notices |
| TFT_eTouch | Git URL without a revision | 0.6.0 at `24d4a29166283f1ae9fa4d2455cf46defca6a7be`, Boost Software License 1.0 |
| OpenFontRender | Git tag `v1.2` | 1.2.0; FreeType License, with individual MIT files |

The UI fonts retain their SIL Open Font License 1.1 notices in
[`src/ui/fonts/licenses/`](../firmware/custom/NerdMiner-CYD-Custom/src/ui/fonts/licenses/).
OpenFontRender's required FreeType attribution, for distributions containing it,
is: “Portions of this software are copyright © The FreeType Project
(www.freetype.org). All rights reserved.” Retain dependency license files with
source/binary distributions as their terms require.

The parent repository tracks the complete application source. It does not pin
every dependency: caret version ranges and the unversioned TFT_eTouch Git URL
can resolve differently on a fresh machine. The installed versions above are
an observed inventory, not a lockfile or a claim of bit-for-bit reproducibility.
For a release, record the resolved package list and Git revisions alongside the
application commit and output digests; immutable dependency revisions are needed
for a stronger reproducibility guarantee.

## Inspect the build and run checks

From the parent workspace, use the existing virtual environment and declared
[`requirements.txt`](../requirements.txt). All Python commands below use it.
These commands build and inspect files; they do not flash the board.

```bash
git status --short
git -C firmware/custom/NerdMiner-CYD-Custom status --short
./.venv/bin/python -m platformio pkg list -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
./.venv/bin/python -m platformio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R -v
```

Inspect `.pio/build/ESP32-2432S028R/firmware.map` within the firmware checkout
for linked application objects, SDK archives and ROM references. The verbose
build and compiler dependency files identify which of the two TFT_eSPI copies
was selected. Inspect `firmware.elf` with the installed Xtensa toolchain's
`nm`/`objdump` when symbol or assembly evidence is required. Record SHA-256
digests of the generated application, bootloader and partition images to tie
the inspection to the exact tested files. Build directories are local artifacts.

```bash
bash firmware/custom/NerdMiner-CYD-Custom/tools/tests/run.sh
bash tests/test_restore_full_flash_backup.sh
make -C firmware/custom/NerdMiner-CYD-Custom/tools/uisim THEME=1
make -C firmware/custom/NerdMiner-CYD-Custom/tools/uisim THEME=2
```

The firmware [host test guide](../firmware/custom/NerdMiner-CYD-Custom/tools/tests/README.md)
describes production-code tests for touch debounce, Stratum framing and
responses, work-progress timeouts, SD paths, target comparison and speaker
behavior. The UI renderer uses the same UI source. Host tests and a successful
link do not establish physical touch operation, SD/display coexistence, Wi-Fi
recovery, cold-power persistence, real pool acceptance or sustained hashrate.
Those results belong in the hardware validation record for the tested binary.

## Work, shares and payment

The firmware sends the configured public payout address/worker to the pool in
Stratum authorization. A successful authorization response confirms that the
pool accepted those credentials; it does not prove wallet ownership or a
payment. The firmware does not need a wallet seed or private key.

The ChainFeed also sends the configured public address, after removing its
worker suffix, to mempool.space's address endpoint over verified HTTPS. It
reads confirmed and pending address totals to populate Stats; no signing or
spending request exists. This discloses that public address to the explorer
service. Its returned balance can include transfers unrelated to mining. The
local status API exposes balances and freshness without returning the address.

An accepted share records pool-accepted work at the pool's share target. A
verified candidate or a higher best difficulty is not a balance increase.
In solo mining, a payout depends on finding a valid network-target block and
the pool's block publication and payout rules. The block/share counters and
sound events report mining events, not confirmed wallet transactions. Zero
accepted shares or blocks alone does not prove a fault; connection,
authorization, jobs, measured work and rejection evidence must be considered
together. This project promises neither revenue nor performance equivalent to
the stock vendor firmware.
