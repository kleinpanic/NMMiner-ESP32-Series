# Phase 7: On-Device Validation - Context

**Gathered:** 2026-06-18 (auto mode; grounded in the existing fork FLASH_GUIDE.md + live device probe)
**Status:** Ready for planning. Device PRESENT and reachable — ESP32-D0WD-V3 (rev 3.1), MAC d4:e9:f4:a7:de:0c, CH340 on /dev/ttyUSB0. The "BLOCKED on /dev/ttyUSB0" roadmap label is STALE; the port is present.

<domain>
## Phase Boundary

Flash the validated v3.0 fork firmware (fork HEAD `9a476e7`, firmware.bin 2,852,928 B,
flash 90.5%) onto the physical CYD and prove it works: a fresh 4 MB esptool backup BEFORE
the flash, `verify_flash` + a hard USB power-cycle AFTER, a serial boot confirmation, a
visual parity checklist against stock, mining confirmed active on-device, and a documented +
proven-reversible restore path. This is the ONLY phase that touches the user's physical,
currently-mining hardware. Closes VALID-01/02/03.
</domain>

<decisions>
## Implementation Decisions

### USER GO-AHEAD GATE before any write (D-70 — load-bearing)
- **D-70:** The device is the user's PHYSICAL miner, actively mining the OLD fork right now.
  Even the *backup* (esptool read_flash) resets the chip into download mode and interrupts
  mining for ~45–60s before it reboots. The user previously chose "leave the device, keep
  building." Therefore: ALL non-destructive prep (backup, build confirm, runbook, restore-
  validation plan) is autonomous, but the **actual flash write requires the user's explicit
  "flash it" go-ahead.** Do not write to the device without it. The backup is the first
  irreversible-state-touch (brief mining interruption) and is bundled into the same go-ahead.

### Fresh 4 MB backup BEFORE flash (D-71 → VALID-01)
- **D-71:** Immediately before flashing, take a fresh full-flash backup:
  `esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 read_flash 0x0 0x400000 hardware/backups/cyd-backup-<UTC-ts>.bin`,
  then `sha256sum` it and assert size == 4194304 bytes. A prior backup exists
  (`hardware/backups/cyd-backup-20260617-172106.bin`, 4 MB) + a 2026-06-01 full-flash — but
  the directive mandates a FRESH backup per flash, so make a new one. NEVER commit the .bin
  (gitignored — it is a dump of the closed stock image; no redistribution, D-11/RE-04).

### Flash the validated build (D-72 → VALID-02)
- **D-72:** Flash via PlatformIO so the 3 images land at the right offsets:
  `pio run -e ESP32-2432S028R -t upload --upload-port /dev/ttyUSB0` (bootloader@0x1000,
  partitions@0x8000, boot_app0@0xe000, firmware@0x10000). Do NOT rebuild first unless the
  tree changed — flash the exact validated artifact (firmware.bin built 12:11, fork 9a476e7).
  upload_speed 921600.

### verify_flash + hard power-cycle AFTER (D-73 → VALID-01)
- **D-73:** After upload, `esptool ... verify_flash 0x10000 .pio/build/ESP32-2432S028R/firmware.bin`
  (and the bootloader/partition offsets) to prove the write matched. Then a HARD USB
  power-cycle (physical unplug/replug — a user action; a soft reset is NOT sufficient per the
  directive). Document both as runbook steps with expected output.

### Serial boot + visual parity + mining-active confirm (D-74 → VALID-02)
- **D-74:** After power-cycle, capture the serial boot log @115200 (`pio device monitor` or
  esptool/cat) — confirm clean boot (no boot-loop/panic), WiFi join, pool connect, hashrate
  > 0, and at least one accepted share (the beep + green LED flash). Walk the FLASH_GUIDE.md
  §5 checklist: 6 screens render with CORRECT colors, /api/system/info JSON, speaker test-tone,
  SD list, dashboard. Record pass/fail per item.

### DISPLAY DRIVER RISK — ILI9341 vs ST7789 (D-75 — top risk)
- **D-75 (RISK):** The `[env:ESP32-2432S028R]` builds with `ILI9341_2_DRIVER`, but this unit's
  stock target was `esp32-2432s028r-st7789`. FLASH_GUIDE.md warns: wrong/inverted colors after
  flash ⇒ panel is the ST7789 variant ⇒ switch the TFT_eSPI driver to ST7789 (`-DST7789_DRIVER`
  + the right offsets/invert) and rebuild+reflash. This is the MOST LIKELY on-device failure and
  plausibly explains the earlier "looks like shit" report. The plan MUST include this as a
  branch: if colors are wrong, swap driver → rebuild → reflash → re-verify (a build change ⇒
  back through the build gate). Keep `invertColors`/Brightness serial keys (Phase-6 carry) handy.

### Restore reversibility (D-76 → VALID-03)
- **D-76:** Document + PROVE the restore path:
  `esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 hardware/backups/cyd-backup-<ts>.bin`.
  Proof of reversibility = (a) the fresh backup is a valid 4 MB image (size + sha recorded) and
  (b) the restore command is exercised at least as a documented dry-validation; a full
  flash→restore→reflash round-trip is OPTIONAL (costs two extra flash cycles + mining downtime)
  and only run if the user wants belt-and-suspenders proof. Default: prove the backup integrity
  + document the one-command rollback (FLASH_GUIDE §Rollback already has it).

### Claude's Discretion
- Serial-log capture mechanism (pio device monitor vs esptool read) + how long to observe for a
  share (pool-dependent; may take minutes — note expected wait, don't fail prematurely).
- Whether to run the optional full restore round-trip (default: no, unless user asks).
- config: reuse this unit's identity (pool.nerdminers.org:3333 + recovered wallet) via
  WiFiManager portal or SD config.json — no secrets committed.
</decisions>

<canonical_refs>
## Canonical References
- firmware/custom/NerdMiner-CYD-Custom/docs/FLASH_GUIDE.md (the runbook: backup → build → flash → §5 checklist → rollback + the ILI9341/ST7789 note).
- firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/{bootloader,partitions,firmware}.bin (the validated artifacts).
- hardware/backups/ (prior 4 MB backups; gitignored, no-redistribution).
- .planning/REQUIREMENTS.md — VALID-01/02/03.
- .planning/phases/06-*/06-VERIFICATION.md (the static HW-01..04 results these on-device checks confirm).
</canonical_refs>

<code_context>
## Existing Code Insights
- esptool v4.5.1 available via `.venv/bin/pio pkg exec -- esptool`. Device confirmed reachable
  (chip_id read OK) — but every esptool op resets the chip, briefly stopping mining.
- The FLASH_GUIDE already encodes the full runbook; Phase 7 EXECUTES it, doesn't reinvent it.
</code_context>

<specifics>
## Specific Ideas
- Atomic, reversible, evidence-first: backup (sha recorded) BEFORE, verify_flash + power-cycle
  AFTER, serial + visual + mining evidence captured into 07-SUMMARY / a flash runbook doc.
- The ST7789/ILI9341 driver branch is the one likely place this needs a rebuild loop — plan for it.
</specifics>

<deferred>
## Deferred Ideas
- Full flash→restore→reflash round-trip proof (only if user wants it).
- Differentiator polish (v2/POLISH).
</deferred>

---
*Phase: 7-on-device-validation*
*Context gathered: 2026-06-18*
