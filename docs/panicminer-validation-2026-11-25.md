# PanicMiner validation — 2026-11-25

The CYD runs the matching Mine, Hash, Chain, Clock, Boot and Setup design in
the fork's `ESP32-2432S028R` environment, with the verified panel inversion.
The full 4 MB device image is backed up before every firmware write.

## Latest build: hardware acceptance remains open

The current source was consolidated from firmware fork commit `bf08ec8` into
the parent repository as ordinary source files. There is no required submodule.
The earlier results below identify an older binary and do not certify this build.

Current application SHA-256:
`d2c601e8cb409714b72ba47269231b1e3e7900bb1bdca13e87b28f71934dd495`.

The SD-isolation install completed on November 25 at approximately 22:48 EST after a
fresh, verified 4 MB backup. Bootloader, partitions, boot app and application
all passed esptool verification. The backup is local and ignored.

Implemented and host-tested changes include:

- Manual pages with stable-press, full-release and cooldown filtering; no
  automatic rotation unless the diagnostic command explicitly enables it.
- Every pool-confirmed share counted separately from a valid block, and best
  difficulty updated from software-verified candidates.
- Correct compact target decoding and full 256-bit little-endian comparison.
  The inherited comparison read a pointer instead of the target and used an
  unsigned reverse loop; these were correctness bugs, not cosmetic changes.
- Bounded fragmented Stratum framing and subscription waits, real authorization
  state, and progress detection independent of the UI's rate accumulator.
- Stats with measured rate windows, CPU sampling, RAM, temperature, Wi-Fi,
  SD rows and SHA verification counts. Numeric placeholders no longer roll
  through fake intermediate zeroes.
- One SD filesystem owner, measured CSV rows, streamed downloads and protected
  config paths; enabled statistics use committed NVS checkpoints every 5 minutes.
- Quiet boot/reconnect and nonblocking, prioritized mining event sounds.

The production-code host suite passes 126 cases: touch 9, SD paths 30, progress
watchdog 7, targets 37 and Stratum 43, with AddressSanitizer/UBSan clean. Speaker
tests pass separately. Both themes render 29 fixtures, including Stats and a
reopened Setup. The CI workflow was linted and updated to run these checks.

On this binary the display/touch bus initializes and LVGL uses 33,328 of 45,056
bytes at boot. SD initialization is deferred until Wi-Fi connects; the SD config
fallback mounts on demand only when internal configuration is missing.
Wi-Fi currently sees the
configured AP but fails authentication/association. A backup comparison confirmed
the stored SSID/password match the private device configuration; values are not
published. Four bounded legacy/mixed-mode and power profiles were tried without
a successful initial join. This is unresolved; proximity alone does not diagnose
the cause. Pool acceptance, live Chain/Stats, SD CSV writes, saved-stat recovery
and the 20-minute hashrate gate must be rerun on this exact binary after recovery.
No new unplug-and-replug success is claimed.

The user authorized a firmware-only SD isolation test while away from the unit.
Over the first 126 seconds, all four join profiles ran, with 43 authentication
timeouts, zero SD mount attempts, zero mining rate readings and zero Chain
refreshes. The test did not resolve the failure. It checks SD initialization and
access; the inserted card remains electrically connected, so its power draw has
not been excluded. A physical card removal and USB power cycle remain pending.
The unit continues its bounded join attempts and portal/restart recovery loop.
No release or public repository publication is authorized until the hardware
acceptance checks pass.

## Chain feed diagnosis and fix

The certificate verification failure came from exhausted/fragmented heap during
RSA verification. Instrumenting `mbedtls_pk_verify_ext` exposed error `-17040`:
the RSA public-key operation could not allocate its big-number workspace.
One failing attempt had 12,312 bytes free but a largest block of only 1,076 bytes.
The higher-level verifier reported untrusted certificate signatures instead.

The pinned USERTrust root matches the root tested successfully with host
mbedTLS 2.28.4. Pausing the hardware miner between SHA chunks still produced
zero successful endpoints. Forcing the Chain task to use software SHA also
failed with the same RSA allocation error. The software-SHA experiment was
removed.

The LVGL reservation is now 44 KB instead of 56 KB on ESP32, releasing 12 KB
to the general heap. The 64-bit renderer uses 80 KB instead of 96 KB. The
native build boots with 26,660 bytes used in its 45,056-byte LVGL pool.
Certificate verification, the pinned root and the miner's normal SHA engine
locking remain enabled.

With Mine running, LVGL used 32,528 bytes with a 12,124-byte free block.
The general heap had about 76,852 bytes free and a largest block of 49,140
bytes, leaving room for certificate verification.

The first on-device fetch after this change succeeded for all five endpoints
in 3,938 ms while the hardware miner was running. The board rejoined its
existing network after automatic retries; authentication remains intermittent.

## Screens

Both themes rendered all 26 original fixtures successfully with the smaller pool.
Fixtures include cold data, waiting for the feed, kH/s and MH/s, six-figure and
near-million prices, midnight, alarm states, and reopening Setup.

Hash, Chain and Clock share Mine's header, two columns, numeric styling and
compact charts. Boot and Setup share the same frame and colours. Reopening
Setup now loads the retained Boot screen and deletes its previous Setup before
allocating the replacement, keeping the smaller LVGL pool within its limit.

## Firmware and flash verification

The earlier firmware changes were committed in the fork as `f8c6ef3`. That build was
flashed before that commit was created, so its generated Git version can
still identify parent `0f0cc75`; use the application digest below to identify
the tested binary.

Application SHA-256:
`03da04d96ebb25658f316e85e569371d91773c992d35c04f815336e8b80591f4`.

The flash helper verified the fresh 4 MB backup against the device, then wrote
and independently verified the bootloader, partition table, boot app and
application. Private backups, serial diagnostics and captured device frames
remain in ignored `hardware/backups/` and `logs/` directories.

## Restore safety

`scripts/restore_full_flash_backup.sh` uses esptool's `write_flash` command.
It requires an explicit 4 MB image, checks its checksum sidecar when present,
and takes and verifies a unique full backup of the current device before
writing. It verifies the restored image before reporting success. Both flash
helpers invoke esptool through the project Python environment.

All eight stubbed safety tests pass: successful ordering, failed read, short
backup, failed current-image verification, invalid input, checksum mismatch,
failed write, and failed restored-image verification. These tests never open
a serial device.

## Mine acceptance

The Mine page ran continuously for **1,200.474 seconds**, from 21:06:51 to
21:26:52 EST on November 25 (02:06:51–02:26:52 UTC on November 26).
The baseline is 994 kH/s; the 5% band is 944.3–1043.7 kH/s. Framebuffer
captures were deferred until after the timed window.

| Measurement | Result |
| --- | --- |
| Mean of 1,160 reported rolling rates | 972.624 kH/s, 2.151% below baseline |
| Independent hardware candidate estimate | 960.979 kH/s, 3.322% below baseline |
| Reported rolling range | 932–1003 kH/s |
| Readings below / above the 5% band | 11 / 0 |
| Hardware candidates verified during the window | 17,603 |
| SHA mismatches / dropped candidates | 0 / 0 |
| Resets, crashes or TLS verification errors | 0 |

Both full-run averages are within 5% of baseline. **The stricter requirement
that every rolling reading remain within 5% did not pass**: 11 of the 1,160
readings fell below 944.3 kH/s, reaching 932 kH/s (6.24% below baseline).
The runner's original `FAIL` status for that per-reading gate is retained in
the ignored result file; it has not been changed to an unconditional pass.

The independent estimate uses the change in checked plus dropped prefilter
candidates, multiplied by 65,536 and divided by elapsed seconds. Candidate
counts are statistical, so this is a cross-check of the reported rate.
All 11 recorded Chain refreshes, including warm-up, fetched all five endpoints.
There were no incomplete refreshes during the timed window.

Raw results and the four native framebuffer captures stay in ignored
`logs/panicminer-final-bench/`. The capture sequence returns the device to Mine.

## Python environment audit

The scoped tools use `./.venv/bin/python`; required dependencies are declared
in the root `requirements.txt`. A repository text audit found one historical
system-interpreter reference in the old monitor notes; it now directs current
tools to the project environment. No matching active invocation remains in
the audited source and documentation. GSD decisions and session state are updated
through its CLI; the scoped audit is in `.planning/evidence/panicminer-runtime/`.
