# Stack Research

**Domain:** Reverse-engineering a closed ESP32 (Xtensa LX6) Arduino-ESP32/ESP-IDF v4.4.6 firmware binary — code/symbol recovery + LVGL asset extraction + filesystem unpacking.
**Researched:** 2026-06-17
**Confidence:** HIGH (Ghidra/loader/FIDB and filesystem facts verified against current repos + the actual 4 MB backup; LVGL specifics MEDIUM — see notes)

---

## TL;DR — What changed vs. the assumptions in the brief

Three corrections fell out of inspecting the actual backup, and they reshape the stack:

1. **The "spiffs" partition is NOT SPIFFS — it is LittleFS.** Partition table entry `spiffs` (type 1 / subtype 0x82) at `0x3e0000` size `0x10000` contains a filesystem whose magic at byte 8 is the ASCII string `littlefs`, first file `apidoc.html`. **`mkspiffs` will not read it.** Use **`mklittlefs` (pioarduino/Jason2866 fork)** or **`littlefs-python`**. This is the single most important tooling correction in this document.
2. **The `.ss` screensavers are NMMiner's own format, already reverse-engineered by this project** (`scripts/nmminer_screensaver.py`): magic `SN@` (`53 4E 40`), RGB565 → RLE → LZSS bit-pack → 8-byte `NS` header. They are **not** LVGL `.bin` files. No third-party tool extracts them; the project's own encoder/decoder is the tool.
3. **The "zero `lv_img_dsc_t` v8 matches" is a non-blocker.** The team has *already* extracted on-screen images to PNG/JPEG (`hardware/recovered-assets/lvgl_*_320x*.png`, `jpg_*.jpg`). The images are stored as NMMiner-managed asset blobs / LVGL C-arrays in DROM, not as scannable `lv_img_dsc_t` C-structs and not as LVGL file-format `.bin`. The remaining open question (v8 vs v9) is answered by **reading the LVGL code paths in Ghidra**, not by a struct scan — which is exactly what the new tooling below unlocks.

The genuine gap the new stack fills: **the project has only used `objdump` for code so far.** The decompiler + symbol-recovery layer (Ghidra + ESP32 flash loader + ESP-IDF FIDB) is the actual new capability for v3.0.

---

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **Ghidra** | **11.4.x** (latest; ≥11.0 required) | Disassembler + decompiler; primary RE workbench | Native **Xtensa** processor support landed in **Ghidra 11.0** — no third-party processor module needed. Free, scriptable (Java/Python-Jython + PyGhidra), and the entire ESP32 RE ecosystem (loaders, FIDB) targets it. JDK 21 already present on this box. |
| **ghidra-esp32-flash-loader** (dynacylabs fork) | **v12.0** (released 2025-12-18) | Ghidra extension: imports the 4 MB flash dump (or app partition) at correct DROM/IROM offsets, auto-loads ESP32 mask-ROM symbols + Espressif SVD peripheral registers | Most-maintained fork of tslater2006's loader. Sets the address map and ROM/peripheral annotations automatically, so you decompile against named ROM functions and named registers instead of bare addresses. Drop-in `.zip` install — no gradle build. |
| **esp32_image_parser** (tenable) | master (HEAD; no tagged releases) | CLI: parse partition table, and **`create_elf`** to wrap an app partition into an ELF with segment/section headers Ghidra/IDA can ingest | Gives an alternative ELF-import path and clean partition dumping. `create_elf` is the canonical way to get a symbol-bearing ELF skeleton from a raw app image. Pairs with the loader; use whichever import path annotates better for this image. |
| **ESP-IDF FIDB** (self-generated, ESP-IDF **v4.4.6**) | built locally | Ghidra Function ID database to auto-name stripped ESP-IDF/Arduino-core library functions in the binary | Per Tarlogic's method, FIDB recovers SDK function names in a stripped statically-linked image. **Must be built against the matching IDF (v4.4.6)** — no public prebuilt v4.4.x FIDB exists (threatrack repo has none for Xtensa/ESP-IDF). This is the highest-leverage symbol-recovery step. |
| **xtensa-esp32-elf-objdump** | toolchain @ `~/.platformio/packages/toolchain-xtensa-esp32` | Fast linear disassembly / sanity cross-check against Ghidra | Already verified working in this project. Keep as a second opinion and for quick `objdump -D` passes; not a replacement for the decompiler. |

### Supporting Libraries / Tools

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| **mklittlefs** (pioarduino/`mklittlefs_esp32`, or Jason2866 fork) | latest | Unpack the LittleFS "spiffs" partition (`-u <dir>`), list (`-l`) | **Primary** filesystem extractor for this image. Build params must match the partition: `-b 4096 -p 256 -s 0x10000`. |
| **littlefs-python** | ≥0.12 (PyPI) | Pure-Python LittleFS read/extract; CLI `extract`/`list` | Fallback / scripting path if `mklittlefs` block-size params mismatch. `pip install littlefs-python` into `.venv`. Easier to drive from Python than the C tool. |
| **esptool** | 5.3.0 (`~/.local/bin`), 5.2.0 (`.venv`) | Already-done flash read/backup; `image_info` to confirm segment load addresses | Already used. Use `esptool image_info hardware/recovered-assets/app_factory.bin` to re-confirm the DROM `0x3f400020` / IROM `0x400d0020` load addresses fed to Ghidra. |
| **Pillow (PIL)** | in `.venv` | LVGL/raw RGB565 ↔ PNG conversion; already drives `nmminer_screensaver.py` | Already in use for the recovered PNGs and `.ss` encoder. Reuse for any new raw-image dumps Ghidra locates. |
| **GhidraLinkerScript** (Tarlogic) | latest | Ghidra script: import function names + addresses from an ESP-IDF linker `.map`/linker-script | Optional booster. If a matching Arduino-ESP32 v4.4.6 build map can be produced, it imports names directly — complements FIDB. |
| **binwalk** | ≥3.x | Entropy/signature sweep over DROM for embedded JPEG/PNG/zlib blobs | Triage tool to locate compressed/embedded assets (the gzipped web UI, embedded JPEGs) before manual carving. |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| **OpenJDK 21** | Ghidra 11.x runtime | Already installed (`21.0.11`). Ghidra 11.x requires JDK 21. No action needed. |
| **PyGhidra / Ghidra Jython** | Scripting Ghidra headlessly for repeatable asset/string sweeps | Use headless (`analyzeHeadless`) for reproducible, committable analysis scripts rather than GUI-only work. |
| **`.venv`** (project) | Python sandbox for esptool/littlefs-python/Pillow | Keep all new Python tools in the existing project `.venv`; do not pollute system site-packages. |

---

## Installation

```bash
# --- Ghidra 11.4 (JDK 21 already present) ---
# Download from official GitHub releases (verify hash on the release page):
#   https://github.com/NationalSecurityAgency/ghidra/releases
cd ~/.local/srcs
curl -LO https://github.com/NationalSecurityAgency/ghidra/releases/download/<RELEASE_TAG>/ghidra_<VER>_PUBLIC_<DATE>.zip
unzip ghidra_*_PUBLIC_*.zip -d ~/.local/srcs/
# launch: ~/.local/srcs/ghidra_*_PUBLIC/ghidraRun

# --- ESP32 flash loader (dynacylabs fork, v12.0 prebuilt zip) ---
#   https://github.com/dynacylabs/ghidra-esp32-flash-loader/releases  (download v12.0 .zip)
# In Ghidra:  File -> Install Extensions... -> "+" -> select the downloaded zip -> restart Ghidra

# --- esp32_image_parser (Tenable) ---
git clone https://github.com/tenable/esp32_image_parser.git ~/.local/srcs/gitclones/esp32_image_parser
# (uses esptool/construct; install its requirements into .venv)

# --- LittleFS extraction (NOT mkspiffs) ---
# Option A: mklittlefs (C tool, matches Arduino/ESP-IDF layout)
git clone https://github.com/pioarduino/mklittlefs_esp32.git ~/.local/srcs/gitclones/mklittlefs_esp32
#   build per its README, then:
#   mklittlefs -u out_dir/ -b 4096 -p 256 -s 0x10000 hardware/recovered-assets/spiffs.bin
# Option B: littlefs-python (pure Python, scriptable)
.venv/bin/pip install littlefs-python

# --- supporting ---
.venv/bin/pip install binwalk pillow   # Pillow likely already present

# --- ESP-IDF v4.4.6 for FIDB generation (Tarlogic method) ---
mkdir -p ~/esp && cd ~/esp
git clone -b v4.4.6 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh esp32
# then compile example projects (-Os AND -O2 variants), and run Ghidra's
# FunctionID "Create New Empty FidDb" + "Populate FidDb from programs" over the
# resulting .a/.elf libraries to build the v4.4.6 FIDB. Apply it during analysis.
```

> Pin the exact Ghidra release tag/version at install time; `<RELEASE_TAG>`/`<VER>` are placeholders so this doc doesn't rot against a specific dot-release. Confirm ≥11.0 (for native Xtensa) and prefer the newest 11.4.x.

---

## Load-Address Map (feed these to the Ghidra loader)

Verified from the project's own segment isolation + the partition table in the 4 MB backup. The flash loader applies these automatically; listed here as the ground truth to confirm against.

| Region | Flash partition | Load (VMA) address | Source |
|--------|-----------------|--------------------|--------|
| **DROM** (rodata/constants — where LVGL assets/strings live) | app0 @ `0x010000` | **`0x3f400020`** | project-verified; `0x20` = 32-byte image-header offset |
| **IROM** (code) | app0 @ `0x010000` | **`0x400d0020`** | project-verified |
| Internal SRAM / IRAM, DRAM | (loaded segments) | per `esptool image_info` | confirm before annotating |
| ESP32 mask ROM | n/a | loader-supplied (ESP32-D0WD-V3) | auto-loaded by dynacylabs loader |

Partition table (from `hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin` @ `0x8000`):

```
nvs      type=1 sub=2   off=0x009000 size=0x005000
otadata  type=1 sub=0   off=0x00e000 size=0x002000
app0     type=0 sub=16  off=0x010000 size=0x3d0000   <- the firmware (DROM+IROM)
spiffs   type=1 sub=130 off=0x3e0000 size=0x010000   <- LittleFS, NOT SPIFFS
coredump type=1 sub=3   off=0x3f0000 size=0x010000
```

---

## LVGL version determination (v8 vs v9) — method, not a tool

The struct scan failed because NMMiner does not ship `lv_img_dsc_t` C-structs in a scannable layout. Determine the version from **code + strings**, using the new Ghidra stack:

1. **String pivot (cheap, do first):** grep DROM for `lvgl`, version strings (`8.`, `9.`), and symbol/format names. v8 uses `LV_IMG_CF_*` (e.g. `LV_IMG_CF_TRUE_COLOR`, `..._TRUE_COLOR_ALPHA`); v9 renamed these to `LV_COLOR_FORMAT_*` (e.g. `LV_COLOR_FORMAT_RGB565`, `LV_COLOR_FORMAT_RGB565A8`). Presence of either family is a strong version tell. The project's `lvgl_color_buf` string already confirms LVGL; look adjacent.
2. **FIDB + decompiler (authoritative):** once the ESP-IDF v4.4.6 FIDB and LVGL are named in Ghidra, the signatures of `lv_img_decoder_*` / `lv_image_decoder_*` and the `lv_img_header_t`/`lv_image_header_t` field access pattern disambiguate v8 vs v9 directly (v9 renamed `lv_img*` → `lv_image*`).
3. **Header-layout check:** v8 `lv_img_header_t` is a 4-byte bitfield — `cf` (5b), `always_zero` (3b), `reserved` (2b), `w` (11b), `h` (11b). v8 `cf` for RGB565 true-color = `LV_IMG_CF_TRUE_COLOR` (4), true-color+alpha = `..._TRUE_COLOR_ALPHA` (5). If you find blobs prefixed by this 4-byte header it's v8; v9's `lv_image_header_t` is larger (adds `magic`, `flags`, `stride`).

**Practical note:** Arduino-ESP32 builds from Oct 2023 on IDF v4.4.6 almost certainly bundle **LVGL v8.x** (v9.0 released Jan 2024, after this binary's compile date). Treat **v8** as the working hypothesis; confirm via step 2. For extraction, v8 true-color = raw RGB565 little-endian rows; true-color-alpha = RGB565 + trailing 8-bit alpha plane — both already handled by the project's RGB565 path in `nmminer_screensaver.py`/`png_to_rgb565.py`.

---

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| Ghidra 11.4 (native Xtensa) | IDA Pro + Xtensa plugin | Only if a paid IDA license is already on hand and preferred; Ghidra is free and the ESP32 loader/FIDB ecosystem is Ghidra-first. |
| Ghidra 11.x native Xtensa | yath/Ebiroll **ghidra-xtensa** processor module | Only on Ghidra <11.0. Not needed here — install 11.x. |
| dynacylabs flash loader v12.0 | tslater2006 original (no releases, Ghidra 9.1-era) | Only for reference; the fork supersedes it with releases + SVD + ROM symbols. |
| Self-built ESP-IDF v4.4.6 FIDB | threatrack prebuilt FIDB repo | Never for this target — that repo has **no** Xtensa/ESP-IDF databases. Self-build is mandatory. |
| mklittlefs / littlefs-python | mkspiffs | **Never** for this image — the partition is LittleFS; mkspiffs cannot read it. mkspiffs only applies if a true SPIFFS image surfaces elsewhere. |
| esp32_image_parser `create_elf` | manual `objcopy` ELF wrapping | Use manual only if the parser chokes on this specific image header; parser is the documented happy path. |

---

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **mkspiffs** | The "spiffs" partition is LittleFS (magic `littlefs` @ byte 8). mkspiffs will fail or emit garbage. | **mklittlefs** (`-b 4096 -p 256 -s 0x10000`) or **littlefs-python** |
| **Third-party Xtensa processor module on Ghidra ≥11** | Conflicts with / duplicates native support, can break disassembly | Ghidra 11.x built-in Xtensa |
| **Generic LVGL image converters / `LVGLImage.py` to *decode* the `.ss` files** | `.ss` is NMMiner's proprietary `SN@` RLE+LZSS format, not an LVGL file | The project's own `scripts/nmminer_screensaver.py` (extend it with a decoder) |
| **`lv_img_dsc_t` struct-signature scan as the LVGL-version oracle** | Already returned zero matches; assets aren't stored that way | Code/decompiler analysis (FIDB-named `lv_image_decoder_*`) + string family check |
| **Prebuilt FIDB for "ESP32"** from generic repos | None match ESP-IDF v4.4.6 / Xtensa; mismatched FIDB yields false-positive names | FIDB compiled against ESP-IDF **v4.4.6** |
| `git add -A` of `~/esp` IDF tree or Ghidra projects into this repo | Huge, non-source, secret-adjacent | Keep IDF + FIDB build artifacts outside git; commit only the small generated `.fidb` if desired, or a build script |

---

## Stack Patterns by Variant

**If the goal is code/behavior recovery (render path, share-accept, market, screensaver logic):**
- Ghidra 11.4 + dynacylabs loader v12.0 + ESP-IDF v4.4.6 FIDB, importing `app_factory.bin`/full backup at the verified DROM/IROM addresses.
- Because objdump alone can't recover control flow or name SDK calls; the decompiler + FIDB do.

**If the goal is asset extraction (images, web UI, screensavers):**
- LittleFS partition → `mklittlefs -u` / `littlefs-python` for the gzipped web UI + on-device files.
- DROM-embedded images → `binwalk` triage + Ghidra-located pointers → Pillow RGB565→PNG (extend the existing PNG/JPEG recovery already in `hardware/recovered-assets/`).
- `.ss` screensavers → extend `scripts/nmminer_screensaver.py` with the inverse (decode) path; no external tool.

**If `mklittlefs` block params mismatch the on-flash geometry:**
- Switch to `littlefs-python`, which lets you set block/read/prog sizes explicitly and inspect errors in Python.

---

## Version Compatibility

| Component | Compatible With | Notes |
|-----------|-----------------|-------|
| Ghidra 11.x | OpenJDK **21** | JDK 21 confirmed installed (`21.0.11`). JDK 17 will NOT run Ghidra 11.x. |
| dynacylabs flash loader v12.0 | Ghidra 11.x | Match loader release to the Ghidra version; rebuild/redownload if you bump Ghidra. |
| ESP-IDF FIDB | the binary's **v4.4.6** | FIDB built against any other IDF version degrades hit-rate and risks mis-naming. |
| mklittlefs params | partition geometry `b=4096 p=256 s=0x10000` | Wrong block/page size silently mis-parses LittleFS. Confirm against the 64 KB partition. |
| LVGL extraction assumption | LVGL **v8.x** (working hypothesis) | Confirm in Ghidra before committing an extractor to a format. v9 only if code shows `lv_image_*`/`LV_COLOR_FORMAT_*`. |

---

## Sources

- [NationalSecurityAgency/ghidra releases](https://github.com/NationalSecurityAgency/ghidra) — native Xtensa support since **11.0**; latest 11.4.x. HIGH.
- [dynacylabs/ghidra-esp32-flash-loader](https://github.com/dynacylabs/ghidra-esp32-flash-loader) — v12.0 (2025-12-18); loads ROM symbols + Espressif SVD; drop-in zip install. HIGH.
- [tslater2006/esp32_flash_loader](https://github.com/tslater2006/esp32_flash_loader) — original loader (Ghidra 9.1-era, no releases). Reference. MEDIUM.
- [tenable/esp32_image_parser](https://github.com/tenable/esp32_image_parser) — `create_elf` workflow for ELF-from-flash. HIGH.
- [Tarlogic: Function Identification in ESP32 Firmware Using Ghidra FIDB](https://www.tarlogic.com/blog/esp32-firmware-using-ghidra-fidb/) — FIDB must match ESP-IDF version; compile example projects (-Os/-O2); GhidraLinkerScript. HIGH.
- [threatrack/ghidra-fidb-repo](https://github.com/threatrack/ghidra-fidb-repo) — confirms **no** Xtensa/ESP-IDF prebuilt FIDB exists (self-build required). HIGH.
- [BlackVS/ESP32-reversing](https://github.com/BlackVS/ESP32-reversing) — curated ESP32 RE resource index. MEDIUM.
- [Olof Astrand — RE of ESP32 flash dumps with Ghidra/IDA](https://olof-astrand.medium.com/reverse-engineering-of-esp32-flash-dumps-with-ghidra-or-ida-pro-8c7c58871e68) — end-to-end ELF + Ghidra workflow, `create_elf` example. MEDIUM.
- [pioarduino/mklittlefs_esp32](https://github.com/pioarduino/mklittlefs_esp32) + [Jason2866/mklittlefs_esp32](https://github.com/Jason2866/mklittlefs_esp32) — LittleFS pack/unpack. HIGH.
- [littlefs-python (PyPI)](https://pypi.org/project/littlefs-python/) — pure-Python LittleFS extract/list CLI. HIGH.
- [igrr/mkspiffs](https://github.com/igrr/mkspiffs) — SPIFFS (rejected: partition is LittleFS). HIGH.
- [LVGL v8.3 image docs](https://lvgl.io/docs/open/8.3/overview/image) + [LVGL color-format changes v8→v9](https://docs.lvgl.io/9.5/main-modules/images/color_formats.html) — `LV_IMG_CF_*` → `LV_COLOR_FORMAT_*` rename; header layout. MEDIUM.
- **Project ground truth** — partition table parse + `littlefs` magic + `SN@` `.ss` header + recovered `lvgl_*.png` / `jpg_*.jpg`, all read from `hardware/` in this repo. HIGH.

---
*Stack research for: ESP32 closed-firmware reverse engineering (Xtensa LX6 / Arduino-ESP32 / ESP-IDF v4.4.6 / LVGL)*
*Researched: 2026-06-17*
