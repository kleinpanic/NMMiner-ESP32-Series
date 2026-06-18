# Phase 1: RE-Spec the UI (read-only) - Research

**Researched:** 2026-06-17 (project research) / 2026-06-17 (phase-specific, this session)
**Domain:** ESP32 (Xtensa LX6) closed firmware decompilation — Ghidra 11.x + ESP32 flash loader + ESP-IDF v4.4.6 FIDB + LVGL v8 widget geometry extraction
**Confidence:** HIGH (tool versions live-verified against GitHub API; binary pipeline confirmed against actual recovered-assets; LVGL function signatures from LVGL v8.3 source)

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- **D-01:** Locate all 6 screen-construction functions (`Loading/Miner/Clock/SPrice/Weather/Config 320x240`) via string→l32r literal→function xref. Record each function address in SPEC.md.
- **D-02:** Depth-prioritize `Miner320x240` and `Clock320x240` (already located) for full per-widget geometry. Others get inventory entry + key geometry.
- **D-03:** Identify share-accept seam and `ss_render` as function addresses + call shape. Do NOT fully spec them — just locate + characterize.
- **D-04:** Attempt self-built ESP-IDF v4.4.6 FIDB (Tarlogic method).
- **D-05:** Time-box the FIDB build. Fallback = dynacylabs loader ROM/SVD annotations + existing string→xref pipeline. FIDB is an accelerator, not a hard dependency.
- **D-06:** Install Ghidra 11.4.x + dynacylabs flash-loader v12.0. OR use tenable/esp32_image_parser `create_elf` as alternate import.
- **D-07:** Confirm v8 vs v9 with two independent signals before writing any extractor. Working hypothesis: v8.
- **D-08:** Cross-check recovered stat-field formatting against live LAN API at `http://192.168.1.136/api/system/info`.
- **D-09 (don't-do):** The `lvgl_*.png` / `jpg_*` files in recovered-assets are mis-carved noise. Do not treat as recovered art.
- **D-10 (don't-do):** Do not run v9 `lv_img_dsc_t` magic-byte scan and conclude "no images" for a v8 binary.

### Claude's Discretion
- Exact Ghidra script vs interactive workflow for bulk symbol import.
- Which specific IDF example projects to compile for the FIDB.
- SPEC.md internal section ordering.

### Deferred Ideas (OUT OF SCOPE)
- Font/glyph C-array extraction + LittleFS unpack + `.ss` decoder reuse → Phase 2.
- Actual screen rebuild on the fork → Phase 3.
- Full `ss_render` byte-layout + idle→screensaver state machine → Phase 5.
- Path-A binary-patch integrity recon → Phase 8.
</user_constraints>

---

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RE-01 | Working Ghidra 11.x + dynacylabs esp32 flash-loader v12.0 + self-built ESP-IDF v4.4.6 FIDB workbench loads the stock image with DROM (`0x3f400020`) / IROM (`0x400d0020`) mapped and stripped SDK/Arduino-core symbols recovered. | §Standard Stack and §Ghidra Install + Loader Compatibility below. |
| RE-02 | Stock LVGL major version (v8 vs v9) confirmed authoritatively in-decompiler (FIDB-named decoder symbols + `LV_IMG_CF_*`/`LV_COLOR_FORMAT_*` string family) before any extractor is written. | §LVGL Version Confirmation below. |
| RE-03 | Located screen-construction functions decompiled into SPEC.md — screen inventory, per-screen widget geometry, fonts/colors, market field set, screensaver render path. | §Per-Screen Geometry Extraction and §Screen Function Locating below. |
| RE-04 | SPEC.md is markdown-only, never linked into firmware; all recovered closed-firmware assets remain gitignored. | §Licensing / No-Redistribution below. |
</phase_requirements>

---

## Summary

Phase 1 stands up the decompiler workbench (Ghidra + ESP32 flash loader + ESP-IDF FIDB), locates the remaining 4 screen-construction functions (Loading, SPrice, Weather, Config), decompiles the priority Miner+Clock screens for full per-widget geometry, characterizes the share-accept seam and `ss_render`, and writes this all to `hardware/recovered-assets/SPEC.md`.

**Critical version correction (verified 2026-06-17):** The prior STACK.md referenced "Ghidra 11.4.x" with the "dynacylabs flash-loader v12.0" as a prebuilt drop-in. Both assumptions are now stale. The current Ghidra latest is **12.1.2** (released 2026-06-05). The dynacylabs loader release **v12.0** exists as a git tag but has **no prebuilt `.zip` asset** — it must be compiled from source against a Ghidra 12 installation. The last prebuilt loader zip is `dynacylabs v11.3.1` for Ghidra 11.3.1. **The clean install path is Ghidra 11.3.1 + dynacylabs v11.3.1 prebuilt zip.** [VERIFIED: github.com/NationalSecurityAgency/ghidra releases + github.com/dynacylabs/ghidra-esp32-flash-loader releases]

**Additional finding from binary analysis this session:** The SPrice320x240 screen string has no direct l32r reference in irom.bin. Instead, the literal pool at `0x400d19a8` loads `0x3f4e41a8` = "Price320x240" (one byte into the string, after the 'S'). The SPrice screen constructor address is recoverable from this literal. The IROM func pointer at `0x400d19ac` = `0x400f37b0` is a strong candidate for the SPrice screen constructor or a close callee. All 5 other screens are confirmed with direct string→literal references. [VERIFIED: binary analysis of hardware/recovered-assets/irom.bin + drom.bin]

**Primary recommendation:** Install Ghidra 11.3.1 (prebuilt) + dynacylabs v11.3.1 loader (prebuilt zip) + self-build ESP-IDF v4.4.6 FIDB. The string→l32r pipeline already works without Ghidra for function discovery; Ghidra is needed for decompilation of the screen bodies and LVGL call pattern reading.

---

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Binary loading + address mapping | RE tool (Ghidra + flash loader) | — | Loader auto-sets DROM/IROM VMA from flash image header |
| Function naming (SDK/LVGL symbols) | RE tool (FIDB) | String→xref pipeline | FIDB names stripped SDK functions; xref handles app-level functions |
| LVGL version determination | RE tool + binary | — | Code/symbol analysis in decompiler; no strings in DROM for format names |
| Per-widget geometry extraction | RE tool (decompiler) + manual reading | irom.asm cross-check | Decompiled C is the geometry source; objdump is a sanity check |
| Stat-field formatting semantics | Live LAN API oracle | Saved API reference | Device is running fork, not stock — must restore stock to get live oracle |
| SPEC.md authoring | Manual (human or agent) | — | Transform decompiler output into structured markdown |
| No-redistribution enforcement | .gitignore (hardware/recovered-assets/) | — | Already in place, line 25 of .gitignore |

---

## Standard Stack

### Core

| Tool | Version | Purpose | Why Standard |
|------|---------|---------|--------------|
| **Ghidra** | **11.3.1** (latest with prebuilt loader zip) | Disassembler + decompiler; primary RE workbench | Native Xtensa support since 11.0. Free, scriptable. JDK 21 already installed (`21.0.11`). [VERIFIED: github.com/NationalSecurityAgency/ghidra releases] |
| **dynacylabs/ghidra-esp32-flash-loader** | **v11.3.1** (prebuilt zip, 2025-02-20) | Ghidra extension: auto-maps DROM `0x3f400020` / IROM `0x400d0020`, loads ESP32 ROM symbols, annotates Espressif SVD peripheral registers | Only prebuilt loader zip for a recent Ghidra. Drop-in install. [VERIFIED: github.com/dynacylabs/ghidra-esp32-flash-loader releases] |
| **ESP-IDF v4.4.6 FIDB** (self-built) | built from `espressif/esp-idf tag v4.4.6` | Ghidra Function ID database to auto-name stripped SDK/Arduino-core functions | Must match the binary's IDF version. No public prebuilt Xtensa/IDF FIDB exists. Tarlogic method: compile IDF examples at -Os and -O2, populate FidDb in Ghidra. [CITED: tarlogic.com/blog/esp32-firmware-using-ghidra-fidb/] |
| **tenable/esp32_image_parser** | master (no tagged releases) | `create_elf` wraps app partition as ELF with segment headers for Ghidra import | Already cloned at `hardware/recovered-assets/esp32_image_parser/`. Alternative import path. [VERIFIED: github.com/tenable/esp32_image_parser] |
| **xtensa-esp32-elf-objdump** | toolchain @ `~/.platformio/packages/toolchain-xtensa-esp32/bin/` | Fast cross-check against Ghidra output; already verified working | Already used. Produces `irom.asm` (476k lines). [VERIFIED: existing project pipeline] |

### Supporting

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| **littlefs-python** | 0.17.1 (already in .venv) | LittleFS partition extract/list | Phase 2 (filesystem assets); this phase: `ls` to confirm web UI + screensaver filenames |
| **esptool** | 5.3.0 (in .venv) | `image_info` to re-confirm segment load addresses fed to Ghidra | Run before Ghidra import to re-verify DROM `0x3f400020` / IROM `0x400d0020` |
| **Pillow** | in .venv | RGB565↔PNG conversion | If Ghidra locates new DROM-embedded image pointers; low priority in Phase 1 |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Ghidra 11.3.1 | Ghidra 12.1.2 | 12.1.2 is newer but dynacylabs loader v12.0 has NO prebuilt zip — must self-build the extension from source against Ghidra 12. Extra gradle build step. Use 12.x only if loader build is straightforward; otherwise 11.3.1 is the no-friction path. |
| dynacylabs loader | saibotk upstream v1.1.0 | saibotk v1.1.0 also has a prebuilt zip for Ghidra 11.3.1. Dynacylabs added import options to disable ROM/SVD loading (PR #7, 2025-12-26) and ESP32-S3 support. Either works for ESP32. |
| dynacylabs loader | tenable `create_elf` ELF import | `create_elf` gives a raw ELF without ROM/SVD annotations; loader gives auto-named ROM calls and peripheral register names. `create_elf` is the fallback if loader fails. |
| Self-built ESP-IDF FIDB | Threatrack FIDB repo | threatrack has no Xtensa/ESP-IDF FIDB — self-build is mandatory. [VERIFIED: github.com/threatrack/ghidra-fidb-repo] |

**Installation:**
```bash
# Ghidra 11.3.1 (JDK 21 already installed at 21.0.11)
cd ~/.local/srcs
curl -LO https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_11.3.1_build/ghidra_11.3.1_PUBLIC_20250219.zip
unzip ghidra_11.3.1_PUBLIC_20250219.zip
# launch: ~/.local/srcs/ghidra_11.3.1_PUBLIC/ghidraRun

# ESP32 flash loader (dynacylabs v11.3.1 prebuilt)
curl -LO https://github.com/dynacylabs/ghidra-esp32-flash-loader/releases/download/v11.3.1/ghidra_11.3.1_PUBLIC_20250220_ghidra-esp32-flash-loader.zip
# In Ghidra: File -> Install Extensions... -> "+" -> select zip -> restart

# ESP-IDF v4.4.6 for FIDB (PlatformIO toolchain GCC 8.4.0 already at correct version)
mkdir -p ~/esp && cd ~/esp
git clone -b v4.4.6 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32
# Then compile examples and build FIDB (see §FIDB Build Workflow below)
```

---

## Package Legitimacy Audit

No npm/PyPI packages are installed in this phase. Tools are:
- Ghidra: NSA GitHub official release [VERIFIED: github.com/NationalSecurityAgency/ghidra]
- dynacylabs loader: GitHub prebuilt zip [VERIFIED: github.com/dynacylabs/ghidra-esp32-flash-loader]
- ESP-IDF: Espressif official repository [VERIFIED: github.com/espressif/esp-idf]
- littlefs-python 0.17.1: already in .venv [VERIFIED: existing project dependency]

No packages flagged SUS or SLOP.

---

## Architecture Patterns

### System Architecture Diagram

```
                  CLOSED NMMiner v2.0.02 BINARY
                  hardware/backups/cyd-backup-*.bin
                           │
                    esptool read_flash
                           │
            ┌──────────────┼───────────────┐
            ▼              ▼               ▼
         drom.bin       irom.bin       spiffs.bin
    [DROM 0x3f400020] [IROM 0x400d0020] [LittleFS]
            │              │               │
     strings/xref    Ghidra 11.3.1         │ (Phase 2)
     pipeline        + ESP32 loader        │
            │              │               │
            ▼              ▼               │
     Screen func    Decompiled C bodies    │
     addresses      (lv_obj_*, lv_label_*) │
            │              │               │
            └──────────────▼───────────────┘
                   hardware/recovered-assets/
                          SPEC.md
                    (gitignored, markdown)
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
           Phase 2      Phase 3      Phase 5
        (font/asset   (screen      (screensaver/
         recovery)     rebuild)      market)
```

### Recommended Project Structure (Ghidra project, not committed)
```
~/ghidra_projects/nmminer-re/
├── nmminer.gpr             # Ghidra project file (gitignored)
├── nmminer-irom.elf        # imported from create_elf (gitignored)
└── ...                     # analysis outputs (gitignored)

hardware/recovered-assets/  # gitignored per .gitignore line 25
├── SPEC.md                 # the deliverable
├── drom.bin, irom.bin      # already present
├── irom.asm, irom_text.elf # already present
└── esp32_image_parser/     # already cloned
```

### Pattern 1: Ghidra Import via Flash Loader (preferred)

**What:** Import the raw app partition (`app_factory.bin`) directly via the dynacylabs flash loader. The loader reads the ESP32 image header, sets DROM/IROM VMAs automatically, loads ESP32-D0WD-V3 ROM symbols, and annotates SVD peripheral registers.

**When to use:** First-choice import path.

```bash
# Source: dynacylabs/ghidra-esp32-flash-loader README
# In Ghidra GUI:
#   File -> Import File... -> select app_factory.bin
#   Format: "ESP32 Flash Image" (provided by the loader extension)
#   Language: Xtensa:LE:32:default (auto-suggested by loader)
#   Loader will auto-set DROM base 0x3f400020 and IROM base 0x400d0020
#   Enable ROM loading and SVD loading (added in PR #7 as optional)
```

### Pattern 2: Alternate ELF Import via esp32_image_parser

**What:** Use `create_elf` to produce an ELF with correct segment headers, then import the ELF. No ROM/SVD annotations from loader, but clean segment layout.

**When to use:** Fallback if flash loader import fails.

```bash
# Source: tenable/esp32_image_parser (already cloned)
cd hardware/recovered-assets/esp32_image_parser
pip install -r requirements.txt  # into .venv
python esp32_image_parser.py create_elf ../app_factory.bin -o ../nmminer-app.elf
# Then import nmminer-app.elf in Ghidra as ELF (architecture = Xtensa:LE:32:default)
```

### Pattern 3: String → l32r Literal → Function Xref (established, no Ghidra needed)

**What:** Find a screen class name string in drom.bin, compute its DROM address, then scan irom.bin for that address as a 4-byte LE literal. The offset of the literal in irom.bin + `0x400d0020` gives the `l32r` instruction address, which is inside (or immediately before) the screen constructor function.

**When to use:** Discovering screen-construction function addresses before Ghidra is up; as a sanity check against Ghidra's analysis.

```python
# Source: docs/nmminer-re-pipeline.md (established working method)
import struct
drom = open('hardware/recovered-assets/drom.bin','rb').read()
irom = open('hardware/recovered-assets/irom.bin','rb').read()

string = b'Miner320x240'
off = drom.find(string)
drom_addr = 0x3f400020 + off           # e.g. 0x3f4be960
addr_bytes = struct.pack('<I', drom_addr)
pos = irom.find(addr_bytes)            # literal pool position in irom.bin
irom_addr = 0x400d0020 + pos           # e.g. 0x400d1960  <- l32r literal address
```

**Known screen function literal addresses (pre-confirmed):**

| Screen | String DROM addr | l32r literal in IROM | Notes |
|--------|-----------------|---------------------|-------|
| `Clock320x240` | `0x3f47389b` | `0x400d1864` | Confirmed |
| `Miner320x240` | `0x3f4be960` | `0x400d1960` | Confirmed |
| `Loading320x240` | `0x3f499108` | `0x400d1930` | Confirmed |
| `Config320x240` | `0x3f4738d4` | `0x400d190c` | Confirmed |
| `Weather320x240` | `0x3f4e41d8` | `0x400d19e8` | Confirmed |
| `SPrice320x240` | `0x3f4e41a7` | `0x400d19a8`* | *l32r loads 0x3f4e41a8 ("Price320x240"); IROM func ptr at 0x400d19ac = `0x400f37b0` |

**SPrice note:** The SPrice string has no direct l32r reference. The literal at `0x400d19a8` loads `0x3f4e41a8` (one byte past 'S'). The IROM func pointer at `0x400d19ac` = `0x400f37b0` is the strong candidate for the SPrice constructor. Ghidra decompilation will confirm the actual entry point.

### Pattern 4: FIDB Build Workflow (Tarlogic method)

**What:** Compile ESP-IDF v4.4.6 example projects at both `-Os` and `-O2` optimization levels. Import the resulting static libraries into a Ghidra project, then use Tools → Function ID → Populate FidDb to generate a `.fidb` database. Apply to the NMMiner binary.

**When to use:** After Ghidra is installed and before detailed decompilation of complex screen bodies. Creates ~20k+ function name matches.

**Which examples to compile (covers NMMiner's SDK usage):**

| Category | Example project(s) | Covers |
|----------|-------------------|--------|
| Core/RTOS | `examples/system/freertos/real_time_stats` | Task/queue primitives |
| NVS | `examples/storage/nvs_rw_value` | NVS settings persistence |
| WiFi | `examples/wifi/getting_started/station` | WiFi stack |
| HTTP server | `examples/protocols/esp_http_server/simple` | HTTP API server |
| HTTP client | `examples/protocols/http_request` | Binance/open-meteo fetch |
| LEDC (speaker) | `examples/peripherals/ledc/ledc_basic` | LEDC/PWM for GPIO26 |
| SPI | `examples/peripherals/spi_master/lcd` | Display/touch SPI |
| GPIO | `examples/peripherals/gpio/generic_gpio` | LED/button |
| Hello world | `examples/get-started/hello_world` | Minimal IDF startup |

```bash
# Build each example at both opt levels:
cd ~/esp/esp-idf
source export.sh
for EXAMPLE_DIR in examples/storage/nvs_rw_value examples/wifi/getting_started/station \
    examples/protocols/esp_http_server/simple examples/peripherals/ledc/ledc_basic \
    examples/peripherals/spi_master/lcd examples/get-started/hello_world; do
  cd ~/esp/esp-idf/$EXAMPLE_DIR
  idf.py set-target esp32
  # -Os variant:
  idf.py -DSDKCONFIG_DEFAULTS="sdkconfig.defaults" build
  # -O2 variant: edit sdkconfig or set via menuconfig -> Compiler Options -> Optimization Level
done
# Then in Ghidra: Tools -> Function ID -> Create New Empty FidDb...
# -> Populate FidDb from Programs... (select ~/esp/esp-idf build dirs, arch = Xtensa:LE:32:default)
```

**Time estimate:** [ASSUMED] IDF clone + recursive is ~2 GB. Single example build ~2-5 min. 9 examples × 2 opt levels = ~45-90 min total. FIDB population in Ghidra: ~15 min for all imported ELFs. Total effort: ~2-3 hours. **This is the D-05 time-box threshold** — if the IDF toolchain or build system has version friction beyond 30 min diagnosis, fall back to the string→xref pipeline.

**Fallback if FIDB build stalls (D-05):** The two priority screen funcs (Miner+Clock) were already located WITHOUT a FIDB. The loader's ROM symbol annotations + the string→xref pipeline provide enough context to read LVGL widget geometry manually from Ghidra decompiler output. FIDB just accelerates naming of the SDK helpers around those calls.

### Pattern 5: Reading Per-Widget Geometry from Ghidra Decompiler Output

**What:** After FIDB is applied (or even without it), the screen-construction function decompiles to a sequence of `lv_obj_*` and `lv_label_*` calls. Each call's arguments are the geometry to record.

**Xtensa calling convention:** Arguments are in registers `a2` (arg0/`obj`), `a3` (arg1), `a4` (arg2), `a5` (arg3). Ghidra's decompiler renders these as named parameters when FIDB provides the function prototype. Without FIDB, they appear as unnamed pointer/int args. [CITED: Xtensa LX6 ABI documentation]

**LVGL v8 geometry functions to watch for** (from `release/v8.3` source): [VERIFIED: raw.githubusercontent.com/lvgl/lvgl/release/v8.3/src/core/lv_obj_pos.h]

```c
// Geometry setters (lv_coord_t = int32_t on 32-bit targets)
void lv_obj_set_pos(lv_obj_t *obj, lv_coord_t x, lv_coord_t y);
void lv_obj_set_x(lv_obj_t *obj, lv_coord_t x);
void lv_obj_set_y(lv_obj_t *obj, lv_coord_t y);
void lv_obj_set_size(lv_obj_t *obj, lv_coord_t w, lv_coord_t h);
void lv_obj_set_width(lv_obj_t *obj, lv_coord_t w);
void lv_obj_set_height(lv_obj_t *obj, lv_coord_t h);
void lv_obj_align(lv_obj_t *obj, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);
void lv_obj_align_to(lv_obj_t *obj, const lv_obj_t *base, lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs);

// Widget creation
lv_obj_t *lv_obj_create(lv_obj_t *parent);
lv_obj_t *lv_label_create(lv_obj_t *parent);

// Label content
void lv_label_set_text(lv_obj_t *obj, const char *text);
void lv_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...);
void lv_label_set_text_static(lv_obj_t *obj, const char *text);

// Style setters (inline, from lv_obj_style_gen.h)
void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *value, lv_style_selector_t selector);
void lv_obj_set_style_text_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_color(lv_obj_t *obj, lv_color_t value, lv_style_selector_t selector);
void lv_obj_set_style_bg_opa(lv_obj_t *obj, lv_opa_t value, lv_style_selector_t selector);
```

**Reading recipe for a screen-construction function in Ghidra:**

1. Navigate to the function address (e.g. `0x400d1960` for Miner screen).
2. With FIDB applied: decompiler shows named calls. Without FIDB: calls appear as `FUN_400xxxxx(arg1, arg2, ...)`. Match by call count/address to the l32r literals nearby.
3. For each `lv_obj_create(parent)` or `lv_label_create(parent)` call — this is a new widget. Note the return value variable.
4. For each `lv_obj_set_pos(widget, x, y)` / `lv_obj_set_size(widget, w, h)` — the integer literal arguments are the x/y/w/h in pixels (screen = 320×240, coordinate origin = top-left).
5. For each `lv_obj_align(widget, align, x_ofs, y_ofs)` — the `align` argument is a `lv_align_t` enum. v8 `LV_ALIGN_*` values: `LV_ALIGN_CENTER=0`, `LV_ALIGN_TOP_LEFT=1`, `LV_ALIGN_TOP_MID=2`, `LV_ALIGN_TOP_RIGHT=3`, etc. [CITED: lvgl.io/docs/open/8.3/overview/coords]
6. For each `lv_obj_set_style_text_font(widget, &font_ptr, selector)` — the font pointer is a DROM address. The symbol name from FIDB (or LVGL font table) identifies the typeface + size (e.g. `lv_font_montserrat_16`, `lv_font_montserrat_24`).
7. For each `lv_obj_set_style_text_color(widget, {r,g,b,_}, selector)` — the `lv_color_t` is a 2-byte RGB565 value passed as a 4-byte argument (upper bytes unused). On a 16-bit ST7789 display: `lv_color16_t` = `{blue:5, green:6, red:5}` packed LE. Read the raw `uint16_t` value and convert: `r=(val>>11)&0x1f, g=(val>>5)&0x3f, b=val&0x1f` → RGB888 by `r8=r<<3, g8=g<<2, b8=b<<3`.
8. For each `lv_label_set_text_static(widget, str_ptr)` or `lv_label_set_text(widget, str_ptr)` — the string pointer is a DROM address. Resolve to the literal string (e.g. `"MH/s"`, `"H/s"`, format string `"%2.2f"`, etc.).
9. Record each widget in SPEC.md as a table row: `widget_id | type | x | y | w | h | align | font | color | text/fmt`.

**lv_coord_t note:** In LVGL v8 for ESP32 (32-bit), `lv_coord_t` is `int32_t`. Integer literals for geometry will appear as plain 32-bit signed ints in the decompiler.

**lv_color_t note:** On a 16-bit display (ST7789), `lv_color_t` = `lv_color16_t` — a 2-byte packed RGB565 value. The decompiler may show it as a `uint32_t` argument (struct passed in register). The low 16 bits are the color; ignore high 16 bits.

### Anti-Patterns to Avoid

- **Don't install Ghidra 12.1.2 assuming the dynacylabs loader v12.0 has a prebuilt zip.** It does not. [VERIFIED: github.com/dynacylabs releases]
- **Don't run a v9 magic-byte (`0x19`) scan on the DROM.** This binary is v8; v8 has no magic byte in `lv_img_header_t`. Zero matches is expected and does not indicate missing assets. (D-10)
- **Don't treat the mis-carved `lvgl_*.png` files as recovered screen art.** They are noise from a raw RGB565 scan. (D-09)
- **Don't try to grep DROM for `LV_IMG_CF_*` or `LV_COLOR_FORMAT_*` as strings.** These are C enum names, not DROM string literals. LVGL version is determined from decompiler symbol names, not DROM strings.
- **Don't treat the SPrice screen as "unfound"** because its exact string has no direct l32r. Use the `0x400d19a8`/`0x400f37b0` leads as the entry point into Ghidra analysis.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| DROM/IROM segment address mapping | Manual binary slicing | Ghidra + dynacylabs ESP32 loader | Loader reads image header, handles 32-byte offset, sets correct VMA automatically |
| ESP32 ROM function identification | Parse ROM source manually | Dynacylabs loader (auto-loads ROM symbols) | ROM has 1000s of functions; loader names them in one step |
| Peripheral register names | Hand-annotate MMIO addresses | Dynacylabs loader SVD support | SVD files cover all Espressif peripheral addresses with field names |
| SDK function names in stripped binary | String search heuristics | ESP-IDF v4.4.6 FIDB | FIDB matches by code hash, handles inlining, much faster than manual |
| LVGL v8 widget geometry parsing | Custom struct scanner | Decompiler output reading (Pattern 5 above) | The geometry IS the decompiled function body — no parser needed |
| LittleFS partition extraction | Raw byte parsing | littlefs-python 0.17.1 (already in .venv) | Handles block size mismatch detection, directory traversal |

---

## LVGL Version Confirmation

**Working hypothesis: v8.x.** Evidence: the binary was compiled on an Arduino-ESP32 build for IDF v4.4.6 dated Oct 2023; LVGL v9.0 was released Jan 2024, after this binary. [ASSUMED]

**Two-signal confirmation method (D-07):**

**Signal 1 (string pivot):** Grep DROM for version-distinguishing format name families:
- v8: function names contain `lv_img_decoder_*` (e.g. `lv_img_decoder_open`, `lv_img_decoder_read_line`)
- v9: function names contain `lv_image_decoder_*` (renamed in v9 across the entire API)
- DROM strings `LV_IMG_CF_*` = v8 enum names (these are C enum names, not runtime strings — unlikely to appear literally, but any log/assert strings that reference them would be v8)

DROM search result from this session: the only LVGL strings in DROM are `"Failed to allocate memory for lvgl_color_buf"`, `"(lvgl)"`, `"NMMiner::init: failed to create LVGL task"`. No `lv_img_decoder` or `lv_image_decoder` strings. Version determination falls to Signal 2. [VERIFIED: binary analysis of drom.bin]

**Signal 2 (FIDB + decompiler, authoritative):** Once FIDB is applied in Ghidra:
- If the image decoder functions are named `lv_img_decoder_open`, `lv_img_decoder_read_line` → **v8 confirmed**
- If named `lv_image_decoder_open`, `lv_image_decoder_read_line` → **v9** (would contradict hypothesis)

The FIDB approach is the definitive gate for RE-02. Do not write any image descriptor extractor until this is confirmed. [CITED: lvgl.io/docs/open/8.3 + lvgl.io/docs/open/9.x API comparison]

**v8 image header layout (for reference, after confirmation):**
```c
// Source: lvgl/lvgl release/v8.3 src/draw/lv_img_buf.h [VERIFIED]
typedef struct {
    uint32_t cf : 5;          // color format: 4=TRUE_COLOR, 5=TRUE_COLOR_ALPHA, 19=RGB565
    uint32_t always_zero : 3; // always 0 in v8 (v9 uses this for magic 0x19)
    uint32_t reserved : 2;
    uint32_t w : 11;          // width in pixels
    uint32_t h : 11;          // height in pixels
} lv_img_header_t;            // 4 bytes, LE

typedef struct {
    lv_img_header_t header;   // 4 bytes
    uint32_t data_size;       // pixel data size in bytes
    const uint8_t *data;      // DROM pointer: 0x3f4xxxxx
} lv_img_dsc_t;               // 12 bytes total; no magic byte
```

---

## Screen Function Locating (all 6 — RE-03 prerequisite)

All 6 screen class strings confirmed in DROM with their associated l32r literal addresses in IROM. [VERIFIED: binary analysis of hardware/recovered-assets/drom.bin + irom.bin]

| Screen class | DROM addr (string) | IROM l32r literal addr | Status |
|---|---|---|---|
| `Clock320x240` | `0x3f47389b` | `0x400d1864` | Located |
| `Miner320x240` | `0x3f4be960` | `0x400d1960` | Located |
| `Loading320x240` | `0x3f499108` | `0x400d1930` | Located |
| `Config320x240` | `0x3f4738d4` | `0x400d190c` | Located |
| `Weather320x240` | `0x3f4e41d8` | `0x400d19e8` | Located |
| `SPrice320x240` | `0x3f4e41a7` | `0x400d19a8`* | *Indirect — l32r loads +1 byte offset. IROM func ptr at `0x400d19ac` = `0x400f37b0` |

**Finding the actual function entry point:** The l32r literal address is inside the function's literal pool, which typically precedes or follows the function body. In Ghidra with the flash loader, the disassembler will detect function boundaries automatically. Navigate to each address above in Ghidra, then use "Find References" or look for the enclosing function start (the `entry` instruction or CALL8/CALL4 target boundary).

**Share-accept seam and ss_render (D-03):**

```python
# Use same string→xref method to locate these:
share_accept_fmt = b'#%d share accepted, %ldms'   # in DROM, locate → l32r → function
ss_render_str = b'ss_render'                         # look for adjacent format/log strings
```

The `#%d share accepted` format string is already confirmed in DROM (ARCHITECTURE.md). Use the same xref pipeline to find the IROM caller. Record the function address and the arguments (share count, time). The share-accept function IS the seam Phase 6 wires the speaker at.

---

## SPEC.md Structure

The downstream consumer (Phase 3 planner) needs the following sections. Claude's discretion on ordering.

```markdown
# NMMiner v2.0.02 UI Spec (RE-derived)
## Metadata
  - Ghidra version, loader version, FIDB source
  - LVGL version confirmed (v8 or v9) with signal evidence
  - Screen resolution: 320×240 (ST7789)
  - Color depth: RGB565 (16-bit)

## Screen Inventory
  | Screen | Class name | IROM function address | Priority |
  
## Per-Screen Widget Geometry
  ### Miner320x240 (full)
    | widget_id | type | x | y | w | h | align | font | text_color | bg_color | text/fmt |
  ### Clock320x240 (full)
    | ... |
  ### Loading320x240 (key geometry, partial)
  ### Config320x240 (key geometry, partial)
  ### Weather320x240 (key geometry, partial)
  ### SPrice320x240 (key geometry, partial)

## Stat Field Formatting
  - hashRate: float GH/s (e.g. 2.45)
  - networkDiff: string with SI suffix (e.g. "92.67T")
  - poolDiff: string with SI suffix (e.g. "8.00K")
  - [per API reference field semantics]

## Share-Accept Seam
  - Function address
  - Call signature (args: share_count, time_ms)
  - Caller chain (where to hook in Phase 6 NerdMiner fork)

## ss_render Characterization
  - Function address
  - Entry point signature (args: buffer ptr, frame index, etc.)
  - NOT a full byte-layout spec (that is Phase 5)

## Color Palette
  - Background color(s) per screen (RGB565 values from style calls)
  - Text colors (normal, alert, price-up, price-down)

## Font Inventory
  - DROM addresses of font descriptors
  - LVGL font names (from FIDB) or size/family (from descriptor analysis)
  - Which screens use which fonts
```

---

## Common Pitfalls

### Pitfall 1: Installing Ghidra 12.x, expecting dynacylabs loader v12.0 to have a prebuilt zip

**What goes wrong:** Download Ghidra 12.1.2, go to dynacylabs releases, find v12.0 has zero assets. Must either downgrade Ghidra or build the loader extension from source.

**Why it happens:** Dynacylabs v12.0 is a git tag only (no release build attached). The v11.4.3 note "Compatible with Ghidra versions < 12" makes clear the split.

**How to avoid:** Use Ghidra 11.3.1 + dynacylabs v11.3.1 prebuilt zip. If Ghidra 12 is preferred, build the dynacylabs extension from source: `export GHIDRA_INSTALL_DIR=<path-to-ghidra-12>; gradle` in the cloned loader repo.

**Warning signs:** "No assets" on the dynacylabs v12.0 release page.

### Pitfall 2: Treating the IROM literal address as the function entry point

**What goes wrong:** The l32r literal address (e.g. `0x400d1960`) is in the function's literal pool, not the entry point. Decompiling from that address gives garbage if it's in the middle of packed literals.

**Why it happens:** The Xtensa literal pool and code are interleaved. The l32r address is where the 4-byte pointer literal lives, not where the function starts.

**How to avoid:** In Ghidra, navigate to the literal address, then use "Find Function Containing" or look at the surrounding disassembly to find the nearest function start (typically a few instructions before, identifiable by the `entry a1, <n>` prologue instruction for windowed-ABI functions).

**Warning signs:** Ghidra shows bytes that don't disassemble cleanly at the literal address (they're 32-bit pointer values, not instructions).

### Pitfall 3: SPrice screen "not found" because the string has no exact l32r reference

**What goes wrong:** `irom.bin.find(struct.pack('<I', 0x3f4e41a7))` returns -1. Conclusion: SPrice constructor is not findable by this method.

**Why it happens:** The SPrice l32r loads `0x3f4e41a8` (one byte into the string, past the 'S'). The class uses the `+1` pointer, possibly because the constructor assembles a string dynamically or the string reference is to a sub-string.

**How to avoid:** Search for `0x3f4e41a8` (not `0x3f4e41a7`). Found at IROM `0x400d19a8`. The IROM func pointer at `0x400d19ac` = `0x400f37b0` is the SPrice constructor candidate — navigate there in Ghidra.

### Pitfall 4: FIDB false-positive symbol names from mismatched optimization

**What goes wrong:** FIDB compiled only at `-Os` misses functions inlined or restructured by `-O2` (and vice versa). A function might get a wrong name if its code hash matches a different function at a different optimization level.

**Why it happens:** FIDB matches by instruction hash. Compiler optimization changes code structure, so the same source function has different hashes at different opt levels.

**How to avoid:** Compile each example project at BOTH `-Os` AND `-O2`, import all resulting ELFs before populating the FidDb. The Tarlogic article documents 23,361 functions added from this multi-opt approach. [CITED: tarlogic.com/blog/esp32-firmware-using-ghidra-fidb/]

### Pitfall 5: lv_color_t argument confusion in decompiler (2 bytes vs 4 bytes)

**What goes wrong:** Reading `lv_obj_set_style_text_color(obj, color, selector)` — the `color` argument is a `lv_color_t` (2-byte `uint16_t` on 16-bit display), but Xtensa passes it in a 32-bit register `a3`. The decompiler shows a 32-bit value and you read the wrong bits as the color.

**How to avoid:** The actual RGB565 color is in the LOW 16 bits of the register value. Decode: `r5=(val>>11)&0x1f`, `g6=(val>>5)&0x3f`, `b5=val&0x1f`. Convert to RGB888: `r8=r5<<3`, `g8=g6<<2`, `b8=b5<<3`.

---

## Runtime State Inventory

This phase is read-only RE (no firmware code written, no device writes). There is no runtime state to migrate. The only "state" this phase produces is `hardware/recovered-assets/SPEC.md` — a new file in an already-gitignored directory.

**Nothing found in any category:** Phase 1 creates no persistent runtime state; deferred categories (NVS, live device config) belong to later phases.

---

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| OpenJDK 21 | Ghidra 11.3.1 / 12.x | ✓ | 21.0.11 | — |
| Xtensa toolchain (objdump/gcc) | FIDB build + irom.asm cross-check | ✓ | GCC 8.4.0 (esp-2021r2-patch5) | — |
| littlefs-python | Filesystem partition list | ✓ | 0.17.1 | — |
| esptool | image_info re-verification | ✓ | 5.3.0 | — |
| Ghidra 11.3.1 | Decompilation (RE-01) | ✗ (not yet installed) | — | `irom.asm` + string→xref pipeline (no decompilation) |
| dynacylabs loader v11.3.1 | DROM/IROM auto-mapping (RE-01) | ✗ (not yet installed) | — | `create_elf` ELF import (no ROM/SVD annotations) |
| ESP-IDF v4.4.6 (for FIDB) | Symbol naming (RE-01) | ✗ (not yet cloned) | — | Loader ROM symbols + manual LVGL xref without FIDB (D-05 fallback) |
| binwalk | Asset triage | ✗ (not installed) | — | Not needed in Phase 1; DROM string analysis is sufficient |
| Live device with stock NMMiner v2.0.02 | D-08 stat-field formatting oracle | ✗ (device runs fork) | — | `docs/research/nmminer-api-reference.md` has field examples (`"92.67T"`, `"8.00K"`, `hashRate: 2.45`) |

**Missing dependencies with no fallback:**
- None — all blockers have fallbacks documented above.

**Missing dependencies with fallback:**
- Ghidra + loader + FIDB → Install per §Standard Stack (no friction expected given JDK 21 present and 160 GB disk free).
- Live stock device for D-08 → Use saved `docs/research/nmminer-api-reference.md` for field formatting semantics.

---

## Validation Architecture

`workflow.nyquist_validation` not explicitly set to false in config.json → treated as enabled.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | None — this phase produces a markdown spec, not code. |
| Quick run command | `python3 -c "import json; open('hardware/recovered-assets/SPEC.md').read()"` (existence check) |
| Full suite command | Manual review checklist against SPEC.md requirements |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| RE-01 | Ghidra project loads with DROM/IROM at correct VMA | manual | Launch Ghidra, verify segment list shows `0x3f400020` / `0x400d0020` | ✗ (manual) |
| RE-02 | LVGL version confirmed with two signals | manual | FIDB names `lv_img_decoder_*` (v8) or `lv_image_decoder_*` (v9) — document result in SPEC.md §LVGL version | ✗ (manual) |
| RE-03 | SPEC.md exists with all 6 screen entries + Miner+Clock full geometry | smoke | `python3 -c "text=open('hardware/recovered-assets/SPEC.md').read(); assert all(s in text for s in ['Miner320x240','Clock320x240','Loading320x240','SPrice320x240','Weather320x240','Config320x240'])"` | ❌ Wave 0 |
| RE-04 | SPEC.md not committed to git | smoke | `git ls-files hardware/recovered-assets/SPEC.md \| wc -l` → must be 0 | ❌ Wave 0 |

### Wave 0 Gaps

- [ ] RE-03 smoke script (inline Python above) — run after SPEC.md is written
- [ ] RE-04 gitignore check — run after writing SPEC.md

---

## Security Domain

This phase is pure read-only RE on a local binary file. No network exposure, no firmware write, no keys or credentials involved.

| ASVS Category | Applies | Control |
|---------------|---------|---------|
| V2 Authentication | No | — |
| V5 Input Validation | No | — |
| V6 Cryptography | No | — |
| Licensing / IP | Yes | All recovered assets stay in gitignored `hardware/recovered-assets/`. SPEC.md is markdown analysis only (RE-04). Never redistribute the closed binary or any extracted verbatim firmware content. |

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Ghidra 11.4.x (per STACK.md) | Ghidra 11.3.1 (last with prebuilt loader zip) | 2025-12-18 (dynacylabs v12.0 tag, no prebuilt) | Use 11.3.1 for zero-friction install; or build loader from source for 12.x |
| dynacylabs flash loader v12.0 "drop-in" (per STACK.md) | v11.3.1 prebuilt zip is the last ready-to-install artifact | 2025-12-18 | The CONTEXT D-06 assumption needs correction at plan time |
| Ghidra requires JDK 17 (legacy) | JDK 21 required for Ghidra 11.x and 12.x | Ghidra 11.x release | Already satisfied (21.0.11 installed) |

**Deprecated/outdated:**
- yath/ghidra-xtensa processor module: obsolete on Ghidra ≥11.0 (native Xtensa built in). Do not install.
- tslater2006/esp32_flash_loader: original (Ghidra 9.1-era, no releases). Use dynacylabs fork instead.

---

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | FIDB build effort is ~2-3 hours | §Pattern 4 FIDB workflow | If friction is higher (toolchain version mismatch, IDF configure errors), FIDB stalls and D-05 fallback activates. Low risk — D-05 explicitly covers this. |
| A2 | LVGL version is v8 (Oct 2023 build predates v9.0 Jan 2024) | §LVGL Version Confirmation | If v9, all v8-specific extraction patterns are wrong. Signal 2 (FIDB in Ghidra) settles this before any extractor is written — risk is caught early. |
| A3 | The IROM func ptr `0x400f37b0` at `0x400d19ac` is the SPrice constructor or a close callee | §Screen Function Locating | If it's an unrelated function, SPrice needs more manual Ghidra navigation. Low risk — Ghidra's xref and function-boundary analysis will identify it. |
| A4 | Ghidra 11.3.1 + dynacylabs v11.3.1 loader correctly auto-maps DROM `0x3f400020` / IROM `0x400d0020` for this image | §Standard Stack | If loader uses different address heuristics, segments are misplaced and all addresses are wrong. Mitigation: run `esptool image_info` first and compare against loader's analysis. |
| A5 | The live device (`192.168.1.136`) is running the fork (not stock NMMiner) and cannot serve as live oracle for stock stat-field formatting | §Environment Availability | If confirmed = stock is actually present, D-08 oracle is directly available. Current evidence: device returned `{"miner":{"sharesAccepted":0,"totalMHashes":1456},"sd":{...},"speaker":{...}}` — the `sd` and `speaker` fields are fork-only, confirming not stock. [VERIFIED: live API query 2026-06-17] |

---

## Open Questions

1. **Is Ghidra 11.3.1 vs 12.1.2 the right trade-off for this project?**
   - Known: 11.3.1 has prebuilt loader zip. 12.1.2 is 6 months newer (Xtensa improvements possible), but loader must be built from source.
   - Recommendation: Start with 11.3.1 (zero install friction). If analysis quality is poor, revisit Ghidra 12 with self-built loader.

2. **What is the exact SPrice320x240 screen constructor entry point?**
   - Known: IROM func ptr `0x400f37b0` at literal `0x400d19ac`. String loads one byte past 'S'.
   - Resolution: Navigate to `0x400f37b0` in Ghidra and confirm it's an LVGL screen-construction function (should see `lv_obj_create` near entry).

3. **Will the FIDB IDF v4.4.6 examples cover LVGL calls?**
   - Known: NMMiner's LVGL is statically linked with the firmware (standard Arduino-ESP32 LVGL bundling). The LVGL functions will be in the firmware binary itself, not in IDF examples.
   - Implication: FIDB names IDF/Arduino-core SDK helpers. LVGL functions are NOT in the FIDB (they come from the LVGL source, not IDF). The planner should plan a SEPARATE step to load LVGL v8.3 built libraries as a second FidDb, or rely on manual reading of the LVGL call patterns using the function signatures in §Pattern 5 above. [ASSUMED — LVGL bundling in Arduino-ESP32]

---

## Sources

### Primary (HIGH confidence)
- `hardware/recovered-assets/drom.bin` + `irom.bin` — direct binary analysis in this session confirming all 6 screen string addresses and l32r literal locations [VERIFIED: binary analysis]
- `github.com/NationalSecurityAgency/ghidra/releases` — Ghidra 12.1.2 (latest), 11.3.1 (last before 12), JDK 21 requirement [VERIFIED: GitHub API]
- `github.com/dynacylabs/ghidra-esp32-flash-loader/releases` — v11.3.1 has prebuilt zip; v12.0 has tag only (no assets) [VERIFIED: GitHub API]
- `github.com/lvgl/lvgl/raw/release/v8.3/src/core/lv_obj_pos.h` — all geometry function signatures [VERIFIED: GitHub raw content]
- `github.com/lvgl/lvgl/raw/release/v8.3/src/core/lv_obj_style_gen.h` — style setter signatures [VERIFIED: GitHub raw content]
- `github.com/lvgl/lvgl/raw/release/v8.3/src/widgets/lv_label.h` — label function signatures [VERIFIED: GitHub raw content]
- `github.com/lvgl/lvgl/raw/release/v8.3/src/draw/lv_img_buf.h` — v8 image header layout, `LV_IMG_CF_*` enum [VERIFIED: GitHub raw content]
- `.planning/research/STACK.md`, `.planning/research/PITFALLS.md`, `.planning/research/FEATURES.md`, `.planning/research/ARCHITECTURE.md` — prior project research [HIGH confidence, from prior session]

### Secondary (MEDIUM confidence)
- `tarlogic.com/blog/esp32-firmware-using-ghidra-fidb/` — FIDB build workflow; ~23,361 functions from IDF examples; -Os and -O2 variants [CITED]
- `olof-astrand.medium.com/analyzing-a-bluetooth-esp32-program-with-ghidra-in-2026-9bec59965e51` — Ghidra 12 + ESP32 workflow in 2026; pyghidra note [CITED]
- `docs/research/nmminer-api-reference.md` — stat field formatting (`"92.67T"`, `"8.00K"`, `hashRate: 2.45 GH/s`) [HIGH — project-saved reference]
- `lvgl.io/docs/open/8.3/overview/coords` — LVGL v8 coordinate system [CITED]

### Tertiary (LOW confidence)
- FIDB time estimate (~2-3 hours): [ASSUMED] based on Tarlogic article description and example count, not verified by running the build.
- LVGL bundling in Arduino-ESP32 static-link model: [ASSUMED] — standard Arduino-ESP32 behavior but not verified in this binary specifically.

---

## Metadata

**Confidence breakdown:**
- Standard stack (tools + versions): HIGH — GitHub API verified all release tags and assets
- Screen function addresses: HIGH — binary analysis confirmed
- LVGL geometry extraction patterns: HIGH — from LVGL v8.3 source
- FIDB build workflow: MEDIUM — Tarlogic article, not run against this specific IDF version this session
- LVGL version hypothesis (v8): MEDIUM — circumstantial (compile date + IDF version); confirmed to HIGH only after Signal 2 in Ghidra

**Research date:** 2026-06-17
**Valid until:** 2026-09-17 (Ghidra releases frequently; re-check loader compatibility before next Ghidra version)
