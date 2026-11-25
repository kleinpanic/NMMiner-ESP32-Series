# Phase 7: On-Device Validation - Context

**Gathered:** 2026-06-18 (auto mode; grounded in the existing fork FLASH_GUIDE.md + live device probe)
**Status:** SUPERSEDED BY 07-02 REPAIR + 07-VERIFICATION. Phase 7 is not ready to advance; it is parked on physical panel/touch proof, operator-confirmed hard power-cycle, 2.4 GHz Wi-Fi/pool/mining proof, API/dashboard checks, and restore proof. Device is present, but any future firmware write still requires explicit operator intent and a fresh full 4 MB backup first.

> Current executor note (2026-06-18): This context captured the original Phase 7 hardware run before
> the 07-02 software repair pass and before the later stock baseline restore, custom decoded-spec
> ST7789 reflash, decoded LittleFS preservation pass, current app reflash, user-report recheck,
> no-flash touch software review, and 2026-06-19T02:06Z read-only serial continuation.
> The live device is now custom decoded-spec ST7789 in WiFiManager recovery AP mode, not stock and
> not mining. Use `07-VALIDATION.md` and `07-VERIFICATION.md` as the current authority for artifact
> size, repaired-build status, live state, and remaining gates. This file is retained for D-70..D-76
> decision history.

<domain>
## Phase Boundary

Flash the post-07-02 repaired v3.0 fork firmware (current 07-VALIDATION.md records
`firmware.bin` payload 2,855,344 B from the final pre-flash repair build) onto the physical
CYD and prove it works: a fresh 4 MB esptool backup BEFORE the flash, `verify_flash` + a hard
USB power-cycle AFTER, a serial boot confirmation, a visual parity checklist against stock,
mining confirmed active on-device, and a documented + proven-reversible restore path. This is
the phase that touches the user's physical hardware; future firmware writes still require a fresh
full backup and explicit operator intent even though the device is no longer currently mining.
Closes
VALID-01/02/03 only after hardware evidence exists.
</domain>

<decisions>
## Implementation Decisions

### USER GO-AHEAD GATE before any write (D-70 — load-bearing)
- **D-70:** Historical context: this gate was written when the device was believed to be
  actively mining the old fork. Current context: the live device is custom decoded-spec ST7789 in
  recovery AP mode. The gate still stands because every firmware write and full backup changes
  device state or resets the chip. Do not write to the device without explicit operator intent,
  and take a fresh full backup before any future firmware write.

### Fresh 4 MB backup BEFORE flash (D-71 → VALID-01)
- **D-71:** Immediately before flashing, take a fresh full-flash backup:
  `esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 read_flash 0x0 0x400000 hardware/backups/cyd-backup-<UTC-ts>.bin`,
  then `sha256sum` it and assert size == 4194304 bytes. A prior backup exists
  (`hardware/backups/cyd-backup-20260617-172106.bin`, 4 MB) + a 2026-06-01 full-flash — but
  the directive mandates a FRESH backup per flash, so make a new one. NEVER commit the .bin
  (gitignored — it is a dump of the closed stock image; no redistribution, D-11/RE-04).

### Flash the validated build (D-72 → VALID-02)
- **D-72:** Flash via PlatformIO so the 3 images land at the right offsets:
  `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R -t upload
  --upload-port /dev/ttyUSB0` (bootloader@0x1000, partitions@0x8000, boot_app0@0xe000,
  firmware@0x10000). Do NOT rebuild first unless the tree changed — flash the exact artifact
  recorded in 07-VALIDATION.md. upload_speed 921600. The active board-driver decision is ST7789;
  do not flash an ILI9341 artifact unless new physical re-identification evidence is recorded first.

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

### DISPLAY DRIVER RISK — ST7789 decision retained (D-75)
- **D-75 (RISK):** This was originally the top risk because the old `[env:ESP32-2432S028R]`
  build used `ILI9341_2_DRIVER` while this unit's stock target was
  `esp32-2432s028r-st7789`. That decision is now resolved in source: the active env uses
  `-DST7789_DRIVER=1`, `TFT_BGR`, and inversion-off. Keep the driver branch only as a
  physical re-identification contingency: if the panel shows wrong/inverted colors, record the
  evidence, edit only justified driver/RGB/inversion/geometry flags, rebuild, reflash after a
  fresh backup, and re-verify.

### Restore reversibility (D-76 → VALID-03)
- **D-76:** Document + PROVE the restore path:
  `esptool --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 hardware/backups/cyd-backup-<ts>.bin`.
  Proof of reversibility = (a) the fresh backup is a valid 4 MB image (size + sha recorded) and
  (b) the restore command is exercised at least as a documented dry-validation; a full
  flash->restore->reflash round-trip is OPTIONAL (costs two extra flash cycles + device downtime)
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
- firmware/custom/NerdMiner-CYD-Custom/docs/FLASH_GUIDE.md (the runbook: backup -> build -> flash -> §5 checklist -> rollback + the historical ILI9341/ST7789 note).
- firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/{bootloader,partitions,firmware}.bin (current post-07-02 pre-flash build artifacts; see 07-VALIDATION.md for size/build evidence).
- hardware/backups/ (prior 4 MB backups; gitignored, no-redistribution).
- .planning/REQUIREMENTS.md — VALID-01/02/03.
- .planning/phases/06-*/06-VERIFICATION.md (the static HW-01..04 results these on-device checks confirm).
- .planning/phases/07-on-device-validation/07-VALIDATION.md — current runbook and repair evidence.
- .planning/phases/07-on-device-validation/07-VERIFICATION.md — current verifier verdict: 0/3 VALID requirements verified until 07-01 runs.
</canonical_refs>

<code_context>
## Existing Code Insights
- esptool v4.5.1 available via `.venv/bin/python ~/.platformio/packages/tool-esptoolpy/esptool.py`. Device confirmed reachable
  (chip_id read OK) — but every esptool op resets the chip, briefly stopping mining.
- The FLASH_GUIDE already encodes the full runbook; Phase 7 EXECUTES it, doesn't reinvent it.
</code_context>

<specifics>
## Specific Ideas
- Atomic, reversible, evidence-first: backup (sha recorded) BEFORE, verify_flash + power-cycle
  AFTER, serial + visual + mining evidence captured into 07-SUMMARY / a flash runbook doc.
- The display-driver branch is now contingency-only; current source already targets ST7789.
</specifics>

<deferred>
## Deferred Ideas
- Full flash→restore→reflash round-trip proof (only if user wants it).
- Differentiator polish (v2/POLISH).
</deferred>

---
*Phase: 7-on-device-validation*
*Context gathered: 2026-06-18*
