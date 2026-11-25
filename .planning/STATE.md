---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: PanicMiner — owned open firmware
current_phase: 1
current_phase_name: Base + See
status: executing
last_updated: "2026-11-24T23:36:00-05:00"
progress:
  total_phases: 5
  completed_phases: 0
---

# State

Current Phase: 1 — Base + See
Milestone: v4.0 PanicMiner. v3.0 closed as superseded on 2026-11-24
(`archive/v3.0-nmminer-binary-re/README.md`).

## Device

- Live firmware at 2026-11-24 23:34: fork `1ed0887`, flashed as `v4-ui-v2g`. It has the v2
  UI (Afterglow theme), panicminer branding, the mempool.space chain feed and the SHA
  instrumentation. Boot banner `panicminer 0.1.0 starting`; setup AP `panicminer-DE0C`,
  password `panicminer`.
- Wi-Fi is down at 2026-11-24 23:34. Router 2.4 GHz radio `14:91:82:B0:04:E5` (SSID
  Homebase, the same box as Homebase5 `…:E7`) rejects the unit. Reason codes: 15
  4WAY_HANDSHAKE_TIMEOUT and 202 AUTH_FAIL, then 2 AUTH_EXPIRE on every later try.
  - The password is the one the unit held while it mined earlier that evening (byte compare
    against the 22:07 and 22:24 backups).
  - Restoring the 22:24 backup image fails the same way. The firmware is not the cause.
  - The 2.4 GHz radio drops in and out of scans from both the laptop and the unit.
  - Next step: reboot the router, or provision a 2.4 GHz hotspot.
- Wi-Fi config: `secrets/device.json` (gitignored, mode 600). Push it with
  `.venv/bin/python scripts/provision_fork_serial.py --log logs/provision.log`.
  Homebase5 is 5 GHz only and cannot work.
- The setup portal re-opens for 180 s after a failed join, then the unit restarts and
  retries. It recovers by itself once the router accepts it.
- Latest full backup: `hardware/backups/cyd-backup-20261124-233327-pre-v4-ui-v2g.bin`
  (checksum sidecar beside it). `scripts/flash_fork_build.sh` takes one before every write.
  To restore a backup:
  `.venv/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 <backup>`.
- Serial commands: `PMDUMP` (framebuffer to `scripts/fbdump.py`), `PMNEXT` (next page),
  `PMFEED` (chain fetch now), `PMLOCK` (toggle SHA lock-all). `NMCFG {json}` in the 6 s
  boot window sets Wi-Fi and pool.

## Done this phase (fork `v4/panicminer`)

- `584ee19`: software share verification before submit.
- `3778d55`: serial no longer prints the Wi-Fi password, wallet or pool password.
- `3b65643`, `577759c`: LVGL UI on the CYD, v2 look on every page; host renderer in
  `tools/uisim`.
- `d21ded3`: 802.11 disconnect reason codes in the log.
- `fe46f9d`: prefilter hashes recomputed in software (`[SHA]` counts every 10 s); SHA
  lock-all.
- `f64ae84`: panicminer AP name, hostname, pool user agent and portal. Fixes the pool
  password field missing from the portal.
- `1ed0887`: mempool.space chain feed with a pinned root CA.

## Repos

- Parent repo has no remote. Laptop disk is the only copy.
- Fork `firmware/custom/NerdMiner-CYD-Custom`: branch `v4/panicminer`, pushed to
  `origin`. Old clone stack preserved on `origin/archive/nmminer-clone-lvgl` (`9777259`).

## Open questions

- Not verified on the unit yet; each needs Wi-Fi:
  - the v2 screens read back with `scripts/fbdump.py`;
  - chain feed `[CHAIN]` lines;
  - `[SHA]` mismatch counts with lock-all on and off;
  - hashrate against the 344 kH/s baseline.
- Panel driver: the base uses `ILI9341_2_DRIVER`; the stock identity says ST7789. Settle
  with a photo.
- Touch: XPT2046 answers with no-press readings. Needs a press test.
