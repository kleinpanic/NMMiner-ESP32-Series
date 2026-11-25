# Phase 7 Clock BTC Source/Color Closure

**Updated:** 2026-06-20
**Target:** official NMMiner `v2.0.02` `esp32-2432s028r-st7789`
**Scope:** Clock BTC text source, recovered runtime struct names, and BTC price
color decision.

## Conclusion

The bottom-right Clock BTC price text route is closed for the current
decompilation pass:

```c
RuntimeServicesRecovered::clock_btc_ticker_cache      // runtime +0x80
ClockBtcTickerCacheRecovered::btc_usd_last_price      // cache +0x158/+0x15c
ClockUiModelRecovered::clock_btc_price_text_source    // global +0x4f4
ClockRecovered::btc_price_label                       // Clock slot +0x30
```

The same `runtime+0x80 +0x158/+0x15c` object is also read by the Miner
publisher at `0x400d69f9` for slot `+0x44`. Inspected ranges show no
sync/bridge from the mining `ctx+0x78` hashrate object into `runtime+0x80`, so
that Miner slot remains cache-derived/no-touch rather than a proven mining
hashrate display.

`runtime+0x74` is not the BTC price source. It is the SPrice/WatchCoins market
controller used by the SPrice row producer/cache writer and by the nearby Clock
header green/red branch.

No BTC-price-specific dynamic color writer was found in the checked Clock scope.
The BTC price label is seeded by the Clock builder with static RGB565 `0xb596`.
The dynamic green/red writes in the same update routine publish to
`global+0x4b8` and `global+0x4e4`, which bind to header slots `+0x04` and
`+0x08`, not BTC slot `+0x30`.

## Recovered Layout Names

The recovered C artifact uses 32-bit ESP32 pointer fields so host syntax checks
do not move target offsets:

```c
typedef struct RuntimeServicesRecovered {
    uint8_t reserved_0x00[0x74];
    target_ptr32_t sprice_market_controller;     // +0x74
    target_ptr32_t unknown_0x78;                 // +0x78
    target_ptr32_t unknown_0x7c;                 // +0x7c
    target_ptr32_t clock_btc_ticker_cache;       // +0x80
    target_ptr32_t scan_controller;              // +0x84
    target_ptr32_t swarm_controller;             // +0x88
} RuntimeServicesRecovered;

typedef struct ClockBtcTickerCacheRecovered {
    uint8_t reserved_0x000[0x158];
    uint32_t btc_usd_last_price_low;             // +0x158
    uint32_t btc_usd_last_price_high;            // +0x15c
} ClockBtcTickerCacheRecovered;
```

## Evidence Table

| VM | App offset | Evidence | Recovered meaning |
|----|------------|----------|-------------------|
| `0x400d57c0` | `0x1657c0` | Runtime singleton accessor/init clears the runtime pointer slots. | Owns distinct runtime service fields including `+0x74`, `+0x80`, `+0x84`, and `+0x88`. |
| `0x400dac58` | `0x16ac58` | Stores a static object pointer into `runtime+0x80`. | Installs `clock_btc_ticker_cache`. |
| `0x400daac3` / `0x400daac9` | `0x16aac3` / `0x16aac9` | Zero-inits object fields `+0x158/+0x15c`. | Initializes `btc_usd_last_price` double words. |
| `0x400d620a` / `0x400d6219` | `0x16620a` / `0x166219` | Loads `runtime+0x80`, stores stack `a1+40`. | Carries the Clock BTC ticker cache into the update block. |
| `0x400d6213` / `0x400d621b` | `0x166213` / `0x16621b` | Loads `runtime+0x74`, stores stack `a1+36`. | Carries the SPrice/WatchCoins market controller and nearby header-color source. |
| `0x400d6216` / `0x400d621d` | `0x166216` / `0x16621d` | Loads `runtime+0x88`, stores stack `a1+48`. | Swarm controller source object; no-touch for Clock BTC route. |
| `0x400d81dc` | `0x1681dc` | Loads stack `a1+40`. | Re-enters the Clock BTC ticker cache before formatting. |
| `0x400d81e4` / `0x400d81ed` | `0x1681e4` / `0x1681ed` | Reads cache `+0x158/+0x15c`. | Direct double input to the BTC price string. |
| `0x400d82a1` | `0x1682a1` | Calls `FUN_400d6104(global+0x4f4, stack_string)`. | Publishes BTC price text only. |
| `0x400f1eb0` | `0x181eb0` | Bound subscriber for `global+0x4f4`. | Updates Clock slot `+0x30` BTC price label text/layout. |
| `0x400d82c5` | `0x1682c5` | Green/red decision reads the `runtime+0x74` side path. | Header-color path, not BTC price cache. |
| `0x400d836e` | `0x16836e` | Calls `FUN_400d38cc(global+0x4b8, color)`. | Header slot `+0x04` color. |
| `0x400d8409` | `0x168409` | Calls `FUN_400d38cc(global+0x4e4, color)`. | Header slot `+0x08` color. |
| `0x400f421c` | `0x18421c` | Generic Clock color subscriber. | Checked Clock color publishers do not target `global+0x4f4` or slot `+0x30`. |

## Patch Status

- Text source route: mapped for reconstruction.
- BTC price color: closed as static-builder evidence, not an isolated runtime
  edit target.
- `runtime+0x80` structural edits: no-touch until full object constructor,
  writer side effects, expected bytes, checksum/hash refresh, and rollback are
  documented.
- Header green/red edits: no-touch for BTC price work because they target header
  slots only.
- No firmware write was performed for this closure.
