---
gsd_state_version: 1.0
milestone: v3.0
milestone_name: — NMMiner RE + Custom Integrations
current_phase: 7
current_phase_name: Official v2.0.02 Binary Customization
current_plan: 07-01
total_phases: 8
total_plans_in_phase: 2
status: executing
stopped_at: Phase 7 remains on 07-01. The active route is official `v2.0.02` ST7789 binary decompilation/custom-edit from the backed-up official app; the fork UI/touch path is recovery context only. Public `v2.0.02` firmware distribution is binary-only. The tracked reconstructed source scaffold at `firmware/reconstructed/nmminer-v2-baseline/` preserves recovered DROM/layout/binding facts plus decoded Miner runtime-state evidence, SPrice market config, WatchCoins tokenizer, runtime+0x74 setter, batch parser, row producer evidence, SPrice patch target records, LittleFS screensaver asset records, Clock BTC route records, the scan/swarm controller proof, the official five-segment/IRAM1 startup proof from entrypoint `0x40086aa4`, raw startup literals/ordered startup steps from `Official_startup.recovered.c`, the BSS clear range `0x3ffc4100..0x3ffd6b30`, constructor predicate/sweep `0x40130cd0 -> 0x40086910`, scheduler handoff `0x4008766c`, runtime service slots `+0x74/+0x80/+0x84/+0x88`, source-baseline gates, and official lifecycle/task evidence through `0x400de1e4`, `0x400de25d`, `(button)` / `(scan)` / `(swarm)` / `(stratum)` / `(tMiner)` helper records, and split `(nMiner)` setup/helper evidence at `0x400dae29` / `0x400dae8a`. Miner accepted/rejected/block-hit/notify-job/hashrate fields are recorded as `state+0x130`, `+0x134`, `+0x138`, `+0x140`, and `+0x158/+0x15c`; `state+0x140` is a notify/job update counter candidate with writer evidence at `0x400f1258` plus seed/increment sites, and it is not accepted shares. Direct publisher `0x400d69f9` / slot `+0x44` reads `runtime+0x80 +0x158/+0x15c`; inspected ranges show no sync/bridge from mining `ctx+0x78` hashrate into that slot. Miner task-object publishers now have source assignment and constructor proof: `runtime+0x74` feeds `0x400d6e1a`/`0x400d6e57`, `runtime+0x84` is the `(scan)` controller, and `runtime+0x88` is the `(swarm)` controller feeding `0x400d6ea5`/`0x400d6ee2`/`0x400d6f31`; executable swarm writer/publisher recovery captures task `0x400dd35c`, count `+0x08`, metrics `+0x0c/+0x10/+0x14`, writer quartet `0x400ddb86..0x400ddb93`, and publisher trio `0x400d6ea5`/`0x400d6ee2`/`0x400d6f31`, while exact swarm float meanings, Serial/UART logging, and cleanup routes remain unresolved/no-touch. `07-SPRICE-PATCH-SAFETY.md` classifies SPrice byte-edit classes: the default WatchCoins DROM literal is local-patch-tool eligible, while SPrice producer/callsite/cap/pointer edits remain blocked or no-go. `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` proves official LittleFS `.ss` add/replace mechanics and corrects the official bundled two-asset image versus recovered expanded 17-asset image distinction. `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md` names `runtime+0x80` as `clock_btc_ticker_cache`, names cache `+0x158/+0x15c` as `btc_usd_last_price`, separates `runtime+0x74` as the SPrice/WatchCoins/header-color side path, and keeps `runtime+0x88` no-touch for Clock BTC work. Flashable ESP32 integration from the recovered startup and begin-style app/task routes, rollback/write gates, exact swarm parser/source semantics, and SPrice instruction-level edit bytes remain open. No firmware write was performed.
last_updated: "2026-06-20T06:24:57Z"
last_activity: "2026-06-20 -- Continued GSD Phase 7 on the official v2.0.02 ST7789 binary route. Added executable recovered swarm writer/publisher behavior to firmware/reconstructed/nmminer-v2-baseline/: offset-preserving layout for count +0x08 and metrics +0x0c/+0x10/+0x14, writer quartet 0x400ddb86..0x400ddb93, publisher trio 0x400d6ea5/0x400d6ee2/0x400d6f31, and the 1000.0 precision threshold. Exact swarm float meanings remain open. Host C++17 model build/print, focused SPrice host test, focused swarm host test, and PlatformIO ESP32 compile passed; lifecycle validation remains 8/10 and flashable=0, firmware.bin is 575616 bytes with flash use 575253 bytes, generated partitions.bin still byte-matches the official v2.0.02 ST7789 partition table, and upload/program/erase/uploadfs/uploadfsota guard checks block. This is source-reconstruction progress, not official behavior parity or a flash approval. No firmware write was performed."
progress:
  total_phases: 8
  completed_phases: 6
  total_plans: 23
  completed_plans: 22
  percent: 96
  phase_percent: 50
---

# State

Current Phase: 7
Current Phase Name: Official v2.0.02 Binary Customization
Current Plan: 07-01
Total Phases: 8
Total Plans in Phase: 2
Progress: 96%

Last updated: 2026-06-20 startup/entrypoint recovery. Active work is official `v2.0.02`
ST7789 binary decompilation/custom-edit from
`hardware/backups/cyd-backup-20260617-172106.bin` and
`firmware/v2.0.02/esp32-2432s028r-st7789/`. The June 1 `v1.8.28` dump is
historical/factory evidence, not the primary customization baseline. The
current public flasher also lists `v2.0.03` for this board, but the repo's
extracted/decompiled official baseline remains verified `v2.0.02`. See
`.planning/phases/07-on-device-validation/07-BINARY-BASELINE-ROUTE.md`,
`07-BINARY-DECOMP-TASKS.md`, and `07-BINARY-EDIT-TARGET-MAP.md`.

## Completed (Phase 8, Plan 01)

**2026-06-04:** Phase 8 Plan 1 executed (08-01): PlatformIO setup and NerdMiner_v2 fork.

- Tasks 1-3, 5 complete: PlatformIO installed, NerdMiner cloned, ST7789 verified, secrets .gitignore hardened.
- Task 4 (baseline build) blocked by upstream TFT_eSPI configuration issue (documented in SUMMARY.md).
- Commit: 1b9ee59 (feat(08-01): establish PlatformIO build environment and clone NerdMiner_v2 fork).
- Baseline firmware compilation deferred; all environment and repository setup complete.

> NOTE: the "Phase 8" above belongs to the ARCHIVED v2.0.02 milestone
> (`.planning/archive/v2.0.02-fork-milestone/`). v3.0 phase numbering resets at 1.

## Prior Completed

- Git repository initialized.
- Manual PDF downloaded to `docs/vendor/NM-CYD-ESP32-2432S028-manual.pdf`.
- Manual text extracted to `docs/vendor/NM-CYD-ESP32-2432S028-manual.txt`.
- Official GitHub README, API reference, flasher JS, and version manifest saved under `docs/research/`.
- Device detected on `/dev/ttyUSB0`.
- Full 4 MB flash backup completed.
- Backup SHA-256: held outside tracked docs.
- Original firmware: `v1.8.28`.
- Latest official firmware at that time: `v2.0.02`. Current public latest
  checked on 2026-06-19 is `v2.0.03`; this repo remains pinned to the backed-up
  official `v2.0.02` ST7789 baseline for decompilation/custom edits.

- Display target: `esp32-2432s028r-st7789`.
- `nmap-2.4g` AP observed locally.
- User clarified that `Homebase` exists and is the 2.4 GHz SSID.
- `NMController_web` README, main Python file, and web monitor template snapshots downloaded.
- Firmware upgraded to official `v2.0.02` using the `esp32-2432s028r-st7789` target.
- License verification passes after upgrade.
- Serial config sent for `Homebase` without disconnecting the laptop from `Homebase5`.
- Device joined LAN at `192.168.1.136`.
- API verified firmware `v2.0.02`, hostname `cyd2.8_1a76f7`, SSID `Homebase`, timezone `-4`, and NTP sync.
- Recovered BTC wallet is configured for both primary and fallback pools.
- Runtime pool testing selected `pool.nerdminers.org:3333` for both primary and fallback so both configured URLs match the Amazon manual PDF and the only PDF-listed pool that completed live subscribe testing.
- Post-reboot API showed active mining on `pool.nerdminers.org:3333`, accepted shares increasing, and zero rejected shares.
- Customization audit completed in `docs/customization-audit.md`.
- Fresh official flasher snapshots match saved snapshots; `v2.0.02` still supports `esp32-2432s028r-st7789`.
- Main firmware repo audit found prebuilt firmware binaries but no published ESP32 firmware source tree.
- LittleFS extraction confirmed embedded web UI and screensaver assets are inspectable.
- GitHub profile review completed for `kleinpanic` and saved in `docs/kleinpanic-customization-basis.md`.
- Official NMminer1024 repo review confirmed `NMminer1024/NMMiner` is the repo for this ESP32 software bundle; no separate public source repo for this CYD firmware was found.
- Confirmed the running `/home/broklein/codeWS/Python/NMController_web` monitor pre-existed this repo work and is a clean upstream checkout, not a customized local dashboard.
- Canonical local dashboard added under `apps/nmminer-control`.
- Local dashboard is running at `http://127.0.0.1:8788`.
- Dashboard implementation split live status polling from settings loading to reduce request pressure on the ESP32.
- A normal API restart recovered mining after a temporary zero-hash state; post-restart readbacks showed about `1.04 MH/s` and accepted shares increasing.
- Rendered visual audit completed for the local dashboard at desktop and mobile widths with no console errors or horizontal overflow.
- Dashboard polish added safe device personalization controls for hostname and time display, fixed partial API error rendering, slowed live polling to reduce ESP32 load, and documented LAN access limits.
- Device display/weather preferences were configured: 12-hour time, Eastern offset, US date format, 70% brightness, LED enabled, image screensaver after 5 minutes, and Fahrenheit/mph weather for Washington, DC.
- Current firmware exposes brightness and timed screensaver behavior, but not the old `AutoBrightness`/`ScreenTimeout` settings found in older firmware strings.
- Laptop wallpaper images were converted into valid NMMiner `.ss` screen savers with `scripts/nmminer_screensaver.py`; generated batches are local artifacts and gitignored.
- Screen saver uploads are stored on the ESP32 internal LittleFS partition, not on SD/TF. Live readback after upload showed `393216` bytes total and `344064` used.
- Current NMMiner firmware should be treated as not using external humidity/sensor, speaker, or SD/TF workflows even though the CYD board family exposes those connectors.
- 2026-06-03: User physically installed a microSD card and a speaker on the CYD board (a Flipper Zero is also plugged into the laptop but is unrelated/charging).
- 2026-06-03: Firmware feasibility researched (Brave + Firecrawl + local binary recon). Findings in `docs/firmware-customization-feasibility.md`: stock NMMiner v2.0.02 is binary-only, unlicensed, and exposes zero SD/audio in its 21-endpoint HTTP API; its image already links the SD (VSPI/FatFS) and LEDC drivers but never uses them for files/audio.
- 2026-06-03 DECISION: do NOT extend the stock binary. Fork the MIT-licensed `BitMaker-hub/NerdMiner_v2`, target the ESP32-2432S028R ST7789 variant, and graft SD + speaker code from `witnessmenow/ESP32-Cheap-Yellow-Display`. Build custom features + our own web API and re-point the dashboard. Binary RE kept only as a fallback. (Archived milestone.)

## Needs User Input

- Hardware photos or pin measurements before wiring JST, CH1/CN1, P3, speaker, or SD/TF modifications.

## Risk Notes

- `Homebase5` was observed only on 5 GHz channels during local scan; use `Homebase` for this ESP32.
- Flash backup and recovered config contain secrets and are gitignored.
- Firecrawl could not scrape the Amazon page because the account has insufficient credits.
- Do not connect the laptop to the miner AP while it needs to stay online through `Homebase5`; use USB serial and LAN API.

## Current Binary Work

- `scripts/nmminer_str_xref.py --all` resolves the official `v2.0.02` screen
  class anchors plus `ss_render`.

- `scripts/nmminer_addr_map.py` maps DROM, DRAM, IRAM0, IROM, and IRAM1 VM
  addresses to app offsets and full-flash offsets for the official ST7789 app
  image.

- `07-BINARY-EDIT-TARGET-MAP.md` now contains concrete first-pass target records:
  fixed-length DROM literal candidates, shared/overlapping literals, startup
  IRAM evidence, and function offset records for decompilation navigation.

- `scripts/nmminer_patch_literal.py` can generate local-only patched app images
  for approved fixed-length DROM literals, refresh the ESP32 checksum/hash
  footer, and validate with `esptool image-info`. A `/tmp` tagline patch was
  tested; no app was flashed.

- `07-SPRICE-PATCH-SAFETY.md` now records SPrice `WatchCoins` byte-safety:
  default literal `0x3f400cda` / app `0xcda` is local-patch-tool eligible, while
  the `USDT` suffix, Binance URL, `PricePageMode` pointer/literal, cache/token
  caps, row trim caps, market task callsites, `runtime+0x74` setter, and
  producer internals remain blocked or no-go byte edits. No app was flashed.

- `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` now records official LittleFS `.ss`
  add/replace mechanics: official bundled `littlefs.bin` at `0x390000` /
  `0x60000` has two `.ss` assets, the separate recovered expanded image has 17,
  `littlefs-python` can rebuild same-geometry images, and rebuilt add/replace
  images list, extract, and decode correctly. No LittleFS image was flashed.

- `.planning/config.json` sets `workflow.use_worktrees=false` because this
  session runs under Codex, and GSD execute-phase explicitly rejects Codex
  worktree isolation. Use sequential/main-worktree execution or read-only
  explorer subagents for Phase 7 until a Codex worktree isolation path exists.

- Official startup recovery is now represented by
  `hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`.
  It records the official five-segment app map, IRAM1 entrypoint `0x40086aa4`,
  constructor sweep `0x40086910`, BSS clear range `[0x3ffc4100, 0x3ffd6b30)`,
  framework-hook targets reached from the entry route, the `0x400de1e4`
  begin-style app bring-up route, the `0x400de25d` all-tasks-started log
  route, task helper evidence for `(button)`, `(scan)`, `(swarm)`, `(stratum)`,
  and `(tMiner)`, plus split `(nMiner)` setup/helper evidence at
  `0x400dae29` / `0x400dae8a`. It passed
  `gcc -fsyntax-only` validation and is navigation/no-touch evidence until the
  recovered route is turned into an ESP32/LVGL firmware entrypoint.

- The reconstructed baseline at
  `firmware/reconstructed/nmminer-v2-baseline/` now carries typed C++ copies of
  the official startup literals/steps, a higher-level official bring-up/task
  route, runtime service slots, source-baseline gates, lifecycle validation
  checks, executable recovered SPrice/WatchCoins behavior, and executable
  recovered swarm writer/publisher behavior. The WatchCoins
  tokenizer now host-tests the official default, empty-string behavior,
  exactly-20-token cap, and truncation before the 21st token; the SPrice row
  layer adapts records into the recovered 24-byte row/cache shape, orders by the
  recovered `+16` price-descending key, and trims to the 16-row rendered window
  while keeping exact equal-price and NaN behavior open. The swarm layer keeps
  float fields offset-named while validating the writer quartet, publisher trio,
  count increment, and precision threshold. 2026-06-20 validation: host C++17
  build/print passed, focused SPrice host test passed, focused swarm host test
  passed, PlatformIO ESP32 build passed, lifecycle validation reports `8/10`
  checks and `flashable=0`, `firmware.bin` size is `575616` bytes with flash
  use `575253` bytes, generated `partitions.bin` byte-matches the official
  `v2.0.02` ST7789 partition table, and guarded write targets block.
  No firmware write was performed.

- BIN-02 runtime update evidence now maps Loading progress/status/IP callbacks,
  Config status text/color callbacks, Miner generic text/color callbacks and
  `FUN_400f4b24` binding fan-out, the corrected Miner runtime/share/hashrate
  range `0x400f0858..0x400f1d2e`, Clock BTC text route through `global+0x4f4`
  and `FUN_400f1eb0`, Weather cb1/cb2/cb3, SPrice event/data/draw-worker
  addresses, and share-result dispatch callsites. These are navigation records,
  not function patch approvals.

- Latest BIN-02 corrections: `FUN_400ef24c` is a parser helper/no-touch, not a
  Miner producer; `0x400f0846..0x400f13e8` is `mining.notify` / `params`
  parsing and internal job-state update, not callback-model publication;
  clean Xtensa objdump of `nmminer-app.elf` confirms `FUN_400f4b24` binds
  Miner text records `global+0x68..global+0x33c` to slots `+0x04..+0x58`;
  callback-model publication is generic through `FUN_400d6104` and
  `FUN_400d38cc`, dispatched by `FUN_4011b6a4`, with expanded direct
  callsites in `0x400d672e..0x400d6fc9`; Miner accepted/rejected/block-hit,
  notify-job, and hashrate runtime fields are now captured in a checked
  evidence table in `Miner_dynamic_model.recovered.c` and the reconstructed
  baseline: `state+0x130`, `state+0x134`, `state+0x138`, `state+0x140`, and
  `state+0x158/+0x15c`; `state+0x140` is a notify/job update counter candidate
  written by `0x400f1258` and supporting seed/increment sites, not accepted
  shares; direct publisher `0x400d69f9` / slot `+0x44` reads `runtime+0x80
  +0x158/+0x15c`; inspected ranges show no bridge from mining `ctx+0x78`
  hashrate into that slot; task-object source slots are split as
  `runtime+0x74` for the market stat/color pair, `runtime+0x84` for `(scan)`,
  and `runtime+0x88` for the `(swarm)` bottom count/metric trio; the
  writer/publisher boundary for count `+0x08` and metrics `+0x0c/+0x10/+0x14`
  is now executable in the scaffold, while exact swarm float meanings,
  Serial/UART logging, and cleanup remain explicitly unresolved/no-touch; Clock BTC text producer callsite
  `0x400d82a1` is
  fed by `FUN_400d57c0()` -> `runtime+0x80 clock_btc_ticker_cache` ->
  `btc_usd_last_price` at cache `+0x158/+0x15c`; `runtime+0x74` is the
  SPrice/WatchCoins/header-color side path; BTC price color is static
  `0xb596` evidence and no BTC-only dynamic color writer was found;
  `FUN_4012da8c` is
  string cleanup/destructor-like reset logic, not a UI notifier;
  `FUN_4012d668` / `DAT_400d00d0` is Arduino Serial/UART logging used by
  hundreds of callsites, not a UI/event hook; SPrice `pricewall` is a live mode
  compare through `FUN_400f4dfc` but not a direct renderer edit anchor;
  the SPrice cache writer at `0x400d73be..0x400d7519` copies `runtime+0x74`
  source-vector rows into inline 24-byte records at `global+0xd94` and count at
  `global+0x1244`; `WatchCoins` config/defaults, body `0x400e798c`,
  tokenizer window `0x400e7b21..0x400e7bc9`, market config path
  `0x400db307..0x400db7d5`, ticker wrapper/parser `0x400e3b84` /
  `0x400e3e84`, runtime setter `0x400ddf61`, batch parser `0x400e2c28`, and
  row producer `0x400e3a80` are now source-recovered as config/tokenizer/ticker
  and source-vector producer evidence; no instruction-level `WatchCoins` patch
  target is approved yet; Weather cb1/cb2/cb3 are update-only and
  `FUN_400f3ab4` is source-recovered as the static builder; `ss_render` code/header constants are
  no-touch and visible customization belongs in LittleFS `.ss` assets.

- First recovered source-equivalent output is now present at
  `hardware/recovered-assets/decompiled/source/Miner_dynamic_model.recovered.c`.
  It reconstructs the official-binary Miner dynamic model binding fan-out,
  direct publisher callsite table, runtime-state evidence table, no-touch route
  records, and generic text/value publisher pseudocode from clean Xtensa
  objdump. It passed `gcc -fsyntax-only`.

- Additional recovered source-equivalent outputs are now present and
  syntax-checked:
  `hardware/recovered-assets/decompiled/source/Weather_static_builder.recovered.c`,
  `hardware/recovered-assets/decompiled/source/Clock_btc_price.recovered.c`,
  `hardware/recovered-assets/decompiled/source/SPrice_market_renderers.recovered.c`, and
  `hardware/recovered-assets/decompiled/source/SPrice_watchcoins_config.recovered.c`, and
  `hardware/recovered-assets/decompiled/source/SPrice_market_runtime.recovered.c`, and
  `hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`.
  These recover Weather `FUN_400f3ab4`, Clock `FUN_400f1eb0` /
  `FUN_400f4204` / `FUN_400f421c`, and SPrice `FUN_400f54e4` /
  `FUN_400f5ba0` / `FUN_400f63d8` / `FUN_400f69a4`, plus WatchCoins
  config/tokenizer/ticker evidence around `0x400e798c` and `0x400e3b84`, plus
  the WatchCoins batch parser `0x400e2c28` and row producer `0x400e3a80`, plus
  the official entrypoint/startup route and app lifecycle/task bring-up route
  from the official binary.

- First tracked reconstructed source scaffold is now present at
  `firmware/reconstructed/nmminer-v2-baseline/`. It converts the official
  binary recovery facts into C++ source for the verified address map, screen
  widget specs, SPrice binding records, SPrice market config/default records,
  Miner runtime-state evidence, WatchCoins tokenizer evidence, SPrice runtime setter/task/row-producer
  evidence, SPrice patch target records, Weather forecast/callback records,
  publisher callsites, runtime evidence records, and SPrice 24-byte row cache
  writer model. It now also carries the five-segment official app map and
  entrypoint offsets plus app lifecycle/task evidence needed for startup and
  begin-style bring-up recovery. It host-compiles with C++17 and now has a
  compile-only PlatformIO ESP32/Arduino/LVGL harness using the recovered
  official partition geometry and ST7789 display flags. It is the active
  baseline scaffold to grow into source-equivalent official behavior. It is not
  flash-approved firmware yet.

- Remaining BIN-02 work is proof and safety: turn the recovered official
  `0x40086aa4` startup route and `0x400de1e4` begin-style app/task route into
  a real ESP32/LVGL firmware entrypoint, finish
  exact swarm parser/source semantics, instruction-level edit contracts, and any whole-binary
  sync check outside the inspected `0x400d61c0..0x400d8957` and
  `0x400f0858..0x400f1d2e` ranges;
  keep the proven accepted/rejected/block-hit/notify-job/hashrate offsets
  separate and no-touch until an instruction contract exists; keep the
  Clock BTC source/color closure in force; document exact Weather
  edit bytes/rollback from the recovered widget map; turn the decoded SPrice
  `WatchCoins` batch parser and row producer chain into an instruction-level
  patch/rollback plan; validate LittleFS `.ss` asset add/replace mechanics; and
  document checksum/hash refresh requirements.

- Current corrections: `FUN_400f4830` is callback registration, not the Loading
  `NMMiner` title builder; `FUN_400f24ac` is a `:` compound-label splitter, not
  the BTC price-color branch; `FUN_400f4204` is not the BTC price producer;
  `FUN_400ef24c` is parser/no-touch; `0x400f0846..0x400f13e8` is not the Miner
  callback-model publisher; generic Miner publication is through
  `FUN_400d6104` / `FUN_400d38cc`; Clock BTC text comes from
  `runtime+0x80 clock_btc_ticker_cache`, not `runtime+0x74`; Clock BTC color is
  static `0xb596` evidence with no BTC-only dynamic writer found; SPrice
  `pricewall` is a mode compare through
  `FUN_400f4dfc` but not a direct renderer edit anchor; SPrice
  `0x400d73be..0x400d7519` is the runtime-to-global cache writer; `WatchCoins`
  source-vector production is decoded through `0x400e2c28` and `0x400e3a80`,
  but no instruction-level patch target is approved yet;
  Weather static geometry is recovered from `FUN_400f3ab4`; `ss_render` visual
  customization belongs to LittleFS `.ss` assets; `0x400f1620` is a mid-block
  share seam and not an approved standalone hook.

- Choose the first real replacement text and output path only after confirming
  the desired visible change. Do not flash before the fresh-backup write gate.

## Legacy Hardware Blockers

- Physical touch is not proven: the active HSPI/direct-probe path reported 31 diagnostics with 0 pressed and 0 IRQ-low observations in the reset-only window, then 23 diagnostics with 0 pressed and 0 IRQ-low observations in the later passive no-write window.
- Physical CYD panel/UI review is failed/unclassified: browser recovery portal screenshots do not validate the on-device LVGL panel, and the user physically observed that the live panel looks wrong and not like the expected NMMiner UI.
- Runtime mining is not proven on the current custom decoded-spec app: 2.4 GHz Wi-Fi/pool join, accepted shares, speaker, SD/API, and dashboard panels remain UAT-gated.
- Restore is not proven end to end: latest full backup and restore command are documented, but the full restore round-trip has not been run.

## Session

Last Date: 2026-06-20T06:24:57Z
Stopped At: Phase 7 `07-01` remains open on the official `v2.0.02` ST7789 binary customization route. Continue from the recovered source-equivalent artifacts and the tracked reconstruction scaffold at `firmware/reconstructed/nmminer-v2-baseline/`. The scaffold now has a compile-only PlatformIO ESP32/Arduino/LVGL harness using official partition geometry and ST7789 flags, executable recovered SPrice/WatchCoins behavior, executable recovered swarm writer/publisher behavior, and a guard that refuses reconstructed upload/program/erase targets unless explicitly overridden after the backup/write gate. Open work is filling that harness with recovered official startup, begin-style app/task routing, runtime model behavior, UI/API/mining/config parity, exact swarm parser/source semantics, instruction-level edit bytes, checksum validation for future modified artifacts, rollback records, and whole-binary sync review outside the inspected ranges. No firmware write was performed.
Resume File: .planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md

## Session Continuity

Last session: 2026-06-19T21:10:10.761Z
Stopped at: Phase 7 authority index `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md` routes work to the official `v2.0.02` ST7789 binary baseline with corrected parser/runtime classifications. Continue from `07-BINARY-DECOMP-TASKS.md`, `07-BINARY-EDIT-TARGET-MAP.md`, and `scripts/nmminer_patch_literal.py`. Fork UI/touch artifacts below are historical unless needed for restore/reversibility.

## Milestone v3.0 — NMMiner RE + Custom Integrations (status: Phase 7 in progress)

**Decided:** Recover and customize the official stock NMMiner `v2.0.02`
ST7789 binary directly. The fork rebuild is historical failure/recovery context,
not the implementation route. Binary work remains local-only: no
license/activation path edits, no secrets, no redistribution, and no device write
without a fresh full backup.

**ROADMAP (8 phases, coarse, versioned `v3.0 / Phase N`):**

1. RE-Spec the UI (read-only) — RE-01..RE-04
2. Recover Fonts/Glyphs + Filesystem Assets — ASSET-01..ASSET-03
3. Rebuild Core Screens + UIManager Carousel — UI-01..UI-06
4. Restore Interactive Touch — TOUCH-01..TOUCH-03 (historical SoftSPI plan; current no-VSPI HSPI/direct-probe path)
5. Data-Driven Screens + HTTP API Parity — DATA-01..DATA-06
6. Wire Custom Features at the Share-Accept Seam — HW-01..HW-04
7. Official v2.0.02 Binary Customization — BIN-01..BIN-05 plus gated restore/write proof
8. Reserved / superseded fallback slot — no active work

Coverage: 35/35 v3.0 requirements mapped, 0 unmapped.

**DONE prior session:**

- v3.0 formalized in PROJECT.md + STATE.md + REQUIREMENTS.md (committed).
- 4-dimension research + synthesis committed: `.planning/research/{STACK,FEATURES,ARCHITECTURE,PITFALLS,SUMMARY}.md`.
- RE FOUNDATION (all on disk, recovered-assets gitignored): ESP32 image parsed with DROM `0x3f400020` + IROM `0x400d0020` isolated; LittleFS images extracted and distinguished: the official bundled image has two `.ss` assets, while the recovered expanded image has 17; `.ss` format fully decoded with `scripts/nmminer_ss_decode.py`; IROM disassembled via objcopy to ELF; string to function xref works, including `Clock320x240` at `0x400d1864` and `Miner320x240` at `0x400d1960`. Pipeline doc: `docs/nmminer-re-pipeline.md`.

**NEXT:** continue Phase 7 on the official `v2.0.02` ST7789 binary
decompilation/custom-edit route. The immediate work is runtime update-path
decompilation and local-only fixed-length DROM literal patch tooling. Hardware
validation, touch/panel tuning, and fork UI work are legacy/recovery context only
unless needed to restore the device.

**Fallback track (v2.0.02 fork):** historical/archived evidence only: device mines + speaker/API were verified on the older fallback track, but that is not current live proof. The current custom decoded-spec firmware still needs Wi-Fi/pool/mining, speaker, dashboard/API, and restore validation.

Resume: `/gsd-resume-work`.
Resume file: `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md`

## Current Position

Phase: 7 — IN PROGRESS / OFFICIAL `v2.0.02` ST7789 BINARY CUSTOMIZATION
Status: milestone NOT complete. The active implementation route is the official
NMMiner `v2.0.02` `esp32-2432s028r-st7789` app image already backed up and
byte-matched to the local official bundle. Current work is binary
decompilation/custom-edit tooling, not further fork UI/touch iteration.

Last activity: 2026-06-19 -- Rechecked official release/flasher/CYD metadata,
kept the repo pinned to the verified local `v2.0.02` ST7789 baseline, integrated
the binary explorer findings into the Phase 7 map and recovered specs, corrected
stale Loading/Clock/SPrice/share-seam assumptions, set GSD Codex execution to
no-worktree mode, and validated a throwaway `/tmp` loading-tagline patch with
`esptool image-info`. No firmware write occurred.

Current handoff: start with
`.planning/phases/07-on-device-validation/07-BINARY-BASELINE-ROUTE.md`,
`.planning/phases/07-on-device-validation/07-BINARY-DECOMP-TASKS.md`,
`.planning/phases/07-on-device-validation/07-BINARY-EDIT-TARGET-MAP.md`, and
`docs/nmminer-re-pipeline.md`. The current device may still contain the prior
custom decoded-spec fork; do not iterate it except to restore or preserve
reversibility.

Binary track status: active primary route. The official stock `v2.0.02` ST7789
binary is the customization baseline because it byte-matches the backed-up app
region and the local official bundle. The current on-device firmware may still
be the custom decoded-spec fork from prior work, but further work should not
iterate that fork except for restore/reversibility. Patching remains local-only:
no license-path edits, no secrets, no redistribution, and no flash without the
fresh-backup write gate.

## Historical Fork Failure Record (2026-06-18)

This section is retained for recovery context only. It is superseded by the
official `v2.0.02` ST7789 binary customization route above.

**What actually happened:** Phases 3–6 were marked "complete/verified" on the strength of
**compile-success + code-review only**. The actual on-device *rendering* was deferred to Phase 7.
When Phase 7 finally flashed to the real ESP32-2432S028R, the rebuilt LVGL UI **did not work**:

- **Boot-loop crash** — default 8 KB Arduino loopTask stack overflowed (LVGL + ArduinoJson +
  .ss decode) → "Stack canary watchpoint triggered" Guru Meditation → reboot loop → black screen.
  FIXED this session (SET_LOOP_TASK_STACK_SIZE 32 KB, commit 054e9f5).

- **Stock screens clobbering LVGL** — ungated stock TFT_eSPI loading/setup screens painted over
  the LVGL panel. Partly fixed (gated), but see CR-02 below.

- **Config-mode LoadProhibited crash** — drawSetupScreen rerouting to the LVGL config page from the
  WiFiManager callback dereferenced an unbuilt page → boot loop. FIXED (no-op), but CR-02 regression.

- **Upside-down render** — nm_display used setRotation(1); stock flips to 3. FIXED (commit b83a12a).
- **WHITE SCREEN (historical failure before 07-02)** — loading + config screens set bg_color black
  WITHOUT bg_opa COVER (only Miner did it right). H5 fixed this in code, but physical panel proof
  is still pending.

- **WiFi regression (historical failure before 07-02)** — a bug introduced "SD-authoritative config"
  (tryConfigFromSD) overriding NVS creds every boot + writing cleartext WifiPW to the card (CR-01).
  H1 fixed this in code; current serial reaches the recovery AP and the recovery web root returns
  HTTP 200.

- **Config mode invisible** — the drawSetupScreen no-op removed the SSID/192.168.4.1 display with
  nothing replacing it (CR-02).

- **Touch** — the current failure is not just calibration. The active HSPI/direct-probe path boots,
  but serial diagnostics still show no physical press signal (`directPressed=0`, `libPressed=0`).
  The next useful step is probing the XPT2046/PENIRQ hardware path or re-identifying the touch variant.

**Process failure (root):** the GSD lifecycle for Phases 3–6 used compile gates, not on-device
verification. "Verified 6/6 / UI 6/6" meant "compiled + code-reviewed," NOT "renders on hardware."
Resume MUST add on-device verification per screen as the gate (user-approved: "harden the LVGL
rebuild, on-device gated").

**Full audit + fix plan:** `.planning/phases/07-on-device-validation/07-REVIEW.md`
(2 Critical, 7 Warning, 5 Info; hardening tasks H1–H5).

### 07-02 gap closure (2026-06-18)

**Software repairs completed and build-passed:**

- **H1 closed in code** — NVS Wi-Fi config wins; SD config is fallback/import-only; firmware no longer writes `WifiPW` to SD; SD config reads reject oversized/truncated files before parsing.
- **H5 closed in code** — Loading and Config LVGL roots use explicit `LV_OPA_COVER`.
- **H2 closed in code** — WiFiManager config mode shows AP SSID + portal IP through the LVGL Config page without persisting `LastPage`.
- **H3 prepared for hardware calibration** — touch mapping tracks landscape rotation 1/3; default rotation 3 inverts both base axes; raw/calibrated diagnostics remain for first on-device presses.
- **H4 closed in code** — display brightness, invert colors, and supported landscape `RotateScreen` are re-applied after config load and through the preference API.
- **Review follow-up closed** — `07-REVIEW-02.md` found WiFiManager parameter lifetime, portal credential sync, and SD path normalization regressions; those were fixed and re-reviewed in `07-REVIEW-03.md`, including the final SD import warning resolution. A later subagent recheck found a legacy `SDCard` fallback/teardown path and unsupported `RotateScreen` persistence; those are now fixed and rechecked.
- **Build gate passed** — `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R` succeeded in 00:00:23.101; RAM 38.0%, flash 90.6%, firmware payload 2,855,344 bytes.
- **Hardware gate later exercised** — after 07-02, Phase 7 did run backup/flash/verify/serial checks on the custom ST7789 build, restored the official stock ST7789 binary as a recovery baseline after touch diagnostics remained unresolved, reflashed the patched custom decoded-spec build and decoded LittleFS assets, then app-reflashed the latest LittleFS/no-legacy-SD code-review fixes after another fresh full backup. Future firmware writes still require a fresh full backup before write.
- **Driver gate resolved to ST7789** — the custom `ESP32-2432S028R` build now uses `-DST7789_DRIVER=1`; the live on-device firmware is the patched custom decoded-spec ST7789 target.
- **CR-03/CR-04 and current code-review fixes flashed in custom build** — the first physical-presence API mutation gate was too broad. The flashed custom fork uses strict config-mode gating, removes casual touch/page unlocks, redacts wallet/pool values from serial/API, stops mirroring wallet/pool passwords to SD, avoids the TFT_eTouch SPIFFS formatter, removes later runtime `LittleFS.begin(true)` format fallbacks, and disables legacy `SDCard` ownership on the CYD target. The earlier 2,826,704-byte app was superseded by the later webfix2 app flash/readback; current live app SHA-256 is `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.
- **Review follow-ups and fallback-pool gap fixed and flashed** — `07-TOUCH-SOFTWARE-REVIEW.md` found the legacy CYD `doLedStuff()` monitor-task path still called the old touch reader; `07-CURRENT-FIRMWARE-REVIEW-02.md` and `07-CURRENT-FIRMWARE-REVIEW-03.md` found an unsafe/raw-logging `readCustomAPName()` parser, config portal AP-name serial leak, and `.ss` upload validation/staging gaps; the UAT audit found missing primary/fallback-pool support; `07-CURRENT-FIRMWARE-REVIEW-05.md` found mining target, compact-target, pool-parse, NVS stats, and stratum password-buffer blockers. These are fixed, `ESP32-2432S028R` build passed, app flash/readback passed, and source mismatch no longer blocks UAT. This does not change the live no-contact touch evidence or prove mining.

**Artifacts:** `.planning/phases/07-on-device-validation/07-02-PLAN.md`, `07-02-SUMMARY.md`, `07-REVIEW-FIX.md`, `07-REVIEW-02.md`, `07-REVIEW-03.md`, `07-TOUCH-SOFTWARE-REVIEW.md`, `07-CURRENT-FIRMWARE-REVIEW-02.md`, `07-CURRENT-FIRMWARE-REVIEW-03.md`, `07-CURRENT-FIRMWARE-REVIEW-04.md`, `07-CURRENT-FIRMWARE-REVIEW-05.md`, and updated `07-VALIDATION.md`.

### Historical Fork Remaining Work (Superseded)

The list below belongs to the custom-fork recovery path and is not the active
implementation queue. The current queue is the BIN-02/BIN-03/BIN-04 work in
`07-BINARY-DECOMP-TASKS.md` and `07-BINARY-EDIT-TARGET-MAP.md`.

1. Get physical confirmation from the operator: screen shows the config portal (`nmap-2.4g`, `192.168.4.1`), colors are readable/not inverted, and LED state is reported.
2. Physically inspect/probe touch before more code tuning using `.planning/phases/07-on-device-validation/07-TOUCH-PROBE-RUNBOOK.md`: confirm the touch controller/FPC, probe XPT2046 PENIRQ/GPIO36 during firm press, and confirm CS=33, CLK=25, MOSI=32, MISO=39 activity or identify a different/no-touch variant.
3. Hard USB unplug/replug and confirm the same clean portal boot returns.
4. Decide intended custom JSON API exposure model. Current recovery portal mode returns HTTP 200 for WiFiManager `/` and `/info`, but 404 for custom JSON API routes.
5. Configure the custom device through the portal or serial config on a 2.4 GHz SSID when physical baseline is accepted.
6. Verify LAN API, pool connection, nonzero hashrate, accepted shares, speaker, SD/API, and dashboard panels.
7. Per-screen on-device verify Loading/Miner/Clock/SPrice/Weather/Config.
8. Complete VALID-01..03 only after the remaining physical/runtime evidence exists.

### Historical Device / Repo State At Pause

This records what the custom-fork flash path looked like before the route
correction. It does not change the current target: official `v2.0.02`
`esp32-2432s028r-st7789` binary customization.

- Fork HEAD: `b83a12a` plus uncommitted 07-02/07-01 hardware-flash repairs.
- Device: currently running the custom decoded-spec `ESP32-2432S028R` ST7789 build with the latest code-review app fixes, fallback-pool support, post-forensics source-blocker fixes, review-05 mining/config hardening fixes, and recovered `v2.0.02` identity. The official stock `v2.0.02` ST7789 binary remains the verified recovery baseline but is no longer the live firmware.
- Current custom recovery state: NVS was cleared after backup; serial boot reaches WiFiManager config mode as `nmap-2.4g` at `192.168.4.1`; recovery web routes `/` and `/info` return HTTP 200 from `192.168.4.1`; custom JSON API routes return 404 in portal mode; decoded LittleFS asset image at `0x310000` verifies after the latest app reflash.
- Stock recovery baseline, historical: serial/web UI recovery was proven earlier by reflashing official stock `v2.0.02`, joining as `192.168.4.2`, and loading the NMMiner setup page.
- Current full-image restore target for the latest pre-write state: `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` (SHA-256 `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed`). Restore command:
  `/home/broklein/.local/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin`.

- Historical 4 MB pre-custom-flash backup for this run: `hardware/backups/cyd-backup-20260618-202520.bin` (checksum held outside tracked docs).

- Fresh 4 MB pre-stock-restore backup: `hardware/backups/cyd-backup-20260618-172750-pre-official-st7789-restore.bin` (checksum held outside tracked docs).
- Fresh 4 MB pre-stock-reflash backup: `hardware/backups/cyd-backup-20260618-174623-pre-stock-st7789-reflash.bin` (checksum sidecar stored next to the ignored backup).
- Fresh 4 MB pre-custom decoded reflash backup: `hardware/backups/cyd-backup-20260618-184025-pre-no-touch-spiffs-format-reflash.bin` (checksum sidecar stored next to the ignored backup).
- Fresh 4 MB pre-NVS-clear backup: `hardware/backups/cyd-backup-20260618-185101-pre-nvs-clear-after-custom-decoded.bin` (checksum sidecar stored next to the ignored backup).
- Fresh 4 MB pre-current-code-review-fix app reflash backup: `hardware/backups/cyd-backup-20260618-231853-pre-current-code-review-fix-reflash.bin` (checksum sidecar stored next to the ignored backup).
- Fresh 4 MB pre-v2.0.02-identity app reflash backup: `hardware/backups/cyd-backup-20260619-001919-pre-v2002-identity-app-reflash.bin` (previous restore target before the fallback-pool app flash).
- Fresh 4 MB pre-fallback-pool app reflash backup: `hardware/backups/cyd-backup-20260618-235938-pre-fallback-pool-flash.bin` (previous restore target before the source-blocker-fix app flash).
- Fresh 4 MB pre-source-blocker-fix app reflash backup: `hardware/backups/cyd-backup-20260619-042756-pre-source-blocker-fix-app-flash.bin` (previous restore target before the review-05 app flash).
- Fresh 4 MB pre-review05-fix app reflash backup: `hardware/backups/cyd-backup-20260619-045710-pre-review05-fix-app-flash.bin` (previous restore target before the recovery-portal CSS app flashes).
- Fresh 4 MB pre-web-ui-fix app reflash backup: `hardware/backups/cyd-backup-20260619-052605-pre-web-ui-fix-app-flash.bin` (previous restore target before the final webfix2 app flash).
- Fresh 4 MB pre-web-ui-fix2 app reflash backup: `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` (current restore target above).

- SD card: formatted FAT32 (label NMMINER), inserted; mounts on-device (HW-02 confirmed live).
- /dev/ttyUSB0 present. ESP32-D0WD-V3; current target remains `esp32-2432s028r-st7789`.

### Earlier blocker (resolved)

- 05-05 dram0_0_seg overflow (D-58) — resolved during Phase 5 (s_pairs resize); build green at 90.5%.
