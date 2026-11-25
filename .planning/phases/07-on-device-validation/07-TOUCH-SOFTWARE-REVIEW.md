---
status: physical_probe_next_warning_fixed_later
supersession_note: "WR-01 was later patched in source and included in the live custom app chain. Touch still remains physical-probe gated because no raw press/IRQ-low signal has been observed."
reviewed: 2026-06-19T01:16:29Z
files_reviewed:
  - AGENTS.md
  - .planning/debug/touch-no-press.md
  - .planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md
  - .planning/phases/07-on-device-validation/07-TOUCH-PROBE-RUNBOOK.md
  - .planning/phases/07-on-device-validation/07-VALIDATION.md
  - firmware/custom/NerdMiner-CYD-Custom/platformio.ini
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/include/sdstore.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/devices/device.h
  - firmware/custom/NerdMiner-CYD-Custom/src/drivers/devices/esp322432s028r.h
  - firmware/custom/NerdMiner-CYD-Custom/.pio/libdeps/ESP32-2432S028R/TFT_eTouch/TFT_eTouchBase.cpp
  - firmware/custom/NerdMiner-CYD-Custom/.pio/libdeps/ESP32-2432S028R/TFT_eTouch/TFT_eTouchBase.inl
  - firmware/custom/NerdMiner-CYD-Custom/.pio/libdeps/ESP32-2432S028R/TFT_eSPI/Extensions/Touch.cpp
  - /tmp/p7-serial-post-v2002-identity-20260619-002803.log
  - /tmp/p7-serial-after-user-report-verify-20260618-193931.log
  - /tmp/p7-serial-touch-direct-probe.log
  - /tmp/p7-serial-hspi-touch.log
findings:
  total: 1
  critical: 0
  warning: 1
  info: 0
---

# Phase 7 Touch Software Review

## Scope

Focused no-flash software review of the current Phase 7 ESP32-2432S028R touch failure. I read the requested planning docs, CYD build flags, active touch/display/UI source, relevant supporting call paths, local TFT_eTouch/TFT_eSPI touch code, and the requested `/tmp` serial logs only for high-level runtime facts. No hardware, serial, esptool, flash, upload, erase, or device-writing command was run.

> Supersession note, 2026-06-19: WR-01 was later patched and included in the live
> custom app chain. This does not change the touch diagnosis: current evidence is
> still no raw contact/no IRQ-low, so touch remains a physical XPT2046/FPC/PENIRQ
> probe problem before more touch-code tuning.

## Verdict

The current source evidence does not justify a critical touch "fix" patch before physical probing. The active `ESP32-2432S028R` env defines the ST7789 target, `TOUCH_ENABLE`, `NM_TOUCH_NO_VSPI`, and the expected XPT2046 pins `CS=33 CLK=25 MOSI=32 MISO=39 IRQ=36` in `platformio.ini:961-1004`. The global display/touch ownership is one `TFT_eSPI tft`, one `SPIClass hSPI(HSPI)`, and one `TFT_eTouch<TFT_eSPI> touch(..., hSPI)` in `esp23_2432s028r.cpp:36-39`; SD uses a separate VSPI instance and pins in `sdstore.cpp:6,27-31` and `sdstore.h:9-14`.

The live CYD setup calls `nm_uimanager_init()` and `nm_touch_init()` (`NerdMinerV2.ino.cpp:157-168`), and the loop calls `nm_touch_poll_gesture()` (`NerdMinerV2.ino.cpp:470-475`). `nm_touch_init()` avoids `TFT_eTouch::init()` and directly configures CS/IRQ/MISO plus HSPI (`nm_touch.cpp:121-139`), which is still necessary because the library base init mounts/formats SPIFFS on ESP32 (`TFT_eTouchBase.cpp:78-94`). The direct XPT2046 commands and pressure proxy in `nm_touch.cpp:42-51,147-178` align with TFT_eSPI's local XPT2046 pressure style (`Touch.cpp:103-118`) and are good enough to distinguish "contact exists" from "idle/no-contact."

The requested logs consistently show the active path running and failing below UI, gesture routing, rotation, and calibration: `irq=1`, `miso=0`, `directX=0`, `directY=4095`, low `directP`, `directPressed=0`, and `libPressed=0` recur in `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`, `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`, and `/tmp/p7-serial-touch-direct-probe.log`; `/tmp/p7-serial-hspi-touch.log` shows the earlier HSPI library reader also stayed unpressed. This pattern is still most consistent with absent physical pen/contact signal, wrong or unpopulated touch variant, disconnected/damaged FPC, controller power/routing fault, or pin mismatch that needs physical measurement.

## Findings

### WR-01: Legacy CYD display driver still polls touch from the monitor task

**Classification:** WARNING

**Files:** `firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp:206`, `firmware/custom/NerdMiner-CYD-Custom/src/mining.cpp:1279-1282`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp:163-165`, `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp:567-604`

**Issue:** The code comments claim the CYD loop's `nm_touch_poll_gesture()` is the sole active touch path, but `runMonitor()` still calls `doLedStuff(frame)` every monitor loop. `display.cpp::doLedStuff()` unconditionally dispatches to the CYD display driver's `esp32_2432S028R_DoLedStuff()`, which still calls `touch.getXY()` every 500 ms and mutates legacy cyclic-screen/backlight state. This is a real bus/input ownership defect: it leaves a second TFT_eTouch reader active on HSPI from a different task, contradicting the new LVGL/no-VSPI architecture and potentially contaminating future touch diagnostics once a physical press signal exists.

**Why it is not the root cause:** This does not explain the current no-contact signature by itself. A real press should still pull PENIRQ/GPIO36 low, and the direct HSPI probe in `nm_touch_log_raw()` repeatedly sees idle IRQ/high and low pressure. The defect should be patched when firmware is next changed, but it does not remove the need for physical XPT2046/FPC/PENIRQ probing.

**Fix:** Make `doLedStuff()` a CYD no-op, matching the existing CYD no-ops for `drawCurrentScreen()` and `animateCurrentScreen()`:

```cpp
void doLedStuff(unsigned long frame)
{
#if defined(ESP32_2432S028R) || defined(ESP32_2432S028_2USB)
  (void)frame;
  return;
#else
  currentDisplayDriver->doLedStuff(frame);
#endif
}
```

## Evidence Reviewed

- `platformio.ini:961-1004`: current default target is `ESP32-2432S028R`; ST7789 display flags, touch pins, `TOUCH_ENABLE`, and `NM_TOUCH_NO_VSPI` are set for the target unit.
- `esp23_2432s028r.cpp:36-39`: the touch library instance is bound to `hSPI(HSPI)`, not VSPI.
- `nm_touch.cpp:121-139`: current init uses direct pin setup, HSPI begin, calibration/measure settings, and avoids `TFT_eTouch::init()`.
- `TFT_eTouchBase.cpp:78-94`: confirms why calling library init would risk SPIFFS formatting.
- `nm_touch.cpp:147-178,275-299`: direct pressure calculation and raw logging emit the exact fields present in the serial evidence.
- `NerdMinerV2.ino.cpp:391-457,470-475`: gesture polling is live in the Arduino loop and routes through `nm_uimanager_on_touch()`.
- `nm_uimanager.cpp:233-256`: UI routing consumes short/long/wake gestures only after a press is detected; it cannot create raw contact.
- `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`: clean boot to recovery AP and repeated no-contact diagnostics from the active path.
- `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`: same recovery state and no-contact touch signature after non-destructive verification.
- `/tmp/p7-serial-touch-direct-probe.log`: direct probe repeatedly reports no press.
- `/tmp/p7-serial-hspi-touch.log`: earlier HSPI library path also reported no press.

## Recommended Next Action

Run the physical probe gate from `07-TOUCH-PROBE-RUNBOOK.md` before more threshold, rotation, calibration, or gesture-routing work: inspect/photograph the touch controller and FPC, probe PENIRQ/GPIO36 during a firm press, and confirm CS/CLK/MOSI/MISO activity at the controller during diagnostic polling.

Do not flash only for WR-01 before that physical gate. If a later diagnostic or normal firmware write is chosen after a fresh full 4 MB backup, include the `doLedStuff()` CYD no-op patch first so the new LVGL touch path is actually the only active software reader.
