# Phase 7 Binary Decompilation Tasks

**Updated:** 2026-06-20
**Route:** official NMMiner `v2.0.02` `esp32-2432s028r-st7789`
binary decompilation/custom-edit.

## Authority

Use the official `v2.0.02` ST7789 baseline only:

- Full backup: `hardware/backups/cyd-backup-20260617-172106.bin`
- Official bundle: `firmware/v2.0.02/esp32-2432s028r-st7789/`
- App import: `hardware/recovered-assets/app_factory.bin`
- Segments: `hardware/recovered-assets/drom.bin`,
  `hardware/recovered-assets/dram.bin`, `hardware/recovered-assets/iram_low.bin`,
  `hardware/recovered-assets/iram_low.asm`,
  `hardware/recovered-assets/irom.bin`, `hardware/recovered-assets/irom.asm`,
  `hardware/recovered-assets/iram_tail.bin`, and
  `hardware/recovered-assets/iram_tail.asm`
- Decompiled work: `hardware/recovered-assets/SPEC.md` and
  `hardware/recovered-assets/decompiled/`
- Source-equivalent recovery outputs:
  `hardware/recovered-assets/decompiled/source/`
- Source-built reconstruction scaffold:
  `firmware/reconstructed/nmminer-v2-baseline/`
- SPrice patch-safety classifier:
  `.planning/phases/07-on-device-validation/07-SPRICE-PATCH-SAFETY.md`
- LittleFS screensaver asset replacement proof:
  `.planning/phases/07-on-device-validation/07-LITTLEFS-SS-ASSET-REPLACEMENT.md`
- Clock BTC source/color closure:
  `.planning/phases/07-on-device-validation/07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`

Do not silently switch to public `v2.0.03`. Do not use the June 1 `v1.8.28`
dump as the active target.

## Immediate Tasks

1. Normalize Ghidra function boundaries for every literal xref from:

   ```bash
   python3 scripts/nmminer_str_xref.py \
     hardware/recovered-assets/drom.bin \
     hardware/recovered-assets/irom.bin \
     --all
   ```

2. Map every candidate VM address to app/full-flash offsets before calling it an
   edit target:

   ```bash
   python3 scripts/nmminer_addr_map.py \
     0x3f49912a 0x400d195c \
     0x3f4008eb 0x400d1954 \
     0x400f2c5c 0x400f2f5c 0x400f1f34
   ```

   The output must be recorded in `07-BINARY-EDIT-TARGET-MAP.md`.

3. Recover the official startup route from IRAM1 before creating any ESP32
   behavior-equivalent baseline:

   - current entrypoint: `0x40086aa4` / app `0x2970e8` / flash `0x2a70e8`;
   - current artifact:
     `hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`;
   - resolved evidence includes the IRAM0 literal pool, BSS clear range
     `0x3ffc4100..0x3ffd6b30`, constructor gate `0x40130cd0 ->
     0x40086910`, framework/IROM startup hooks, the `0x400de1e4`
     begin-style app bring-up route, the `0x400de25d`
     `NMMiner::begin: all tasks started` log route, task helper evidence for
     `(button)`, `(scan)`, `(swarm)`, `(stratum)`, and `(tMiner)`, plus split
     `(nMiner)` setup/helper evidence at `0x400dae29` / `0x400dae8a`;
   - exact IDF/Arduino helper names and the downstream app task handoff remain
     unresolved and must be mapped before calling the baseline flashable.
   - current implementation surface:
     `firmware/reconstructed/nmminer-v2-baseline/` now contains a compile-only
     PlatformIO ESP32/Arduino/LVGL harness using official partition geometry
     and ST7789 flags. It now exposes typed raw startup literals, ordered
     startup steps, a higher-level official bring-up/task model, runtime service
     slots, source-baseline gates, and a lifecycle validation layer derived from
     `Official_startup.recovered.c` and the existing runtime evidence tables.
     It also now includes executable recovered SPrice/WatchCoins behavior:
     official default parsing, null versus empty-string behavior,
     exactly-20-token cap, truncation before the 21st token, 16-byte row payload
     adapter, recovered `+16` price-descending ordering key, 16-row rendered
     trim, cache-copy seam, and null ArrayView guards. It also includes the
     recovered swarm writer/publisher boundary: task `0x400dd35c`, count
     `+0x08`, metric fields `+0x0c/+0x10/+0x14`, writer quartet
     `0x400ddb86..0x400ddb93`, publisher trio `0x400d6ea5`/`0x400d6ee2`/
     `0x400d6f31`, and the `1000.0` precision threshold for `+0x10`.
     The latest host validation reports `8/10` lifecycle checks passing:
     entrypoint, BSS range, constructor route, scheduler handoff, begin route,
     required task registrations, runtime slots, and flash guard are modeled;
     official behavior parity and the fresh-backup write gate remain open, so
     `flashable=0`. Exact equal-price and NaN SPrice row ordering behavior and
     exact swarm float field meanings remain open. The next task is to turn those typed recovery tables into
     behavior-equivalent ESP32 code for the official startup, begin-style task
     routing, runtime model, UI/API/mining/config behavior. The harness build is
     source-reconstruction progress, not a flash approval.

4. Decompile runtime update paths:

   - `Miner`: hashrate, shares, pool/net state, difficulty fields. Generic
     text/color callbacks and the `FUN_400f4b24` binding fan-out are mapped.
     `FUN_400ef24c` is now corrected to a JSON/parser-style helper, not the
     Miner producer. The mining/share/hashrate evidence lives in the larger
     runtime block around `0x400f0858..0x400f1d2e`. `0x400f0846..0x400f13e8`
     is now classified as `mining.notify`/`params` parsing plus internal job
     state update, not the callback-model publisher. Generic callback-model
     publication is through `FUN_400d6104` (string records) and `FUN_400d38cc`
     (numeric/color records), dispatched by `FUN_4011b6a4`; direct Miner-bound
     callsites are in the large update routine around `0x400d672e..0x400d6fc9`.
     Clean Xtensa objdump of `nmminer-app.elf` confirms `FUN_400f4b24` binds
     text records `global+0x68..global+0x33c` to Miner slots `+0x04..+0x58`;
     the direct publisher list in that window now includes the formerly missed
     `0x400d6a49`, `0x400d6a65`, `0x400d6ba6`, `0x400d6cca`, `0x400d6d9b`,
     `0x400d6e57`, `0x400d6ec2`, `0x400d6efd`, and `0x400d6f4a` callsites.
     `FUN_4012da8c` is string cleanup, not a notifier. The recovered
     source-equivalent artifact now carries confidence-labeled semantics for
     the direct publisher callsites plus a checked runtime-state evidence layer:
     accepted shares are `state+0x130`, rejected shares are `state+0x134`,
     block hits are `state+0x138`, notify/job updates are `state+0x140`, and
     combined hashrate is `state+0x158/+0x15c`. Direct publisher
     `0x400d69f9` / slot `+0x44` is downgraded: it reads the update loop's
     `runtime+0x80` stack object at `+0x158/+0x15c`, the same object family as
     the Clock BTC route. Inspected ranges show no bridge/sync from mining
     `ctx+0x78` hashrate into that slot.
     The visible `state+0x140` publisher is now labeled as a notify/job update
     counter candidate, with writer evidence at `0x400f1258` and supporting
     seed/increment sites; it is not accepted shares. Task-object telemetry
     source slots are now split: `runtime+0x74` feeds the Miner market stat/color
     pair, `runtime+0x84` is the `(scan)` controller, and `runtime+0x88` is the
     `(swarm)` controller feeding the bottom count/metric trio. The scaffold now
     executes the recovered swarm writer/publisher boundary, but exact swarm
     float field meanings, Serial/UART logging, and cleanup paths are recorded
     as unresolved/no-touch.
     The artifact for
     this layer is
     `hardware/recovered-assets/decompiled/source/Miner_dynamic_model.recovered.c`.
   - `Clock`: time/date, BTC price, green/red price-color decision. BTC price
     text/layout ownership is now mapped as `global+0x4f4 -> FUN_400f1eb0 ->
     slot +0x30`; producer callsite `0x400d82a1` is inside a large update/poll
     routine at `0x400d61c0..0x400d8957`. The upstream text source route is now
     mapped through `FUN_400d57c0() -> runtime+0x80
     clock_btc_ticker_cache -> cache +0x158/+0x15c btc_usd_last_price ->
     stack string -> FUN_400d6104(global+0x4f4, stack_string)`.
     `runtime+0x74` is separately named as the SPrice/WatchCoins market
     controller and nearby header-color source, not the BTC text source.
     `FUN_400f24ac` and `FUN_400f4204` are ruled out as BTC price producers.
     Header green/red records are mapped and all direct/scoped Clock
     `FUN_400d38cc` color writes checked avoid `global+0x4f4`; BTC price color
     is closed as static-builder evidence at `0xb596`, with no BTC-only dynamic
     color writer found. The recovered source-equivalent artifact for this
     route is
     `hardware/recovered-assets/decompiled/source/Clock_btc_price.recovered.c`,
     with closure details in `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`.
   - `Loading`: version/branding/progress text and `FUN_400f4830`. Progress,
     status, color, and IP/address callbacks are now mapped; `NMMiner` title
     binding remains unproven.

5. Complete screen coverage:

   - `Weather` lazy-create/update helpers. Runtime cb1/cb2/cb3 are mapped and
     update existing labels; they do not create widgets. The missed Weather
     vtable builder is now identified and source-recovered as `FUN_400f3ab4`
     (`0x400f3ab4`, app `0x183ab4`, flash `0x193ab4`) in
     `hardware/recovered-assets/decompiled/source/Weather_static_builder.recovered.c`.
     Patch safety remains open until expected bytes and rollback path are
     documented.
   - `SPrice` chart/pricewall pipeline. Event adapter, main renderer, data-fill
     helper, series helper, child draw worker, and per-bar draw callback are
     mapped. Kline/candle rendering is confirmed. `pricewall` is live as a mode
     compare through `FUN_400f4dfc`, but that function only toggles
     layout/object visibility and is not a direct renderer edit anchor.
     `WatchCoins` remains config/API input. The SPrice cache writer is now
     proven at `0x400d73be..0x400d7519`: it copies `runtime+0x74`
     source-vector rows into inline 24-byte records at `global+0xd94` and
     writes count to `global+0x1244`. The `WatchCoins` config/tokenizer path is
     now source-recovered in
     `hardware/recovered-assets/decompiled/source/SPrice_watchcoins_config.recovered.c`:
     wrapper `0x400e7978`, body `0x400e798c`, and
     `0x400e7b21..0x400e7bc9` prove the key lookup, comma scan, and 20-symbol
     cap/truncation. The market config/ticker path
     `0x400db307..0x400db7d5`, callsite `0x400db511`, ticker wrapper
     `0x400e3e84`, and parser `0x400e3b84` prove the config defaults and
     `lastPrice` / `priceChangePercent` parsing. The runtime source-vector
     producer is now proven: `0x400ddf61` assigns static market object
     `0x3ffc4618` to `runtime+0x74`; `(market)` task `0x400db278` loads the
     WatchCoins config string; `0x400dbbc9` calls batch refresh/parser
     `0x400e2c28`; and `0x400dbbd1` calls `0x400e3a80`, which materializes
     object `+0x24/+0x28/+0x2c` as 24-byte SPrice rows for the cache writer.
     The reconstructed baseline now compiles the proven tokenizer, row/cache
     shape, `+16` price-descending ordering key, and 16-row rendered trim into
     `recovered_sprice_behavior.cpp`, while equal-price and NaN behavior remain
     unresolved decompilation targets.
     Runtime binding/rendering source-equivalent is recovered in
     `hardware/recovered-assets/decompiled/source/SPrice_market_renderers.recovered.c`.
     Runtime producer evidence is recovered in
     `hardware/recovered-assets/decompiled/source/SPrice_market_runtime.recovered.c`.
     Patch safety for this route is now classified in
     `07-SPRICE-PATCH-SAFETY.md`: the default `WatchCoins` DROM literal at
     `0x3f400cda` / app `0xcda` is approved for local-only fixed-length
     literal patch tooling, while the `USDT` suffix, Binance batch URL,
     `PricePageMode` pointer/literal, tokenizer caps, row trim caps, market task
     callsites, `runtime+0x74` setter, and producer internals remain blocked or
     no-go byte edits.
   - `Config` mode/status fields. Builder and status text/color callbacks are
     mapped; AP/IP strings remain HTTP/captive-portal strings, not LVGL labels.
   - `ss_render` decode/render call shape. The external LittleFS asset route is
     proven: UI manager scan/build code discovers `/ss/saver_{W}_{H}_{NNN}.ss`
     assets and passes the path to `FUN_400f6ff4`. Function patches remain
     no-touch; customization belongs in `.ss` asset add/replace work. The local
     replacement proof distinguishes the official bundled two-asset
     `littlefs.bin` from the separate recovered expanded 17-asset image, rebuilds
     `0x60000` images with `littlefs-python`, re-lists, extracts, and decodes
     the serialized `.ss` files.

6. Maintain `07-BINARY-EDIT-TARGET-MAP.md` with:

   - candidate literal/function edit sites;
   - byte-length and relocation constraints;
   - DROM, DRAM, IRAM0, IROM, or IRAM1 address, file offset, and segment ownership;
   - ESP32 image checksum/hash refresh requirements;
   - partition-bound checks;
   - explicit no-touch zones.

7. Use the first local patch tool only for approved fixed-length literal records:

   ```bash
   python3 scripts/nmminer_patch_literal.py \
     --target-id BIN02-LIT-LOAD-TAGLINE \
     --replacement "NNMiner Base" \
     --output hardware/generated-firmware/nmminer-v2.0.02-st7789-tagline.bin \
     --validate
   ```

   Generated artifacts must stay under gitignored `hardware/generated-firmware/`
   or another local-only path. No flash is considered until the write gate below.

## First Literal Candidates

The first viable edit class is same-length or shorter DROM literal replacement.
Do not touch code branches/trampolines yet.

| Target | Current | Max bytes | Status |
|--------|---------|-----------|--------|
| `BIN02-LIT-LOAD-TAGLINE` | `Make it better` | 14 | Best first patch-tooling candidate |
| `BIN02-LIT-LOAD-IP-SEED` | `192.168.124.222` | 15 | Runtime-overwrite risk; verify visually before relying on it |
| `BIN02-LIT-GLOBAL-VERSION` | `v2.0.02` | 7 | Shared/width-sensitive; not a first target and requires explicit shared-target opt-in |
| `BIN02-LIT-SPRICE-SEEDS` | `---`, `$0` | 3, 2 | Cold-start placeholder only |
| `BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT` | `BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO` | 39 | Local-only literal-patcher eligible; defaults only, NVS/config may override |
| `BIN02-SEAM-SHARE-ACCEPT` | `#%d share accepted, %ldms` | 25 | Map only; not approved for mutation |

## Runtime Evidence Mapped

The current decompilation pass resolved the following dynamic owners:

| Area | Owner(s) | Current conclusion |
|------|----------|--------------------|
| Loading | `FUN_400f444c`, `FUN_400f44bc`, `FUN_400f44d4`, `FUN_400f4504`, registered by `FUN_400f4830` | Progress percent/bar, status text/color, and IP/address text are mapped. `FUN_400f4830` is callback registration, not a title builder. |
| Config | `FUN_400f424c`, `FUN_400f4264`, registered by `FUN_400f43e8` | Config slot `+0x08` text and color are mapped. AP/IP strings are not LVGL labels. |
| Miner | `FUN_400f34d0`, `FUN_400f4b24`, `FUN_400f48a8`, `FUN_400f48c0`, registered by `FUN_400f48f0`; parser helper `FUN_400ef24c`; runtime block `0x400f0858..0x400f1d2e`; generic publishers `FUN_400d6104` / `FUN_400d38cc`; dispatcher `FUN_4011b6a4`; string cleanup `FUN_4012da8c`; recovered C at `hardware/recovered-assets/decompiled/source/Miner_dynamic_model.recovered.c`; mirrored in `firmware/reconstructed/nmminer-v2-baseline/` | Generic field text/color callbacks, clean slot binding fan-out, share-result block, notify/job sequence writers, hashrate accumulator, telemetry table dispatch, and producer record formatting are mapped as navigation evidence. `0x400f0846..0x400f13e8` is `mining.notify`/`params` parsing and internal mining-job state update, not UI publication. Callback-model publication is generic via `FUN_400d6104` and `FUN_400d38cc`, with expanded Miner-bound direct callsites in `0x400d672e..0x400d6fc9`. The binding/publisher layer now has syntax-checked source-equivalent artifacts with confidence-labeled field semantics, a checked runtime-state evidence table for accepted/rejected/block-hit/notify-sequence/hashrate fields, task-object source assignment proof for `runtime+0x74` (`0x400d6e1a`, `0x400d6e57`), and constructor/task proof that `runtime+0x84` is `(scan)` while `runtime+0x88` is `(swarm)` feeding `0x400d6ea5`, `0x400d6ee2`, and `0x400d6f31`. The reconstructed baseline now executes the recovered swarm count/metric writer quartet and bottom-row publisher trio with offset-named float fields; exact swarm float meanings, Serial/UART telemetry, and cleanup remain unresolved/no-touch; safe byte edits stay open. |
| Clock | `FUN_400f1d68`, `FUN_400f1eb0`, `FUN_400f24ac`, `FUN_400f284c`, `FUN_400f2a8c`, `FUN_400f4204`, `FUN_400f421c`; producer callsite `0x400d82a1` inside `0x400d61c0..0x400d8957`; recovered C at `hardware/recovered-assets/decompiled/source/Clock_btc_price.recovered.c`; closure at `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md` | Timer, BTC price text/layout, generic text setter, and compound-label binding are mapped. BTC text source is `global+0x4f4`; producer input is traced through `FUN_400d57c0()` to `runtime+0x80` named `clock_btc_ticker_cache`, with `btc_usd_last_price` double fields at cache `+0x158/+0x15c`. `runtime+0x74` is the SPrice/WatchCoins market controller and nearby header-color source, not the BTC price source. `FUN_400f24ac` and `FUN_400f4204` are not the BTC producer. Header green/red writes target `global+0x4b8/+0x4e4`; no direct or scoped non-direct color write to `global+0x4f4` was found. BTC price color is static-builder evidence at `0xb596`; no BTC-only color patch is approved. |
| Weather | `FUN_400f3ab4`, `FUN_400f3a34`, `FUN_400f3850`, `FUN_400f386c`, `FUN_400f3928`; recovered C at `hardware/recovered-assets/decompiled/source/Weather_static_builder.recovered.c` | Static builder `FUN_400f3ab4` is source-recovered. Dispatcher/on-show `FUN_400f3a34` registers cb1/cb2/cb3, and the callbacks update existing labels. Geometry edits are still blocked until exact patch bytes and rollback are documented. |
| SPrice | `FUN_400f3570`, `FUN_400f6984`, `FUN_400f69a4`, `FUN_400f63d8`, `FUN_400f5ba0`, `FUN_400f54e4`, `FUN_400f53f8`, cache-writer block `0x400d73be..0x400d7519`, WatchCoins tokenizer `0x400e7978` / `0x400e798c` / `0x400e7b21..0x400e7bc9`, market config/ticker path `0x400db307..0x400db7d5` / `0x400e3e84`, runtime setter/task/producer `0x400ddf61` / `0x400db278` / `0x400e2c28` / `0x400e3a80`; recovered C at `hardware/recovered-assets/decompiled/source/SPrice_market_renderers.recovered.c`, `hardware/recovered-assets/decompiled/source/SPrice_watchcoins_config.recovered.c`, and `hardware/recovered-assets/decompiled/source/SPrice_market_runtime.recovered.c`; patch safety at `07-SPRICE-PATCH-SAFETY.md` | Builder, `+0x24` event adapter, data-fill helper, kline renderer, child draw worker, per-bar adapter, series helper, runtime-to-global cache writer, market config defaults, WatchCoins comma tokenizer, batch fetch/parser, and row materializer are mapped. `pricewall` is a live mode compare through `FUN_400f4dfc`, but not a direct renderer edit anchor. The reconstructed baseline now executes the recovered WatchCoins tokenizer, 24-byte row/cache adapter, `+16` price-descending ordering key, and 16-row rendered trim; equal-price and NaN behavior remain open. The default `WatchCoins` DROM literal is now local literal-patcher eligible; function/callsite/cap/pointer edits remain blocked or no-go until exact instruction contracts, side effects, checksum/hash refresh, and rollback are documented. |
| Screensavers | `FUN_400f6ff4`; official `firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin`; recovered expanded `hardware/recovered-assets/spiffs.bin`; proof at `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` | Renderer is path-based and no-touch. Official bundled LittleFS has two `/ss` assets (`001`, `002`); recovered expanded image has 17 (`002` through `018`). Local add/replace images preserve `0x60000` size and `4096 x 96` geometry, list/extract cleanly, and decode with `scripts/nmminer_ss_decode.py`. |
| Share accept | containing candidate `0x400ef724..0x400f1d2e`, block `0x400f15d0..0x400f174c`, callsites `0x400f1632` / `0x400f16ed` / `0x400f173d`, callee `FUN_4012d668` | Accepted/rejected/block-hit format dispatch shape and counters are mapped. `FUN_4012d668` is a printf-style Arduino Serial/UART stream writer via `DAT_400d00d0`; it is not a UI/event hook. `0x400f1620` is a mid-block seam, not an approved function entry or hook point. |

Do not add any of the runtime function records to the patch tool until expected
bytes, instruction encoding, caller side effects, checksum/hash refresh, and a
rollback path are all documented.

## Patch Tool Status

- `scripts/nmminer_patch_literal.py` added for same-length or shorter ASCII DROM
  literal replacements.
- It verifies original bytes, null-pads shorter replacements, refreshes the ESP32
  checksum/hash footer, and can run `esptool image-info`.
- A `/tmp` test patch of `BIN02-LIT-LOAD-TAGLINE` validated with checksum/hash
  marked valid by `esptool`.
- The default SPrice `WatchCoins` DROM literal is now in the local patch tool as
  `BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT`; it is defaults-only and may be
  runtime-overridden by saved config/NVS.
- Function edits, share-seam edits, `NMMiner` xrefs, Clock price/color edits,
  Miner telemetry edits, SPrice renderer/function/callsite/cap/pointer edits,
  and overlapping shared-unit edits are not patch-ready until the actual
  publisher paths, expected bytes, branch/register safety, and rollback
  validation are proven. Do not hook `FUN_4012da8c` or `FUN_4012d668`; those
  are cleanup and Serial logging. Do not patch `FUN_400f6ff4`; the screensaver
  route is external LittleFS `.ss` asset work and the local add/replace
  mechanics are already proven.

## Latest Reconstructed Baseline Validation

2026-06-20 no-device validation of
`firmware/reconstructed/nmminer-v2-baseline/`:

- Host C++17 model build and print pass succeeded.
- Focused SPrice and swarm host tests succeeded.
- `uvx platformio run -d firmware/reconstructed/nmminer-v2-baseline` succeeded;
  output firmware size is `575616` bytes with PlatformIO flash use `575253`
  bytes after adding lifecycle, recovered SPrice behavior validation, and
  recovered swarm writer/publisher validation.
- Generated `partitions.bin` matched the official v2.0.02 ST7789
  `partitions.bin` SHA-256
  `f1882db7f6e352825314f3dcc5db01085e74625655c5f1decf58dd84fa118226`.
- `python3 -m py_compile scripts/nmminer_addr_map.py
  scripts/nmminer_patch_literal.py scripts/nmminer_str_xref.py` succeeded.
- `git diff --check` succeeded, and no `std::span`/`<span>` use remains in the
  reconstructed baseline.
- Guard-negative checks for PlatformIO `upload`, `program`, `erase`, `uploadfs`,
  and `uploadfsota` all failed immediately through `scripts/guard_no_upload.py`.

No flash, upload, program, erase, LittleFS write, or serial/device mutation was
performed.

## No-Touch Zones

- License/activation verification and related server trust paths.
- Wi-Fi credentials, wallet values, device license values, NVS secrets.
- Raw serial logs that may contain secrets.
- Full flash backups and modified stock binaries.

## Device Write Gate

No patched official artifact gets flashed until:

- the reconstructed source-built baseline reaches the agreed official-behavior
  validation target, not just compile success;
- a fresh full 4 MB backup is taken;
- backup size and SHA-256 are recorded;
- the patched image passes static validation;
- the target remains `esp32-2432s028r-st7789`;
- `verify_flash`, hard USB power-cycle, and restore command are ready.
