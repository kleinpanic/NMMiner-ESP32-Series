# Official LittleFS `.ss` Asset Replacement Proof

**Updated:** 2026-06-20
**Route:** official NMMiner `v2.0.02` `esp32-2432s028r-st7789` binary
decompilation/custom-edit.
**Status:** local proof complete; no hardware write performed.

## Scope

This artifact closes the Phase 7 proof item for screensaver customization
mechanics on the official baseline. It does not approve a firmware flash and it
does not patch `FUN_400f6ff4`.

The approved customization route for screensavers is LittleFS asset replacement
under `/ss`, because the official renderer is path based:

- `hardware/recovered-assets/decompiled/ss_render.c`
- `FUN_400f6ff4(char *path, uint w, uint h)`
- render callsite passes the selected asset path plus display width/height
- `.ss` header read is `SN`, little-endian width/height, flag `0x01`, HS params
  `0xA4`

## Image Identity

The official `v2.0.02` ST7789 bundle remains the baseline:

| Image | SHA-256 | Size | Role |
|-------|---------|------|------|
| `firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin` | `700681e91194ca233dfdef62725541f917270604279253c4233e5551bf93a2cf` | `393216` | official bundle LittleFS |
| `hardware/recovered-assets/spiffs.bin` | `44adf024f9b3291d55e3aa28d58536bca201cfb4249d95aa375dd32d9693144d` | `393216` | observed/recovered expanded asset image |
| `hardware/recovered-assets/original-20260601/spiffs.bin` | `cb87c25b046a86d50f6a929f954cf98d1305f8a3b8ab301dafad5e0b19d23737` | `65536` | historical `v1.8.28` image; not active baseline |

Do not collapse the first two rows. They are both valid LittleFS images with the
same `0x60000` size, but they are not byte-identical and their `/ss` contents
differ.

Official ST7789 flash layout for the bundle:

| Partition | Offset | Length | Notes |
|-----------|--------|--------|-------|
| `factory` app | `0x10000` | `0x380000` | app image |
| `spiffs` label | `0x390000` | `0x60000` | actually LittleFS |

The later custom fork used a different decoded-LittleFS offset (`0x310000`).
That is recovery/fork context only and must not be used for official bundle
replacement records.

## LittleFS Listing Proof

Read-only listing command:

```sh
python3 -m littlefs list --verbose --block-size 4096 \
  firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin
```

Result:

```text
Block Size: 4096
Image Size: 393216 / 0x60000
Block Count: 96
/ss
/apidoc.html.gz
/index.html.gz
/ss/saver_320_240_001.ss
/ss/saver_320_240_002.ss
```

Official bundled `.ss` assets:

| Path | Bytes | Header |
|------|-------|--------|
| `/ss/saver_320_240_001.ss` | `69975` | `53 4e 40 01 f0 00 01 a4` |
| `/ss/saver_320_240_002.ss` | `83057` | `53 4e 40 01 f0 00 01 a4` |

The recovered expanded image lists 17 files:

```sh
python3 -m littlefs list --verbose --block-size 4096 \
  hardware/recovered-assets/spiffs.bin
```

It contains `/ss/saver_320_240_002.ss` through
`/ss/saver_320_240_018.ss`. These assets all carry the same canonical header
and decode to `320x240`, but they are not the same image as the official bundled
`littlefs.bin`.

## Decode Proof

Commands:

```sh
python3 -m littlefs extract --block-size 4096 \
  firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin \
  /tmp/nmminer-official-littlefs-extract

python3 scripts/nmminer_ss_decode.py \
  /tmp/nmminer-official-littlefs-extract/ss \
  /tmp/nmminer-official-ss-decode-check
```

Result: both official `.ss` files decoded to PNG.

Recovered expanded image decode command:

```sh
python3 -m littlefs extract --block-size 4096 \
  hardware/recovered-assets/spiffs.bin \
  /tmp/nmminer-recovered-spiffs-extract

python3 scripts/nmminer_ss_decode.py \
  /tmp/nmminer-recovered-spiffs-extract/ss \
  /tmp/nmminer-recovered-ss-decode-check
```

Result: all 17 recovered expanded `.ss` files decoded to PNG.

## Rebuild Proof

Add proof, using the official bundle as the source tree and a valid recovered
`.ss` file as a donor asset:

```sh
cp -a /tmp/nmminer-official-littlefs-extract /tmp/nmminer-lfs-add-proof
cp /tmp/nmminer-recovered-spiffs-extract/ss/saver_320_240_003.ss \
  /tmp/nmminer-lfs-add-proof/ss/saver_320_240_003.ss

python3 -m littlefs create --block-size 4096 --fs-size 393216 \
  /tmp/nmminer-lfs-add-proof \
  /tmp/nmminer-lfs-add-proof.bin

python3 -m littlefs list --block-size 4096 /tmp/nmminer-lfs-add-proof.bin
```

Result:

```text
/ss
/apidoc.html.gz
/index.html.gz
/ss/saver_320_240_001.ss
/ss/saver_320_240_002.ss
/ss/saver_320_240_003.ss
```

The rebuilt image is exactly `393216` bytes. It was extracted again and all
three serialized `.ss` files decoded successfully.

Replace proof:

```sh
cp -a /tmp/nmminer-official-littlefs-extract /tmp/nmminer-lfs-replace-proof
cp /tmp/nmminer-recovered-spiffs-extract/ss/saver_320_240_003.ss \
  /tmp/nmminer-lfs-replace-proof/ss/saver_320_240_001.ss

python3 -m littlefs create --block-size 4096 --fs-size 393216 \
  /tmp/nmminer-lfs-replace-proof \
  /tmp/nmminer-lfs-replace-proof.bin
```

Result:

```text
/ss
/apidoc.html.gz
/index.html.gz
/ss/saver_320_240_001.ss
/ss/saver_320_240_002.ss
```

The replacement changed `saver_320_240_001.ss` from SHA-256
`0e02f3f3650d69c195c9aa4768473f0367ead4ce488439e6d64e207fa3184784` to
`65dba62211574d2710d8aab6e30b8d3445a9630cb3e90158eab6a0b64c8a5446`.
The rebuilt image is exactly `393216` bytes, extracts cleanly, and both
serialized `.ss` files decode successfully.

## Patch Classification

| Item | Status | Reason |
|------|--------|--------|
| `FUN_400f6ff4` / `ss_render` code | No-touch | Path-based renderer already supports external assets; code edits add risk without need. |
| `.ss` header constants | No-touch | Stock assets and generated valid assets use canonical `53 4e 40 01 f0 00 01 a4`. |
| LittleFS `/ss/saver_320_240_NNN.ss` add/replace | Locally proven | Rebuilt images preserve geometry, list correctly, extract, and decode. |
| Official LittleFS flash write | Gated | Requires fresh 4 MB backup, explicit write gate, `verify_flash`, hard power-cycle, and restore command. |

## Future Write Gate

If this route is ever flashed, write the whole LittleFS partition image to the
official bundle offset `0x390000`, not the custom fork's `0x310000` offset.

Before any write:

1. Take a fresh full 4 MB backup.
2. Keep the modified LittleFS image local-only and gitignored.
3. Verify image size is exactly `393216`.
4. Re-list the image with `python3 -m littlefs list --block-size 4096`.
5. Decode every `.ss` file with `scripts/nmminer_ss_decode.py`.
6. Prepare `verify_flash 0x390000 <rebuilt-littlefs.bin>`.
7. Preserve the restore command using the official bundled `littlefs.bin`.

No firmware write was performed for this proof.
