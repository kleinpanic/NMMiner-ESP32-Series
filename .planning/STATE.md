---
gsd_state_version: "1.0"
milestone: v4.0
milestone_name: "PanicMiner: owned open firmware"
current_phase: 1
status: executing
stopped_at: USB/SD endurance and saved-settings reboot PASS on 23236906 / source 49afce5. Owner authorized public source and release; final secret scan, publication and release verification are underway. Remaining hardware checks are explicitly release-candidate limitations.
last_updated: "2026-11-26T17:56:42.617Z"
state_head: e15c836dc3d8b96ee4af5b4d7d2634cdd200d9bd
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

- Current installed application 23236906a43745274836183fa75348ceca927dee2350919bea08c25ce96de986 (1,781,040 bytes), firmware source 49afce5. Fresh full 4 MB backup and all four written images verified; matching factory inputs/ELF archived. USB/SD composite qualification PASS: strict 1,203.988 seconds / 222 samples / 967.899 kH/s, all 17 checks; same-boot hour 3,614.418 seconds / 236 samples / 967.925 kH/s, one new pool-accepted share, 60 SD records and 78 complete in-window feeds. All 23 continuous checks passed: zero API errors, reboots, native faults, radio drops, SHA mismatches or uncommanded navigation. Full post-window CSV contained 409 nine-column records. Smoke/private paths, complete/half-closed exports, immediate follow-up/cancellation, ten pressure exports / 66 status requests and 65-second JSON/SD port reuse passed. Five pages and controlled Wi-Fi recovery passed in 36.216 seconds. Subsequent observation reached 164.51 minutes on one boot without native faults. A separate saved-settings EN reset with SD passed: pool, wallet data and persisted lifetime 3 accepted / best 3.077973282 restored. This is laptop USB restart evidence, not charger cold boot. C++ sanitizers/build and 105 Python tests / 29 subtests pass. Earlier rejected images and failures remain in the validation record. No firmware changed after source 49afce5.
- Earlier scheduling image f9b46f96 passed the original 1,200.984-second performance window at 968.076 kH/s (225 samples), then its SD CSV download stalled and radio dropped. Composite qualification failed and soak was interrupted. The first bounded-transfer image a21a35e3 then aborted in JobPush under download memory pressure and restarted; it is rejected. Heap tracing measured a 1,396-byte largest block during another transfer.
- Earlier continuous-work f8da51e3 passed feed/page/reconnect checks, then failed after 63.264 seconds at 944.1116 kH/s. Native timing isolated a 272.937 ms job-preparation pause; hardware SHA lock waits stayed at most 25 µs. The failed attempt and interrupted soak are retained.

- Current evidence is in [runtime validation](../docs/panicminer-validation-2026-11-25.md). SD/card-removed results are separate; no composite SD-enabled qualification PASS is established yet.
- The parser-memory image 696e07eb avoided the earlier heap abort and recovered after a controlled Wi-Fi drop in 44.047 seconds, but failed a strict 10-second rate reading (932.41 kH/s). Its timed run and soak are preserved as failed/interrupted.
- The earlier experiment e5ab82cf (1,779,840 bytes) mounted SD and authorized the pool, but equal feed/software-miner priority produced repeated TLS connection resets and the feed smoke check timed out. It is rejected.
- Earlier application 4f318f75542122b963d6303676ac025550609cc995854066f6da00887b1e87f4 (1,779,856 bytes) retains authenticated HTTPS across 45-second updates at feed priority 2. Fresh full 4 MB backup checksum/flash comparison and all four installed images verified. Repeated 5/5 Chain/wallet cycles passed; reuse reduced fetch time from 10.0 to 1.3–1.6 seconds with stable active TLS scratch and zero SDK fallbacks.
- Connection-reuse prequalification passed five pages, Mine restore and controlled Wi-Fi recovery in 41.811 seconds. Its strict timed attempt then failed after 76.995 seconds: 10-second rate 931.66 kH/s, 60-second rate 962.12 kH/s. Pool, SD and wallet stayed valid and no native fault occurred. The failed run and interrupted soak are preserved; no endurance PASS.
- Earlier bounded-work application e0bf4c691ba7d073a94b2ab267e59b98966e8d0b16e6d59650ff88893412c71f (1,779,488 bytes), source commit b686681, records actual completed work at most every 32K nonces and removes per-job queue/accounting delay. Its fresh full 4 MB backup passed checksum and full flash comparison; all four installed images independently verified. Sixteen batch sanitizer cases, full host suite, board build and 91 Python tests pass.
- Bounded-work native prequalification passed SD, complete Chain/wallet cycles, all five pages and controlled Wi-Fi recovery in 42.250 seconds, with no native fault or SHA mismatch/drop. Its strict attempt failed after 10.401 seconds at 943.44 kH/s (minimum 944.3); the 60-window rate was 964.05. Pool, SD and wallet remained valid. Failed run and interrupted soak are preserved. Timing diagnostics measured 52 ms between-job and 129 ms preparation gaps; they recovered from a controlled Wi-Fi drop in 43.063 seconds and were interrupted for the continuous-work repair. No endurance acceptance is inferred.
- Earlier native Stats capture is complete and SD CSV contains measured nine-column records. Configured payout address and confirmed/pending amounts match an independent explorer query privately; no address or wallet amount belongs in tracked evidence.
- After the passing endurance results, the owner authorized publishing the single repository and releasing the project. Publish as v4.0.0-rc.1 with remaining physical/egress/portal acceptance explicit; the full milestone is still open. Charger cold boot with SD is not established by the EN reset.
- Airtime fairness remains disabled on the EA8300; WPA2 is unchanged. The original radio failure is not isolated. No router changes are part of this qualification.
- Saved settings, backups, captures and raw logs remain ignored. Every firmware write requires a fresh verified full 4 MB backup. Daily use is saved 2.4 GHz Wi-Fi/pool, manual left/right pages and BOOT hold for Setup.

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

- Finish native pressure and endurance qualification of d67f5107/source d5a39e4. Preserve all prior failures. Feed evidence now uses the actual sample window, with regression coverage; intentional preflight disconnects cannot falsely fail the timed window. All real API/native/rate/wallet/SD gates remain.
- Standalone charger cold boot with SD awaits the owner's physical availability. Continue all unblocked USB work; do not prompt again tonight or infer this gate from an EN reset.
- One-hour soak and native egress capture remain broader milestone gates. Accepted shares are observed; no accepted block or payout is claimed. Do not mark all five phases complete from this repair.
- Manual debounced pages supersede the old timer rotation requirement. Physical left/right and held-press behavior were confirmed earlier; native post-SD Stats capture is complete.

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

**Last session:** 2026-11-26T17:56:42.594Z
**Stopped at:** USB/SD endurance and saved-settings reboot PASS on 23236906 / source 49afce5. Owner authorized public source and release; final secret scan, publication and release verification are underway. Remaining hardware checks are explicitly release-candidate limitations.
**Resume file:** docs/panicminer-validation-2026-11-25.md

### Blockers

- No blocker remains for the authorized release candidate. Full milestone acceptance still requires charger cold boot with SD, complete egress capture, portal/fallback-pool persistence and observed audio/LED hardware checks.
