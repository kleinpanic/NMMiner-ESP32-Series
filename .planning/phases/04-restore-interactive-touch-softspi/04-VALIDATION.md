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
| SoftSPI bus (no VSPI bind) — D-42 | `grep -q 'digitalRead(TOUCH_MISO' src/ui_nm/nm_touch.cpp && ! grep -q 'SPIClass\|#include <xpt2046' src/ui_nm/nm_touch.cpp` | PENDING |
| Touch enabled for CYD env — D-41 | `grep -v '^#' platformio.ini \| grep -A40 '\[env:ESP32-2432S028R\]' \| grep -q TOUCH_ENABLE` | PENDING |
| Short touch → next page | `grep -q 'nm_uimanager_next_page' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Long-press → config mode | `grep -q 'reset_configuration' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Tap → wake (re-ramp brightness) | `grep -q 'nm_display_wake' src/ui_nm/nm_uimanager.cpp` | PENDING |
| Boot button → nm_ui (CYD) | `grep -q 'nm_uimanager_on_button_next\|nm_uimanager_on_touch' src/NerdMinerV2.ino.cpp` | PENDING |
| Auto-advance OFF by default | `grep -A3 'ifndef NM_UI_AUTOADVANCE' src/ui_nm/nm_uimanager.cpp \| grep -q 'NM_UI_AUTOADVANCE 0'` | PENDING |
| Single CYD touch path (no legacy VSPI handler) | `grep -q 'nm_touch_init' src/NerdMinerV2.ino.cpp` | PENDING |

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
