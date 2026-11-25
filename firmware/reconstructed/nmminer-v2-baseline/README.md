# NMMiner v2.0.02 ST7789 Reconstructed Baseline

This tree is the source-recovery baseline for the official NMMiner
`v2.0.02` `esp32-2432s028r-st7789` firmware already extracted in this repo.

It is not the public NerdMiner fork and it is not a UI rewrite. The source in
this directory is built from the local official binary recovery artifacts:

- `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin`
- `hardware/recovered-assets/nmminer-app.elf`
- `hardware/recovered-assets/irom.asm`
- `hardware/recovered-assets/drom.bin`
- `hardware/recovered-assets/decompiled/`

## Current Scope

The current code captures the parts of the official binary that are already
source-recovered with strong evidence:

- verified app VM-to-file address mapping;
- official app segment mapping including DRAM/IRAM, and the recovered IRAM1
  entrypoint evidence at `0x40086aa4`;
- official app lifecycle/task evidence: LVGL failure, hardware init,
  peripherals-ready logs, the `0x400de1e4` begin-style bring-up route,
  `NMMiner::begin: all tasks started` at `0x400de25d`, and task registrations
  for `(button)`, `(scan)`, `(swarm)`, `(stratum)`, and `(tMiner)`, plus
  `nMiner` name/object setup at `0x400dae29` and helper evidence at
  `0x400dae8a`;
- raw official startup literals and ordered startup steps recovered from
  `Official_startup.recovered.c`, including the BSS clear range
  `0x3ffc4100..0x3ffd6b30`, constructor predicate/sweep
  `0x40130cd0 -> 0x40086910`, framework hooks, and scheduler handoff
  `0x4008766c`;
- a typed official app model summary covering bring-up steps, task
  registrations, runtime service slots, and source-baseline gates so the host
  tool and ESP32 compile-only harness expose what is recovered versus still
  blocked;
- a lifecycle validation layer that turns the decoded entrypoint, BSS clear,
  constructor sweep, scheduler handoff, begin route, required task
  registrations, runtime service slots, and write gates into explicit pass/open
  checks. Current validation is `8/10`; behavior parity and fresh-backup write
  gate remain open, so `flashable=0`;
- LVGL v8 screen widget layouts for Miner, Clock, SPrice, and Weather,
  including recovered DROM literal pointers and computed-layout notes;
- Miner, Clock, and SPrice runtime publisher callsites;
- Miner runtime-state evidence for accepted/rejected share counters, block-hit
  count, the `state+0x140` notify/job update counter, and hashrate accumulator
  fields, plus the Miner task-object source split: `runtime+0x74` feeds the
  SPrice/WatchCoins market stat/color pair, `runtime+0x84` is the `(scan)`
  controller, and `runtime+0x88` is the `(swarm)` controller feeding the bottom
  count/metric trio. The exact two swarm float meanings, Serial logging, and
  cleanup routes stay unresolved or no-touch;
- executable recovered swarm writer/publisher behavior from the official task
  at `0x400dd35c`: the object layout preserves count `+0x08`, float metrics
  `+0x0c/+0x10/+0x14`, stop/running flags `+0x50/+0x51`, the writer quartet
  `0x400ddb86..0x400ddb93`, and the proven Miner bottom-row publishers
  `0x400d6ea5`, `0x400d6ee2`, and `0x400d6f31`. The float fields remain
  offset-named until parser/source semantics are proven;
- the corrected Miner slot `+0x44` status: it reads `runtime+0x80
  +0x158/+0x15c`; inspected ranges do not bridge mining hashrate from
  `ctx+0x78` into that slot;
- the recovered Clock BTC route split: `runtime+0x80` is the Clock BTC ticker
  cache, `runtime+0x74` is the SPrice/WatchCoins market controller, the BTC
  price double is cache `+0x158/+0x15c`, and no BTC-only dynamic color writer
  was found for slot `+0x30`;
- the full recovered SPrice binding table from `FUN_400f69a4`;
- executable recovered SPrice market config/default behavior for the
  `WatchCoins` key, including the official comma scan, 20-symbol cap, and
  truncation point from `0x400e798c` / `0x400e7b21..0x400e7bc9`;
- the recovered SPrice runtime producer chain: `runtime+0x74` setter
  `0x400ddf61`, `(market)` task `0x400db278`, WatchCoins batch parser
  `0x400e2c28`, and 24-byte row producer evidence at `0x400e3a80`; the current
  source layer adapts ticker records into the recovered 24-byte row shape and
  implements the recovered `+16` price-descending ordering key and 16-row
  rendered trim while keeping exact equal-price/NaN ordering behavior open;
- SPrice patch-safety records showing the default `WatchCoins` literal as the
  only local literal-patcher eligible SPrice edit in this pass, with function,
  callsite, cap, pointer, URL, and suffix edits blocked or no-go;
- Weather forecast templates and callback registrations from `FUN_400f3ab4`;
- the proven SPrice market-cache row writer at `0x400d73be..0x400d7519`;
- official LittleFS geometry and `.ss` screensaver asset records, including
  the distinction between the bundled two-asset `littlefs.bin` and the separate
  recovered expanded 17-asset image;
- a first compile-only ESP32/Arduino/LVGL PlatformIO harness using the official
  ST7789 partition geometry and board display pins. This harness initializes
  TFT_eSPI/LVGL and renders recovered baseline evidence, but it does not yet
  recreate official mining, Wi-Fi/config, touch, task scheduling, or full UI
  behavior.

This is a source-equivalent reconstruction layer, not a byte-for-byte recovery
of the vendor's original private source tree. It is intended to become the
buildable baseline before custom SD-card and speaker features are added.

## Local Validation

The tree is host-validatable without ESP32 tools:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic \
  -Ifirmware/reconstructed/nmminer-v2-baseline/include \
  firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/tools/print_baseline.cpp \
  -o /tmp/nmminer-v2002-baseline
/tmp/nmminer-v2002-baseline
```

The recovered SPrice/WatchCoins slice has a focused host test:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic \
  -Ifirmware/reconstructed/nmminer-v2-baseline/include \
  firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/tools/test_sprice_behavior.cpp \
  -o /tmp/nmminer-sprice-test
/tmp/nmminer-sprice-test
```

The recovered swarm writer/publisher slice has a focused host test:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic \
  -Ifirmware/reconstructed/nmminer-v2-baseline/include \
  firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp \
  firmware/reconstructed/nmminer-v2-baseline/tools/test_swarm_behavior.cpp \
  -o /tmp/nmminer-swarm-test
/tmp/nmminer-swarm-test
```

The compile-only ESP32 harness builds with PlatformIO:

```sh
uvx platformio run -d firmware/reconstructed/nmminer-v2-baseline
```

The PlatformIO target has an upload guard in `scripts/guard_no_upload.py`.
Normal `upload`, `program`, `erase`, `uploadfs`, and `uploadfsota` targets are
refused unless `NMMINER_ALLOW_RECONSTRUCTED_UPLOAD=1` is explicitly set after
the project write gate is satisfied. The bypass is intentionally not a write
approval by itself.

2026-06-20 validation: host C++17 build/print passed, the SPrice/WatchCoins
host test passed, the swarm writer/publisher host test passed, PlatformIO ESP32
build passed, `firmware.bin` is `575616` bytes with PlatformIO flash use
`575253` bytes, generated `partitions.bin` byte-matches the official
`v2.0.02` ST7789 partition table
(`f1882db7f6e352825314f3dcc5db01085e74625655c5f1decf58dd84fa118226`), and
all guarded write targets listed above fail before any device write path.

An ESP32 behavior-equivalent baseline is still gated on:

- turning the now-typed official startup, begin-style bring-up, runtime-slot,
  and task-registration model into behavior-equivalent ESP32 code;
- converting the recovered screen/model layer into the actual official
  UI/API/mining/config runtime behavior;
- validating behavior against the official `v2.0.02` ST7789 binary;
- preserving a fresh full 4 MB flash backup before any write.
