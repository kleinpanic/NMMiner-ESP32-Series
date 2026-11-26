---
gsd_state_version: "1.0"
milestone: v4.0
milestone_name: "PanicMiner: owned open firmware"
current_phase: 1
status: executing
stopped_at: SD shared-SPI repair built and host-tested; waiting for laptop USB with SD inserted to back up, flash, verify card logging/display/touch and complete the 20-minute Mine test. Publication held.
last_updated: "2026-11-26T05:48:57.215Z"
state_head: c17498a4d71b1a199a7f3be42f2f8724966a7c2d
progress:
  total_phases: 5
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
current_phase_name: Base + See
---

# State

Current Phase: 1 — Base + See
Milestone: v4.0 PanicMiner. v3.0 closed as superseded on 2026-11-24
(`archive/v3.0-nmminer-binary-re/README.md`).

## Device

- Current hardware evidence is in [runtime validation](../docs/panicminer-validation-2026-11-25.md). Earlier failed joins below are historical, superseded by the November 26 recovery.
- Installed normal application SHA-256: `82dca8181f64c04be39835fc3db2ea94e5076265988061d77bd2affe51644548`. The common success-path reconnect safeguard in source commit `92a0f4d` is build-checked but not installed; the saved-network boot path tested on hardware already enables reconnection.
- Owner confirmed USB charger power near the router with the SD card removed. Laptop USB disappeared; JNAP independently listed the 2.4 GHz client and its API reported the saved worker pool-authorized, best restored to 0.702206052, and a 999.4 kH/s startup reading. Saved-settings EN restart also passed before moving to the charger.
- All five Chain endpoints, native Stats/Chain frames and zero SHA mismatches/drops were observed before USB removal. These serial observations do not establish continuous post-unplug TLS/SHA coverage.
- Airtime fairness is disabled on the EA8300; WPA2 remains configured. Placement, power cycling and the router setting changed together, so no single root cause is established.
- Two API observers stopped on status-read timeouts; later responses remained authorized with increasing work. The continued charger observer was interrupted for the owner-requested SD update after 54 samples through 787 seconds. HTTP/ICMP and router association were lost around the card change; no cause is established without native logs. There is no new 20-minute PASS.
- Saved configuration and full verified backups remain ignored. Every future firmware write requires a fresh verified full 4 MB backup. Use the checked flash/restore scripts.
- Daily use: USB power, saved 2.4 GHz Wi-Fi and pool; tap left/right for manual pages, hold BOOT for Setup at 192.168.4.1.

## Implementation history (earlier fork commits)

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
  - The historical work-counter race found in this review was repaired in root commit `599e170` using coherent locked increments and snapshots.

## Repos

- Canonical repository: https://github.com/kleinpanic/NMMiner-ESP32-Series (root branch `main`). Firmware, tooling, tests and reverse-engineering notes are ordinary tracked files in this repository.
- Source consolidation is commit `a730def`; the previous nested Git history is preserved locally under ignored `external/`.
- The former `kleinpanic/NerdMiner-CYD-Custom` repository is historical. The owner requests one canonical public repository, with no release claim before hardware acceptance.
- GitHub description and topics have been updated. Secrets and raw hardware evidence remain local and excluded from publication.

## Open questions

- Owner requests SD re-enabled and tested. SDStore/display VSPI ownership repair builds and passes the production host checks. Candidate SHA-256 is `2a58b816ae8b1cbe94c8814f37b0c11208c89ca9ee0cad47f475400249e28fb9`; it is not installed. Wait for laptop USB, take a fresh verified full backup, install, verify native SD CSV/display/touch and restart the full 20-minute Mine run.
- Intermittent HTTP status timeouts need diagnosis if they recur. At the last check the miner stayed associated, pool-authorized and advancing work, with 12/12 ICMP replies.
- SD reinsertion, a one-hour unattended soak, native egress capture and a pool-confirmed share/block remain unverified. Zero accepted shares is an observed result, not evidence of a payout or a license problem.
- The owner's manual-page requirement supersedes the old UI-04 timer-rotation wording. Touch direction and one action per held press were confirmed on hardware.
- Historical phase checklists have broader gates than the current delivery; do not mark all five phases or all requirements complete from this closeout.

## Decisions

- [Phase 1]: 2026-11-25 hardware update supersedes the earlier Device/Open questions: Chain TLS fixed by freeing 12 KB of LVGL reservation; matching screens flashed; four native captures complete; two automatic restart recoveries passed. Full evidence: docs/panicminer-validation-2026-11-25.md. — RSA verification failed on fragmented heap, including with the HW miner paused. The 20-minute mean was 972.624 kH/s against 994 (minus 2.151%); 11 rolling readings reached below the 5% band, so per-reading acceptance remains open.
- [Phase 1]: 2026-11-25 latest runtime work supersedes earlier device notes: manual debounced touch, verified block targets, honest pool acceptance, CPU/RAM/SD stats, nonblocking audio, and five-minute enabled NVS checkpoints are implemented with 126 passing native host cases plus audio tests. Runtime-stats firmware flashed with verified full backup; automatic Wi-Fi association is currently failing. Adaptive radio-profile recovery build is being installed; native acceptance remains open. — Do not mark hardware or hashrate acceptance complete from host tests. Current evidence is docs/panicminer-validation-2026-11-25.md; source and Espressif binary boundaries are docs/firmware-provenance.md.
- [Phase 1]: 2026-11-25 owner requests one canonical public repo with firmware source and reverse-engineering notes, and explicitly forbids release until hardware works. Firmware snapshot bf08ec8 is consolidated into ordinary source files under firmware/custom/NerdMiner-CYD-Custom; nested Git history remains local under ignored external/. Both GitHub repositories remain private; no public release or parent push is authorized while acceptance is open. — Latest firmware verifies after full backup but sees the AP and fails association. Stored credentials match the private configuration. Host tests pass; native pool/Chain/SD persistence and current performance still need verification. The owner is near the router; distance is not established as the cause.
- [Phase 1]: SD-isolation native test failed: four Wi-Fi profiles, zero SD access, 43 AUTH_EXPIRE events in 126 seconds; app d2c601e8cb409714b72ba47269231b1e3e7900bb1bdca13e87b28f71934dd495. — The user is away and authorized disabling SD access. SD now mounts only after association or missing-internal-config recovery. Software initialization did not explain this failure; physical card power and USB power-cycle tests remain pending. Keep publication and hardware acceptance open.
- [Phase 1]: Stored wallet checksum and recovered-wallet match verified; host pool subscription and authorization succeeded and received difficulty/work using the backed-up settings. — This isolates configuration and host reachability from the unresolved board Wi-Fi failure. No native mining or payout claim; raw configuration and logs remain ignored. Source consolidation is locally committed as a730def and publication is held.
- [Phase 1]: User confirmed physical SD removal plus USB reconnection. The d2c601e8 build now joins, authorizes the saved wallet with the pool, mines, restores saved best on EN reset, and renders complete native Stats and Chain screens. The first timed run then failed with a Wi-Fi drop and a 415 kH/s minimum; do not mark stability passed. — Card removal and USB power cycle changed together, so neither is isolated as the cause. Preserve failed evidence locally. Fix the verified merkle buffer overrun, long-coinbase truncation, hot-path serial logging and incoherent work snapshots before retesting; keep publication held.
- [Phase 1]: Owner disclosed moving the miner farther from the router while the laptop charged, then moved it back. Failed joins after the initial recovery are confounded by that relocation; native scans ranged from -62 to -80 dBm. RF-only recalibration succeeded but did not restore joining at that position. — Do not diagnose SD, firmware load, radio hardware or credentials as the sole cause. A read-only Wi-Fi isolation test was interrupted after 49 seconds to restore normal mining firmware before the owner relocates it; it did not complete all profiles. Restore app e8cfc642 and verify from the new position.
- [Phase 1]: Owner requires laptop USB; alternate power test unavailable. Near-router scan sees two WPA2 APs; association still fails. — Do not treat distance or power as established causes. Test early reconnect policy and explicit AP/channel selection in normal firmware.
- [Phase 1]: JNAP verified credentials and no MAC filter. Proximity improved signal to -38 dBm with airtime fairness disabled; native mining, pool authorization and all five Chain feeds recovered. Saved-settings EN restart passed. — Placement, USB power cycle and router option changed together, so root cause is not isolated. Keep working firmware and router configuration; verify charger cold boot and fresh timed run before publication.
- [Phase 1]: Owner confirmed charger power. Laptop USB device is absent; JNAP and the miner API independently confirm 2.4 GHz, pool authorization, preserved best difficulty and about 999 kH/s after cold boot. — Standalone startup is verified. A fresh API-based 20-minute run is measuring reported rates and work-counter progress; full post-unplug serial SHA and TLS events are unavailable.
- [Phase 1]: Owner requests SD card use and a firmware update, superseding the temporary card-removed closeout. Shared-VSPI ownership repair is committed as c17498a, builds for CYD and passes production SD tests plus an old-code negative control. — The display and SD previously owned separate SPIClass(VSPI) instances, which reconfigure the same controller with independent locks. New code uses the display SPI instance and GPIO-matrix output sharing; touch retains HSPI. A fresh verified backup, native SD CSV/display/touch checks and the full 20-minute Mine test require laptop USB. Do not claim installation or hardware acceptance from host tests.

## Session

**Last session:** 2026-11-26T05:48:10.302Z
**Stopped at:** SD shared-SPI repair built and host-tested; waiting for laptop USB with SD inserted to back up, flash, verify card logging/display/touch and complete the 20-minute Mine test. Publication held.
**Resume file:** docs/panicminer-validation-2026-11-25.md

### Blockers

- Native SD update and endurance test require the owner to reconnect the miner to laptop USB with the SD inserted. Shared-VSPI repair builds and passes production host tests, but is not flashed. Miner API and router association disappeared around the card change. No public release or push until the directed hardware checks pass.
