# NMMiner v2.0.02 Reverse-Engineering Pipeline (working)

Reproducible RE pipeline for the stock closed NMMiner firmware, established for
milestone v3.0. Source = the full 4 MB flash backup
`hardware/backups/cyd-backup-20260617-172106.bin` (gitignored).
All `hardware/recovered-assets/` outputs are gitignored (no-redistribution).

## Image map (parsed from the ESP32 image header)

| Segment | Load addr | File off (in app) | Contents |
|---------|-----------|-------------------|----------|
| seg0 DROM | `0x3f400020` | `0x000020` | strings, const data, fonts, image descriptors |
| seg1 RAM  | `0x3ffbdb60` | — | .data |
| seg2 RAM  | `0x40080000` | — | IRAM |
| seg3 IROM | `0x400d0020` | `0x160020` | **main code** (1.25 MB) |
| seg4 RAM  | `0x40081c60` | — | IRAM |

Partitions: `nvs@0x9000`, `factory(app)@0x10000 len 0x380000`, `spiffs@0x390000
len 0x60000` (actually **LittleFS**), `coredump@0x3f0000`.

## Assets — DECODED ✓

- **LittleFS** (`spiffs.bin`, block_size 4096 × 96): mount with `littlefs-python`.
  Contains `index.html.gz` + `apidoc.html.gz` (the `NM Monitor` web UI/API — gunzip)
  and `/ss/saver_320_240_*.ss` (17 screensavers).
- **`.ss` screensavers**: format = `"SN"` + w(LE) + h(LE) + flags(0x01,0xA4) + body,
  body = RGB565 → RLE → LZSS(10-bit offset, 4-bit len). Decoder: `scripts/nmminer_ss_decode.py`.
  All 17 decode to clean 320×240 images.

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

## Next (Phase 1): Ghidra 11 + dynacylabs/ghidra-esp32-flash-loader v12.0 +
self-built ESP-IDF v4.4.6 FIDB → decompile the located screen/render/share-accept
functions to a `SPEC.md`, extract LVGL fonts as C-arrays, then rebuild the screens
natively on the fork. See `.planning/research/SUMMARY.md`.
