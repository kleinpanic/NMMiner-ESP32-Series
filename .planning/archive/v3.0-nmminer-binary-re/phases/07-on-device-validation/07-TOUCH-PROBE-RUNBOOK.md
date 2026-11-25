# Phase 7 Touch Probe Runbook

**Status:** required before more touch code tuning
**Target:** `ESP32-2432S028R` / ST7789 / assumed XPT2046 touch path
**Current verified firmware:** custom decoded-spec app at `0x10000` plus decoded LittleFS assets at `0x310000`

## Why This Exists

The live board now verifies against the current custom ST7789 build and the decoded LittleFS image. Fresh read-only serial through 2026-06-19T17:33Z still reports no touch contact:

```text
irq=1 miso=0 directX=0 directY=4095 directPressed=0 libPressed=0
```

That is below LVGL, page routing, debounce, and calibration. More blind threshold or mapping edits are not justified until the physical touch controller path is identified.

## Guardrails

- Do not write firmware for this probe unless a new diagnostic patch is deliberately chosen.
- If any firmware write is chosen later, take a fresh full 4 MB backup first.
- Do not enter Wi-Fi passwords, wallet values, pool credentials, device licenses, or serial logs containing secrets into tracked docs.
- The AP join/auth popup is expected captive-portal behavior from WiFiManager in config mode; it does not prove stock firmware is live.

## Physical Checks

1. Photograph or visually inspect the back of the panel and note the touch controller marking if present. Expected family is `XPT2046` or a compatible clone such as `HR2046`.
2. Confirm the resistive touch FPC/ribbon is fully seated and not torn, folded sharply, or offset in the connector.
3. Confirm whether this exact board revision actually has a resistive touch controller populated. If no controller or no touch FPC exists, treat it as a no-touch variant and stop touch-code tuning.

## Electrical Probe Table

Use a firm press on the glass while probing. A DMM can prove slow IRQ changes; a logic analyzer is better for SPI activity.

| Signal | Expected Pin | Expected Idle | Expected During Firmware Poll / Press | Meaning If Not Seen |
|--------|--------------|---------------|----------------------------------------|---------------------|
| PENIRQ | GPIO36 | high | pulls low during firm press | If it never changes, suspect FPC/panel/controller/pin mismatch or no-touch variant. |
| CS | GPIO33 | high | pulses low during touch diagnostic reads | If absent, firmware/pin map is not reaching the controller. |
| CLK | GPIO25 | idle low-ish | clocks during diagnostic reads | If absent with CS pulses, bus setup/pin route is wrong. |
| MOSI | GPIO32 | idle low-ish | command bits during reads | If absent with CLK, bus setup/pin route is wrong. |
| MISO | GPIO39 | not stuck forever | toggles/responds when controller is selected | If stuck at 0 with CS/CLK/MOSI active, suspect unpowered/wrong/disconnected controller or wrong MISO route. |

## Decision Matrix

| Observation | Next Action |
|-------------|-------------|
| PENIRQ drops on press and MISO toggles | Keep hardware. Next code pass should tune pressure/calibration from raw corner samples. |
| PENIRQ drops but MISO stays stuck | Inspect CS/MISO route and controller power; consider a pin-map diagnostic build after fresh backup. |
| PENIRQ never drops | Inspect FPC/controller/population first. Do not keep changing LVGL/touch thresholds. |
| CS/CLK/MOSI never pulse | Recheck build flags and pin map; this would contradict current serial path expectations. |
| No touch controller or FPC is present | Record board as no-touch/different variant and remove touch as a validation requirement for this unit. |

## Evidence To Collect

- No-secret photo of the controller/FPC area or written controller marking.
- PENIRQ idle voltage and pressed voltage.
- Whether CS/CLK/MOSI/MISO activity appears during the serial diagnostic window.
- Fresh serial sample showing the same run while the physical probe is performed.

## Serial Summary Tool

After a physical probe capture, summarize the raw serial log locally and copy only derived counts into planning docs:

```bash
python3 scripts/phase7_serial_touch_summary.py /tmp/<serial-log>.log
```

Record the output counts for `touch_diagnostics`, `pressed_observations`, `irq_low_observations`, `panic_markers`, `reset_markers`, and `ap_config_markers`. Keep raw serial logs out of tracked files.

## Current Reference Evidence

- `/tmp/p7-verify-current-custom-app-after-user-report-20260618-193918.log`
- `/tmp/p7-verify-decoded-littlefs-after-user-report-20260618-193918.log`
- `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`
- `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`
- `/tmp/p7-serial-readonly-continuation-20260619-020602.log` (SHA-256 `ae240cbed25eb7588dfe05f69615401cc83524c992632ffbab0d48cd03a3a045`)
- `/tmp/p7-serial-passive-inline-20260619-173344.log` (SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`; 23 touch diagnostics, 0 pressed, 0 IRQ-low, 1 reset marker, 2 AP/config markers)
- `scripts/phase7_serial_touch_summary.py`
- `.planning/debug/touch-no-press.md`
- `.planning/phases/07-on-device-validation/07-TOUCH-SOFTWARE-REVIEW.md`
