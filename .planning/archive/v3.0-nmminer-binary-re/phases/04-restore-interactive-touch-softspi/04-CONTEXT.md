# Phase 4: Restore Interactive Touch (SoftSPI) - Context

**Gathered:** 2026-06-18 (auto mode; grounded in fork scout + Phase 3 UIManager + research PITFALLS)
**Status:** Ready for planning

> The user confirmed the device's touch is DEAD on the current fork. Root cause found
> below. Phase 4 restores it on software SPI (freeing hardware VSPI for SD in Phase 6)
> and wires it into the Phase-3 UIManager carousel.

<domain>
## Phase Boundary

Make the CYD's XPT2046 touch work and drive the new NMMiner UIManager by touch +
boot button: short touch = next page, long-press (10s) = config mode, tap wakes a
sleeping/dimmed screen; boot button: double-click = next page, long-press = force
config, single-click = wake. Touch must run on **software/bit-bang SPI** on the
touch pins so the hardware VSPI peripheral stays free for SD (Phase 6). Build SUCCESS
+ a documented raw-Z diagnosis are the gate; on-device touch confirmation is Phase 7.
</domain>

<decisions>
## Implementation Decisions

### Root cause + enablement (D-41)
- **D-41:** The CYD env `[env:ESP32-2432S028R]` defines the touch PINS (`ETOUCH_CS=33`, `TOUCH_CLK=25`, `TOUCH_MISO=39`, `TOUCH_MOSI=32`) but does **NOT** define `TOUCH_ENABLE` (only `lilygoT_HMI.h` does), so the fork's `TouchHandler` (`#ifdef TOUCH_ENABLE`, uses `arduino_xpt2046_library`) is compiled OUT → touch disabled. Phase 4 enables touch for the CYD env. Note `T_IRQ` (penirq, GPIO36) — confirm it's wired (the fork's TouchHandler takes an irqPin); the existing pin set omits it, so default to polled pressure (no IRQ) if 36 isn't available.

### SoftSPI, not the VSPI peripheral (D-42 — the touch-before-SD bus lock)
- **D-42 (load-bearing):** the XPT2046 must be driven by a **software/bit-bang SPI** on CLK=25/MOSI=32/MISO=39/CS=33 — do NOT bind it to the hardware **VSPI** peripheral. Display already owns HSPI (TFT_eSPI on 13/14/15); SD will own hardware VSPI (5/18/19/23) in Phase 6. If touch grabbed VSPI (remapped to the touch pins), SD could not use VSPI. SoftSPI for touch leaves both hardware SPI peripherals correctly allocated (HSPI=display, VSPI=SD). This is the CYD dual-SPI fix from research PITFALLS #3. The XPT2046 is low-speed (~2.5MHz) so bit-bang is fine.

### Diagnose raw-Z before calibration (D-43)
- **D-43:** Per PITFALLS, first read **raw Z-pressure / raw X/Y** to confirm the bus/wiring works (separating "bus dead" from "calibration wrong"). Log raw values over serial. Only after raw reads are sane, apply the pixel calibration. Record the raw-Z diagnosis in the SUMMARY (the gate evidence, since on-device tap is Phase 7).

### Calibration (D-44)
- **D-44:** Map XPT2046 raw → pixel for rotation 1 (320×240). Use the known CYD ESP32-2432S028R calibration range as the starting point (RNT/marcelrv CYD examples); store calibration constants; expose for later tuning. Keep it simple — no on-screen calibration UI this phase.

### Wire into the UIManager (D-45)
- **D-45:** Replace the Phase-3 auto-advance with REAL touch + button nav into the existing `nm_uimanager` API:
  - **Short touch** → `nm_ui` next page (goto_page_ex with persist=true).
  - **Long-press (10s)** → enter config mode (the fork's WiFiManager/config path; trigger the same entry the boot long-press uses).
  - **Tap** → wake a sleeping/dimmed screen (re-ramp brightness; reset the idle timer). Idle→dim/sleep timer machinery can be minimal here (screensaver carousel is Phase 5).
  - **Boot button** (OneButton on `PIN_BUTTON_1` = GPIO0, already wired in NerdMinerV2.ino.cpp to switchToNextScreen/alternateScreenRotation): re-point for the CYD env to nm_ui — double-click = next page, long-press = force config, single-click = wake. Keep auto-advance available behind the `NM_UI_AUTOADVANCE` flag (default OFF now that touch drives nav).

### Claude's Discretion
- SoftSPI impl: a small bit-bang routine vs an existing soft-SPI lib vs the xpt2046 lib's software-SPI mode.
- Idle-timeout value for wake (align with stock screensaver timeout in Phase 5).
- Whether to keep the arduino_xpt2046_library or read the XPT2046 directly over the bit-bang bus.
</decisions>

<canonical_refs>
## Canonical References
- firmware/custom/NerdMiner-CYD-Custom/src/TouchHandler.{cpp,h} (the fork's XPT2046 handler, gated by TOUCH_ENABLE; uses arduino_xpt2046_library + a SPIClass — the thing to enable + move to SoftSPI).
- firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp (OneButton wiring @~37-99: button1 attachClick/attachDoubleClick — re-point to nm_ui for CYD).
- firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h (the carousel API: goto_page_ex/next/prev + NM_UI_AUTOADVANCE).
- firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.{cpp,h} (brightness ramp — reuse for wake).
- firmware/custom/NerdMiner-CYD-Custom/platformio.ini §[env:ESP32-2432S028R] (touch pins; add TOUCH_ENABLE + any SoftSPI flag).
- .planning/research/PITFALLS.md #3 (touch-on-SoftSPI before SD; diagnose raw-Z first).
- .planning/REQUIREMENTS.md — TOUCH-01/02/03.
</canonical_refs>

<code_context>
## Existing Code Insights
- The fork ALREADY has TouchHandler + OneButton + screen-switch callbacks — Phase 4 re-enables/re-wires them for the CYD env onto the new UIManager, rather than writing touch from scratch (reuse over rewrite).
- nm_uimanager exposes next/prev/goto — touch/button handlers call these.
- nm_display brightness ramp → reuse for tap-to-wake.
</code_context>

<specifics>
## Specific Ideas
- Touch dead = TOUCH_ENABLE not set for CYD (D-41) — likely the whole fix is enable + SoftSPI + calibrate + wire. Confirm via raw-Z that the panel responds before calibrating.
- Keep mining + the Phase-3 screens intact; touch is additive.
</specifics>

<deferred>
## Deferred Ideas
- Screensaver idle carousel + black mode + full idle state machine → Phase 5.
- SD on VSPI (the bus this phase frees) → Phase 6.
- On-device tap/long-press confirmation → Phase 7.
</deferred>

---
*Phase: 4-restore-interactive-touch-softspi*
*Context gathered: 2026-06-18*
