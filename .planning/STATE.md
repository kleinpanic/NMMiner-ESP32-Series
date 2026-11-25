---
gsd_state_version: 1.0
milestone: v4.0
milestone_name: PanicMiner — owned open firmware
current_phase: 1
current_phase_name: Base + See
status: executing
last_updated: "2026-11-25T00:35:00-05:00"
progress:
  total_phases: 5
  completed_phases: 0
---

# State

Current Phase: 1 — Base + See
Milestone: v4.0 PanicMiner. v3.0 closed as superseded on 2026-11-24
(`archive/v3.0-nmminer-binary-re/README.md`).

## Device

- Live firmware at 2026-11-25 00:55: fork `4c1ecc7`, flashed as `v4-review-fixes`. It has
  the v2 UI (Afterglow theme), panicminer branding, the mempool.space chain feed, the SHA
  instrumentation, the pipelined hardware SHA loop and the review fixes. Boot banner
  `panicminer 0.1.0 starting`; setup AP `panicminer-DE0C`, password `panicminer`.
- Wi-Fi is down at 2026-11-24 23:34. Router 2.4 GHz radio `14:91:82:B0:04:E5` (SSID
  Homebase, the same box as Homebase5 `…:E7`) rejects the unit. Reason codes: 15
  4WAY_HANDSHAKE_TIMEOUT and 202 AUTH_FAIL, then 2 AUTH_EXPIRE on every later try.
  - The password is the one the unit held while it mined earlier that evening (byte compare
    against the 22:07 and 22:24 backups).
  - Restoring the 22:24 backup image fails the same way. The firmware is not the cause.
  - The 2.4 GHz radio drops in and out of scans from both the laptop and the unit.
  - 2026-11-25 00:50 rescan from the laptop: 3 neighbouring 2.4 GHz networks and both
    Homebase5 radios (`…:E6` ch 40, `…:E7` ch 132) are visible; `…:E5` is not.
  - The laptop cannot stand in as a hotspot. Its card allows 1 channel with an AP up, and
    it uses that channel for Homebase5 (5 GHz ch 132).
  - 2026-11-25 13:22: `…:E5` is back in laptop scans (ch 5) and the unit still fails.
  - Diagnosis, 2026-11-25 13:28: the password exchange fails, and the router then ignores
    the station for a while.
    - A test build with a locally administered station MAC (`D6:E9:F4:A7:DE:0C`) got past
      authentication and then failed the 4-way handshake (reason 15).
    - After that, the router ignored the new MAC as well (reason 2).
    - Overnight, the factory MAC reached the handshake once (01:06:17) and failed it too;
      all 650 other attempts were ignored.
  - The radio is plain WPA2-PSK with CCMP and no required MFP. The board's password
    matches the laptop's saved Homebase5 PSK.
  - Conclusion: the 2.4 GHz radio does not accept the Homebase5 password now, though the
    unit mined on it before 23:00. Either its password changed, or the radio is in a bad state.
  - Also seen: an ad-hoc (IBSS) network named Homebase, `DE:86:7F:08:AB:D2`, on ch 1. It is
    not the router, and the ESP32 cannot join IBSS.
  - Next step (operator): check the 2.4 GHz password in the Linksys app and reboot the
    router. Then enter the right password in the unit's portal, or in `secrets/device.json`.
- Wi-Fi config: `secrets/device.json` (gitignored, mode 600). Push it with
  `.venv/bin/python scripts/provision_fork_serial.py --log logs/provision.log`.
  Homebase5 is 5 GHz only and cannot work.
- The setup portal re-opens for 180 s after a failed join, then the unit restarts and
  retries. It recovers by itself once the router accepts it.
- Latest full backup: `hardware/backups/cyd-backup-20261125-133426-pre-v4-review-fixes.bin`
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

## Repos

- Parent repo has no remote. Laptop disk is the only copy.
- Fork `firmware/custom/NerdMiner-CYD-Custom`: branch `v4/panicminer`, pushed to
  `origin`. Old clone stack preserved on `origin/archive/nmminer-clone-lvgl` (`9777259`).

## Open questions

- Not verified on the unit yet; each needs Wi-Fi:
  - the v2 screens read back with `scripts/fbdump.py`;
  - chain feed `[CHAIN]` lines;
  - `[SHA]` mismatch counts (lock-all is always on in the pipelined build);
  - the boot self-test line and the loop the miner picks;
  - real hashrate. To measure it, use the change in `[SHA]` checked + dropped, × 65536 ÷
    seconds, since the attempt counter moves in 256K-nonce chunks;
  - the first accepted share with the zero-padded nonce.
- Panel driver: the base uses `ILI9341_2_DRIVER`; the stock identity says ST7789. Settle
  with a photo.
- Touch: XPT2046 answers with no-press readings. Needs a press test.
