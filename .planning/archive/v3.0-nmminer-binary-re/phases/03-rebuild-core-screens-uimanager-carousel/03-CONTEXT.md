# Phase 3: Rebuild Core Screens + UIManager Carousel - Context

**Gathered:** 2026-06-18 (auto mode — standing autonomy; decisions grounded in Phase 1 SPEC.md + Phase 2 assets + fork scout)
**Status:** Ready for planning

> THIS is the phase that fixes the user's device. The unit currently boots the fork
> but the screen is black + touch dead. Phase 3 produces the firmware with the
> rebuilt stock-NMMiner-look screens on a working display init. (Flash = Phase 7,
> now unblocked — `/dev/ttyUSB0` is present.)

<domain>
## Phase Boundary

Rebuild the stock NMMiner v2.0.02 **structural UI identity** natively as LVGL v8 on
the `NerdMiner-CYD-Custom` fork, driven by `hardware/recovered-assets/SPEC.md`
(geometry) + Phase 2 assets (`recovered/fonts/recovered_fonts.h`,
`recovered/nmminer_palette.h`): the **6 named screen classes** + a **UIManager**
page carousel + **LastPage NVS persistence** + a **kleinpanic-branded Loading
splash**. Loading/Miner/Clock/Config are fully built this phase; SPrice/Weather are
registered as page stubs (their data-driven content is Phase 5). Build target is the
CYD env; **build SUCCESS is the phase gate** (on-device flash is Phase 7).
</domain>

<decisions>
## Implementation Decisions

### Build base + target (D-31)
- **D-31:** Build on the fork at `firmware/custom/NerdMiner-CYD-Custom`, env **`ESP32-2432S028R`** (`pio run -e ESP32-2432S028R`; `-D ESP32_2432S028R=1`; ST7789 320x240 via TFT_eSPI). Keep mining (`src/mining.cpp`, stratum) untouched. Commits go INSIDE the fork's nested git repo (per Phase 2 pattern); orchestrator bumps the parent gitlink.

### Replace the disliked hansolov2 UI with NMMiner-look screens (D-32)
- **D-32:** The fork currently renders the SquareLine `hansolov2` set (`lib/hansolov2/screens/ui_{Home,Stats,Clock,Splash,Settings}Screen.c`) — the look the user rejected. Phase 3 builds NEW NMMiner-look screen modules (e.g. `src/ui_nm/ui_{Loading,Miner,Clock,Config}.c` or under the fork's recovered/ area) reproducing SPEC.md geometry, and routes boot to the new UIManager instead of the hansolov2 `ui_init()`. Do NOT try to restyle hansolov2 in place — build the stock layout fresh from SPEC. Keep hansolov2 in the tree (don't delete) but stop driving it, to minimize blast radius.

### Screen scope this phase (D-33)
- **D-33:** Fully build **Loading320x240, Miner320x240, Clock320x240, Config320x240** from their SPEC.md widget tables (exact x/y/align/width/font/color per row). Register **SPrice320x240 + Weather320x240** as UIManager pages but as minimal stubs (title + "loading…") — Phase 5 fills market/weather data. All 6 must exist in the page list (structural identity = UI-02).

### UIManager + persistence (D-34)
- **D-34:** Implement a `UIManager` page list (array of the 6 screen builders) with `goto_page(index)` carousel + `LastPage` persisted to NVS (Preferences), restored on boot (resume on last page — matches stock "Resumes on last page"). Touch/button nav to drive it is **Phase 4**; Phase 3 provides the page machinery + a programmatic/auto advance so screens are verifiable without touch.

### Display-render fix (D-35) — the black-screen root cause
- **D-35:** Ensure the ST7789 actually renders on boot: correct init (SPI MODE3), **brightness ramp on boot** (stock readme v1.8.20 — avoids ST7789 distortion; a brightness-0 or stuck-screensaver state is the likely black-screen cause). The Loading screen must paint within the first seconds of boot. This is the concrete fix for the user's black display. (Touch is Phase 4; this phase = pixels on screen.)

### Fonts/palette/colors (D-36)
- **D-36:** Use Phase 2 outputs: `#include "recovered/fonts/recovered_fonts.h"` (Montserrat 12/14/16/22/24/26 built-ins + FA subset) and `recovered/nmminer_palette.h` (bit-replicated #hex). Confirm the **Clock price up/down colors** (the Phase 2 tentative `0xf01d` red) by decompiling `FUN_400f24ac` (already dumped at `hardware/recovered-assets/decompiled/vtables/Clock_priceColor_*.c`) — resolve green-up/red-down exact constants here (Phase 3 SC #4).

### Branding (D-37)
- **D-37:** kleinpanic branding layered on the Loading splash + accents (per goal), over the recovered NMMiner look — additive, doesn't break stock parity (keep "NMMiner"/"Make it better" identity strings so controllers keying off `model` still work).

### Claude's Discretion
- New UI module directory/naming within the fork.
- Whether the auto-advance for verification is a debug timer or a build flag.
- Exact LVGL screen-object lifecycle (lazy-create vs all-at-boot) — prefer lazy per-page create to fit RAM.
</decisions>

<canonical_refs>
## Canonical References
- `hardware/recovered-assets/SPEC.md` (gitignored) — per-screen widget geometry tables (the build contract) + LVGL API map + Clock price-color note.
- `firmware/custom/NerdMiner-CYD-Custom/recovered/fonts/recovered_fonts.h`, `recovered/nmminer_palette.h` (Phase 2 — fonts + palette to #include).
- `firmware/custom/NerdMiner-CYD-Custom/platformio.ini` §`[env:ESP32-2432S028R]` — build env.
- `firmware/custom/NerdMiner-CYD-Custom/lib/hansolov2/` — the LVGL v8 UI lib + the to-be-replaced screens (pattern reference for LVGL-on-this-fork, e.g. `ui_helpers.c`).
- `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/` — display driver (ST7789) for the render-fix.
- `.planning/REQUIREMENTS.md` — UI-01..UI-06.
- `docs/research/nmminer-api-reference.md` — Miner stat-field formatting to match.
</canonical_refs>

<code_context>
## Existing Code Insights
- Fork is LVGL v8 (hansolov2) on TFT_eSPI — same LVGL major as the stock RE (RE-02=v8), so recovered geometry/fonts drop in directly.
- `lib/hansolov2/ui_helpers.c` shows the fork's LVGL object-creation idiom to follow.
- `src/mining.cpp` / stratum run independently — UI work must not touch them (keep mining alive = HW-04, validated later).
</code_context>

<specifics>
## Specific Ideas
- First visible win = Loading + Miner rendering on the CYD; prioritize getting pixels on screen (D-35) before polishing all 6.
- Build must succeed for `ESP32-2432S028R`; that is the Phase 3 acceptance gate (flash is Phase 7).
</specifics>

<deferred>
## Deferred Ideas
- Touch/button navigation → Phase 4 (Phase 3 provides page machinery + auto-advance only).
- SPrice/Weather data (Binance/open-meteo), screensaver carousel, swarm, full HTTP API → Phase 5.
- Speaker/SD/LED wiring → Phase 6. On-device flash + visual parity → Phase 7.
</deferred>

---
*Phase: 3-rebuild-core-screens-uimanager-carousel*
*Context gathered: 2026-06-18*
