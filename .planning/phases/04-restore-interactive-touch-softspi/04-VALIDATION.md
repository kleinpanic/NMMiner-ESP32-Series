---
phase: 04-restore-interactive-touch-softspi
plan: 02
kind: validation
nyquist_compliant: true
build_gate: PENDING
build_command: "cd firmware/custom/NerdMiner-CYD-Custom && /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R"
build_result: PENDING
ram_used_pct: PENDING
flash_used_pct: PENDING
fork_commit: PENDING
date: PENDING
---

# Phase 4 Validation — Restore Interactive Touch (SoftSPI) (CYD)

This is the Phase 4 acceptance gate. On-device tap / long-press confirmation is **Phase 7**
(out of scope here). This phase validates that touch is restored at the BUS layer on
software SPI (D-42, freeing hardware VSPI for SD in Phase 6), wired into the Phase-3
`nm_uimanager` carousel, and that the firmware **compiles and links**, plus that a raw-Z
diagnosis path exists for the future on-device session (D-43).

> **Executor:** fill the `PENDING` frontmatter + the result blocks below from the final
> `pio run -e ESP32-2432S028R` (Plan 04-02 Task 3). Mirror the structure of
> `.planning/phases/03-rebuild-core-screens-uimanager-carousel/03-VALIDATION.md`.

## Gate A — Build SUCCESS (the phase gate)

```
cd firmware/custom/NerdMiner-CYD-Custom && \
  /home/broklein/codeWS/Projects/Devices/NMMiner-ESP32-Series/.venv/bin/pio run -e ESP32-2432S028R
```

Paste the tail of `/tmp/nm04_build_final.log` (RAM/Flash %, `[SUCCESS]`, the
`ESP32-2432S028R  SUCCESS` line). Note Flash headroom vs the 87.3% Phase-3 baseline.

Expected artifacts:
- `.pio/build/ESP32-2432S028R/firmware.bin`
- `.pio/build/ESP32-2432S028R/firmware.elf`

## Gate B — Wiring facts (grep, automated)

Run against `firmware/custom/NerdMiner-CYD-Custom/`:

| Fact (D-45 / TOUCH-02/03) | Check | Result |
|---|---|---|
| SoftSPI bus (no VSPI bind in nm_touch) — D-42 | `grep -q 'digitalRead(TOUCH_MISO' src/ui_nm/nm_touch.cpp && ! grep -q 'SPIClass\|#include <xpt2046' src/ui_nm/nm_touch.cpp` | PENDING |
| Touch enabled for CYD env — D-41 | `grep -v '^#' platformio.ini \| grep -A40 '\[env:ESP32-2432S028R\]' \| grep -q TOUCH_ENABLE` | PENDING |
| Short touch → next page | `grep -q 'nm_uimanager_next_page' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Long-press → config mode | `grep -q 'reset_configuration' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Tap → wake (re-ramp brightness) | `grep -q 'nm_display_wake' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Boot button → nm_ui (CYD) | `grep -q 'nm_uimanager_on_button_next\|nm_uimanager_on_touch' src/NerdMinerV2.ino.cpp` | PENDING |
| Auto-advance OFF by default | `grep -A3 'ifndef NM_UI_AUTOADVANCE' src/ui_nm/nm_uimanager.cpp \| grep -q 'NM_UI_AUTOADVANCE 0'` | PENDING |
| nm_touch is the sole ACTIVE CYD touch path | `grep -q 'nm_touch_init' src/NerdMinerV2.ino.cpp` (and legacy `touchHandler.isTouched()` not referenced for the CYD env) | PENDING |
| Shared long-press constant (no drift) | `grep -q 'NM_TOUCH_LONG_MS' src/ui_nm/nm_uimanager.h && grep -q 'setPressMs(NM_TOUCH_LONG_MS' src/NerdMinerV2.ino.cpp` | PENDING |

### Dormant legacy HSPI touch object (plan-check MEDIUM — must be addressed, not glossed)

`src/drivers/displays/esp23_2432s028r.cpp:30-31` instantiates `SPIClass hSPI(HSPI)` +
`TFT_eTouch<TFT_eSPI> touch(tft, ETOUCH_CS, 0xFF, hSPI)` and calls `touch.init()` /
`touch.setCalibration()` / `touch.getXY()`. This file is compiled under the CYD env macro, so
the object **links into the binary**. It is reached only inside `esp32_2432S028R_Init()` and the
cyclic-screen functions, which run via `currentDisplayDriver->init()` inside `initDisplay()`.
The CYD env **skips** `initDisplay()` (ino.cpp 120-125 calls `nm_uimanager_init()` instead), so
`touch.init()` **never executes** → HSPI is never bound at runtime → D-42 not violated.

**Confirm and record here:** (a) `initDisplay()` is not called on the CYD boot path; (b) no other
CYD-active code path calls `esp32_2432S028R_Init()` / `touch.*`. Scope the "single touch path"
claim to the **active runtime path** — do NOT assert "no HSPI/VSPI touch binding exists anywhere
in the binary" (the dormant object is still linked). Flag for the SD phase (6): any future call to
`esp32_2432S028R_Init()` would double-bind HSPI.

## Gate C — Raw-Z diagnosis path (D-43 gate evidence)

Document the `nm_touch_log_raw()` seam and the exact serial line shape a Phase-7 on-device
session will read to confirm **bus-before-calibration** (raw Z changes on press ⇒ bus OK ⇒
any remaining failure is calibration, not wiring — per PITFALLS #3). On-device tap
confirmation itself is explicitly **Phase 7** (gated on `/dev/ttyUSB0`).

- Serial seam: where `nm_touch_log_raw()` is called from (debug flag / loop), and a sample
  `Serial.printf` format showing raw Z / raw X / raw Y / mapped pixel.
- Calibration constants chosen (rotation 1, 320x240) and their source (RNT/marcelrv CYD range).

## Nyquist note

Every Plan 04-01/04-02 task carries an `<automated>` verify (grep sentinels + the
`pio run` build gate). No on-device test is required for this phase's gate; the runtime
raw-Z read is documented here as the Phase-7 on-device step.
