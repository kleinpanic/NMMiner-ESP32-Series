# Architecture Research

**Domain:** ESP32 (Xtensa LX6) closed-firmware reverse-engineering + custom LVGL miner firmware
**Researched:** 2026-06-17
**Confidence:** HIGH (claims backed by local binary recon of the actual 4 MB image + DROM/IROM extraction)

## Decision Up Front

**Recommended path: (B) ASSET-INFORMED REIMPLEMENT — rebuild NMMiner's exact screens as native LVGL on the working NerdMiner-CYD-Custom fork, using RE of the closed binary as the *specification* (layout, fonts, colors, screen list, market/screensaver behavior), not as a source of extractable image assets.**

This is a hybrid of the brief's (B) and (C): the brief's clean "asset-transplant" (B) assumes NMMiner's screens are stored as bitmaps that can be lifted out. **They are not.** Local extraction proves the screens are runtime-drawn LVGL widgets, so there are no PNG/JPG screen assets to transplant. What *is* recoverable is the *design contract* — screen inventory, fonts, layout geometry, market API shapes, screensaver format — which we reimplement natively on a base we control. Path (A) binary-patch is rejected as primary (kept as a last-resort spike). Pure (C) "reimplement from imagination" is rejected because we can and should drive it from concrete RE evidence.

## Evidence Gathered (this session, against the real image)

| Finding | Evidence | Implication |
|---|---|---|
| Screens are runtime LVGL widgets, **not** stored bitmaps | The 8 "recovered LVGL PNGs" in `hardware/recovered-assets/` are **noise** — raw byte regions mis-carved as RGB (verified by rendering `lvgl_167ed4_320x240.png` = static). The 2 JPGs have valid SOI/EOI but garbage SOF dimensions = false-positive carves. | **Asset-transplant (clean B) is infeasible.** No screen images exist to extract. The look lives in code. |
| ~787 KB free flash in app partition | `app_factory.bin` (3.67 MB): 135 runs of `0xFF`≥256 totalling 786,921 bytes; last non-FF byte at offset 3,666,176. | Path A code-cave is *technically* possible (room exists) — but see brick-risk below. |
| DROM exposes full internal contract | `strings drom.bin`: `LVGL task`, `Clock320x240`, market subsystem (`fetch_available_usdt_pairs`, `fetch_klines`, `refresh_watchlist`, `get_coin_ticker_24hr`, Binance `data-api.binance.vision` klines/ticker URLs), `ScreenOrient`/`ScreenSaver`/`ScrSaverMode`/`RotateScreen`, `#%d share accepted, %ldms`, `#%d share rejected`, screensaver upload format `saver_%u_%u_`. | The **spec is fully recoverable** from strings + disasm without lifting binaries. This is the real value of the RE foundation. |
| Share-accept hook point exists as a log site | DROM format string `#%d share accepted, %ldms` is referenced by IROM code (objdump available at `~/.platformio/packages/toolchain-xtensa-esp32/bin/xtensa-esp32-elf-objdump`). | If path A were pursued, this is the call site to hook for "beep on share." But we don't need to. |
| Working fork already has the hard parts done | `firmware/custom/NerdMiner-CYD-Custom`: LVGL screen modules (`ui_HomeScreen.c`, `ui_ClockScreen.c`, `ui_StatsScreen.c`, `ui_SettingsScreen.c`, `ui_SplashScreen.c`), plus already-built `speaker.cpp`, `sdstore.cpp`, `drivers/storage/SDCard.cpp`, custom web API, branding. v1.0.0 builds green, 73 tests @94% branch cov. | The transplant *target* exists and is LVGL-native, so reproducing NMMiner layouts is ordinary UI work, and speaker/SD/LED are **already implemented**. |

The single most important architectural fact: **NMMiner's "look" is not data, it is behavior.** It is the sum of LVGL widget construction calls (layout, fonts, colors, animations) executed at runtime. You cannot copy it as a file; you reproduce it as code. That collapses the strategic question from "extract assets" to "read the spec, rebuild on our base."

## Path Comparison

| Dimension | (A) Binary Patch | (B-clean) Asset Transplant | **(B*) Asset-Informed Reimplement [RECOMMENDED]** | (C) Blind Reimplement |
|---|---|---|---|---|
| Feasible on ESP32/Xtensa? | Yes, but fragile | **No** — no assets exist to transplant | **Yes** — LVGL-on-LVGL is native | Yes |
| Delivers original look? | Yes (it *is* the original) | N/A | High fidelity (driven by RE spec + glyph/color recovery) | Low (guesswork) |
| Speaker/SD/LED integration | Author Xtensa, find call sites blind, hope free flash reachable | n/a | **Already built in fork** (`speaker.cpp`, `sdstore.cpp`) — trivial | Same as B* |
| Effort | Very high | n/a | Moderate (UI rebuild) | Moderate |
| Brick risk | **High** — cache-line/flash-map errors, `0xFF` regions not contiguously executable, IROM/DROM offset math, license-check tripwires | n/a | **Low** — flash the fork, full 4 MB backup restores | Low |
| Maintainability | **Lost on every NMMiner update**; opaque hand-patched binary | n/a | **Owned MIT source**, version-controlled, testable | Owned source |
| License | Patching an unlicensed binary (legal grey) | n/a | MIT fork — clean | MIT |
| Mining stays functional | Yes (untouched core) | n/a | Yes (NerdMiner mining core) | Yes |

### Why not (A) Binary Patch
- The ~787 KB free flash is **fragmented** (135 separate runs); a code cave needs a contiguous, executable, correctly-mapped region. IROM executes from `0x400d0020`; injected code must be position-correct, cache-aligned, and reachable by a `call`/`j` from the patched site within Xtensa range. One offset error = boot loop.
- You would still author the speaker/SD logic in Xtensa or as relocatable C and stitch it in by hand — re-doing work the fork *already has in clean C++*.
- The closed binary is **unlicensed** (no LICENSE in `NMminer1024/NMMiner`); patching and redistributing it is legally murkier than shipping an MIT fork.
- Every upstream NMMiner release nukes the patch. Zero maintainability.
- Verdict: **fallback spike only**, time-boxed, used only if a *specific* NMMiner-exclusive behavior proves impossible to reproduce on the fork (none identified so far).

### Why not (B-clean) Asset Transplant as written
- The premise — "extract NMMiner's real LVGL screen assets/fonts/screensavers" — is falsified by this session's extraction: the screens aren't stored as assets. What's storable (screensavers) is the *user upload* format (`.ss`, RGB565+RLE), which we already decode (`scripts/nmminer_screensaver.py`), not the built-in screen art.

### Why (B*) Asset-Informed Reimplement wins
- LVGL→LVGL: NMMiner draws screens with LVGL; the fork draws screens with LVGL. Reproducing a layout is reading the disasm/strings for "what widgets, where, what font, what color" and writing the same `lv_obj`/`lv_label`/`lv_img` calls. Native, testable, ours.
- The hard custom features (speaker/LEDC, SD/VSPI, LED, branding) are **already implemented and tested** in the fork.
- Full 4 MB backup means flashing the fork is fully reversible — near-zero brick risk.

## Recommended Architecture

```
┌──────────────────────────────────────────────────────────────────────┐
│                    RE SPEC LAYER (read-only, from closed binary)       │
│  drom.bin / irom.bin  →  objdump + strings  →  DESIGN CONTRACT         │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐  │
│  │ Screen list  │ │ Font / glyph │ │ Layout geom. │ │ Market+SS    │  │
│  │ (Clock,Stats,│ │ (which LVGL  │ │ (x/y/w/h per │ │ API shapes   │  │
│  │  Market,...) │ │  font,colors)│ │  widget)     │ │ (Binance)    │  │
│  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘ └──────┬───────┘  │
└─────────┼────────────────┼────────────────┼────────────────┼─────────┘
          │ informs        │ informs        │ informs        │ informs
┌─────────▼────────────────▼────────────────▼────────────────▼─────────┐
│              CUSTOM FIRMWARE (NerdMiner-CYD-Custom, MIT, OURS)         │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │  LVGL UI layer (rebuilt to match NMMiner)                        │ │
│  │  ui_HomeScreen ui_ClockScreen ui_StatsScreen ui_MarketScreen*   │ │
│  │  ui_ScreensaverScreen*   ui_SplashScreen(kleinpanic branding)   │ │
│  └───────────────┬───────────────────────────────┬─────────────────┘ │
│  ┌───────────────▼─────────┐  ┌──────────────────▼──────────────────┐ │
│  │ Mining core (NerdMiner) │  │ Custom feature layer (ALREADY BUILT)│ │
│  │ stratum, hashing, shares│  │ speaker.cpp(LEDC GPIO26)            │ │
│  │   └─ share-accept event ─┼─▶│ sdstore/SDCard.cpp(VSPI 5/18/19/23) │ │
│  └─────────────────────────┘  │ LED, custom JSON web API           │ │
│                                └─────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────────────┤
│                    HARDWARE (ESP32-2432S028R, ST7789)                  │
│   TFT_eSPI(ST7789) · touch · LEDC buzzer · microSD(VSPI) · RGB LED     │
└──────────────────────────────────────────────────────────────────────┘
   * = new screen modules to add to the fork during this milestone
```

### Component Responsibilities

| Component | Responsibility | Implementation |
|---|---|---|
| RE Spec Layer | Produce the design contract: screen inventory, per-screen layout geometry, fonts/colors/glyphs, market + screensaver behavior | `objdump`/`strings` on `drom.bin`/`irom.bin`; write findings to `hardware/recovered-assets/SPEC.md` (markdown contract, not code) |
| LVGL UI layer | Reproduce NMMiner screens pixel-close on the fork | New `ui_*.c` modules following existing `ui_HomeScreen.c` pattern (SquareLine-style) |
| Mining core | Hashing, stratum, share accept/reject, emit share-accept event | NerdMiner upstream (unchanged) |
| Custom feature layer | Beep on share / alarm on disconnect; SD file I/O; LED; custom web API | `speaker.cpp`, `sdstore.cpp`, `SDCard.cpp` — **already exist in fork** |
| Branding | kleinpanic boot splash + accents over recovered UI | `ui_SplashScreen.c` + branding assets dir |

### Data Flow — share-accept → beep (the integration seam)

```
[stratum accepts share]
    ↓
mining core sets accepted-share counter / emits event
    ↓
UI tick reads counter delta  ──▶  ui_StatsScreen updates "shares" label (NMMiner look)
    ↓                                          │
speaker.cpp::beepOnShare()  ◀──────────────────┘   (LEDC tone, GPIO26)
    ↓
optional: sdstore logs share to SD (VSPI)
```

In the fork this is a normal in-process function call at the share-accept site — no Xtensa hooking, no call-site discovery. Contrast path A, where the same wiring requires finding the IROM site that emits `#%d share accepted, %ldms` and patching a `call` to injected cave code.

## Build Order (roadmap phase seed)

1. **RE-Spec the UI (read-only).** Disassemble the LVGL screen-construction functions; produce `SPEC.md`: screen list, per-screen widget geometry, fonts/colors, market screen fields, screensaver render path. Deliverable = the design contract, no flashing. *(This is where the existing RE foundation pays off.)*
2. **Recover fonts/glyphs + palette.** Identify which LVGL font(s) and color scheme NMMiner uses; reproduce or substitute closest match in the fork. (Fonts may be custom LVGL font arrays in IROM — extractable as C arrays, unlike the screens.)
3. **Rebuild core screens on the fork.** Clock + Stats/Home first (highest-value, simplest), matching the spec. Reuse existing `ui_*.c` scaffolding.
4. **Add Market + Screensaver screens.** Market reuses the recovered Binance API shapes; screensaver reuses our existing `.ss` decoder.
5. **Wire custom features (mostly done).** Confirm `speaker.cpp` beep-on-share at the NerdMiner share-accept site; SD logging; LED; kleinpanic splash.
6. **On-device validation.** Backup → flash fork → serial + visual checklist (fork `docs/FLASH_GUIDE.md`). **Blocked until `/dev/ttyUSB0` (USB DATA port) appears** — current blocker per STATE.md.
7. **(Fallback only) Path-A spike.** Only if step 1 reveals a NMMiner-exclusive behavior that cannot be reproduced on the fork — time-box a binary-patch proof against the free-flash cave; abandon if not clean.

## Anti-Patterns

### Anti-Pattern 1: Treating the recovered PNG/JPG carves as real screens
**What people do:** Assume `hardware/recovered-assets/lvgl_*.png` are NMMiner screens and try to display/transplant them.
**Why it's wrong:** They are noise (mis-carved byte regions); the JPGs are false-positive `0xFFD8/0xFFD9` matches with garbage dimensions. Verified this session.
**Do this instead:** Treat DROM/IROM as a *spec source* (strings + disasm), not an *asset source*. Recover layout/fonts/behavior; rebuild as LVGL code.

### Anti-Pattern 2: Binary-patching the closed image as the primary path
**What people do:** Inject Xtensa hooks into NMMiner to add speaker/SD.
**Why it's wrong:** High brick risk (fragmented non-contiguous free flash, offset/cache-alignment hazards, possible license tripwires), unlicensed-binary legal grey, and lost on every NMMiner update — all to re-implement features the MIT fork *already has in clean C++*.
**Do this instead:** Ship the fork; keep path A as a time-boxed last-resort spike only.

### Anti-Pattern 3: Reimplementing screens from memory/imagination (pure C)
**What people do:** Eyeball the device once, then write LVGL screens from vibes.
**Why it's wrong:** Drifts from the real look; wastes RE foundation already built.
**Do this instead:** Drive the rebuild from the disasm-derived `SPEC.md` (geometry, fonts, colors) so fidelity is evidence-based.

## Integration Points

| Boundary | Communication | Notes |
|---|---|---|
| RE Spec ↔ UI rebuild | Static `SPEC.md` contract | One-way; spec never linked into firmware |
| Mining core ↔ speaker/SD | In-process call at share-accept site | Native function call in fork — no Xtensa hooking |
| UI ↔ TFT_eSPI(ST7789) | LVGL flush callback | Fork must use ST7789 (not mainline ILI9341 default) — already flagged |
| Speaker ↔ HW | LEDC PWM, GPIO26 | Built (`speaker.cpp`); verify pin vs silkscreen |
| SD ↔ HW | VSPI SPI, GPIO5/18/19/23 | Built (`SDCard.cpp`); verify pins |
| Firmware ↔ dashboard | Custom JSON web API | `apps/nmminer-control` re-pointed (Phase 8 work) |

## Scaling Considerations
Single-device novelty miner — no horizontal scaling concern. The only "scale" axis is flash/RAM budget on the ESP32: LVGL screen count and font tables consume PSRAM/flash; keep the recovered-font tables and screen modules within the fork's existing partition budget (mining core + LVGL already fit in the NerdMiner CYD build).

## Sources

- Local binary recon (this session): `hardware/recovered-assets/{app_factory,drom,irom}.bin`; PNG/JPG carve verification; `0xFF`-run free-flash analysis; DROM string dump (market/screensaver/share-accept contract).
- `docs/firmware-customization-feasibility.md` (path tradeoffs, free-flash headroom, driver-link evidence).
- Fork inventory: `firmware/custom/NerdMiner-CYD-Custom/{lib/hansolov2/screens,src/speaker.cpp,src/sdstore.cpp,src/drivers/storage/SDCard.cpp}`.
- LVGL — screens are runtime-drawn widgets/C code, not stored bitmaps: https://github.com/lvgl/lvgl , https://docs.lvgl.io/master/index.html
- ESP32 Xtensa firmware patching feasibility/fragility: https://github.com/Matheus-Garbelini/esp32_firmware_patching_framework , https://github.com/BlackVS/ESP32-reversing , https://olof-astrand.medium.com/reverse-engineering-of-esp32-flash-dumps-with-ghidra-or-ida-pro-8c7c58871e68
- NMMiner repo (binary-only, no LICENSE): https://github.com/NMminer1024/NMMiner

---
*Architecture research for: ESP32 closed-firmware RE + custom LVGL miner firmware*
*Researched: 2026-06-17*
