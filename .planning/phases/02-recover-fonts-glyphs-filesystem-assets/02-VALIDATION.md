# Phase 2 Validation — Recover Fonts/Glyphs + Filesystem Assets

Maps each requirement to an automated check the executor/verifier runs. All paths absolute-resolvable
from repo root `/home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series`.

## Requirement coverage

| Req | Plan | Validation ID | Check |
|-----|------|---------------|-------|
| ASSET-01 | 02-02 | V-ASSET-01-id | `grep -q 0x3f4454a0 hardware/recovered-assets/FONT-IDENTIFICATION.md` (descriptors identified) |
| ASSET-01 | 02-02 | V-ASSET-01-fonts | `grep -q lv_font_t firmware/custom/NerdMiner-CYD-Custom/recovered/fonts/fa_nmminer_subset.c` AND matched `LV_FONT_MONTSERRAT_NN 1` toggles in lv_conf.h |
| ASSET-01 | 02-02 | V-ASSET-01-palette | `grep -q NMMINER_COLOR firmware/custom/NerdMiner-CYD-Custom/recovered/nmminer_palette.h` |
| ASSET-01 | 02-02 | V-ASSET-01-provenance | FONT-IDENTIFICATION.md records the OFL/FA-Free TTF source+version; no committed font byte traces to closed firmware (D-21) |
| ASSET-02 | 02-01 | V-ASSET-02 | `grep -qi 'NOT mkspiffs' hardware/recovered-assets/LITTLEFS-MANIFEST.md` AND 17 .ss + web UI listed intact |
| ASSET-03 | 02-01 | V-ASSET-03 | `.venv/bin/python -m pytest tests/test_ss_decode.py -x -q` green; all-17 decode + bit-replication tests pass |
| ASSET-03 | 02-01 | V-ASSET-03-spec | `grep -q FUN_400f6ff4 hardware/recovered-assets/SS-ALGORITHM-SPEC.md` (C-port spec for Phase 5) |

## Nyquist automated gate (run all)

```bash
cd /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series

# ASSET-03: decoder regression (all 17 + bit-replication)
.venv/bin/python -m pytest tests/test_ss_decode.py -x -q

# ASSET-02: LittleFS manifest
test -f hardware/recovered-assets/LITTLEFS-MANIFEST.md \
  && grep -qi 'NOT mkspiffs\|not.*mkspiffs' hardware/recovered-assets/LITTLEFS-MANIFEST.md

# ASSET-03 spec
grep -q 'FUN_400f6ff4' hardware/recovered-assets/SS-ALGORITHM-SPEC.md \
  && grep -qi 'bit-replication\|bit replication' hardware/recovered-assets/SS-ALGORITHM-SPEC.md

# ASSET-01: identification + fonts + palette
grep -q '0x3f4454a0' hardware/recovered-assets/FONT-IDENTIFICATION.md
grep -q 'lv_font_t' firmware/custom/NerdMiner-CYD-Custom/recovered/fonts/fa_nmminer_subset.c
grep -q 'fa_nmminer_subset' firmware/custom/NerdMiner-CYD-Custom/recovered/fonts/recovered_fonts.h
grep -qE 'LV_FONT_MONTSERRAT_(14|16|18|20)[[:space:]]+1' firmware/custom/NerdMiner-CYD-Custom/lib/hansolov2/lv_conf.h
grep -q 'NMMINER_COLOR' firmware/custom/NerdMiner-CYD-Custom/recovered/nmminer_palette.h
```

## Provenance (D-21) manual confirmation

The `V-ASSET-01-provenance` check is partly judgment: confirm in 02-02-SUMMARY.md that every committed
font file was produced by `lv_font_conv` from an open TTF (Montserrat OFL / FontAwesome Free) or is an
LVGL built-in enabled via lv_conf.h — and that NO glyph bitmap was byte-copied from the closed firmware.
FONT-IDENTIFICATION.md (the only artifact derived from closed descriptors) remains gitignored.
