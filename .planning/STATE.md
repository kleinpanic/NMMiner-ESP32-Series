---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: PanicMiner — owned open firmware
current_phase: 1
current_phase_name: Base + See
status: executing
last_updated: "2026-11-24T21:20:00-05:00"
progress:
  total_phases: 5
  completed_phases: 0
---

# State

Current Phase: 1 — Base + See
Milestone: v4.0 PanicMiner. v3.0 closed as superseded on 2026-11-24
(`archive/v3.0-nmminer-binary-re/README.md`).

## Device

- Live firmware at 2026-11-24 21:16: the archived NMMiner-clone fork, setup AP
  mode, no Wi-Fi credentials in NVS. SD config present without Wi-Fi.
- Latest full backup: `hardware/backups/cyd-backup-20261124-211638-pre-v4-panicminer.bin`
  (checksum sidecar beside it). Restore:
  `.venv/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 <backup>`.
- esptool 4.8.1 lives in `.venv`.

## Repos

- Parent repo has no remote. Laptop disk is the only copy.
- Fork `firmware/custom/NerdMiner-CYD-Custom`: work branch `v4/panicminer` from `61aae97`.
  Old clone stack preserved on `origin/archive/nmminer-clone-lvgl` (`9777259`).

## Open questions

- Panel driver: base uses `ILI9341_2_DRIVER`, stock identity says ST7789. Settle with a photo.
- Touch: XPT2046 answers with no-press readings. Needs a press test.
- Open-core hashrate on this unit: not yet measured.
