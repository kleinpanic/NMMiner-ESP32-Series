# Phase 7 — On-Device Validation Runbook & Evidence

**Phase:** 07-on-device-validation · **Plan:** 07-01 · **Requirements:** VALID-01, VALID-02, VALID-03
**Status:** PENDING (populated incrementally across T1–T7 during execution)

> Evidence-first flash runbook. The executor fills each section below with live output as it
> walks the plan. This file IS the proof artifact for VALID-01/02/03.

## Device & artifact facts (locked at plan time)

| Item | Value |
|------|-------|
| Port | `/dev/ttyUSB0` (CH340) |
| Chip | ESP32-D0WD-V3 rev 3.1 |
| esptool | `firmware/custom/NerdMiner-CYD-Custom/.venv/bin/pio pkg exec -- esptool` (v4.5.1) |
| pio | `firmware/custom/NerdMiner-CYD-Custom/.venv/bin/pio` |
| Env | `ESP32-2432S028R` (huge_app, upload 921600, monitor 115200) |
| firmware.bin | `.pio/build/ESP32-2432S028R/firmware.bin` — 2,852,928 B, fork `9a476e7` |
| Offsets | bootloader@0x1000, partitions@0x8000, boot_app0@0xe000, firmware@0x10000 |
| Backup dir | `hardware/backups/` (parent-repo root, gitignored `.gitignore:8`) |

---

## T1 — Pre-flight probe + USER GO-AHEAD GATE (D-70, VALID-01)

- [ ] `/dev/ttyUSB0` present
- [ ] Read-only `chip_id` reports ESP32-D0WD-V3
- [ ] **User go-ahead captured (verbatim + UTC timestamp):**

```
<go-ahead text + timestamp here>
```

---

## T2 — Fresh 4 MB backup (D-71, VALID-01)

| Field | Value |
|-------|-------|
| Backup file | `hardware/backups/cyd-backup-<UTC-ts>.bin` |
| Size (bytes) | `<expect 4194304>` |
| sha256 | `<recorded>` |
| Git status | `<not staged — confirmed>` |

---

## T3 — Flash validated build (D-72, VALID-02)

- [ ] No rebuild (tree unchanged) / or rebuild reason recorded
- [ ] Upload reports "Hash of data verified" / SUCCESS

```
<pio upload log excerpt: /tmp/p7-upload.log>
```

---

## T4 — verify_flash + hard USB power-cycle (D-73, VALID-01)

- [ ] verify_flash OK @ 0x1000 (bootloader)
- [ ] verify_flash OK @ 0x8000 (partitions)
- [ ] verify_flash OK @ 0x10000 (firmware)
- [ ] **HARD physical USB unplug/replug confirmed by user** (soft reset NOT sufficient)

```
<verify_flash output: /tmp/p7-verify.log>
```

---

## T5 — Serial boot + visual parity + mining-active (D-74, VALID-02)

**Serial boot excerpt** (`/tmp/p7-serial.log`):

```
<clean boot, WiFi join, pool connect>
```

**FLASH_GUIDE §5 checklist** (pass/fail):

| # | Item | Result |
|---|------|--------|
| 1 | 6 screens render, CORRECT (non-inverted) colors | |
| 2 | WiFi join + pool connect (boot chime) | |
| 3 | Hashrate > 0 | |
| 4 | Accepted share → short beep (HW-01) + green LED (HW-03) | |
| 5 | Pull WiFi/pool → alarm tone | |
| 6 | `/api/system/info` stock-shape JSON (DATA-05) + `/api/status` sd/speaker blocks | |
| 7 | `POST /api/speaker/test-tone` → audible (HW-01) | |
| 8 | microSD → `/api/sd/list` lists + CSV log grows (HW-02) | |
| 9 | Dashboard (apps/nmminer-control) SD + speaker panels live | |

**Color verdict:** `<correct → skip T6 | wrong/inverted → run T6>`

---

## T6 — ST7789 driver branch (D-75, CONDITIONAL)

`<T6 SKIPPED — T5 colors correct, ILI9341 matches panel>`
— OR —
| Field | Before | After |
|-------|--------|-------|
| Driver flag | `-DILI9341_2_DRIVER` | `-DST7789_DRIVER` |
| Invert/offset flags | | |
| Rebuild | | SUCCESS |
| Reflash (T3) re-run | | |
| T5 re-verify colors | | correct |

---

## T7 — Restore reversibility (D-76, VALID-03)

- [ ] Backup integrity re-proven: size 4194304 + sha256 matches T2
- [ ] One-command restore documented:

```bash
firmware/custom/NerdMiner-CYD-Custom/.venv/bin/pio pkg exec -- esptool --chip esp32 \
  --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 hardware/backups/cyd-backup-<UTC-ts>.bin
```

- [ ] Full round-trip: `<run (opt-in) | not run — default per D-76>`

---

## Requirement verdicts

| Req | Verdict | Evidence |
|-----|---------|----------|
| VALID-01 | PENDING | T2 backup + T4 verify_flash + power-cycle |
| VALID-02 | PENDING | T3 flash + T5 boot/visual/mining |
| VALID-03 | PENDING | T7 restore proof |
