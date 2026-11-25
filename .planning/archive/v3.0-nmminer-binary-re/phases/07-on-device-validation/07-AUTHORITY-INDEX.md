# Phase 7 Authority Index

**Updated:** 2026-06-20T04:36:27Z
**Status:** current routing and evidence map for Phase 7 recovery.
**Scope:** use this to decide which Phase 7 artifacts are authoritative before resuming, flashing, verifying, or closing any gate.

## Route Override

The active route is now the official NMMiner `v2.0.02` ST7789 binary
decompilation/custom-edit route, not further fork UI/touch iteration. Use
`07-BINARY-BASELINE-ROUTE.md` as the routing authority for the baseline.

- Target backup: `hardware/backups/cyd-backup-20260617-172106.bin`
- Target official bundle: `firmware/v2.0.02/esp32-2432s028r-st7789/`
- Target RE artifacts: `hardware/recovered-assets/app_factory.bin`,
  `drom.bin`, `irom.bin`, `irom.asm`, and `decompiled/`
- Historical/factory snapshot only:
  `hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin`
  (`v1.8.28`)
- Public flasher now also lists `v2.0.03` for this board, but the repo's backed
  up/decompiled official baseline is `v2.0.02`; do not silently switch versions.

## Current Route

- GSD route remains Phase 7 / `07-01`, but the work focus is corrected to the
  official `v2.0.02` binary baseline route above.
- `07-02` and later UI/touch repair docs are historical diagnostics unless
  needed for restore/reversibility.
- Do not start Phase 8 or create `07-01-SUMMARY.md` unless the binary baseline
  route is either validated or explicitly abandoned with a failure summary.
- GSD execution under Codex must use `workflow.use_worktrees=false`. Codex
  execute-phase cannot safely use the workflow's isolated worktree-agent path, so
  read-only explorer subagents are allowed for decompilation research, while file
  edits stay centralized in the main orchestrator.

## Live Firmware Identity

Current live firmware is the custom decoded-spec `ESP32-2432S028R` ST7789 fork in WiFiManager recovery AP mode.

| Field | Current Value |
|-------|---------------|
| Live app type | Custom rebuilt fork, not proprietary stock |
| Version text | Intentionally reports recovered NMMiner `v2.0.02` |
| Current app artifact | 2,840,400 bytes |
| Current app SHA-256 | `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` |
| App flash offset | `0x10000` |
| Decoded LittleFS asset offset | `0x310000` |
| Recovery AP | `nmap-2.4g` / `192.168.4.1` |

Do not use `v2.0.02` text alone to identify stock firmware. The official stock `v2.0.02` image is a verified recovery baseline only; the current live device is the custom rebuilt firmware that deliberately reports the same version string for parity.

## Current Authority Files

| File | Use For | Authority Notes |
|------|---------|-----------------|
| `.planning/STATE.md` | GSD resume state and blockers | Must route to Phase 7 / `07-01`; currently records the source-equivalent official-binary recovery artifacts as resume context. |
| `.planning/forensics/report-20260619-204301.md` | Current process-failure, route-correction, and state audit | Latest forensic report. Treat older forensic reports as historical snapshots. |
| `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md` | Operator-facing live fork truth table | Best concise source for what is live and what failed on the current custom fork. Its "next action" section is recovery/reversibility context, not the active binary-decomp route. |
| `.planning/phases/07-on-device-validation/07-VERIFICATION.md` | VALID-01..03 gate status | Current score is `0/3` VALID requirements fully verified. |
| `.planning/phases/07-on-device-validation/07-UAT-AUDIT.md` | Cross-phase human UAT queue | Current queue is 8 active prerequisite-gated items, 0 stale. |
| `.planning/phases/07-on-device-validation/07-VALIDATION.md` | Detailed flash/build/readback chronology | Use for hashes, offsets, supersession history, and detailed evidence. |
| `.planning/phases/07-on-device-validation/07-SPRICE-PATCH-SAFETY.md` | SPrice `WatchCoins` byte-safety classification | Current authority for the decoded SPrice source-vector patch classes. The default `WatchCoins` literal is local-patch-tool eligible; SPrice function/callsite/cap/pointer edits remain blocked or no-go. |
| `.planning/phases/07-on-device-validation/07-LITTLEFS-SS-ASSET-REPLACEMENT.md` | Official LittleFS `.ss` replacement mechanics | Current authority for screensaver asset replacement. It distinguishes the official bundled two-asset `littlefs.bin` from the separate recovered expanded 17-asset image, proves add/replace with `littlefs-python`, and keeps `ss_render` code no-touch. |
| `.planning/phases/07-on-device-validation/07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md` | Clock BTC source/color closure | Current authority for recovered Clock BTC runtime names: `runtime+0x80` is `clock_btc_ticker_cache`, cache `+0x158/+0x15c` is `btc_usd_last_price`, `runtime+0x74` is SPrice/WatchCoins/header-color side path, and no BTC-only dynamic color writer was found. |
| `.planning/phases/07-on-device-validation/07-TOUCH-PROBE-RUNBOOK.md` | Next physical touch/panel procedure | Use before any more touch code tuning. |
| `.planning/phases/07-on-device-validation/07-UI-REVIEW.md` | UI review status | Current score is 8/24. Browser recovery UI evidence does not prove physical CYD panel UI; user-observed physical panel parity is failed/unclassified. |
| `.planning/debug/touch-no-press.md` | Touch no-contact debug thread | Use for consolidated touch failure context. |
| `.planning/debug/physical-ui-mismatch.md` | Physical display mismatch debug thread | Use for panel visual parity triage: config page vs miner page, color/inversion/swap, rotation, LVGL screen parity, or flush/SPI artifacts. |
| `scripts/phase7_serial_touch_summary.py` | Serial-log summarization | Use on `/tmp` serial logs and copy only derived counts into tracked docs. |

## Historical Snapshots

Older forensic reports and earlier Phase 7 sections are still useful for chronology, but they may name superseded app hashes or intermediate failure states.

| Snapshot | Current Handling |
|----------|------------------|
| `.planning/forensics/report-20260618-195307.md` through `report-20260619-042211.md` | Historical. Read only with later supersession in mind. |
| Official stock `v2.0.02` restore evidence | Historical recovery baseline, not current live firmware. |
| Earlier custom app hashes before `f9547fbb...` | Superseded by the current webfix2 app flash/readback. |
| Phase 4 SoftSPI wording | Historical plan language. Current source/evidence is no-VSPI HSPI/direct-probe touch. |
| Browser recovery screenshots | Prove recovery portal reachability and CSS clipping fix only, not the physical CYD LVGL panel. |

## Active Binary Open Gates

| Gate | Status | Evidence Required |
|------|--------|-------------------|
| BIN-01 baseline provenance | Closed | `07-BINARY-BASELINE-ROUTE.md` records official bundle hashes, backup byte matches, image metadata, and `v2.0.03` non-switch decision. |
| BIN-02 decompilation map | In progress | Screen-class anchors, builder entries, generic setters, Miner binding fan-out, Miner runtime/share/hashrate range `0x400f0858..0x400f1d2e`, Clock BTC text route through `global+0x4f4`, SPrice kline/candle renderer, SPrice cache writer, SPrice WatchCoins runtime producer, Weather static builder, share-result call shapes, official five-segment startup/lifecycle evidence, and LittleFS `.ss` asset add/replace mechanics are mapped. New source-equivalent artifacts are syntax-checked under `hardware/recovered-assets/decompiled/source/`: `Miner_dynamic_model.recovered.c`, `Weather_static_builder.recovered.c`, `Clock_btc_price.recovered.c`, `SPrice_market_renderers.recovered.c`, `SPrice_watchcoins_config.recovered.c`, `SPrice_market_runtime.recovered.c`, and `Official_startup.recovered.c`. Corrections in force: `Official_startup.recovered.c` records the IRAM1 app entrypoint `0x40086aa4`, constructor sweep `0x40086910`, BSS clear range, framework hooks, `0x400de1e4` begin-style app bring-up route, `0x400de25d` all-tasks-started route, `(button)` / `(scan)` / `(swarm)` / `(stratum)` / `(tMiner)` helper evidence, and split `(nMiner)` setup/helper evidence at `0x400dae29` / `0x400dae8a`; `FUN_400ef24c` is a parser helper/no-touch, not a Miner producer; `0x400f0846..0x400f13e8` is `mining.notify` / `params` parsing and internal job-state update, not callback-model publication; clean Xtensa objdump of `nmminer-app.elf` confirms `FUN_400f4b24` binds Miner text records `global+0x68..global+0x33c` to slots `+0x04..+0x58`; Miner callback-model publication is generic through `FUN_400d6104` and `FUN_400d38cc`, dispatched by `FUN_4011b6a4`; `FUN_4012da8c` is string cleanup, not a UI notifier; `FUN_4012d668` / `DAT_400d00d0` is Arduino Serial/UART logging, not a UI/event hook; SPrice `pricewall` is a mode compare through `FUN_400f4dfc` but not a direct renderer edit anchor; SPrice `0x400d73be..0x400d7519` copies `runtime+0x74` source-vector rows into inline 24-byte records at `global+0xd94` and count at `global+0x1244`; `WatchCoins` config/defaults, body `0x400e798c`, tokenizer window `0x400e7b21..0x400e7bc9`, market task `0x400db278`, batch parser `0x400e2c28`, row producer `0x400e3a80`, and ticker wrapper/parser `0x400e3e84` / `0x400e3b84` are source-recovered as config/tokenizer/ticker and source-vector producer evidence; `07-SPRICE-PATCH-SAFETY.md` classifies the default `WatchCoins` literal as local-patch-tool eligible and keeps producer/callsite/cap/pointer edits blocked; `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` proves official LittleFS add/replace mechanics and distinguishes the official bundled two-asset image from the recovered 17-asset image; `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md` closes the Clock BTC route by naming `runtime+0x80` as `clock_btc_ticker_cache`, cache `+0x158/+0x15c` as `btc_usd_last_price`, and `runtime+0x74` as SPrice/WatchCoins/header-color side path; no BTC-only dynamic color writer was found and BTC price color remains static-builder evidence at `0xb596`; Weather cb1/cb2/cb3 are update-only and Weather static geometry is recovered from `FUN_400f3ab4`; `ss_render` code/header constants are no-touch and the LittleFS `.ss` asset route is proven. Remaining work is proof/safety: unresolved Miner writer/field semantics, SPrice instruction-level edit bytes/rollback beyond the default literal, checksum/hash refresh, and ESP32/LVGL integration. |
| BIN-03 edit target map | In progress | `07-BINARY-EDIT-TARGET-MAP.md` has concrete multi-segment offsets, first literal candidates, runtime callback records, producer/caller navigation records, SPrice `WatchCoins` patch-safety linkage, and explicit stale-claim corrections. No function/code patch is approved yet. |
| BIN-04 local patch tooling | In progress | `scripts/nmminer_patch_literal.py` patches approved fixed-length DROM literals, refreshes checksum/hash, validates with `esptool image-info`, and keeps outputs under gitignored local paths. |
| BIN-05 gated write/restore | Open | Fresh 4 MB backup before any patched-artifact write, `verify_flash`, hard power-cycle, and restore command preserved. |

## Legacy / Recovery Device Gates

These gates are not the current implementation route. Keep them only as recovery
and reversibility context if the live device must be restored, re-flashed, or
physically validated later.

| Gate | Status | Evidence Required |
|------|--------|-------------------|
| VALID-01 hard power cycle | Open | Operator physically unplugs/replugs USB and the same clean recovery state returns. |
| Physical panel / LED / UI | Failed / unclassified | User observed the live panel as visibly wrong. Need no-secret photo/video or checklist classifying actual CYD panel content, colors, rotation, clipping/overlap, stale regions/tearing, and LED state. |
| Physical touch signal | Open | Touch controller/FPC identified, PENIRQ/GPIO36 idle/pressed behavior, and CS=33/CLK=25/MOSI=32/MISO=39 activity or absence recorded. |
| Wi-Fi / pool / mining runtime | Open | 2.4 GHz Wi-Fi join, pool connection, nonzero hashrate, accepted share, and safe LAN/API/dashboard evidence. |
| Custom hardware outputs | Open | Accepted-share beep, disconnect alarm if tested, mining/share/disconnect LED states, and SD CSV stats/share rows. |
| Custom JSON API exposure | Open | Product decision and proof for API behavior in recovery portal mode versus LAN/mining mode. |
| Restore | Open | Full restore round-trip run by explicit opt-in, or restore explicitly deferred with backup integrity and command preserved. |

## Non-Authoritative Signals

- Red LED alone does not prove a firmware identity failure; red is expected while connecting or waiting for config.
- AP/captive-portal auth popups are expected WiFiManager behavior in config mode.
- Browser `/`, `/info`, `/wifi`, and `/0wifi` HTTP 200 responses prove the recovery portal, not the physical LVGL panel.
- The latest physical-panel observation is authoritative evidence that visual parity failed, but it is not yet specific enough to prove the root cause.
- Current touch failure is below LVGL/page routing/calibration: latest no-write evidence shows 23 touch diagnostics, 0 pressed observations, and 0 IRQ-low observations.
- The high GSD progress counter (`22/23`, 96%) is an artifact counter, not product completion.

## Next Evidence To Capture

1. Field/caller proof for the remaining unresolved generic Miner publisher
   writers/fields that feed the callback model records bound by `FUN_400f4b24`;
   SPrice source-vector patch follow-through after the now-recovered
   `WatchCoins` tokenizer/ticker path and `07-SPRICE-PATCH-SAFETY.md`
   classification; LittleFS `.ss` replacement policy after the local
   add/replace proof; and
   instruction-level byte edits/rollback.
2. Keep the current corrections in force: `FUN_400f4830` is Loading callback
   registration, not the `NMMiner` title builder; `FUN_400f24ac` is a
   `:` compound-label splitter, not the BTC price-color handler;
   `FUN_400f4204` is not the BTC price producer; Clock header color writes
   target `global+0x4b8/+0x4e4`, not the BTC price text record at
   `global+0x4f4`; `runtime+0x80` is the Clock BTC ticker cache and
   `runtime+0x74` is the SPrice/WatchCoins/header-color side path; BTC price
   color has static `0xb596` evidence and no BTC-only dynamic color writer was
   found; SPrice `pricewall` is a mode compare through
   `FUN_400f4dfc` but not a direct renderer edit anchor, while the default
   `WatchCoins` DROM literal is local-patch-tool eligible and the upstream
   config-to-cache builder/function edits remain blocked; `0x400e7978`,
   `0x400e798c`,
   `0x400e7b21..0x400e7bc9`, `0x400db307..0x400db7d5`, and
   `0x400e3e84` are decoded as config/tokenizer/ticker evidence;
   `FUN_400ef24c` is a parser
   helper/no-touch; `0x400f0846..0x400f13e8` is `mining.notify` parsing and
   internal job-state update, not the callback-model publisher; Weather
   cb1/cb2/cb3 are update-only while `FUN_400f3ab4` is the source-recovered
   static builder; `ss_render` code/header constants are no-touch and customization
   should use LittleFS `.ss` assets; the default `WatchCoins` DROM literal is
   local literal-patcher eligible while SPrice producer/callsite/cap/pointer
   edits are blocked or no-go; `FUN_4012da8c` is string cleanup, not
   notification; `FUN_4012d668` is Serial/UART logging and not an approved
   UI/event hook.
3. Keep function edits blocked until expected bytes, instruction encoding,
   caller side effects, checksum/hash refresh, and rollback are documented. A
   visible proof-of-edit may use only an approved fixed-length literal with
   `scripts/nmminer_patch_literal.py`; do not flash until a fresh full backup is
   taken.

## Hard Rules

- No firmware write without a fresh full 4 MB backup first.
- Do not flash an ILI9341 build unless the hardware is re-identified.
- Do not commit Wi-Fi passwords, wallet values, device licenses, raw serial logs containing secrets, or flash backups.
- Do not keep tuning touch thresholds, rotation, debounce, or LVGL gesture routing while raw touch contact remains absent.
