# PanicMiner validation — 2026-11-25

The CYD runs the matching Mine, Hash, Chain, Clock, Boot and Setup design in
the fork's `ESP32-2432S028R` environment, with the verified panel inversion.
The full 4 MB device image is backed up before every firmware write.

## November 26: SD update installed; offline card mount verified

The owner requested reinserting the purchased card and updating the firmware.
The charger observer was preserved as interrupted, with 54 samples through
787 seconds. It did not complete the 20-minute gate. Around the requested card
change, HTTP and ICMP stopped responding and JNAP stopped listing an active
miner. This does not isolate card insertion, a power interruption or a firmware
fault; no USB serial connection was available to inspect the event.

Source inspection found a concrete peripheral ownership conflict: TFT_eSPI and
SDStore each constructed a separate `SPIClass(VSPI)` on different pins, while
touch already owned HSPI. The second VSPI owner could reinitialize the display's
hardware and had a separate transaction lock. The repair uses TFT_eSPI's public
`getSPIinstance()` for the card, fans VSPI clock/data out to GPIO18/23 without
detaching display pins GPIO14/13, and sets the common input to the SD's GPIO19.
Separate GPIO15/5 chip selects isolate display/card transactions. SD unmount
keeps the display controller running. The display deselects the card before its
first traffic; touch and its calibration remain on the existing HSPI path.

The checked Arduino SPI implementation returns early from `begin()` on an
already started instance and serializes transactions on that instance's lock.
The GPIO routing mechanism is also documented in Espressif's
[SPI reference](https://github.com/espressif/esp-idf/blob/v4.4/docs/en/api-reference/peripherals/spi_master.rst).
This repair addresses a source-level conflict; it does not establish the cause
of previous Wi-Fi failures.

The installed application builds for `ESP32-2432S028R`: 1,794,416 bytes, SHA-256
`161f02c2831ac32a2265dc5e49706757968e33fa5eca513f6fc72cd6dfbaf3e9`.
Startup now probes the shared SD bus before Wi-Fi, so the card can be checked
even when the saved network is out of range. The previous uninstalled build
`2a58b816…` was superseded by this startup change.
Production SDStore host checks cover shared ownership, retained display routes,
missing-card/retry paths, non-disruptive unmount, capacity, CSV records and
short-write accounting. Existing 132 regression cases and speaker tests pass.
The negative control compiles the previous production SDStore with the same
host boundaries and fails the shared-bus assertion; the repaired source passes.
The owner reconnected laptop USB with the card inserted and explicitly reported
that this location is out of Wi-Fi range. The flash helper read a fresh 4 MB
backup, checked its SHA-256, verified it against flash, then wrote and separately
verified the bootloader, partitions, boot-app selector and application. Saved
configuration partitions were preserved. An ordinary EN boot reported
`[SD] SD card mounted successfully` about 3.8 seconds after reset, before the
Wi-Fi join. No SD failure or directory warning, panic, brownout or heap assertion
was observed in the offline USB check. Touch initialization also completed;
this does not establish visual/touch coexistence after card use.

The USB observer was released for the owner's move downstairs. A fresh observer
waits for USB removal, router association, mounted SD and pool authorization,
then warms up the 10/60-second rates before starting a new 20-minute window.
It preserves API gaps and out-of-band observations, checks independent work
progress and SD rows, and downloads CSV after the timed window. Mounted
capacity, real downloaded CSV records, post-mount display/touch behavior and
the new 20-minute result remain pending that charger boot near the router.
An offline zero rate is not evidence of a failure to mine while connected.

## November 26: JNAP recovery and saved-settings restart

Authenticated JNAP confirmed that the stored 2.4 GHz SSID and Wi-Fi password
match the router, MAC filtering is disabled, and the stronger scanned AP is the
actual router. The router is an EA8300. Its local UI source supplied the API
names and request schema; radio settings come from `GetRadioInfo3`, not the
unsupported `GetRadioSettings` actions.

The owner moved the equipment as close as practical and unplugged/replugged the
miner. Airtime fairness was disabled using `SetAirtimeFairnessSettings`; readback
confirmed disabled. Applying it briefly interrupted router Wi-Fi. NetworkManager
subsequently reported full laptop connectivity. WPA2 remained configured. The
working configuration retains the disabled fairness setting, rather than making
another radio change during acceptance testing.

At the earlier recovery, the installed application was
`82dca8181f64c04be39835fc3db2ea94e5076265988061d77bd2affe51644548`.
A fresh EN boot joined the stronger AP with the first normal b/g/n profile,
authorized the existing wallet worker and loaded **5/5 Chain endpoints**.
Signal was -38 dBm; JNAP independently listed the miner on the 2.4 GHz radio.
Thirty-four early samples included 957.0–986.7 kH/s in the latest ten; SHA
verification showed 455 checked, zero mismatches and zero dropped candidates.
These are startup observations, not a completed performance gate. Movement,
USB power cycling and fairness changed together, so root cause remains
confounded; do not attribute recovery solely to the router option.

`PMSAVE` acknowledged `ESP_OK` with best difficulty `0.702206`. A subsequent EN
reset, without USB provisioning, rejoined at -34 dBm, restored the same best,
authorized the pool and loaded all five Chain endpoints. Later live readings
were near 966–991 kH/s. That validates restart with stored settings, but EN reset
is not a charger-powered cold boot. The user confirmed moving the unit to a USB charger near the router. The USB
device disappeared from the laptop, JNAP independently listed the miner on
2.4 GHz with its current lease, and its HTTP API reported pool authorization,
restored best `0.702206052`, and a 10-second rate of `999.4349976 kH/s`.
This verifies the charger-powered cold boot and mining without the laptop USB
connection. The initial 60-second average was still warming up and is not used
as a completed benchmark result.

The original serial 20-minute observer was stopped before completion for the
requested restart and charger check. A separate API observer attempted a
fresh charger-powered 20-minute run after six consecutive in-band 10/60-second
rate samples. It checks reported rates, pool authorization, API reachability and
the independent work-counter delta. Serial SHA/Chain evidence is limited to the
pre-unplug test; this API run does not establish continuous post-unplug SHA or
TLS-event monitoring. The first API attempt stopped after about 40 seconds because one status read
exceeded its three-second limit. JNAP still showed an associated miner and the
next API response was authorized at 966.96/975.61 kH/s (10/60-second windows).
That failed observation is retained; it does not establish a Wi-Fi disconnect.
The second attempt also timed out, with a ten-second status limit. A subsequent
response took 0.113 seconds, remained authorized, and showed increasing work at
972.77/977.43 kH/s. All 12 ICMP probes responded during that later check. Both
failed observations are retained. The continued work-counter observer was
interrupted for the requested SD update as recorded above. There is no new
20-minute PASS. At that point the installed code was
unchanged; the common success-path reconnection safeguard in `92a0f4d` was
build-checked but not flashed. It is included in the SD update above. The older
image's saved-network success path already enables
automatic reconnection and is the path exercised in the successful EN test.
Secrets, full responses containing credentials and device logs remain excluded
from Git; JNAP diagnostic output is projected and redacted.

## Current repair and physical SD test

The owner confirmed **SD removed and USB reconnected**. With the card absent,
the SD-isolation binary below joined the saved network, authorized its wallet
worker with the pool and mined. A subsequent EN reset restored best difficulty
`0.139651`, rejoined and recovered all five Chain endpoints without provisioning.
This confirms automatic recovery in that configuration; card removal and USB
power cycling happened together, so it does not isolate their individual effects.

Complete native frame captures showed live Stats and Chain data: RAM, sampled
CPU use, internal chip temperature, pool authorization, a nonzero best candidate,
price, network difficulty and next retarget. Frame dumps were outside the timed
performance window because their serial traffic interrupts the monitor task.

The first fresh Mine run **failed** after about 300 seconds. It recorded 281
rolling samples, a 415–1001 kH/s range, 16 below 944.3 kH/s, a Wi-Fi disconnect
and an incomplete Chain refresh. The board reconnected automatically, but this
is insufficient for unattended-use acceptance. The earlier successful join does
not mean Wi-Fi stability is solved.

Source repair `599e170` addresses verified application defects:

- `merkle_root[65]` wrote past a 65-byte array. ASan/UBSan reproduce the stack
  overrun in the previous production source; the repaired source passes.
- Fixed-size coinbase concatenation silently truncated inputs above 511 hex
  characters. SHA now streams all four fragments through a bounded 64-byte
  workspace. Six independent SHA256d fixtures exercise short/long transactions,
  complete 13/32-branch merkle paths and 80-byte header byte ordering.
- Per-job payload and merkle logging no longer block the release mining path.
- Work increments, compaction and read snapshots share a mutex. Monitor and
  watchdog receive a coherent count; API totals include the current job, and
  checkpoints include completed megahashes from that job.
- Normal b/g/n rates at moderate power are attempted first; legacy and other
  power profiles remain bounded fallbacks. This is a recovery change to test,
  not proof of the physical cause of the radio failures.
- Flash/restore use the stable USB device link across ttyUSB re-enumeration.

The production-code host suite passes **132 cases** under ASan/UBSan, plus the
separate speaker tests. Eight restore safety scenarios and workflow lint pass.
Host mining-data tests require OpenSSL development headers; CI installs them.

Repair application SHA-256:
`ccb973bcbaac03902626019a4bc7221d13b0a2f688a516f3b8a1b7a987930ded`
(1,791,680 bytes). The build preceded the source commit, so the embedded revision
can say `b4805cf-dirty`; identify the tested application by its full digest.
Installation and native acceptance results are recorded below when complete.
Raw logs, failed runs, captures, credentials and backup images remain ignored.
No release, public visibility change or push is cleared while acceptance is open.

## Radio retest and location change

The first post-repair boot and an EN reset both failed association with the
card physically absent. An explicit Espressif RF-calibration reset returned
`ESP_OK`, but the next join still failed. This rules out claiming that the
application buffer repair or calibration alone fixed the observed radio issue.
The official [ESP-IDF RF calibration guide](https://github.com/espressif/esp-idf/blob/v4.4/docs/en/api-guides/RF_calibration.rst)
describes the selective calibration reset; this test did not erase the app's
Wi-Fi, pool, wallet or statistics configuration.

RF-diagnostic-capable normal application SHA-256:
`e8cfc6427ec8ea1be002f0fadf5e0f9febbf3412857fd0b35c9c0c15a0c6c870`
(1,792,256 bytes; source `4126b7e`, embedded `599e170-dirty`).

The owner then disclosed moving the miner farther from the router so the laptop
could charge. That confounds the failed joins after the first successful run.
Native AP scans during failures returned signals between -62 and -80 dBm;
proximity, power and SD state have not been independently isolated. Distance is
plausible and remains a testable explanation, not an established root cause.

A separate, read-only Wi-Fi-only diagnostic was built and flashed after a
fresh verified backup. Its SHA-256 was
`8e09df2be97f4a7030fa8cee1d38e70395d68c473d17d44c50e512c1105c9b97`.
It disables the backlight and omits mining, touch/display SPI, SD access and
sound. It was interrupted after about 49 seconds to restore the normal firmware
when the owner moved the unit back toward the router. It had not joined, but
**did not complete all four profiles**; do not report an isolation-test pass or
failure from that partial run. The temporary diagnostic is documented under
`firmware/diagnostics/wifi-only/` and is not the normal device firmware.

The normal e8cfc642 application was restored through a fresh verified full
backup; all images passed verification. After the owner returned near the
router, association still failed. Two WPA2 access points advertised the saved
network at approximately -58 and -70 dBm on different channels. These are scan
observations, not proof of successful bidirectional radio traffic. A manually
pinned alternate-AP attempt used channel zero and returned `NO_AP_FOUND`; it
does not certify an explicit known-channel attempt.

The owner requires laptop USB power and cannot perform a different-supply test.
Power remains unisolated, not a demonstrated cause. No further power-source or
physical relocation test is required to continue the authorized software work.

The next normal application disables automatic reconnection before starting
STA mode, uses RAM for driver credentials while retaining the existing SPIFFS
application configuration, scans all channels, and retries each matching AP
on its detected channel. It starts with normal b/g/n at the SDK's maximum power
and includes the previously untested maximum-power 802.11b fallback. BSSIDs
are not saved in the application configuration, so a cold boot rescans after
moving. Authentication timeouts no longer label the password as likely wrong.
The normal retry application was built, installed after a fresh verified 4 MB
backup, and all four images passed verification. Installed application SHA-256:
`82dca8181f64c04be39835fc3db2ea94e5076265988061d77bd2affe51644548`
(1,794,400 bytes). The scan saw channel 5 at -63 dBm and channel 1 at -71 dBm.
All **eight** profile/AP attempts completed without joining: the native log
recorded three `AUTH_EXPIRE`, four `NO_AP_FOUND`, and one application-initiated
`ASSOC_LEAVE`. The configured maximum-power profile's driver readback was
15.25–15.75 dBm; requested power is not proof of radiated power.

A subsequent EN reset sent the saved settings with only the stronger AP's BSSID
and known channel selected. That separate **25-second** attempt timed out and
recorded two `AUTH_EXPIRE` events. Neither test establishes a wrong password,
a bad supply, or a defective board. Laptop USB remained the power source and
the SD card remained absent. No fresh 20-minute mining run was started because
there was no joined network or authorized pool session.

Final source review additionally restored automatic reconnection on successful
serial and Setup paths, which can bypass the saved-network loop. This final
source safeguard is build-checked but is not in the installed 82dca818 image;
the failed cold-boot test above exercises the same retry logic. No further flash
was started while the owner needed to move the laptop back to its charger.
The installed normal firmware remains in its network recovery path. Hardware
acceptance, SD reinsertion and publication remain held until an actual network
join and sustained mining can be verified. AP-side authentication evidence or a
controlled physical radio/network test is still needed to narrow the cause.

## Earlier SD-isolation build and failure

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
Before physical card removal, Wi-Fi saw the
configured AP but failed authentication/association. A backup comparison confirmed
the stored SSID/password match the private device configuration; values are not
published. Four bounded legacy/mixed-mode and power profiles were tried without
a successful initial join. This is unresolved; proximity alone does not diagnose
the cause. Pool acceptance, live Chain/Stats, SD CSV writes, saved-stat recovery
and the 20-minute hashrate gate must be rerun on this exact binary after recovery.
At this point, no unplug-and-replug success had been observed.

The user authorized a firmware-only SD isolation test while away from the unit.
Over the first 126 seconds, all four join profiles ran, with 43 authentication
timeouts, zero SD mount attempts, zero mining rate readings and zero Chain
refreshes. The test did not resolve the failure. It checks SD initialization and
access; the inserted card remains electrically connected, so its power draw has
not been excluded. That physical test was later completed as recorded above.
The unit continues its bounded join attempts and portal/restart recovery loop.
No release or public repository publication is authorized until the hardware
acceptance checks pass.

An independent host check used the settings recovered from the verified backup.
The saved Wi-Fi credentials match the private device configuration. The saved
wallet matches the previously recovered device wallet and passes its Bitcoin
address checksum. Statistics persistence is enabled. The pool accepted a host
subscription and authorization with those settings and returned difficulty and
mining work. No share was fabricated or submitted by this check. This verifies
the stored configuration and pool reachability from the computer; it does not
establish that the board is connected, mining or receiving a payout.

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
