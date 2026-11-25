# NMMiner v2.0.02 Reverse-Engineering Pipeline (working)

> Route correction, 2026-06-19: this is the active baseline for the official
> firmware decompilation/custom-edit route. The June 1 full-device dump identifies
> as `v1.8.28` and is a historical factory/setup snapshot, not the target
> decompilation baseline. The public flasher now also lists `v2.0.03` for
> `esp32-2432s028r-st7789`, but this repo's backed-up and extracted official app
> baseline is the verified local `v2.0.02` image.

Reproducible RE pipeline for the stock closed NMMiner firmware, established for
milestone v3.0. Source = the full 4 MB flash backup
`hardware/backups/cyd-backup-20260617-172106.bin` (gitignored).
All `hardware/recovered-assets/` outputs are gitignored (no-redistribution).

Local provenance check: `cyd-backup-20260617-172106.bin` byte-matches the official
`firmware/v2.0.02/esp32-2432s028r-st7789/` bootloader, partition table, boot app,
and app firmware at their flasher offsets. The filesystem partition differs from
the pristine official `littlefs.bin` because the full backup captured the live
device filesystem after setup/use; the extracted filesystem is
`hardware/recovered-assets/spiffs.bin`.

## Image map (parsed from the ESP32 image header)

| Segment | Load addr | File off (in app) | Contents |
|---------|-----------|-------------------|----------|
| seg0 DROM | `0x3f400020` | `0x000018` segment header / `0x000020` payload | strings, const data, fonts, image descriptors |
| seg1 DRAM | `0x3ffbdb60` | `0x157e08` segment header / `0x157e10` payload | initial DRAM data |
| seg2 IRAM0 | `0x40080000` | `0x15e3b0` segment header / `0x15e3b8` payload | low IRAM vectors and literal pool |
| seg3 IROM | `0x400d0020` | `0x160018` segment header / `0x160020` payload | **main code** (1.25 MB) |
| seg4 IRAM1 | `0x40081c60` | `0x29229c` segment header / `0x2922a4` payload | IRAM code containing app entrypoint `0x40086aa4` |

Partitions: `nvs@0x9000`, `factory(app)@0x10000 len 0x380000`, `spiffs@0x390000
len 0x60000` (actually **LittleFS**), `coredump@0x3f0000`.

## Assets — DECODED ✓

- **LittleFS** (`littlefs.bin` / `spiffs.bin`, block_size 4096 x 96): mount
  with `littlefs-python`. The official bundled ST7789 `littlefs.bin` contains
  `index.html.gz`, `apidoc.html.gz`, and two `/ss/saver_320_240_*.ss` assets
  (`001`, `002`). The separate recovered expanded `hardware/recovered-assets/spiffs.bin`
  contains the same web asset names and 17 screensavers (`002` through `018`);
  it is valid asset evidence, but it is not byte-identical to the official
  bundled `littlefs.bin`.
- **`.ss` screensavers**: format = `"SN"` + w(LE) + h(LE) + flags(0x01,0xA4) + body,
  body = RGB565 → RLE → LZSS(10-bit offset, 4-bit len). Decoder: `scripts/nmminer_ss_decode.py`.
  The two official bundled assets and all 17 recovered expanded assets decode
  to clean 320x240 images.

## Code — disassembling ✓ (decompile = Phase 1 / Ghidra)

```bash
OBJCOPY=~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objcopy
OBJDUMP=~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump
# wrap the raw IROM segment as an Xtensa ELF so objdump knows the arch/config
$OBJCOPY -I binary -O elf32-xtensa-le \
  --rename-section .data=.text,contents,alloc,load,readonly,code \
  irom.bin irom_text.elf
$OBJDUMP -d --adjust-vma=0x400d0020 irom_text.elf > irom.asm   # 476k lines
```

**String→function xref:** Xtensa loads full DROM addresses as `l32r` literals.
Find a string's DROM address (`grep` in `drom.bin`, add base `0x3f400020`), then
find that 4-byte LE address in `irom.bin` (the literal pool) → the referencing
function. Confirmed working:
- `Clock320x240` (DROM `0x3f47389b`) → literal at code `0x400d1864`
- `Miner320x240` (DROM `0x3f4be960`) → literal at code `0x400d1960`

These are the screen-construction functions — the UI spec source.

**Address→offset map:** use `scripts/nmminer_addr_map.py` to convert any DROM,
DRAM, IRAM0, IROM, or IRAM1 VM address into an app-image offset and a
full-flash offset for the official `v2.0.02` ST7789 app:

```bash
python3 scripts/nmminer_addr_map.py 0x3f49912a 0x400d195c 0x400f2c5c
```

Current first literal candidates are recorded in
`.planning/phases/07-on-device-validation/07-BINARY-EDIT-TARGET-MAP.md`.
Function addresses in that map are navigation/decompilation records only until
runtime update paths and ESP32 image checksum/hash refresh are proven.

**Literal patch tooling:** `scripts/nmminer_patch_literal.py` patches approved
fixed-length DROM literal records, verifies original bytes, null-pads shorter
ASCII replacements, refreshes the ESP32 image checksum/hash footer, and can run
`python3 -m esptool image-info` on the generated app:

```bash
python3 scripts/nmminer_patch_literal.py \
  --target-id BIN02-LIT-LOAD-TAGLINE \
  --replacement "NNMiner Base" \
  --output hardware/generated-firmware/nmminer-v2.0.02-st7789-tagline.bin \
  --validate
```

`hardware/generated-firmware/` is gitignored. The tool does not flash hardware.

**Recovered source-equivalent outputs:** official-binary recovered C lives in
`hardware/recovered-assets/decompiled/source/`:

- `Miner_dynamic_model.recovered.c`: callback-model bindings, generic publisher
  callsites, and Miner slot fanout from the official `v2.0.02` app.
- `Clock_btc_price.recovered.c`: BTC price label updater, Clock text/color
  setters, `runtime+0x80` Clock BTC ticker-cache route, and producer callsites
  for `global+0x4f4`.
- `Weather_static_builder.recovered.c`: static Weather widget geometry, slot
  offsets, forecast rows, timer setup, and callback registrations.
- `SPrice_market_renderers.recovered.c`: SPrice field bindings, market cache
  renderer, kline renderer, WatchCoins rotation, and `pricewall` mode compare.
- `SPrice_watchcoins_config.recovered.c`: market config defaults, WatchCoins
  tokenizer, and ticker parser/wrapper evidence.
- `SPrice_market_runtime.recovered.c`: `runtime+0x74` setter, market task,
  WatchCoins batch parser, and 24-byte row producer.
- `Official_startup.recovered.c`: official app segment map and first IRAM1
  entrypoint recovery for `0x40086aa4`, including BSS clear ranges,
  constructor-gate evidence, framework/IROM hook callsites, lifecycle log
  routes, `0x400de1e4` begin-style app bring-up evidence, all-tasks-started
  log route, `(button)` / `(scan)` / `(swarm)` / `(stratum)` / `(tMiner)`
  helper callsites, and the split `nMiner` name/object setup plus helper
  evidence at `0x400dae29` / `0x400dae8a`.

These are syntax-checked source-equivalent artifacts, not replacement source
trees yet. They are the current decompilation authority for UI edit targeting.

The first tracked reconstructed source scaffold now lives at
`firmware/reconstructed/nmminer-v2-baseline/`. It converts the recovered binary
facts into C++ source: address mapping, LVGL screen widget specs, publisher
callsites, runtime evidence, the recovered Clock BTC ticker-cache route, the
proven SPrice 24-byte cache-row writer, and LittleFS screensaver asset records.
It host-compiles and prints the expected official address/row-stride summary,
but it is not yet an ESP32-flashable firmware app.

**Current runtime callback map:** the first dynamic pass has resolved several
screen-owned setter callbacks from the official `v2.0.02` app:

- Loading: `FUN_400f4830` registers progress/status/IP callbacks; it is not the
  `NMMiner` title builder.
- Config: `FUN_400f424c` and `FUN_400f4264` update the status/countdown label
  text/color; AP/IP strings are captive-portal HTTP strings, not LVGL labels.
- Miner: `FUN_400f48a8` and `FUN_400f48c0` are generic field text/color
  setters; `FUN_400f4b24` binds callback records to concrete Miner slots after
  `FUN_400f34d0`. Clean Xtensa objdump of `nmminer-app.elf` confirms text
  records `global+0x68..global+0x33c` bind to slots `+0x04..+0x58`.
  `FUN_400ef24c` is now corrected to a parser helper, not the Miner producer.
  Share results, hashrate accumulation, telemetry row
  formatting, and producer record cleanup are mapped in the larger runtime
  range `0x400f0858..0x400f1d2e`. `0x400f0846..0x400f13e8` is now classified as
  `mining.notify` / `params` parsing and internal job-state update, not UI
  publication. Callback-model publication is generic through `FUN_400d6104`
  and `FUN_400d38cc`, dispatched by `FUN_4011b6a4`, with expanded direct
  callsites in `0x400d672e..0x400d6fc9`. `FUN_4012da8c` is cleanup, not
  notification. The direct publisher table now carries confidence-labeled
  semantics; unresolved writer/object identities remain marked open instead of
  guessed. Source-equivalent recovery is recorded in
  `source/Miner_dynamic_model.recovered.c`.
- Clock: `FUN_400f1eb0` owns BTC price text/layout for slot `+0x30`, with the
  first proven producer write at `0x400d82a1` into `global+0x4f4`. The source
  text route is `FUN_400d57c0() -> runtime+0x80 clock_btc_ticker_cache ->
  btc_usd_last_price at cache +0x158/+0x15c -> stack string ->
  FUN_400d6104(global+0x4f4, stack_string)`. `runtime+0x74` is the
  SPrice/WatchCoins market controller and nearby header-color side path, not
  the BTC text source. `FUN_400f4204` is not the BTC price producer, and
  `FUN_400f24ac` splits compound labels containing `:`. Header green/red
  records target `global+0x4b8/+0x4e4`; the checked direct and scoped
  non-direct color writers do not hit `global+0x4f4`, so BTC price color is
  static `0xb596` builder evidence and no BTC-only dynamic color writer was
  found. Source-equivalent recovery is recorded in
  `source/Clock_btc_price.recovered.c`, with closure details in
  `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`.
- Weather: cb1/cb2/cb3 text and `HH:MM` formatter callbacks are mapped as
  update-only callbacks; they do not create widgets. The static builder
  `FUN_400f3ab4` is source-recovered in
  `source/Weather_static_builder.recovered.c`; widget offsets and layout
  geometry are decoded. Exact patch bytes and rollback records are still
  required before layout edits.
- SPrice: chart event adapter `FUN_400f6984`, data-fill helper
  `FUN_400f69a4`, renderer `FUN_400f63d8`, series helper `FUN_400f53f8`,
  child draw worker `FUN_400f54e4`, and per-bar adapter `FUN_400f5ba0` are
  mapped in `source/SPrice_market_renderers.recovered.c`. The upstream market
  config/tokenizer/ticker path is mapped in
  `source/SPrice_watchcoins_config.recovered.c`: `0x400e7978` is a wrapper,
  `0x400e798c` is the real body entry, `0x400e7b21..0x400e7bc9` scans
  `WatchCoins` commas and caps at 20 symbols, and `0x400e3b84` /
  `0x400e3e84` parse ticker JSON fields. Kline/candle behavior is confirmed;
  `pricewall` is a live mode compare through `FUN_400f4dfc`, but not a direct
  renderer edit anchor. The SPrice cache writer is now proven at
  `0x400d73be..0x400d7519`: it copies `runtime+0x74` source-vector rows into
  inline 24-byte records at `global+0xd94` and writes count to `global+0x1244`.
  The raw config/API to `runtime+0x74` source-vector construction is decoded
  through the market task, batch parser, and row producer. There is still no
  instruction-level patch-safe `WatchCoins` function edit beyond the approved
  default literal.
- Share accept: the `#%d share accepted, %ldms` path is a mid-block seam around
  `0x400f1620`; the narrow share-result block is `0x400f15d0..0x400f174c`.
  Accepted/rejected/block-hit dispatch callsites and counters are mapped.
  `FUN_4012d668` is a printf-style Arduino Serial/UART stream writer via
  `DAT_400d00d0`, so it is not a share-specific code-hook target.
- Screensavers: `FUN_400f6ff4` is path-based `ss_render(char *path, uint w,
  uint h)` and remains code no-touch. The customization path is external
  LittleFS `.ss` asset work under `/ss/saver_{W}_{H}_{NNN}.ss`. The local
  add/replace proof in `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` rebuilds the
  official `0x60000` LittleFS image with the same `4096 x 96` geometry,
  re-lists it, extracts it, and decodes the serialized `.ss` files.

## Current Workbench

Use the workbench recorded in `hardware/recovered-assets/WORKBENCH.md`:

- Ghidra `11.3.1`
- dynacylabs `ghidra-esp32-flash-loader` `v11.3.1` prebuilt extension
- Native Ghidra 11.3.1 Xtensa support
- fallback import target: `hardware/recovered-assets/irom_text.elf`

Do not follow stale notes that point to dynacylabs loader `v12.0`; this project
intentionally stayed on `v11.3.1` because that is the matching prebuilt loader
for the installed Ghidra version.

## Active Decompilation Tasks

1. Continue producer/caller recovery from the official `v2.0.02` app only:
   remaining unresolved Miner writer/field semantics for the expanded generic
   publisher callsites in `0x400d672e..0x400d6fc9`; keep the Clock BTC
   source/color closure in force; SPrice instruction-level patch bytes after
   the now-recovered `WatchCoins` tokenizer/ticker/row-producer path; exact
   Weather patch bytes and rollback records; and any follow-up
   screensaver policy work after the now-proven LittleFS `.ss` add/replace
   mechanics.
2. Normalize any remaining stale function-boundary notes from earlier Ghidra
   passes, especially anything that treats the `0x400f1620` share seam as a
   clean standalone entry.
3. Keep the SPrice raw `WatchCoins`/config/API to `runtime+0x74` source-vector
   builder decoded through the comma-scan wrapper, market config/ticker path,
   batch parser, and row producer. Remaining work is instruction-level edit
   safety and rollback, plus remaining Miner writer identities and ambiguous
   `NMMiner` xrefs.
4. Continue the `SPrice` upstream pipeline from the proven
   `0x400d73be..0x400d7519` cache writer back toward config/API and ticker
   parser ownership; keep the recovered chart/render workers as consumers.
5. Grow `firmware/reconstructed/nmminer-v2-baseline/` from the host-validated
   recovered source model into an ESP32/LVGL firmware entrypoint from the
   official `0x40086aa4` startup route and the `0x400de1e4` begin-style
   app/task bring-up route after the remaining framework/runtime boundaries are
   resolved.
6. Build a binary edit target map covering candidate literals/functions,
   replacement constraints, checksum/image-validation requirements, and explicit
   no-touch zones such as license/config-sensitive paths.
7. Expand local-only patch tooling only after each target is mapped. Generated
   patched apps must stay out of git and must pass `python3 -m esptool
   image-info` before any device write is considered.
