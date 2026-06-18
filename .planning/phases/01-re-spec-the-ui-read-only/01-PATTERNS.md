# Phase 1: RE-Spec the UI (read-only) — Pattern Map

**Mapped:** 2026-06-17
**Files analyzed:** 2 (1 prose output, 1 optional script)
**Analogs found:** 1 / 2 (SPEC.md has no code analog; optional script maps to `scripts/nmminer_ss_decode.py`)

---

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|---|---|---|---|---|
| `hardware/recovered-assets/SPEC.md` | documentation / design contract | transform (RE output → structured markdown) | none | no analog |
| `scripts/nmminer_str_xref.py` *(optional)* | utility | file-I/O + transform (binary bytes → address table) | `scripts/nmminer_ss_decode.py` | role-match |

---

## Pattern Assignments

### `hardware/recovered-assets/SPEC.md` (documentation, transform)

**Analog:** none — this is a hand-authored markdown file produced by reading Ghidra decompiler output. There is no existing spec or design-contract markdown in the repo to copy structure from.

**Not-analog explanation:** The closest committed file class would be a `docs/` markdown, but none covers binary RE geometry extraction. The SPEC.md structure is specified explicitly in RESEARCH.md §SPEC.md Structure and has no codebase precedent to copy from.

**Planner directive:** Use RESEARCH.md §SPEC.md Structure (lines 435–480) verbatim as the section template. Key sections in order:
1. `## Metadata` — Ghidra version, loader, FIDB source, confirmed LVGL version with signal evidence, display spec (320×240, RGB565)
2. `## Screen Inventory` — table: Screen | Class name | IROM function address | Priority
3. `## Per-Screen Widget Geometry` — one sub-section per screen; Miner+Clock are full tables, others partial
4. `## Stat Field Formatting` — field names, value formats, SI suffix examples (`"92.67T"`, `"8.00K"`)
5. `## Share-Accept Seam` — function address, call signature, caller chain
6. `## ss_render Characterization` — address + entry-point signature only (full spec is Phase 5)
7. `## Color Palette` — RGB565 raw values + RGB888 conversions per screen
8. `## Font Inventory` — DROM addresses, LVGL names or size/family, which screens use which

Widget geometry table row format (from RESEARCH.md §Pattern 5):
```
| widget_id | type | x | y | w | h | align | font | text_color | bg_color | text/fmt |
```

Color decode recipe (from RESEARCH.md §Pitfall 5 + Pattern 5 lines 334):
```
rgb565 value in low 16 bits of register:
r5=(val>>11)&0x1f  g6=(val>>5)&0x3f  b5=val&0x1f
r8=r5<<3  g8=g6<<2  b8=b5<<3
```

**Output path constraint:** `hardware/recovered-assets/SPEC.md` — gitignored (`.gitignore` line 25). Never commit this file.

---

### `scripts/nmminer_str_xref.py` *(optional — if planner decides to codify the string→l32r pipeline)*

**Analog:** `scripts/nmminer_ss_decode.py` (lines 1–101)

This script may or may not be needed — the string→l32r pipeline is already documented in `docs/nmminer-re-pipeline.md` and the inline Python in RESEARCH.md §Pattern 3 is runnable as-is. The planner should create this script only if it determines a reusable CLI tool saves significant repeated work during SPEC.md authoring.

**Imports pattern** (copy from `scripts/nmminer_ss_decode.py` lines 1–12):
```python
#!/usr/bin/env python3
import sys, os, struct
```
No argparse. Use `sys.argv` directly, print usage via `__doc__` on error.

**CLI pattern** (copy from `scripts/nmminer_ss_decode.py` lines 87–101):
```python
def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    # ... positional args from sys.argv[1], sys.argv[2], etc.

if __name__ == "__main__":
    main()
```

**Core pattern** — the string→l32r xref logic (from RESEARCH.md §Pattern 3, lines 223–235):
```python
import struct
drom = open('hardware/recovered-assets/drom.bin', 'rb').read()
irom = open('hardware/recovered-assets/irom.bin', 'rb').read()

string = b'Miner320x240'
off = drom.find(string)
drom_addr = 0x3f400020 + off           # e.g. 0x3f4be960
addr_bytes = struct.pack('<I', drom_addr)
pos = irom.find(addr_bytes)            # literal pool position in irom.bin
irom_addr = 0x400d0020 + pos           # l32r literal address in IROM VMA space
```

**File I/O pattern** (copy from `scripts/nmminer_ss_decode.py` lines 71–72):
```python
d = open(path, "rb").read()
```
No context managers. Simple open/read. Match existing style.

**Output pattern** (copy from `scripts/nmminer_ss_decode.py` lines 96–97):
```python
print(f"{input} -> {result}")
```
stdout only. No logging framework.

---

## Shared Patterns

### Binary file reading
**Source:** `scripts/nmminer_ss_decode.py` lines 71–72
**Apply to:** Any new `scripts/` helper that reads `.bin` files
```python
d = open(path, "rb").read()
```

### RGB565 → RGB888 decode
**Source:** `scripts/nmminer_ss_decode.py` lines 80–83
**Apply to:** SPEC.md color table entries; any future color-extraction helper
```python
r = (v >> 11) & 0x1F; g = (v >> 5) & 0x3F; b = v & 0x1F
img[idx * 3]     = (r * 255) // 31
img[idx * 3 + 1] = (g * 255) // 63
img[idx * 3 + 2] = (b * 255) // 31
```

### CLI entry point (no argparse)
**Source:** `scripts/nmminer_ss_decode.py` lines 87–101
**Apply to:** Any new `scripts/` helper
```python
def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    ...

if __name__ == "__main__":
    main()
```

### Screen function address table (established, do not re-derive)
**Source:** `docs/nmminer-re-pipeline.md` + RESEARCH.md §Screen Function Locating (verified binary analysis)
**Apply to:** SPEC.md §Screen Inventory — copy these as the pre-confirmed starting set

| Screen | DROM string addr | IROM l32r literal addr | Notes |
|---|---|---|---|
| `Clock320x240` | `0x3f47389b` | `0x400d1864` | Confirmed |
| `Miner320x240` | `0x3f4be960` | `0x400d1960` | Confirmed |
| `Loading320x240` | `0x3f499108` | `0x400d1930` | Confirmed |
| `Config320x240` | `0x3f4738d4` | `0x400d190c` | Confirmed |
| `Weather320x240` | `0x3f4e41d8` | `0x400d19e8` | Confirmed |
| `SPrice320x240` | `0x3f4e41a7` | `0x400d19a8`* | *l32r loads `0x3f4e41a8` (+1 byte). Func ptr at `0x400d19ac` = `0x400f37b0` |

### Share-accept seam locating (same xref method)
**Source:** RESEARCH.md §Share-Accept Seam (lines 420–427)
```python
share_accept_fmt = b'#%d share accepted, %ldms'  # find in drom.bin, xref to IROM caller
ss_render_str = b'ss_render'
```

### objcopy + objdump ELF wrap (do not re-derive)
**Source:** `docs/nmminer-re-pipeline.md` lines 33–39
```bash
OBJCOPY=~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objcopy
OBJDUMP=~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump
$OBJCOPY -I binary -O elf32-xtensa-le \
  --rename-section .data=.text,contents,alloc,load,readonly,code \
  irom.bin irom_text.elf
$OBJDUMP -d --adjust-vma=0x400d0020 irom_text.elf > irom.asm
```
`irom.asm` (476k lines) already exists — do not regenerate unless the source `.bin` changes.

---

## No Analog Found

| File | Role | Data Flow | Reason |
|---|---|---|---|
| `hardware/recovered-assets/SPEC.md` | design contract | transform | No existing RE-derived markdown spec in this repo. Structure prescribed by RESEARCH.md §SPEC.md Structure; content comes from Ghidra decompiler output (genuinely novel per-screen LVGL geometry extraction). |

**Genuinely novel work in this phase (no codebase analog exists):**
- Reading Xtensa decompiler output in Ghidra to extract `lv_obj_set_pos` / `lv_obj_set_size` / `lv_obj_align` argument values per widget — this is interactive RE work, not a script pattern.
- LVGL v8 version confirmation via FIDB symbol naming in Ghidra (Signal 2 in RESEARCH.md §LVGL Version Confirmation).
- Building the ESP-IDF v4.4.6 FidDb (shell commands from RESEARCH.md §Pattern 4, not a codebase pattern).

The planner should plan these as interactive/manual steps with the RESEARCH.md command sequences as the reference, not as code to copy.

---

## Metadata

**Analog search scope:** `scripts/`, `docs/`, `hardware/recovered-assets/` (read-only)
**Files scanned:** 3 (`scripts/nmminer_ss_decode.py`, `docs/nmminer-re-pipeline.md`, `.planning/phases/01-re-spec-the-ui-read-only/01-RESEARCH.md`)
**Pattern extraction date:** 2026-06-17
