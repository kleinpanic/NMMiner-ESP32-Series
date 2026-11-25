# Phase 7 Binary Edit Target Map

**Updated:** 2026-06-20
**Baseline:** official NMMiner `v2.0.02` `esp32-2432s028r-st7789`
`firmware.bin`
**Status:** concrete offset map started. Literal-only candidates exist;
producer/caller mapping has advanced for Miner, Clock, SPrice, and the share
path, but function patches are still not patch-ready. SPrice `WatchCoins`
patch safety is now split out in `07-SPRICE-PATCH-SAFETY.md`; Clock BTC
source/color closure is split out in `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`.

## Image Facts

| Field | Value |
|-------|-------|
| App size | `2794160` bytes |
| Entry point | `0x40086aa4` (IRAM1 app offset `0x2970e8`, full-flash offset `0x2a70e8`) |
| DROM load | `0x3f400020` |
| DROM app file offset | `0x000018` segment header / `0x000020` payload |
| DROM length | `0x157de8` |
| DRAM load | `0x3ffbdb60` |
| DRAM app file offset | `0x157e08` segment header / `0x157e10` payload |
| DRAM length | `0x0065a0` |
| IRAM0 load | `0x40080000` |
| IRAM0 app file offset | `0x15e3b0` segment header / `0x15e3b8` payload |
| IRAM0 length | `0x001c60` |
| IROM load | `0x400d0020` |
| IROM app file offset | `0x160018` segment header / `0x160020` payload |
| IROM length | `0x13227c` |
| IRAM1 load | `0x40081c60` |
| IRAM1 app file offset | `0x29229c` segment header / `0x2922a4` payload |
| IRAM1 length | `0x017fe4` |
| ESP-IDF | `v4.4.6-dirty` |
| App validation hash | `df782f8b1f66aa01a39ad4fdf4c55addd062caa45e287af149301b7071bc5638` |
| Official LittleFS offset / size | `0x390000` / `0x60000` |
| Official LittleFS SHA-256 | `700681e91194ca233dfdef62725541f917270604279253c4233e5551bf93a2cf` |
| Official LittleFS `.ss` assets | `/ss/saver_320_240_001.ss`, `/ss/saver_320_240_002.ss` |
| Recovered expanded LittleFS SHA-256 | `44adf024f9b3291d55e3aa28d58536bca201cfb4249d95aa375dd32d9693144d` |
| Recovered expanded `.ss` assets | 17 files, `/ss/saver_320_240_002.ss` through `/ss/saver_320_240_018.ss` |

## Address Mapping Rule

Use `scripts/nmminer_addr_map.py` for all Phase 7 offset records. It maps a
DROM, DRAM, IRAM0, IROM, or IRAM1 VM address to:

- `app_offset`: byte offset inside
  `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin`.
- `flash_offset`: byte offset in a full 4 MB flash image, assuming the app is
  written at factory partition offset `0x10000`.

Verifier phrase: every edit record must include an app offset and no-touch
classification when applicable.

Example:

```bash
python3 scripts/nmminer_addr_map.py 0x3f49912a 0x400d195c 0x40086aa4
```

## Known Anchors

These are location anchors, not final patch approvals.

| Anchor | DROM address | IROM literal ref | Canonical entry | Status |
|--------|--------------|------------------|-----------------|--------|
| `Loading320x240` | `0x3f499108` | `0x400d1930` | `FUN_400f2c5c` | builder located; `FUN_400f4830` registers progress/status/IP callbacks |
| `Miner320x240` | `0x3f4be960` | `0x400d1960` | `FUN_400f2f5c` | builder, generic setters, and `FUN_400f4b24` callback-model binding fan-out located; `FUN_400ef24c` corrected to parser helper; runtime/share/hashrate evidence is in `0x400f0858..0x400f1d2e`; `0x400f0846..0x400f13e8` is `mining.notify` parsing/internal job update, while callback-model publication is generic through `FUN_400d6104` / `FUN_400d38cc` |
| `Clock320x240` | `0x3f47389b` | `0x400d1864` | `FUN_400f1f34` | builder and field binding located; BTC price text route is `runtime+0x80 clock_btc_ticker_cache +0x158/+0x15c -> global+0x4f4 -> FUN_400f1eb0 -> slot +0x30`; `runtime+0x74` is SPrice/WatchCoins/header-color side path; BTC price color is static `0xb596` evidence and no BTC-only dynamic color writer was found |
| `SPrice320x240` | `0x3f4e41a8` / `0x3f4e41a7` | `0x400d19a8` | `FUN_400f3570` | builder, `+0x24` event adapter, child draw callback, data-fill helper, chart renderer, WatchCoins rotation, `pricewall` mode compare, SPrice cache writer, and WatchCoins source-vector producer mapped; `pricewall` is not a direct renderer edit anchor and WatchCoins edits still need exact patch bytes/rollback |
| `Weather320x240` | `0x3f4e41d8` | `0x400d19e8` | `FUN_400f3ab4` static builder / `FUN_400f3a34` dispatcher | static builder source-recovered; runtime cb1/cb2/cb3 mapped; patch safety still needs exact edit bytes/rollback |
| `Config320x240` | `0x3f4738d4` | `0x400d190c` | `FUN_400f2b58` | builder and status text/color callbacks located |
| `ss_render` | `0x3f4e42db` | `0x400d1c14` | `FUN_400f6ff4` | path-based renderer; external `/ss/saver_{W}_{H}_{NNN}.ss` LittleFS asset route proven; official bundled image has 2 `.ss` files and recovered expanded image has 17; function patch not approved |
| `#%d share accepted, %ldms` | `0x3f4736c8` | `0x400d1800` | `0x400f15d0..0x400f174c` | narrow share-result block located; accepted/rejected/block-hit call shapes mapped; formatter callee is a printf-style stream writer, not an approved hook |
| Official app entry | N/A | N/A | `0x40086aa4` | IRAM1 startup route recovered from the official image header and `iram_tail.asm`; navigation/no-touch until ESP32 entry integration is source-built |

## Startup / IRAM Evidence

These records are entrypoint and framework navigation evidence, not edit
approvals. The source-equivalent artifact is
`hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`.

| Record | VM address | App offset | Flash offset | Segment | Evidence | Status |
|--------|------------|------------|--------------|---------|----------|--------|
| Official app entry | `0x40086aa4` | `0x2970e8` | `0x2a70e8` | IRAM1 | image header entrypoint plus `iram_tail.asm` | Navigation/no-touch |
| Constructor sweep | `0x40086910` | `0x296f54` | `0x2a6f54` | IRAM1 | gated from entry through predicate target `0x40130cd0` | Navigation/no-touch |
| Constructor invoker helper | `0x4008693c` | `0x296f80` | `0x2a6f80` | IRAM1 | called by constructor sweep path | Navigation/no-touch |
| System/config startup helper | `0x400869e0` | `0x297024` | `0x2a7024` | IRAM1 | direct call from app entry before framework hooks | Navigation/no-touch |
| Startup flag branch | `0x40086b0e` | `0x297152` | `0x2a7152` | IRAM1 | post-framework conditional branch site in entry route | Navigation/no-touch |
| Early memory/helper call | `0x4009640c` | `0x2a6a50` | `0x2b6a50` | IRAM1 | called with literal base `0x40080000` | Navigation/no-touch |
| BSS clear helper | `0x4008deac` | `0x29e4f0` | `0x2ae4f0` | IRAM1 | clears `[0x3ffc4100, 0x3ffd6b30)` | Navigation/no-touch |
| Constructor gate predicate | `0x40130cd0` | `0x1c0cd0` | `0x1d0cd0` | IROM | literal-loaded predicate before constructor sweep | Navigation/no-touch |
| Framework hook A | `0x40167080` | `0x1f7080` | `0x207080` | IROM | indirect call from entry route | Navigation/no-touch |
| Framework hook B | `0x4015ebe0` | `0x1eebe0` | `0x1febe0` | IROM | stack-context hook from entry route | Navigation/no-touch |

## App Lifecycle / Task Evidence

These records bridge the low-level startup route to the app-level NMMiner
bring-up path. They are source-reconstruction evidence only.

| Record | VM address | App offset | Flash offset | Literal / DROM | Evidence | Status |
|--------|------------|------------|--------------|----------------|----------|--------|
| `NMMiner::begin`-style routine entry | `0x400de1e4` | `0x16e1e4` | `0x17e1e4` | N/A | chains service/task setup calls before all-tasks-started log | Navigation/no-touch |
| All tasks started log | `0x400de25d` | `0x16e25d` | `0x17e25d` | literal `0x400d0a88`, DROM `0x3f401068` | `NMMiner::begin: all tasks started` string xref | Navigation/no-touch |
| LVGL init failure route | `0x400da489` | `0x16a489` | `0x17a489` | literal `0x400d082c`, DROM `0x3f400c0f` | `(lvgl) NMMiner::init: failed to create LVGL task` string xref | Navigation/no-touch |
| Hardware init route | `0x400da749` | `0x16a749` | `0x17a749` | literal `0x400d083c`, DROM `0x3f400c40` | `Initializing hardware...` string xref | Navigation/no-touch |
| Peripherals-ready route | `0x400da76b` | `0x16a76b` | `0x17a76b` | literal `0x400d0848`, DROM `0x3f400c62` | `Peripherals ready.` string xref | Navigation/no-touch |
| Button task helper | `0x400da765` | `0x16a765` | `0x17a765` | literal `0x400d0840`, DROM `0x3f400c59` | `(button)`, entry `0x400d5078`, stack `0x0c00`, priority `3` via helper `0x400d9c68` | Navigation/no-touch |
| Scan task helper | `0x400da82e` | `0x16a82e` | `0x17a82e` | literal `0x400d085c`, DROM `0x3f400c75` | `(scan)`, entry `0x400dbe38`, stack `0x0a00`, priority `4`, object `runtime+0x84` via helper `0x400d9c68` | Navigation/no-touch |
| Swarm task helper | `0x400da90a` | `0x16a90a` | `0x17a90a` | literal `0x400d0874`, DROM `0x3f400c7c` | `(swarm)`, entry `0x400dd35c`, stack `0x0c00`, priority `5`, object `runtime+0x88` via helper `0x400d9c68` | Navigation/no-touch |
| Stratum task helper | `0x400dad56` | `0x16ad56` | `0x17ad56` | literal `0x400d08a4`, DROM `0x3f400c8c` | `(stratum)`, entry `0x400ef724`, stack `0x1800`, priority `9` via helper `0x400d9c68` | Navigation/no-touch |
| tMiner task helper | `0x400dadcd` | `0x16adcd` | `0x17adcd` | literal `0x400d08b8`, DROM `0x3f400c96` | `(tMiner)` name/object setup precedes helper; entry `0x400e3f00`, stack `0x0800`, priority `1` | Navigation/no-touch |
| nMiner name/object setup | `0x400dae29` | `0x16ae29` | `0x17ae29` | literal `0x400d08d4`, DROM `0x3f400c9f` | `(nMiner)` string/object setup via `0x4012ddb4`; not the task helper itself | Navigation/no-touch |
| nMiner task helper | `0x400dae8a` | `0x16ae8a` | `0x17ae8a` | literal `0x400d08d4`, DROM `0x3f400c9f` | helper after setup; entry `0x400e3f00`, stack `0x0800`, priority `2`; object field typing unresolved | Navigation/no-touch |

## Concrete Target Records

These records are for the official `v2.0.02` ST7789 app only. `replacement`
stays `TBD` until the desired custom text/behavior is chosen.

| Target id | Current | Segment | VM address | App offset | Flash offset | Max bytes | Replacement | Patch status |
|-----------|---------|---------|------------|------------|--------------|-----------|-------------|--------------|
| `BIN02-LIT-LOAD-TAGLINE` | `Make it better` | DROM | `0x3f49912a` | `0x9912a` | `0xa912a` | 14 | TBD | Best first literal candidate; same-length or shorter null-padded text only |
| `BIN02-LIT-LOAD-IP-SEED` | `192.168.124.222` | DROM | `0x3f49911a` | `0x9911a` | `0xa911a` | 15 | TBD | Loading seed; may be runtime-overwritten, so visual effect must be verified |
| `BIN02-LIT-GLOBAL-VERSION` | `v2.0.02` | DROM | `0x3f4008eb` | `0x8eb` | `0x108eb` | 7 | TBD | Shared by Loading/Miner/Config/Clock; not a first patch target and requires explicit shared-target opt-in |
| `BIN02-LIT-CONFIG-COUNTDOWN-WIDTH` | `300s` | DROM | `0x3f4738e2` | `0x738e2` | `0x838e2` | 4 | TBD | Width reference, not the initial visible label; defer unless countdown layout is being changed |
| `BIN02-LIT-SPRICE-COIN-SEED` | `---` | DROM | `0x3f4737d4` | `0x737d4` | `0x837d4` | 3 | TBD | Runtime placeholder; defer unless cold-start visuals are the target |
| `BIN02-LIT-SPRICE-PRICE-SEED` | `$0` | DROM | `0x3f4e41b5` | `0xe41b5` | `0xf41b5` | 2 | TBD | Runtime placeholder; defer unless cold-start visuals are the target |
| `BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT` | `BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO` | DROM | `0x3f400cda` | `0xcda` | `0x10cda` | 39 | TBD | Approved for local-only fixed-length literal patch tooling; affects defaults only and existing NVS/config may override |
| `BIN02-LIT-KHS-HS-SHARED` | `kH/s` / overlapping `H/s` | DROM | `0x3f4e42d6` / `0x3f4e42d7` | `0xe42d6` / `0xe42d7` | `0xf42d6` / `0xf42d7` | 4 / 3 | TBD | Constrained/no-direct-edit record; overlapping shared units across screens |
| `BIN02-SEAM-SHARE-ACCEPT` | `#%d share accepted, %ldms` | DROM + IROM | `0x3f4736c8` / `0x400d1800` / `0x400f1620` | `0x736c8` / `0x161800` / `0x181620` | `0x836c8` / `0x171800` / `0x191620` | 25 format bytes | TBD | High-risk seam record only; do not alter until containing function, share-specific consumers, expected bytes, and rollback are documented |

## SPrice WatchCoins Patch Safety

Detailed SPrice byte records live in
`.planning/phases/07-on-device-validation/07-SPRICE-PATCH-SAFETY.md`.

Current classification:

| Target | Address | App offset | Status |
|--------|---------|------------|--------|
| Default `WatchCoins` literal | `0x3f400cda` | `0xcda` | Local literal-patcher eligible, same/shorter ASCII only |
| `USDT` suffix literal | `0x3f4008fb` | `0x8fb` | Blocked/no-touch; changes all pair symbols |
| Binance batch URL | `0x3f47222c` | `0x7222c` | Blocked/no-touch; network/trust/parser surface |
| `PricePageMode` `kline` literal | `0x3f400d31` | `0xd31` | Blocked; `pricewall` does not fit in-place |
| `PricePageMode` default pointer | `0x400d0914` | `0x160914` | Blocked; shared default pointer surface |
| Cache writer publish cap | `0x400d7428` | `0x167428` | Blocked except lower-cap research |
| WatchCoins tokenizer cap | `0x400e7b65` | `0x177b65` | Blocked; only lower/same-contract research |
| Row trim caps | `0x400e390a` / `0x400e3b28` | `0x17390a` / `0x173b28` | Blocked; layout/vector assumptions |
| Market task callsites | `0x400dbbc9` / `0x400dbbd1` | `0x16bbc9` / `0x16bbd1` | No-go; can leave stale/half-updated state |
| `runtime+0x74` setter | `0x400ddf61` | `0x16df61` | No-go structural path |

## Function Offset Records

Function records are for navigation and decompilation, not edit approval.

| Target id | Function | VM address | App offset | Flash offset | Evidence |
|-----------|----------|------------|------------|--------------|----------|
| `BIN02-FN-OFFICIAL-APP-ENTRY` | recovered official app entry | `0x40086aa4` | `0x2970e8` | `0x2a70e8` | official image header entrypoint; source-equivalent record in `Official_startup.recovered.c` |
| `BIN02-FN-OFFICIAL-CONSTRUCTOR-SWEEP` | constructor sweep path | `0x40086910` | `0x296f54` | `0x2a6f54` | reached from entry after predicate target `0x40130cd0`; navigation/no-touch |
| `BIN02-FN-OFFICIAL-CONSTRUCTOR-INVOKER` | constructor invoker helper | `0x4008693c` | `0x296f80` | `0x2a6f80` | helper in the constructor sweep path; navigation/no-touch |
| `BIN02-FN-OFFICIAL-SYSTEM-STARTUP` | system/config startup helper | `0x400869e0` | `0x297024` | `0x2a7024` | direct call from official entry before framework hooks; navigation/no-touch |
| `BIN02-CS-OFFICIAL-STARTUP-FLAG` | post-framework startup branch | `0x40086b0e` | `0x297152` | `0x2a7152` | branch site inside entry route; navigation/no-touch |
| `BIN02-CS-OFFICIAL-IROM-PREDICATE` | constructor-gate predicate target | `0x40130cd0` | `0x1c0cd0` | `0x1d0cd0` | literal-loaded IROM predicate before constructor sweep; navigation/no-touch |
| `BIN02-CS-OFFICIAL-IROM-HOOKS` | framework hook targets | `0x40167080` / `0x4015ebe0` | `0x1f7080` / `0x1eebe0` | `0x207080` / `0x1febe0` | indirect entry-route framework hooks; names unresolved, no-touch |
| `BIN02-FN-OFFICIAL-BEGIN` | begin-style app bring-up routine | `0x400de1e4` | `0x16e1e4` | `0x17e1e4` | chains service/task setup calls and reaches all-tasks-started log at `0x400de25d`; navigation/no-touch |
| `BIN02-CS-OFFICIAL-BEGIN-ALL-TASKS` | all-tasks-started log route | `0x400de25d` | `0x16e25d` | `0x17e25d` | loads DROM `0x3f401068` / literal `0x400d0a88` for `NMMiner::begin: all tasks started`; navigation/no-touch |
| `BIN02-CS-OFFICIAL-TASK-REGISTRATIONS` | task/name helper callsites | `0x400da765` / `0x400da82e` / `0x400da90a` / `0x400dad56` / `0x400dadcd` / `0x400dae29` / `0x400dae8a` | `0x16a765` / `0x16a82e` / `0x16a90a` / `0x16ad56` / `0x16adcd` / `0x16ae29` / `0x16ae8a` | `0x17a765` / `0x17a82e` / `0x17a90a` / `0x17ad56` / `0x17adcd` / `0x17ae29` / `0x17ae8a` | `(button)`, `(scan)`, `(swarm)`, `(stratum)`, `(tMiner)` helper evidence plus split `(nMiner)` setup/helper evidence |
| `BIN02-SRC-OFFICIAL-STARTUP` | recovered source-equivalent artifact | `0x40086aa4` / `0x40086910` / `0x400869e0` / `0x400de1e4` / `0x400de25d` | `0x2970e8` / `0x296f54` / `0x297024` / `0x16e1e4` / `0x16e25d` | `0x2a70e8` / `0x2a6f54` / `0x2a7024` / `0x17e1e4` / `0x17e25d` | `hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`; startup segment table, literal table, BSS clear range, constructor gate, framework hook evidence, and app lifecycle/task evidence |
| `BIN02-FN-LOADING-BUILDER` | `FUN_400f2c5c` | `0x400f2c5c` | `0x182c5c` | `0x192c5c` | `SPEC_Loading.md`, `vtables/Loading_FUN_400f2c5c.c` |
| `BIN02-FN-MINER-BUILDER` | `FUN_400f2f5c` | `0x400f2f5c` | `0x182f5c` | `0x192f5c` | `SPEC_Miner.md`, `vtables/Miner_FUN_400f2f5c.c` |
| `BIN02-FN-MINER-CONTAINER` | `FUN_400f34d0` | `0x400f34d0` | `0x1834d0` | `0x1934d0` | creates/stylizes panel and calls `FUN_400f4b24` |
| `BIN02-CS-MINER-BIND-FANOUT` | callsite to `FUN_400f4b24` | `0x400f3551` | `0x183551` | `0x193551` | `FUN_400f34d0` -> `FUN_400f4b24` |
| `BIN02-FN-MINER-TEXT-CB` | `FUN_400f48a8` | `0x400f48a8` | `0x1848a8` | `0x1948a8` | generic field `lv_label_set_text` callback |
| `BIN02-FN-MINER-COLOR-CB` | `FUN_400f48c0` | `0x400f48c0` | `0x1848c0` | `0x1948c0` | generic field text-color callback |
| `BIN02-FN-MINER-CB-REG` | `FUN_400f48f0` | `0x400f48f0` | `0x1848f0` | `0x1948f0` | registers generic Miner callbacks |
| `BIN02-FN-MINER-BIND-FANOUT` | `FUN_400f4b24` | `0x400f4b24` | `0x184b24` | `0x194b24` | binds callback records to concrete Miner slots |
| `BIN02-FN-GLOBAL-MODEL-ACCESSOR` | `FUN_400d53d4` | `0x400d53d4` | `0x1653d4` | `0x1753d4` | global callback-model accessor/initializer used by Miner callback registration and runtime update paths |
| `BIN02-FN-GENERIC-TEXT-PUBLISHER` | `FUN_400d6104` | `0x400d6104` | `0x166104` | `0x176104` | generic string-record publisher; compares/assigns string and dispatches subscribers via `FUN_4011b6a4` |
| `BIN02-FN-GENERIC-VALUE-PUBLISHER` | `FUN_400d38cc` | `0x400d38cc` | `0x1638cc` | `0x1738cc` | generic 32-bit value/color publisher; dispatches subscribers via `FUN_4011b6a4` |
| `BIN02-FN-GENERIC-PUBLISH-DISPATCH` | `FUN_4011b6a4` | `0x4011b6a4` | `0x1ab6a4` | `0x1bb6a4` | downstream subscriber dispatcher/scheduler used by text and value publishers |
| `BIN02-SRC-MINER-DYNAMIC-MODEL` | recovered source-equivalent artifact | `0x400f4b24` / `0x400d6104` / `0x400d38cc` | `0x184b24` / `0x166104` / `0x1638cc` | `0x194b24` / `0x176104` / `0x1738cc` | `hardware/recovered-assets/decompiled/source/Miner_dynamic_model.recovered.c`; syntax-checked source-equivalent of the binding fan-out, direct publisher table, runtime-state evidence table, confidence-labeled field semantics, no-touch route records, and generic publisher pseudocode |
| `BIN02-SRC-MINER-RUNTIME-EVIDENCE` | recovered source-equivalent artifact | `0x400f1607` / `0x400f16a4` / `0x400f16f4` / `0x400f1258` / `0x400f1817` / `0x400f1a17` / `0x400d621b` / `0x400d621d` / `0x400de20b` / `0x400da894` / `0x400da90a` / `0x400ddb86..0x400ddb93` | `0x181607` / `0x1816a4` / `0x1816f4` / `0x181258` / `0x181817` / `0x181a17` / `0x16621b` / `0x16621d` / `0x16e20b` / `0x16a894` / `0x16a90a` / `0x16db86..0x16db93` | `0x191607` / `0x1916a4` / `0x1916f4` / `0x191258` / `0x191817` / `0x191a17` / `0x17621b` / `0x17621d` / `0x17e20b` / `0x17a894` / `0x17a90a` / `0x17db86..0x17db93` | mirrored in `firmware/reconstructed/nmminer-v2-baseline/`; names accepted `state+0x130`, rejected `state+0x134`, block-hit `state+0x138`, notify/job update counter `state+0x140`, combined hashrate `state+0x158/+0x15c`, task-object source slots `runtime+0x74` / `runtime+0x88`, and `runtime+0x88` as the `(swarm)` controller installed with `(scan)` at `runtime+0x84`; keeps runtime+0x80 slot `+0x44`, exact swarm float meanings, Serial/UART dispatch, and cleanup no-touch |
| `BIN02-SRC-CLOCK-BTC-PRICE` | recovered source-equivalent artifact | `0x400f1eb0` / `0x400d57c0` / `0x400d82a1` | `0x181eb0` / `0x1657c0` / `0x1682a1` | `0x191eb0` / `0x1757c0` / `0x1782a1` | `hardware/recovered-assets/decompiled/source/Clock_btc_price.recovered.c`; syntax-checked source-equivalent of BTC price text route, `runtime+0x80` Clock BTC ticker cache, and header color route |
| `BIN02-CLOSURE-CLOCK-BTC-SOURCE-COLOR` | recovered source/color closure | `0x400dac58` / `0x400d81e4` / `0x400d836e` / `0x400d8409` | `0x16ac58` / `0x1681e4` / `0x16836e` / `0x168409` | `0x17ac58` / `0x1781e4` / `0x17836e` / `0x178409` | `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`; names `runtime+0x80` as `clock_btc_ticker_cache`, `+0x158/+0x15c` as BTC last-price double words, and refutes a BTC-only dynamic color writer in the checked Clock scope |
| `BIN02-SRC-WEATHER-BUILDER` | recovered source-equivalent artifact | `0x400f3ab4` | `0x183ab4` | `0x193ab4` | `hardware/recovered-assets/decompiled/source/Weather_static_builder.recovered.c`; syntax-checked source-equivalent of Weather static builder/widget map |
| `BIN02-SRC-SPRICE-MARKET-RENDERERS` | recovered source-equivalent artifact | `0x400f54e4` / `0x400f5ba0` / `0x400f63d8` / `0x400f69a4` | `0x1854e4` / `0x185ba0` / `0x1863d8` / `0x1869a4` | `0x1954e4` / `0x195ba0` / `0x1963d8` / `0x1969a4` | `hardware/recovered-assets/decompiled/source/SPrice_market_renderers.recovered.c`; syntax-checked source-equivalent of market binding/rendering routes |
| `BIN02-SRC-SPRICE-WATCHCOINS-CONFIG` | recovered source-equivalent artifact | `0x400e7978` / `0x400e798c` / `0x400e7b21..0x400e7bc9` / `0x400e3b84` / `0x400e3e84` | `0x177978` / `0x17798c` / `0x177b21..0x177bc9` / `0x173b84` / `0x173e84` | `0x187978` / `0x18798c` / `0x187b21..0x187bc9` / `0x183b84` / `0x183e84` | `hardware/recovered-assets/decompiled/source/SPrice_watchcoins_config.recovered.c`; syntax-checked source-equivalent of market config defaults, WatchCoins 20-symbol tokenizer, and ticker parser evidence; config sanitizer, not the row materializer |
| `BIN02-SRC-SPRICE-MARKET-RUNTIME` | recovered source-equivalent artifact | `0x400ddf61` / `0x400db278` / `0x400e2c28` / `0x400e3a80` | `0x16df61` / `0x16b278` / `0x172c28` / `0x173a80` | `0x17df61` / `0x17b278` / `0x182c28` / `0x183a80` | `hardware/recovered-assets/decompiled/source/SPrice_market_runtime.recovered.c`; syntax-checked source-equivalent of `runtime+0x74` setter, `(market)` task, WatchCoins batch fetch/parser, and 24-byte SPrice row producer |
| `BIN02-FS-OFFICIAL-LITTLEFS` | official LittleFS image | `spiffs` partition label | N/A | `0x390000` | `firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin`; 393216 bytes, block size 4096, block count 96, SHA-256 `700681e91194ca233dfdef62725541f917270604279253c4233e5551bf93a2cf`, contains two `.ss` files |
| `BIN02-FS-RECOVERED-EXPANDED-LITTLEFS` | recovered expanded LittleFS image | `spiffs` partition label | N/A | `0x390000` if used with official partition layout | `hardware/recovered-assets/spiffs.bin`; 393216 bytes, block size 4096, block count 96, SHA-256 `44adf024f9b3291d55e3aa28d58536bca201cfb4249d95aa375dd32d9693144d`, contains 17 `.ss` files and is not byte-identical to official bundle |
| `BIN02-FS-SS-ASSET-REPLACE-PROOF` | local LittleFS add/replace proof | N/A | N/A | no write | `07-LITTLEFS-SS-ASSET-REPLACEMENT.md`; rebuilt official `0x60000` images with `littlefs-python`, listed/extracted them, and decoded all serialized `.ss` files |
| `BIN02-FN-PARSER-HELPER` | `FUN_400ef24c` | `0x400ef24c` | `0x17f24c` | `0x18f24c` | JSON/parser-style helper; handles token bytes such as `f`, `t`, `n`, `{`, `[`, and quoted strings; no-touch and not a Miner producer |
| `BIN02-BLOCK-MINER-RUNTIME` | mining/runtime block candidate | `0x400f0858..0x400f1d2e` | `0x180858..0x181d2e` | `0x190858..0x191d2e` | contains parser seeding, share-result counters/logging, hashrate accumulator, telemetry formatting, and cleanup paths; navigation only |
| `BIN02-BLOCK-MINER-NOTIFY-PARSER` | former local publisher candidate, now parser/internal state | `0x400f0846..0x400f13e8` | `0x180846..0x1813e8` | `0x190846..0x1913e8` | `mining.notify` / `params` parser plus internal job-state update at `*(ctx+0x78)`; no direct or literal-pool refs to callback-model helpers |
| `BIN02-BLOCK-MINER-HASHRATE` | hashrate accumulator block | `0x400f1817` | `0x181817` | `0x191817` | computes/stores hashrate deltas under state offsets `+0x148..+0x15c` |
| `BIN02-BLOCK-MINER-STATUS-TABLE` | status/table formatter block | `0x400f1949` | `0x181949` | `0x191949` | formats uptime/status/telemetry table values before record notifications |
| `BIN02-CS-MINER-TELEMETRY-ROW` | callsite to `FUN_4012d668` | `0x400f1bc2` | `0x181bc2` | `0x191bc2` | passes full telemetry table row format to Serial/UART logging |
| `BIN02-CS-MINER-STRING-CLEANUP` | calls to `FUN_4012da8c` | `0x400f1bd2..0x400f1c2d` | `0x181bd2..0x181c2d` | `0x191bd2..0x191c2d` | cleans temporary/state-relative string records after formatting; not notification |
| `BIN02-FN-MINER-NUM-FMT` | `FUN_400f9200` | `0x400f9200` | `0x189200` | `0x199200` | compact numeric string formatter used for hashrate and difficulty fields |
| `BIN02-FN-MINER-TIME-FMT` | `FUN_400f94c4` | `0x400f94c4` | `0x1894c4` | `0x1994c4` | time/status formatter candidate |
| `BIN02-FN-STRING-CLEANUP` | `FUN_4012da8c` | `0x4012da8c` | `0x1bda8c` | `0x1cda8c` | thin wrapper around string cleanup/reset; not a UI notifier or patch hook |
| `BIN02-FN-STRING-CLEANUP-CORE` | `FUN_4012da70` | `0x4012da70` | `0x1bda70` | `0x1cda70` | frees external string storage, clears record mode bits and pointers |
| `BIN02-FN-STRING-RESET-EMPTY` | `FUN_4012da50` | `0x4012da50` | `0x1bda50` | `0x1cda50` | helper used after cleanup to reset string record length/value to empty |
| `BIN02-FN-CLOCK-BUILDER` | `FUN_400f1f34` | `0x400f1f34` | `0x181f34` | `0x191f34` | `SPEC_Clock.md`, `vtables/Clock_FUN_400f1f34.c` |
| `BIN02-FN-CLOCK-PRICE-TEXT` | `FUN_400f1eb0` | `0x400f1eb0` | `0x181eb0` | `0x191eb0` | high-confidence BTC price text/layout owner for slot `+0x30` |
| `BIN02-FN-CLOCK-COMPOUND-SPLIT` | `FUN_400f24ac` | `0x400f24ac` | `0x1824ac` | `0x1924ac` | colon compound-label splitter; not BTC price color |
| `BIN02-FN-CLOCK-TEXT-SETTER` | `FUN_400f4204` | `0x400f4204` | `0x184204` | `0x194204` | generic Clock text setter for the byte `+0x0c` path; not the BTC price producer |
| `BIN02-FN-CLOCK-COLOR-HELPER` | `FUN_400f421c` | `0x400f421c` | `0x18421c` | `0x19421c` | generic color helper; no BTC-specific dynamic color path is proven in the checked direct and scoped non-direct writers |
| `BIN02-FN-CLOCK-UPDATE-POLL` | containing routine for BTC text write | `0x400d61c0..0x400d8957` | `0x1661c0..0x168957` | `0x1761c0..0x178957` | large update/poll routine; contains the BTC text producer callsite and reads `clock_btc_ticker_cache` via `FUN_400d57c0() -> runtime+0x80` |
| `BIN02-CS-CLOCK-PRICE-PRODUCER` | callsite to `FUN_400d6104` | `0x400d82a1` | `0x1682a1` | `0x1782a1` | first proven write into `global+0x4f4`; formats cached market double at cache `+0x158/+0x15c` into the BTC price slot `+0x30` |
| `BIN02-CS-CLOCK-HEADER-COLOR-DECISION` | color decision block | `0x400d82c5` | `0x1682c5` | `0x1782c5` | chooses green/red for header records from the `runtime+0x74` side path, not BTC price slot |
| `BIN02-CS-CLOCK-HEADER-COLOR-A` | callsite to `FUN_400d38cc` | `0x400d836e` | `0x16836e` | `0x17836e` | applies selected color to `global+0x4b8` |
| `BIN02-CS-CLOCK-HEADER-COLOR-B` | callsite to `FUN_400d38cc` | `0x400d8409` | `0x168409` | `0x178409` | applies selected color to `global+0x4e4` |
| `BIN02-FN-SPRICE-BUILDER` | `FUN_400f3570` | `0x400f3570` | `0x183570` | `0x193570` | `SPEC_SPrice.md`, `vtables/SPrice_FUN_400f3570.c` |
| `BIN02-FN-SPRICE-SERIES` | `FUN_400f53f8` | `0x400f53f8` | `0x1853f8` | `0x1953f8` | chart/series child-object helper |
| `BIN02-FN-SPRICE-DRAW-CB` | `FUN_400f5ba0` | `0x400f5ba0` | `0x185ba0` | `0x195ba0` | per-bar draw callback adapter |
| `BIN02-FN-SPRICE-DRAW-WORKER` | `FUN_400f54e4` | `0x400f54e4` | `0x1854e4` | `0x1954e4` | real child-object draw worker below `FUN_400f5ba0`; market/list formatting and color branches live here |
| `BIN02-FN-SPRICE-MAIN-HELPER` | `FUN_400f5bc0` | `0x400f5bc0` | `0x185bc0` | `0x195bc0` | helper called by `FUN_400f63d8`; not the `FUN_400f5ba0` adapter |
| `BIN02-FN-SPRICE-RENDER` | `FUN_400f63d8` | `0x400f63d8` | `0x1863d8` | `0x1963d8` | main chart event renderer; candle/kline behavior confirmed; separate `FUN_400f4dfc` handles the live `pricewall` mode compare and visibility toggle |
| `BIN02-FN-SPRICE-EVENT` | `FUN_400f6984` | `0x400f6984` | `0x186984` | `0x196984` | SPrice screen event adapter |
| `BIN02-FN-SPRICE-DATA-BIND` | `FUN_400f69a4` | `0x400f69a4` | `0x1869a4` | `0x1969a4` | data-fill/producer binding helper |
| `BIN02-BLOCK-SPRICE-CACHE-WRITER` | writer inside update/poll routine | `0x400d73be..0x400d7519` | `0x1673be..0x167519` | `0x1773be..0x177519` | copies `runtime+0x74` source-vector rows into inline 24-byte records at `global+0xd94` and writes count to `global+0x1244` |
| `BIN02-FN-WEATHER-DISPATCH` | `FUN_400f3a34` | `0x400f3a34` | `0x183a34` | `0x193a34` | `SPEC_Weather.md`, `vtables/Weather_FUN_400f3a34.c` |
| `BIN02-FN-WEATHER-STATIC-BUILDER` | `FUN_400f3ab4` | `0x400f3ab4` | `0x183ab4` | `0x193ab4` | missed Weather vtable builder; creates labels/geometry including slots used by cb3; needs per-widget byte map before edits |
| `BIN02-FN-CONFIG-BUILDER` | `FUN_400f2b58` | `0x400f2b58` | `0x182b58` | `0x192b58` | `SPEC_Config.md`, `vtables/Config_FUN_400f2b58.c` |
| `BIN02-FN-SS-RENDER` | `FUN_400f6ff4` | `0x400f6ff4` | `0x186ff4` | `0x196ff4` | path-based `.ss` renderer; code no-touch because LittleFS `/ss` asset route is proven |
| `BIN02-FN-SHARE-CONTAINING-CANDIDATE` | containing routine candidate | `0x400ef724` | `0x17f724` | `0x18f724` | likely routine range `0x400ef724..0x400f1d2e`; objdump alignment needs cleanup |
| `BIN02-BLOCK-SHARE-RESULT` | share result block | `0x400f15d0` | `0x1815d0` | `0x1915d0` | high-confidence accepted/block-hit/rejected block range `0x400f15d0..0x400f174c` |
| `BIN02-FN-SHARE-SEAM` | accepted literal load seam | `0x400f1620` | `0x181620` | `0x191620` | mid-block literal load, not a standalone function entry |
| `BIN02-CS-SHARE-ACCEPTED-DISPATCH` | callsite to `FUN_4012d668` | `0x400f1632` | `0x181632` | `0x191632` | accepted-share format dispatch |
| `BIN02-CS-SHARE-BLOCKHIT-DISPATCH` | callsite to `FUN_4012d668` | `0x400f16ed` | `0x1816ed` | `0x1916ed` | block-hit format dispatch |
| `BIN02-CS-SHARE-REJECTED-DISPATCH` | callsite to `FUN_4012d668` | `0x400f173d` | `0x18173d` | `0x19173d` | rejected-share format dispatch |
| `BIN02-FN-SHARE-FORMAT-DISPATCH` | `FUN_4012d668` | `0x4012d668` | `0x1bd668` | `0x1cd668` | printf-style stream writer to Arduino Serial/UART via `DAT_400d00d0`; not a UI/event hook |
| `BIN02-DAT-SERIAL0-POOL` | `DAT_400d00d0` -> `0x3ffcfe6c` | `0x400d00d0` | `0x1600d0` | `0x1700d0` | global Serial0 object pointer pool used by 114 direct loads |
| `BIN02-DAT-SERIAL0-ALT-POOL` | pointer pool containing `0x3ffcfe6c` | `0x40111284` | `0x1a1284` | `0x1b1284` | xrefs around Serial singleton setup |
| `BIN02-FN-SERIAL-INIT` | `FUN_4012d08c` | `0x4012d08c` | `0x1bd08c` | `0x1cd08c` | initializes Serial object family with UART index |
| `BIN02-FN-SERIAL-SINGLETON-INIT` | init sequence | `0x4012d40c` | `0x1bd40c` | `0x1cd40c` | initializes `0x3ffcfe6c`, `0x3ffcfe1c`, and `0x3ffcfdcc` as UART-like objects |
| `BIN02-DAT-SERIAL-VTABLE` | Serial vtable | `0x3f514f94` | `0x114f94` | `0x124f94` | vtable slot `+0x0c` targets write function `0x4012d044` |

## Runtime Update Evidence

These records are the current decoded ownership map for dynamic text/color paths.
They are not edit approvals.

| Area | Runtime owner | VM address | App offset | Evidence | Edit status |
|------|---------------|------------|------------|----------|-------------|
| Loading progress percent/bar | `FUN_400f444c` | `0x400f444c` | `0x18444c` | `irom.asm` calls percent formatter and bar setter for slots `+0x10/+0x14` | Map only |
| Loading status text | `FUN_400f44bc` | `0x400f44bc` | `0x1844bc` | `lv_label_set_text` on slot `+0x0c` | Map only |
| Loading status color | `FUN_400f44d4` | `0x400f44d4` | `0x1844d4` | RGB565-to-LVGL color path for slot `+0x0c` | Map only |
| Loading IP/address text | `FUN_400f4504` | `0x400f4504` | `0x184504` | measures new text width, resizes slot `+0x20`, then sets text | Map only |
| Config status text | `FUN_400f424c` | `0x400f424c` | `0x18424c` | `lv_label_set_text` on Config slot `+0x08` | Map only |
| Config status color | `FUN_400f4264` | `0x400f4264` | `0x184264` | color update on Config slot `+0x08` | Map only |
| Miner field text | `FUN_400f48a8` | `0x400f48a8` | `0x1848a8` | generic `lv_label_set_text` callback bound repeatedly by `FUN_400f48f0` | Map only; generic publisher callsites now mapped separately |
| Miner field color | `FUN_400f48c0` | `0x400f48c0` | `0x1848c0` | generic text-color callback bound repeatedly by `FUN_400f48f0` | Map only |
| Miner binding fan-out | `FUN_400f4b24` | `0x400f4b24` | `0x184b24` | binds callback records to concrete Miner slots after `FUN_400f34d0` panel init | Map only; not a producer |
| Parser helper, not Miner producer | `FUN_400ef24c` | `0x400ef24c` | `0x17f24c` | recursively handles parser tokens and record flags/pointers; it is called from larger runtime code but does not publish Miner UI values | No-touch |
| Miner runtime containing block | range `0x400f0858..0x400f1d2e` | `0x400f0858` | `0x180858` | contains parser setup at `ctx+0x3b0`, share-result, hashrate, telemetry formatting, Serial logging, and cleanup paths | Map only; not the callback-model publisher |
| Miner notify parser/internal job update | range `0x400f0846..0x400f13e8` | `0x400f0846` | `0x180846` | parses `mining.notify` / `params`, builds job/coinbase buffers, updates internal mining state at `*(ctx+0x78)`, and signals internal locks/events | No-touch as a UI publisher; navigation only |
| Generic text publisher | `FUN_400d6104` | `0x400d6104` | `0x166104` | mutates callback model string records and dispatches subscribers with `FUN_4011b6a4` | Map only; candidate edit sites are callsites, not the shared helper |
| Generic value/color publisher | `FUN_400d38cc` | `0x400d38cc` | `0x1638cc` | mutates callback model 32-bit value/color records and dispatches subscribers with `FUN_4011b6a4` | Map only; candidate edit sites are callsites, not the shared helper |
| Miner notify/job sequence mirror | mining.notify parser/update range | `0x400f1258` | `0x181258` | increments `ctx+0x48`, then stores it into `[ctx+0x78]+0x140`; visible through `0x400d6ad1` / slot `+0x24`; not accepted shares | Map only |
| Miner hashrate accumulator | block in runtime range | `0x400f1817` | `0x181817` | snapshots counters, computes deltas, stores combined hashrate under `state+0x158/0x15c`; inspected ranges show no bridge to slot `+0x44` | Map only |
| Miner telemetry table dispatch | callsite to `FUN_4012d668` | `0x400f1bc2` | `0x181bc2` | dispatches row format including hashrate, shares, difficulty, temperature, and RSSI fields to Serial/UART logging | Map only; not UI |
| Miner string cleanup sequence | calls to `FUN_4012da8c` | `0x400f1bd2..0x400f1c2d` | `0x181bd2..0x181c2d` | cleanup of temporary/string records after telemetry formatting | Do not hook |
| Clock compound label | `FUN_400f24ac` | `0x400f24ac` | `0x1824ac` | splits input on `:`, writes slots `+0x38/+0x3c/+0x40`, colors separator white | Not the BTC price-color branch |
| Clock timer tick | `FUN_400f1d68` | `0x400f1d68` | `0x181d68` | 500 ms timer; toggles compound slot state/color, not full updater | Map only |
| Clock BTC price text | `FUN_400f1eb0` | `0x400f1eb0` | `0x181eb0` | reads slot `+0x30`, recalculates width, and sets BTC price label text from `global+0x4f4` binding | Map only; text owner found |
| Clock BTC price producer containing routine | large update/poll routine | `0x400d61c0..0x400d8957` | `0x1661c0..0x168957` | contains the BTC price producer callsite and the upstream ticker-cache read through `FUN_400d57c0() -> runtime+0x80 clock_btc_ticker_cache` | Map only; source structs are named in `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`; no BTC-only dynamic color writer found |
| Clock BTC price producer callsite | `FUN_400d6104(global+0x4f4, stack_string)` | `0x400d82a1` | `0x1682a1` | first proven producer write into the BTC price source record after formatting cached double fields `+0x158/+0x15c` | Map only; source route is mapped, but not a function-patch approval |
| Clock generic text setter | `FUN_400f4204` | `0x400f4204` | `0x184204` | generic text setter for byte `+0x0c` path; the `global+0x520` path writes version-style text, not BTC price | Map only |
| Clock header color decision | decision block | `0x400d82c5` | `0x1682c5` | chooses green/red constants and writes `global+0x4b8/+0x4e4`, which bind to header slots `+0x04/+0x08` | Map only; not the price slot |
| Clock color helper | `FUN_400f421c` | `0x400f421c` | `0x18421c` | generic color setter; checked direct and scoped non-direct color paths do not target `global+0x4f4` | Map only; BTC price color is static-builder evidence at `0xb596`; no BTC-only dynamic color edit target was found |
| Weather static builder | `FUN_400f3ab4` | `0x400f3ab4` | `0x183ab4` | missed Weather vtable builder; creates labels including slots `+0x74/+0x78/+0x7c/+0x80` and default Weather literals such as `--%`, `--hPa`, `Feels --`, `PM2.5`, and `Updated --:--` | Needs byte-level decomp before edits |
| Weather dispatcher/on-show | `FUN_400f3a34` | `0x400f3a34` | `0x183a34` | clears state, registers cb1/cb2/cb3, tears down `+0x84` if set, and tail-calls zero-init | Map only |
| Weather text cb1 | `FUN_400f3850` | `0x400f3850` | `0x183850` | generic text set to `*param_2`; updates an existing label and does not create widgets | Map only |
| Weather text cb2 | `FUN_400f386c` | `0x400f386c` | `0x18386c` | text set to slot `+0x80`; updates an existing label and does not create widgets | Map only |
| Weather time cb3 | `FUN_400f3928` | `0x400f3928` | `0x183928` | splits `HH:MM` across slots `+0x74/+0x78/+0x7c/+0x80` and recomputes row geometry; does not create widgets | Map only |
| SPrice screen event | `FUN_400f6984` | `0x400f6984` | `0x186984` | dispatches through `FUN_400f63d8`; data fill begins near `0x400f69a4` | Map only |
| SPrice data-fill helper | `FUN_400f69a4` | `0x400f69a4` | `0x1869a4` | binding table / consumer fan-out; not cache construction and not raw `WatchCoins` parsing | Map only |
| SPrice main renderer | `FUN_400f63d8` | `0x400f63d8` | `0x1863d8` | event renderer for the `+0x24` draw surface; candle/kline record stride and color branch confirmed; `pricewall` compare is separated into `FUN_400f4dfc` | Map only; no direct renderer edit anchor |
| SPrice per-bar draw adapter | `FUN_400f5ba0` | `0x400f5ba0` | `0x185ba0` | draw callback at pointer pool `0x400d1b14`; source-recovered adapter dispatches to worker `FUN_400f54e4` | Source-recovered; map only |
| SPrice cache writer | block inside `0x400d61c0..0x400d8957` | `0x400d73be..0x400d7519` | `0x1673be..0x167519` | source-vector begin/end at `runtime+0x74 +0x24/+0x28`; count capped at 50; copies label[16], lastPrice, changePct into `global+0xd94 + i*24`; publishes count at `global+0x1244` | Map only; upstream producer now mapped, but writer edit bytes are not approved |
| SPrice child draw worker | `FUN_400f54e4` | `0x400f54e4` | `0x1854e4` | source-recovered draw worker consumes populated inline market-cache records, not raw `WatchCoins`; market/list formatting and green/red branches live here | Consumer only; producer path is separate at `0x400e2c28` / `0x400e3a80` |
| SPrice series helper | `FUN_400f53f8` | `0x400f53f8` | `0x1853f8` | chart/series child-object helper called by `FUN_400f3570`; registers `FUN_400f5ba0` on child at `screen+0x7bc` | Source-recovered; map only |
| SPrice WatchCoins tokenizer | wrapper/body/tokenizer | `0x400e7978`, body `0x400e798c`, focus `0x400e7b21..0x400e7bc9` | `0x177978`, body `0x17798c`, focus `0x177b21..0x177bc9` | source-recovered config sanitizer; scans comma-separated symbols, caps at 20, and can truncate before the 21st symbol | Evidence only; row build is in market task path |
| SPrice market config/ticker path | containing market task | `0x400db307..0x400db7d5`, callsite `0x400db511` | `0x16b307..0x16b7d5`, callsite `0x16b511` | reads `MainCoin`, `WatchCoins`, `KlineRotate`, `KlineInterval`, `PricePageMode`; calls single-symbol ticker wrapper `0x400e3e84` | Source-recovered evidence; not patch-safe |
| SPrice ticker parser/wrapper | parser/wrapper | `0x400e3b84..0x400e3e84` | `0x173b84..0x173e84` | parses `lastPrice` / `priceChangePercent` and stores fields into caller object offsets `+4/+8` | Downstream evidence; not patch-safe |
| SPrice runtime+0x74 setter | runtime market object install | `0x400ddf61` | `0x16df61` | stores static market object `0x3ffc4618` into `runtime+0x74`; task registration at `0x400ddfcd` passes that object to `(market)` task `0x400db278` | Map only; not patch-safe |
| SPrice WatchCoins batch refresh | batch API parser | `0x400e2c28` | `0x172c28` | builds Binance `/api/v3/ticker/24hr?symbols=` request from WatchCoins, appends `USDT`, parses `symbol`, `lastPrice`, `priceChangePercent`, and stores floats to node `+0x20/+0x24` | Source-recovered evidence; edit bytes not approved |
| SPrice row producer | materializer | `0x400e3a80` | `0x173a80` | clears/reserves object `+0x24/+0x28/+0x2c`, copies node symbol and floats into 24-byte rows, then sorts/trims rows | Source-recovered evidence; edit bytes not approved |
| Share containing routine candidate | range `0x400ef724..0x400f1d2e` | `0x400ef724` | `0x17f724` | likely large mining/log state routine; objdump/Ghidra alignment needs cleanup | Map only |
| Share result block | block `0x400f15d0..0x400f174c` | `0x400f15d0` | `0x1815d0` | accepted, block-hit, and rejected share logging/counter paths converge here | Map only |
| Share formatter callee | `FUN_4012d668` | `0x4012d668` | `0x1bd668` | printf-style stream writer with heap fallback and virtual write through Arduino Serial/UART object `DAT_400d00d0` | Map only; log-only, not behavior hook |
| Share accepted branch | mid-block around `0x400f1620` | `0x400f1620` | `0x181620` | accepted branch loads `#%d share accepted, %ldms`, increments `ctx+0x130`, and calls `FUN_4012d668` | Map only; no function patch |

## Generic Callback-Model Publisher Evidence

The Miner UI slot binding path and the runtime publication path are separate.
`FUN_400f4b24` subscribes labels/colors to global model records; the shared
publishers mutate those records and dispatch callbacks.

| Component | VM | App offset | Current reading |
|-----------|----|------------|-----------------|
| global model accessor | `FUN_400d53d4` | `0x1653d4` | returns/initializes the callback-model base used by Miner, Clock, and SPrice binding/update paths |
| string publisher | `FUN_400d6104` | `0x166104` | compares/assigns a string record, packages a subscriber event, then calls `FUN_4011b6a4` |
| value/color publisher | `FUN_400d38cc` | `0x1638cc` | compares/stores a 32-bit value or color record, packages a subscriber event, then calls `FUN_4011b6a4` |
| subscriber dispatcher | `FUN_4011b6a4` | `0x1ab6a4` | downstream scheduler/dispatcher for publisher events |

Direct publisher callsites found inside the `0x400d672e..0x400d6fc9`
updater window. Bound slots come from clean Xtensa objdump of
`FUN_400f4b24`; the confidence-labeled semantics live in
`hardware/recovered-assets/decompiled/source/Miner_dynamic_model.recovered.c`
and remain evidence labels, not final source names:

| Callsite | App offset | Publisher | Target record | Bound slot |
|----------|------------|-----------|---------------|------------|
| `0x400d672e` | `0x16672e` | `FUN_400d6104` | `global+0x68` | text `+0x04` |
| `0x400d68a9` | `0x1668a9` | `FUN_400d6104` | `global+0x94` | text `+0x08` |
| `0x400d68e6` | `0x1668e6` | `FUN_400d6104` | `global+0xc0` | text `+0x10` |
| `0x400d6939` | `0x166939` | `FUN_400d6104` | `global+0xec` | text `+0x18` |
| `0x400d6966` | `0x166966` | `FUN_400d38cc` | `global+0x118` | color `+0x20` |
| `0x400d699e` | `0x16699e` | `FUN_400d6104` | `global+0x2b8` | text `+0x4c` |
| `0x400d69f9` | `0x1669f9` | `FUN_400d6104` | `global+0x260` | text `+0x44` |
| `0x400d6a49` | `0x166a49` | `FUN_400d6104` | `global+0xec` | text `+0x18` |
| `0x400d6a65` | `0x166a65` | `FUN_400d38cc` | `global+0x118` | color `+0x20` |
| `0x400d6a9e` | `0x166a9e` | `FUN_400d6104` | `global+0x234` | text `+0x40` |
| `0x400d6ad1` | `0x166ad1` | `FUN_400d6104` | `global+0x154` | text `+0x24` |
| `0x400d6af8` | `0x166af8` | `FUN_400d38cc` | `global+0x180` | color `+0x30` |
| `0x400d6b35` | `0x166b35` | `FUN_400d6104` | `global+0x190` | text `+0x2c` |
| `0x400d6ba6` | `0x166ba6` | `FUN_400d38cc` | `global+0x1e8` | color `+0x38` |
| `0x400d6cca` | `0x166cca` | `FUN_400d6104` | `global+0x1bc` | text `+0x34` |
| `0x400d6d55` | `0x166d55` | `FUN_400d6104` | `global+0x1f8` | text `+0x3c` |
| `0x400d6d9b` | `0x166d9b` | `FUN_400d38cc` | `global+0x224` | color `+0x38` |
| `0x400d6e1a` | `0x166e1a` | `FUN_400d6104` | `global+0x128` | text `+0x1c` |
| `0x400d6e57` | `0x166e57` | `FUN_400d38cc` | `global+0x144` | color `+0x1c` |
| `0x400d6ea5` | `0x166ea5` | `FUN_400d6104` | `global+0x33c` | text `+0x58` |
| `0x400d6ec2` | `0x166ec2` | `FUN_400d38cc` | `global+0x358` | color `+0x58` |
| `0x400d6ee2` | `0x166ee2` | `FUN_400d6104` | `global+0x310` | text `+0x54` |
| `0x400d6efd` | `0x166efd` | `FUN_400d38cc` | `global+0x32c` | color `+0x54` |
| `0x400d6f31` | `0x166f31` | `FUN_400d6104` | `global+0x2e4` | text `+0x50` |
| `0x400d6f4a` | `0x166f4a` | `FUN_400d38cc` | `global+0x300` | color `+0x50` |
| `0x400d6fc9` | `0x166fc9` | `FUN_400d6104` | `global+0x28c` | text `+0x48` |

These callsites are not patch approvals. Before any function edit, the field
semantics, expected instruction bytes, register side effects, image validation,
and rollback plan must be documented.

Miner runtime-state evidence now mirrored in
`firmware/reconstructed/nmminer-v2-baseline/`:

| Evidence | VM/App | Source field | Current reading |
|----------|--------|--------------|-----------------|
| accepted writer | `0x400f1607` / `0x181607` | `state+0x130` | accepted share counter, high confidence |
| rejected path | `0x400f16f4` / `0x1816f4` | `state+0x134` | rejected share counter path, medium-high confidence; align exact instruction before edits |
| block-hit writer | `0x400f16a4` / `0x1816a4` | `state+0x138` | block-hit counter, high confidence |
| hashrate accumulator | `0x400f1817` / `0x181817` | `state+0x148..+0x15c` | stores deltas and combined hashrate words |
| notify/job sequence writer | `0x400f1258` / `0x181258` | `state+0x140` | mirrors incremented `ctx+0x48` into `[ctx+0x78]+0x140`; not accepted shares |
| hashrate formatter read | `0x400f1a17` / `0x181a17` | `state+0x158/+0x15c` | formats combined hashrate into runtime context at `ctx+0x1dc` |
| runtime-cache formatted stat publisher | `0x400d69f9` / `0x1669f9` | `runtime+0x80 +0x158/+0x15c` | writes `global+0x260` / slot `+0x44`; inspected ranges show runtime+0x80 is separate from mining `ctx+0x78` and no hashrate sync/bridge was found |
| visible notify/job publisher | `0x400d6ad1` / `0x166ad1` | `state+0x140` | publishes notify/job update counter to slot `+0x24`; do not alias to accepted `state+0x130` |
| task-source stack stores | `0x400d621b`, `0x400d621d` / `0x16621b`, `0x16621d` | `runtime+0x74 -> a1+36`; `runtime+0x88 -> a1+48` | proves source objects for the later Miner market/telemetry publishers |
| scan/swarm installer | `0x400de20b`, `0x400da778`, `0x400da894`, `0x400da90a` / `0x16e20b`, `0x16a778`, `0x16a894`, `0x16a90a` | `runtime+0x84` `(scan)`; `runtime+0x88` `(swarm)` | constructor path installs the swarm object and registers task `0x400dd35c` named `(swarm)` |
| swarm display writers | `0x400ddb86`, `0x400ddb8e`, `0x400ddb90`, `0x400ddb93` / `0x16db86`, `0x16db8e`, `0x16db90`, `0x16db93` | swarm object `+0x0c`, `+0x08`, `+0x10`, `+0x14` | writes the fields consumed by bottom slots `+0x54`, `+0x58`, and `+0x50`; exact float meanings remain unresolved |
| task-object telemetry publishers | `0x400d6e1a`, `0x400d6e57`, `0x400d6ea5`, `0x400d6ee2`, `0x400d6f31` | `runtime+0x74` object `+0x04/+0x08`; `runtime+0x88` swarm object `+0x08/+0x0c/+0x10` | `runtime+0x74` is SPrice/WatchCoins market controller; `runtime+0x88` is `(swarm)`; exact swarm float meanings unresolved/no-touch |
| Serial/UART telemetry | `0x400f1bc2` / `0x181bc2` | N/A | log-only route through `FUN_4012d668`, not UI |
| cleanup sequence | `0x400f1bd2..0x400f1c2d` | N/A | cleanup through `FUN_4012da8c`, not notification |

Raw regex scans for `+0x140` return many unrelated object/stack offsets across
IROM. Treat only object-proven stores in the Miner runtime context as writer
evidence; offset equality alone is not proof.

Key corrections from the current pass:

- `FUN_400f4830` does not bind a Loading `NMMiner` title. It dispatches the
  virtual builder and registers Loading progress/status/IP callbacks. Treat
  `NMMiner` xrefs as ambiguous until the exact caller is proven.
- `FUN_400f24ac` is not the BTC price green/red branch. It is a compound-label
  splitter for strings containing `:`.
- `FUN_400f4204` is not the BTC price producer. The proven BTC text producer
  writes `global+0x4f4` at `0x400d82a1`; the neighboring `global+0x520` path is
  version/date-style text for Clock byte `+0x0c`.
- `FUN_400ef24c` is not the Miner producer/state-machine hook. It is a
  JSON/parser-style helper called by larger runtime code.
- `0x400f0846..0x400f13e8` is not the Miner callback-model publisher. It parses
  `mining.notify` / `params`, builds job buffers, updates internal mining state
  at `*(ctx+0x78)`, and signals internal state locks/events.
- Miner callback-model publication is generic through `FUN_400d6104` and
  `FUN_400d38cc`, with direct Miner-bound callsites in `0x400d672e..0x400d6fc9`.
- Direct publisher `0x400d69f9` writes `global+0x260` / slot `+0x44`, but its
  input is the update loop's `runtime+0x80` stack object at `+0x158/+0x15c`
  (the same object family named by the Clock BTC closure). The inspected
  ranges show `runtime+0x80` is separate from mining `ctx+0x78`, and no
  hashrate sync/bridge was found. Keep the mining hashrate accumulator evidence
  at `0x400f1817` / `0x400f1a17` separate from slot `+0x44`.
- `FUN_400f63d8` confirms kline/candle rendering. `pricewall` is a live mode
  compare through `FUN_400f4dfc`, but not a direct renderer edit anchor.
  `WatchCoins` is still config/API input; only the fixed DROM default literal
  is local-patcher eligible, while upstream function/cap/callsite edits remain
  blocked or no-go.
- `FUN_4012d668` is a printf-style Serial/UART stream writer. It is not a
  UI/event dispatcher, so share-seam behavior edits are blocked there.
- `FUN_4012da8c` is string cleanup/reset logic, not a UI notification path.
- Config AP/IP strings are HTTP/captive-portal strings, not LVGL label targets.

## Producer/Caller Evidence From 2026-06-19 Pass

Share-accept branch evidence:

| Item | VM address | App offset | Current reading |
|------|------------|------------|-----------------|
| containing routine candidate | `0x400ef724..0x400f1d2e` | `0x17f724..0x181d2e` | likely large mining/log state routine; previous code returns at `0x400ef720`, clean next prologue at `0x400f1d30` |
| accepted branch/state block | `0x400f15d0` | `0x1815d0` | surrounding block for accepted-share handling |
| accepted counter store | `0x400f1607` | `0x181607` | increments/stores accepted count at `ctx + 0x130` |
| accepted literal load seam | `0x400f1620` | `0x181620` | mid-block seam, not a proven standalone function entry |
| accepted dispatch callsite | `0x400f1632` | `0x181632` | calls `FUN_4012d668(a3, fmt, accepted_count, elapsed_ms)` |
| block-hit dispatch callsite | `0x400f16ed` | `0x1816ed` | adjacent status path; preserve semantics |
| rejected dispatch callsite | `0x400f173d` | `0x18173d` | adjacent rejected-share path; rejected count at `ctx + 0x134` |
| formatter/log Serial callee | `0x4012d668` | `0x1bd668` | printf-style Arduino Serial/UART stream writer via `DAT_400d00d0`; not a UI/event sink |

Miner binding path:

| Item | Address | Current reading |
|------|---------|-----------------|
| container/style init | `FUN_400f34d0` | creates panel and calls the bind fan-out |
| bind fan-out callsite | `0x400f3551` | `FUN_400f34d0` -> `FUN_400f4b24` |
| bind fan-out | `FUN_400f4b24` | binds callback records to slots; not a telemetry formatter |
| generic text setter | `FUN_400f48a8` | calls `lv_label_set_text` |
| generic color setter | `FUN_400f48c0` | converts RGB565/LVGL color and sets text color |

Confirmed Miner text slot binding callsites in `FUN_400f4b24`:

| Slot | Text record | Callsite VM |
|------|-------------|-------------|
| `+0x04` | `global+0x68` | `0x400f4b3a` |
| `+0x08` | `global+0x94` | `0x400f4b53` |
| `+0x10` | `global+0xc0` | `0x400f4b6a` |
| `+0x18` | `global+0xec` | `0x400f4b83` |
| `+0x1c` | `global+0x128` | `0x400f4bab` |
| `+0x24` | `global+0x154` | `0x400f4bc6` |
| `+0x2c` | `global+0x190` | `0x400f4bee` |
| `+0x34` | `global+0x1bc` | `0x400f4c09` |
| `+0x3c` | `global+0x1f8` | `0x400f4c31` |
| `+0x40` | `global+0x234` | `0x400f4c59` |
| `+0x44` | `global+0x260` | `0x400f4c74` |
| `+0x48` | `global+0x28c` | `0x400f4c8f` |
| `+0x4c` | `global+0x2b8` | `0x400f4caa` |
| `+0x50` | `global+0x2e4` | `0x400f4cc5` |
| `+0x54` | `global+0x310` | `0x400f4cde` |
| `+0x58` | `global+0x33c` | `0x400f4cf9` |

Candidate Miner producer/caller blocks inside the runtime range
`0x400f0858..0x400f1d2e`:

| Item | VM address | App offset | Current reading |
|------|------------|------------|-----------------|
| share-result block | `0x400f15d0` | `0x1815d0` | accepted/rejected/block-hit counter/log path |
| notify/job sequence seed/increment | `0x400ef844`, `0x400ef9b8`, `0x400f0370`, `0x400f076f`, `0x400f1258` | `0x17f844`, `0x17f9b8`, `0x180370`, `0x18076f`, `0x181258` | resets/seeds/increments `state+0x140`; decisive mirror copies incremented `ctx+0x48` into `[ctx+0x78]+0x140` |
| hashrate accumulator | `0x400f1817` | `0x181817` | snapshots counters, computes deltas, stores `state+0x148..+0x15c`; inspected ranges do not bridge this to slot `+0x44` |
| status/table formatter | `0x400f1949` | `0x181949` | formats uptime/status and telemetry table values |
| hashrate compact-string consumer | `0x400f1a17` | `0x181a17` | reads combined hashrate `state+0x158/0x15c` and calls `FUN_400f9200` into `ctx+0x1dc`; direct `0x400d69f9` UI publisher remains separate |
| telemetry row dispatch | `0x400f1bc2` | `0x181bc2` | calls `FUN_4012d668` with the full telemetry table row format for Serial/UART logging |
| string cleanup sequence | `0x400f1bd2..0x400f1c2d` | `0x181bd2..0x181c2d` | calls `FUN_4012da8c` after formatting; cleanup/reset, not notification |
| parser helper call | `0x400f090d` | `0x18090d` | calls `FUN_400ef24c` after setting up a record at `ctx+0x3b0`; parser setup, not UI publication |

Clock and SPrice status:

| Area | Current reading | Remaining gap |
|------|-----------------|---------------|
| Clock price text | `runtime+0x80 clock_btc_ticker_cache -> cache +0x158/+0x15c btc_usd_last_price -> global+0x4f4 -> FUN_400f1eb0 -> slot +0x30`; producer callsite `0x400d82a1` is inside `0x400d61c0..0x400d8957` | closed as source-equivalent route evidence; no function-patch approval |
| Clock generic setter | `FUN_400f4204` is not BTC price; `global+0x520` updates Clock byte `+0x0c` version/date-style text | none for BTC edits; keep only as non-price reference |
| Clock color | `0x400d82c5` chooses green/red from the `runtime+0x74` side path for `global+0x4b8/+0x4e4`, which bind to header slots `+0x04/+0x08`; direct and scoped non-direct color paths avoid `global+0x4f4` | BTC price color is static `0xb596` builder evidence; no BTC-only runtime color edit target was found |
| SPrice renderer/cache | `FUN_400f63d8` confirms kline/candle behavior; `FUN_400f5ba0` dispatches child draws to `FUN_400f54e4`; `FUN_400f69a4` is binding/fan-out; `FUN_400f4dfc` performs the live `pricewall` mode compare and visibility toggle; `0x400d73be..0x400d7519` writes the inline market cache from `runtime+0x74` to `global+0xd94/global+0x1244`; `0x400e7978` / `0x400e798c` / `0x400e7b21..0x400e7bc9`, market config path `0x400db307..0x400db7d5`, batch parser `0x400e2c28`, row producer `0x400e3a80`, and ticker wrapper/parser `0x400e3e84` / `0x400e3b84` are decoded | source-vector builder is proven; instruction-level edit bytes and rollback are still absent |

## Candidate Edit Classes

| Class | Risk | Notes |
|-------|------|-------|
| Fixed-length DROM literal replacement | Low to medium | Safest first target if replacement is same length or shorter with null padding and no shared-string side effects. |
| LittleFS web/static asset edit | Medium | Add/replace mechanics are locally proven. Must rebuild `spiffs@0x390000` size `0x60000`; preserve LittleFS geometry (`4096 x 96`), decode every `.ss`, and keep generated image out of git. |
| Existing function branch/literal tweak | High | Requires exact instruction encoding, relocation awareness, and checksum/hash refresh. |
| New code injection / trampoline | Very high | Needs code cave or relocation plan; not allowed until function boundaries and image validation are proven. |

## Local Patch Tool

`scripts/nmminer_patch_literal.py` is the first BIN-04 tool. It:

- accepts only mapped fixed-length literal target IDs;
- refuses blocked records such as `BIN02-SEAM-SHARE-ACCEPT` and the overlapping
  `kH/s`/`H/s` unit record;
- verifies the original bytes before patching;
- null-pads shorter ASCII replacements;
- refreshes the ESP32 image checksum and validation hash in the existing image
  layout;
- optionally runs `python3 -m esptool image-info` on the generated app.

Example local-only command:

```bash
python3 scripts/nmminer_patch_literal.py \
  --target-id BIN02-LIT-LOAD-TAGLINE \
  --replacement "NNMiner Base" \
  --output hardware/generated-firmware/nmminer-v2.0.02-st7789-tagline.bin \
  --validate
```

`hardware/generated-firmware/` is gitignored. A test patch written to `/tmp`
validated with `esptool image-info`: checksum `0x07`, validation hash
`13eb5b997b056d230e5becef8abe2728e69c632e018504012254fb9247ee8f92`.

## Patch Preconditions

Before any byte edit is attempted:

1. Map virtual address to app file offset with segment ownership recorded.
2. Confirm the target is not part of license/activation, credential, wallet, NVS,
   or server trust logic.
3. Confirm replacement length and encoding constraints.
4. Recompute or refresh ESP32 image checksum/hash as required.
5. Run `python3 -m esptool image-info <patched-app.bin>` and record valid
   checksum/hash output.
6. Confirm the app still fits the `factory@0x10000` partition length
   `0x380000`.

## No-Touch Zones

- License/activation verification and related server trust paths.
- Wi-Fi credentials, wallet values, device license values, NVS secrets.
- NVS sectors and raw full-flash backups except for read-only provenance checks.
- Any patch requiring redistribution of a modified official binary.

## Open Decompilation Gaps

- Keep the normalized Clock corrections in force: `SPEC_Clock.md` and
  `vtables/Clock_FUN_400f1f34.c` identify the builder, `FUN_400f24ac` is not
  the BTC price-color branch, and `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`
  closes the current BTC source/color evidence.
- Finish producer/caller proof before any function edit: keep proven Miner
  runtime fields (`state+0x130`, `+0x134`, `+0x138`, notify/job counter
  `+0x140`, and hashrate `+0x158/+0x15c`) separate from the proven task-object
  source split (`runtime+0x74` for the Miner market pair, `runtime+0x88` for
  the `(swarm)` bottom count/metric trio) and from the separate `runtime+0x80`
  publisher at `0x400d69f9` / slot `+0x44`; the remaining telemetry gap is the
  exact swarm float meanings and edit safety. Keep the Clock BTC
  closure in force from
  `07-CLOCK-BTC-SOURCE-COLOR-CLOSURE.md`;
  keep `WatchCoins` edits blocked until the now-decoded `0x400e2c28` batch
  parser and `0x400e3a80` row producer have exact patch bytes, side effects,
  checksum/hash refresh, and rollback documented; map `FUN_400f3ab4` at
  per-widget byte level before Weather layout edits; and keep screensaver
  customization on the proven LittleFS `.ss` add/replace route.
  `FUN_4012da8c`, `FUN_4012d668`, and `FUN_400f6ff4` are not hook points.
- Treat Config AP SSID/IP strings as HTTP/HTML until proven LVGL-bound. They are
  not display-label targets.
- Treat Loading `NMMiner` title binding as unproven. `FUN_400f4830` is now
  identified as callback registration, not the title builder.

## Next Update

Continue producer/caller decompilation upstream of the mapped setters before any
function edit. If a visible proof-of-edit is needed first, use only an approved
fixed-length literal target and generate a local-only app image with
`scripts/nmminer_patch_literal.py`; do not flash until the Phase 7 write gate is
met.
