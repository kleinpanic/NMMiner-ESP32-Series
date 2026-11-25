---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: PanicMiner — owned open firmware
current_phase: 1
current_phase_name: Base + See
status: executing
last_updated: "2026-11-25T16:45:00-05:00"
progress:
  total_phases: 5
  completed_phases: 0
---

# State

Current Phase: 1 — Base + See
Milestone: v4.0 PanicMiner. v3.0 closed as superseded on 2026-11-24
(`archive/v3.0-nmminer-binary-re/README.md`).

## Device

- Live firmware at 2026-11-25 16:38: fork `ac9d35e` (UI "D", Afterglow Blockspace),
  flashed as `blockspace-d2`. It boots clean: LVGL pool 24 KB used of 56 KB, heap 145 KB
  free. Boot banner `panicminer 0.1.0 starting`; setup AP `panicminer-DE0C`, password
  `panicminer`.
- Wi-Fi fails again since 2026-11-25 16:08: reason 2 AUTH_EXPIRE on every try, some
  NO_AP_FOUND, and the router does not list the unit.
  - The pre-D image (backup `cyd-backup-20261125-160543-pre-blockspace-d.bin`) fails
    the same way at 16:25, so the UI is not the cause.
  - 8.5 dBm + 802.11b (read back as 6.75 dBm) fails the same way at 16:36. Reverted to 13 dBm.
  - Next step needs the operator: move the unit nearer the router, or join another
    2.4 GHz network.
- Wi-Fi worked from 2026-11-25 15:07 (IP 192.168.1.144). Cause: this unit's radio
  is not heard by the router at full transmit power.
  - At full power the router ignored nearly every auth request (reason 2 AUTH_EXPIRE) from
    2026-11-24 23:34 on, on any MAC and after a router restart. One join got through at
    15:00:51 with the router reading the unit at the weakest signal of all its clients.
  - At 7 dBm the router answered auth and association every time; the WPA2 handshake
    then timed out (reason 15).
  - 13 dBm with 802.11b-only rates joined on the first try. Fix `ef5e330`: build flags
    `PM_WIFI_TXPOWER` and `PM_WIFI_11B` on the ESP32-2432S028R env.
  - Ruled out: the password (NVS and the router's JNAP settings match), MAC filtering
    (off), band steering (5 GHz only), the router's 2.4 GHz radio (5 other clients rejoined
    after the restart), and the firmware (the 22:24 image failed the same way).
  - Router: Linksys EA8300, JNAP at `192.168.1.1/JNAP/`, admin password in
    `secrets/router.json` (gitignored, mode 600). No router setting was changed.
- Wi-Fi config: `secrets/device.json` (gitignored, mode 600). Push it with
  `.venv/bin/python scripts/provision_fork_serial.py --log logs/provision.log`.
  Homebase5 is 5 GHz only and cannot work.
- The setup portal re-opens for 180 s after a failed join, then the unit restarts and
  retries. It recovers by itself once the router accepts it.
- Latest full backup: `hardware/backups/cyd-backup-20261125-163752-pre-blockspace-d2.bin`
  (checksum sidecar beside it). `scripts/flash_fork_build.sh` takes one before every write.
  To restore a backup:
  `.venv/bin/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 write_flash 0x0 <backup>`.
- Serial commands: `PMDUMP` (framebuffer to `scripts/fbdump.py`), `PMNEXT` (next page),
  `PMFEED` (chain fetch now). `PMLOCK` reports "all" and changes nothing on the pipelined
  build. `NMCFG {json}` in the 6 s boot window sets Wi-Fi and pool.

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
- `d24c187`..`4b93f0b`: HenrysCat's pipelined classic-ESP32 SHA loop (MIT), with a boot
  self-test and a runtime fallback. The HW miner has core 1 to itself; Monitor, Stratum,
  ChainFeed and the SW miner run on core 0. `d24c187` fixes `[SHA]` counting every hit as
  a mismatch, because the worker published the engine's word-swapped header.
- Bench, 2026-11-25 00:15, env `ESP32-2432S028R-bench` (`SHA_BENCH=2`), on this unit (rev 3):
  - The shipping variants V0, V2 and V3 read `OK` with `A0 B0 C0` in all 9 stress modes.
    There are no `K 0`, `K 2` or `K 3` lines.
  - The controls work: V1 always reads BAD, and V4 to V10 go BAD under DPORT stress.
  - V0 runs at 247.2 cycles per nonce, a ceiling of about 971 kH/s at 240 MHz. The old
    loop measured 345 kH/s.
- Code review of the port: `.planning/audits/2026-11-25-pipelined-sha-review.md`. No
  blocker in how the C code drives the loop. Fixed: `96f2b62` (nonce submitted without
  zero padding, inherited from upstream), `0bd8907` (demote a loop that goes quiet),
  `ce04d99` (reseed on a new header only), `4c1ecc7` (count dropped ring slots).
  The open findings are listed in the audit.
- `ac9d35e`: UI "D", Afterglow Blockspace. Mine shows the block as a hashing cube, the
  rolling hashrate, a scope, best zeros against a block's, and a fee and block strip. The
  old Mine view is now the Hash page. Chain has the 2,016-dot period grid. `tools/uisim`
  renders 19 frames in both themes. Mockup: https://claude.ai/artifact/16gC8qPdh5aGHearUekHD9
  - A review found 1 blocker (the strip missed feed updates) and 4 warnings; all are fixed
    in the same commit.
  - Open from the review: `mining.cpp:565` rolls `Mhashes` and `hashes` over without a
    lock, so the Monitor can read a torn pair and show a 10 s hashrate spike. This predates D.

## Repos

- Parent repo has no remote. Laptop disk is the only copy.
- Fork `firmware/custom/NerdMiner-CYD-Custom`: branch `v4/panicminer`, pushed to
  `origin`. Old clone stack preserved on `origin/archive/nmminer-clone-lvgl` (`9777259`).

## Open questions

- Not verified on the unit yet; each needs Wi-Fi:
  - the four D pages read back with `scripts/fbdump.py --pages 4`;
  - the D hashrate against the 994 kH/s baseline (UI-05: at most 5 % lower), on the Mine page;
  - chain feed `[CHAIN]` lines;
  - `[SHA]` mismatch counts (lock-all is always on in the pipelined build);
  - the boot self-test line and the loop the miner picks;
  - real hashrate. To measure it, use the change in `[SHA]` checked + dropped, × 65536 ÷
    seconds, since the attempt counter moves in 256K-nonce chunks;
  - the first accepted share with the zero-padded nonce.
- Panel driver: the base uses `ILI9341_2_DRIVER`; the stock identity says ST7789. Settle
  with a photo.
- Touch: XPT2046 answers with no-press readings. Needs a press test.
