---
gsd_state_version: "1.0"
milestone: v4.0
milestone_name: "PanicMiner: owned open firmware"
current_phase: 4
status: verifying
stopped_at: Remote follow-up verified on 0e32d00a / source 07c3931; release v4.0.0-rc.2 prepared. HW-05 actual charger/SD cold-power test is the only physical closeout item.
last_updated: "2026-11-26T21:54:38.350Z"
state_head: 73d63947c4982534e0ae9cf2c9c769ca24ecc4d1
progress:
  total_phases: 5
  completed_phases: 4
  total_plans: 0
  completed_plans: 0
  percent: 80
current_phase_name: Hardware extras
---

# State

Current Phase: 4
Phase Name: Hardware extras
Status: verifying
Milestone: v4.0 PanicMiner. Three earlier phases and the mining-core hour are
verified. Only HW-05 needs a physical charger/SD cold-power test.

## Device

- Installed application `0e32d00af0c9cb8d996f1a284cc9a04768707402d336ae5ec8f9e64c4c27b835` (1,797,328 bytes), firmware source `07c3931cbcb297f330eabe68e7feabbc2f330c2f`.
- Fresh full 4 MB backup checked against flash; all four installed inputs independently verified; matching ELF/factory inputs archived privately.
- Exact-image strict Mine PASS: 1,201.807 seconds, 220 samples, 968.720 kH/s (-2.54% against 994); all 17 strict checks passed. SD added 20 records; complete nine-column export; wallet current; no API/native error, reboot or SHA mismatch.
- Setup saved/reopened both pools and the same wallet. A deliberately unreachable primary switched to an authorized, working fallback after three failures. Original primary restored; optional fallback remains disabled as before.
- Native GPIO latches verified green, amber and both red/off phases. Production tests cover cyan share timing, restored counts and wrap. Audio owner-confirmed working; playback code unchanged.
- Pool and SD recovered after a controlled board-only Wi-Fi drop. No router/laptop network setting changed.
- Steady-window native IP audit: 2023 packets, 6 IPv4 entries, all accounted; zero unknown IPv4 destinations, overflow or unparsed frames; 2 IPv6 frames counted without destination decoding. Audit starts after saved Wi-Fi initialization; it is not a radio/ROM audit. Reconnect trace separately counted two unattributed IPv6 frames.
- Prior rc.1 application 23236906 / source 49afce5 passed its independent 60.24-minute USB/SD soak at 967.925 kH/s with new accepted work. This is not this follow-up image's hour.

## Repository and checks

- Release v4.0.0-rc.2 published from tag 73d6394. All six assets downloaded anonymously and matched local/GitHub digests and both checksum manifests. All 62 release files/ZIP entries passed the private-value scan.
- Canonical public source: https://github.com/kleinpanic/NMMiner-ESP32-Series, branch main.
- Historical kleinpanic/NerdMiner-CYD-Custom is private, archived and points to the canonical repo.
- Public CI 45897395376 passed exact firmware source 07c3931: both builds, C++ sanitizers, restore safety, Python tests, both UI themes and artifacts.
- Local Python: 108 tests, 29 subtests; public: 107 tests, 29 subtests, one explicit private-image skip. All execution used root .venv; no dependency change.
- History, exact private-value and binary scans passed. Secrets, wallet values, backups and raw logs stay ignored.
- Source and license boundary: owned open application; Espressif radio binaries and ROM remain platform components.

## Remaining physical acceptance

HW-05 only: remove/reapply charger power with SD inserted and verify saved Wi-Fi,
pool authorization and advancing work. Earlier charger confirmation preceded SD
changes. Current SD-enabled saved-settings EN resets pass on laptop USB. Remote
USB software cannot establish an actual charger unplug result; do not request
another move while the owner is unavailable or call this physical test passed.
There is no remaining coding, portal, audio, LED-output or steady-window IPv4 gate.

## Decisions

- Preserve the verified ILI9341_2_DRIVER + inversion setup; vendor calls the observed unit ST7789.
- Wi-Fi is 2.4 GHz. BOOT hold opens non-destructive Setup; saved startup retries in the background.
- Pages stay manual. The 40 ms Monitor target is 25 fps; preserve the measured 5% hashrate limit.
- Both pools use the same owner-configured payout address. Accepted shares are work, not payouts; no accepted block or payout is claimed.
- Full 4 MB verified backup before firmware writes. Keep raw evidence and recovered vendor assets private.

## Session

**Last session:** 2026-11-26
**Stopped at:** Remote follow-up verified on 0e32d00a / source 07c3931; release v4.0.0-rc.2 prepared. HW-05 actual charger/SD cold-power test is the only physical closeout item.
**Resume file:** docs/panicminer-validation-2026-11-25.md
