# Phase 7 SPrice Patch Safety

**Updated:** 2026-06-20T02:03:02Z
**Baseline:** official NMMiner `v2.0.02` `esp32-2432s028r-st7789`
**Status:** SPrice `WatchCoins` source-vector chain is decoded. One default
literal is now local-patch-tool eligible; instruction/function edits remain
blocked.

## Scope

This document converts the decoded SPrice `WatchCoins` route into patch-safety
records for BIN-02/BIN-03/BIN-04. It is not a firmware write plan.

Inputs:

- App image: `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin`
- ELF: `hardware/recovered-assets/nmminer-app.elf`
- Recovered source-equivalent evidence:
  `hardware/recovered-assets/decompiled/source/SPrice_watchcoins_config.recovered.c`
  and
  `hardware/recovered-assets/decompiled/source/SPrice_market_runtime.recovered.c`
- Address mapper: `scripts/nmminer_addr_map.py`
- Local literal patcher: `scripts/nmminer_patch_literal.py`

No flash is allowed from this artifact. BIN-05 still requires a fresh full
4 MB backup, image validation, `verify_flash`, hard power-cycle, and a restore
command.

## Proven Runtime Chain

The SPrice visible market rows do not come directly from the screen renderer.
The decoded route is:

1. Static market object `0x3ffc4618` is installed into `runtime+0x74` at
   `0x400ddf61`.
2. The `(market)` task at `0x400db278` reads config keys including
   `MainCoin`, `WatchCoins`, `KlineRotate`, `KlineInterval`, and
   `PricePageMode`.
3. Batch refresh/parser `0x400e2c28` builds the Binance
   `/api/v3/ticker/24hr?symbols=` request from `WatchCoins`, appends `USDT`,
   parses `symbol`, `lastPrice`, and `priceChangePercent`, then stores floats
   into node fields.
4. Row producer `0x400e3a80` clears/reserves object row vector
   `+0x24/+0x28/+0x2c`, copies symbol and float fields into 24-byte rows, then
   sorts by the row `+16` value/price float descending and trims rows to the
   `0x180` byte / 16-row rendered window. Equal-price and NaN ordering behavior
   remains unresolved.
5. Cache writer `0x400d73be..0x400d7519` copies rows from `runtime+0x74` into
   `global+0xd94` and publishes row count at `global+0x1244`.
6. SPrice renderers consume the inline cache. `pricewall` is a mode/visibility
   compare, not the upstream producer and not a direct edit anchor.

## Patch Records

| Target id | Class | VM address | App offset | Flash offset | Current bytes/value | Status |
|-----------|-------|------------|------------|--------------|---------------------|--------|
| `BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT` | DROM literal | `0x3f400cda` | `0x000cda` | `0x010cda` | `BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO` | Approved for local literal patch tooling only |
| `BIN02-LIT-SPRICE-USDT-SUFFIX` | DROM literal | `0x3f4008fb` | `0x0008fb` | `0x0108fb` | `USDT` | Blocked/no-touch |
| `BIN02-LIT-SPRICE-PRICE-PAGE-KLINE` | DROM literal | `0x3f400d31` | `0x000d31` | `0x010d31` | `kline` | Blocked/no-touch |
| `BIN02-LIT-SPRICE-PRICEWALL` | DROM literal | `0x3f4e4287` | `0x0e4287` | `0x0f4287` | `pricewall` | Existing target string only |
| `BIN02-LIT-SPRICE-BINANCE-BATCH-URL` | DROM literal | `0x3f47222c` | `0x07222c` | `0x08222c` | `http://data-api.binance.vision:80/api/v3/ticker/24hr?symbols=` | Blocked/no-touch |
| `BIN02-PTR-SPRICE-PRICE-PAGE-DEFAULT` | IROM pointer literal | `0x400d0914` | `0x160914` | `0x170914` | bytes `31 0d 40 3f`, pointer to `kline` | Blocked |
| `BIN02-INS-SPRICE-CACHE-WRITER-PUBLISH-CAP` | IROM instruction | `0x400d7428` | `0x167428` | `0x177428` | bytes `3c 24`, `movi.n a4, 50` | Blocked except lower-cap research |
| `BIN02-INS-SPRICE-WATCHCOINS-TOKEN-CAP` | IROM instruction | `0x400e7b65` | `0x177b65` | `0x187b65` | bytes `1c 48`, `movi.n a8, 20` | Blocked |
| `BIN02-INS-SPRICE-ROW-TRIM-CAP-A` | IROM instruction | `0x400e390a` | `0x17390a` | `0x18390a` | bytes `72 a1 80`, `movi a7, 0x180` | Blocked |
| `BIN02-INS-SPRICE-ROW-TRIM-CAP-B` | IROM instruction | `0x400e3b28` | `0x173b28` | `0x183b28` | bytes `22 a1 80`, `movi a2, 0x180` | Blocked |
| `BIN02-CS-SPRICE-BATCH-PARSER-CALL` | IROM callsite | `0x400dbbc9` | `0x16bbc9` | `0x17bbc9` | bytes `e5 05 07`, `call8 0x400e2c28` | Blocked/no-go |
| `BIN02-CS-SPRICE-ROW-PRODUCER-CALL` | IROM callsite | `0x400dbbd1` | `0x16bbd1` | `0x17bbd1` | bytes `e5 ea 07`, `call8 0x400e3a80` | Blocked/no-go |
| `BIN02-INS-SPRICE-RUNTIME74-SETTER` | IROM instruction | `0x400ddf61` | `0x16df61` | `0x17df61` | bytes `42 62 1d`, `s32i a4, a2, 116` | No-go |
| `BIN02-FN-SPRICE-BATCH-PARSER` | IROM function | `0x400e2c28` | `0x172c28` | `0x182c28` | parser/function body | Navigation only |
| `BIN02-FN-SPRICE-ROW-PRODUCER` | IROM function | `0x400e3a80` | `0x173a80` | `0x183a80` | row-producer body | Navigation only |

## Approved Local Literal Target

`BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT` is the only SPrice record approved for
the local literal patch tool.

Constraints:

- Visible string length is 39 bytes.
- The original terminating NUL remains at app offset `0x000d01`; the next key
  `KlineRotate` starts at `0x000d02`.
- Replacement must be ASCII, same length or shorter, and null-padded by the
  patch tool.
- Do not include `USDT` in replacement symbols; the batch parser appends the
  `USDT` suffix itself.
- The edit affects the default config literal only. Existing NVS/config values
  may override it at runtime.
- This is a generated local app-image edit only. It does not flash hardware and
  does not bypass any license, Wi-Fi, wallet, NVS, or server-trust path.

Evidence bytes:

```text
00000cda: 42 54 43 2c 45 54 48 2c 42 4e 42 2c 4c 54 43 2c
00000cea: 45 4f 53 2c 45 54 43 2c 58 52 50 2c 42 43 48 2c
00000cfa: 5a 45 43 2c 4e 45 4f 00 4b 6c 69 6e 65 52 6f 74
```

Local patch command shape:

```bash
python3 scripts/nmminer_patch_literal.py \
  --target-id BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT \
  --replacement "BTC,ETH,BNB,LTC,SOL,DOGE,ADA,XRP,BCH" \
  --output hardware/generated-firmware/nmminer-v2.0.02-st7789-watchcoins.bin \
  --validate
```

The example replacement is only a byte-shape example. Choose the actual
watchlist deliberately before generating an artifact.

## Blocked SPrice Byte Classes

`BIN02-LIT-SPRICE-USDT-SUFFIX` is blocked because the batch parser appends it to
each requested symbol. Changing it changes every market pair and may break the
Binance endpoint, parser assumptions, and display semantics. The slot is only
4 visible bytes.

`BIN02-LIT-SPRICE-BINANCE-BATCH-URL` is blocked because it is network endpoint
and server-trust behavior. Even same-length URL replacement would need protocol,
TLS/HTTP, response-shape, parser, and trust review. It is not needed for default
watchlist customization.

`BIN02-LIT-SPRICE-PRICE-PAGE-KLINE` is blocked as a direct string overwrite
because `pricewall` does not fit in the five-byte `kline` slot. The existing
`pricewall` literal is only evidence for the runtime mode value.

`BIN02-PTR-SPRICE-PRICE-PAGE-DEFAULT` is blocked because pointer replacement
would change the default page mode through a shared config/default setup pointer
surface. It may be viable later as a deliberate default-mode change, but it is
not isolated to one renderer branch and saved config can still override it.

`BIN02-INS-SPRICE-CACHE-WRITER-PUBLISH-CAP` can only be researched in the
lower-cap direction. Raising it risks fixed global cache storage and renderer
assumptions. Lowering it would limit rows copied to the global SPrice cache but
would not reduce upstream network/parser work.

`BIN02-INS-SPRICE-WATCHCOINS-TOKEN-CAP` is blocked because it changes parser
control flow. The current cap is 20 tokens, while the row trim path keeps only
16 rows (`0x180 / 24`). Raising the tokenizer cap without row/UI/memory proof
can silently discard rows or stress unverified allocation paths.

`BIN02-INS-SPRICE-ROW-TRIM-CAP-A` and
`BIN02-INS-SPRICE-ROW-TRIM-CAP-B` are blocked because both encode the 0x180-byte
row trim bound. Changing one without the other is inconsistent, and changing
both changes the maximum rendered row set, heap/vector behavior, and screen
layout assumptions.

`0x400e2c28` and `0x400e3a80` stay navigation-only. Behavioral changes in those
functions should be expressed first in recovered source-equivalent form, then
validated against exact instruction bytes and side effects before any binary
patch is considered.

The market task callsites to `0x400e2c28` and `0x400e3a80`, the `runtime+0x74`
setter, the row producer entry, and the cache writer entry are no-go byte edits
for this pass. Skipping or redirecting them can leave stale rows, half-updated
vectors, or inconsistent task state.

## Validation Commands

Address mapping:

```bash
python3 scripts/nmminer_addr_map.py \
  0x3f400cda 0x3f4008fb 0x3f47222c \
  0x3f400d31 0x400d0914 0x3f4e4287 \
  0x400d7428 0x400dbbc9 0x400dbbd1 0x400ddf61 \
  0x400e7b65 0x400e390a 0x400e3b28 \
  0x400e2c28 0x400e3a80
```

Literal byte checks:

```bash
xxd -g1 -s 0xcda -l 80 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
xxd -g1 -s 0x8fb -l 64 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
xxd -g1 -s 0x7222c -l 128 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
xxd -g1 -s 0xd31 -l 32 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
xxd -g1 -s 0x160914 -l 16 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
xxd -g1 -s 0xe4287 -l 24 firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin
```

Instruction byte checks:

```bash
/home/broklein/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump \
  -d -s --start-address=0x400e7b50 --stop-address=0x400e7bb0 \
  hardware/recovered-assets/nmminer-app.elf

/home/broklein/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump \
  -d -s --start-address=0x400e3904 --stop-address=0x400e3b40 \
  hardware/recovered-assets/nmminer-app.elf

/home/broklein/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump \
  -d -s --start-address=0x400d7400 --stop-address=0x400d7440 \
  hardware/recovered-assets/nmminer-app.elf

/home/broklein/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump \
  -d -s --start-address=0x400dbbb8 --stop-address=0x400dbbe0 \
  hardware/recovered-assets/nmminer-app.elf
```

Patch-tool smoke check:

```bash
python3 scripts/nmminer_patch_literal.py --list-targets
python3 -m py_compile scripts/nmminer_patch_literal.py
```

## Rollback

For generated local app images, rollback is deleting the generated image and
using the unchanged official app at
`firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin`.

For any future hardware write, rollback requires the latest fresh 4 MB backup
taken immediately before that write. Existing historical backups do not satisfy
the next-write gate.
