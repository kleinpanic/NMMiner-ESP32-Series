---
gsd_state_version: "1.0"
milestone: v4.0
current_phase: 5
status: completed
stopped_at: Stable v4.0.0 published from f555a22; all six assets anonymously downloaded and checksum-verified. Charger/SD acceptance passed. Milestone archived; no remaining acceptance item.
last_updated: "2026-11-26T22:58:32.416Z"
last_activity: 2026-11-26
last_activity_desc: Milestone v4.0 completed and archived
state_head: f555a22c141e1155fbb338503ebd6dca32c692e2
progress:
  total_phases: 5
  completed_phases: 5
  total_plans: 0
  completed_plans: 0
  percent: 100
milestone_name: "PanicMiner: owned open firmware"
current_phase_name: Hashrate + Soak
---

# State

Current Phase: 5
Phase Name: Hashrate + Soak
Status: v4.0 milestone complete
Milestone: v4.0 PanicMiner. All five phases and all 22 requirements verified.

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

- Stable v4.0.0 is the latest public release at tag f555a22; all six assets downloaded anonymously and matched local/GitHub digests and both checksum manifests. All 63 release files/ZIP entries passed the private-value scan. The application/factory bytes are identical to rc.2.
- Canonical public source: https://github.com/kleinpanic/NMMiner-ESP32-Series, branch main.
- Historical kleinpanic/NerdMiner-CYD-Custom is private, archived and points to the canonical repo.
- Public CI 45897395376 passed exact firmware source 07c3931: both builds, C++ sanitizers, restore safety, Python tests, both UI themes and artifacts.
- Local Python: 108 tests, 29 subtests; public: 107 tests, 29 subtests, one explicit private-image skip. All execution used root .venv; no dependency change.
- History, exact private-value and binary scans passed. Secrets, wallet values, backups and raw logs stay ignored.
- Source and license boundary: owned open application; Espressif radio binaries and ROM remain platform components.

## Completed charger acceptance

Owner-confirmed transfer to charger with SD, absent laptop USB serial device,
then read-only LAN verification: 70.234 seconds at 969.003 kH/s,
authorized saved primary, advancing work, new SD record and current wallet.
Initial wallet warmup and three transient API errors recovered automatically;
the stable window had no transport error or observed work/SD counter reset.
HW-05 complete. No coding or physical acceptance item remains.

## Decisions

- Preserve the verified ILI9341_2_DRIVER + inversion setup; vendor calls the observed unit ST7789.
- Wi-Fi is 2.4 GHz. BOOT hold opens non-destructive Setup; saved startup retries in the background.
- Pages stay manual. The 40 ms Monitor target is 25 fps; preserve the measured 5% hashrate limit.
- Both pools use the same owner-configured payout address. Accepted shares are work, not payouts; no accepted block or payout is claimed.
- Full 4 MB verified backup before firmware writes. Keep raw evidence and recovered vendor assets private.

## Session

**Last session:** 2026-11-26
**Stopped at:** Stable v4.0.0 published from f555a22; all six assets anonymously downloaded and checksum-verified. Charger/SD acceptance passed. Milestone archived; no remaining acceptance item.
**Resume file:** docs/panicminer-validation-2026-11-25.md

## Current Position

Phase: Milestone v4.0 complete
Plan: —
Status: Complete; no next milestone requested
Last activity: 2026-11-26 — Milestone v4.0 completed and archived

## Operator Next Steps

None. Miner remains authorized on charger power with SD: final 60-second rate 967.595 kH/s, 13 saved SD records and wallet age 12 seconds. Leave the qualified firmware running; take a fresh full backup before any future write.
