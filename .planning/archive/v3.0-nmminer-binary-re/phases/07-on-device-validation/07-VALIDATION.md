# Phase 7 — On-Device Validation Runbook & Evidence

**Phase:** 07-on-device-validation · **Plan:** 07-01 / 07-02 · **Requirements:** VALID-01, VALID-02, VALID-03
**Status:** CUSTOM DECODED-SPEC ST7789 FLASHED + VERIFIED; recovery AP web clipping fixed; touch input unresolved
**Verifier status:** See `07-VERIFICATION.md`; flash/partition evidence is strong, physical touch/display and custom mining proof remain pending.

> Evidence-first flash runbook. The executor fills each section below with live output as it
> walks the plan. This file IS the proof artifact for VALID-01/02/03.
>
> `07-01-SUMMARY.md` is intentionally absent as of the latest recheck. The 07-01 hardware
> validation plan is not complete: hard physical power-cycle, physical panel/touch proof,
> Wi-Fi/pool/mining evidence, API/dashboard checks, and restore proof remain open. Do not
> create a 07-01 summary unless those gates close or the plan is formally abandoned.

## 2026-06-20 reconstructed v2.0.02 ST7789 baseline no-write validation

The active route is official `v2.0.02` `esp32-2432s028r-st7789` binary
source-equivalent reconstruction, not fork UI iteration.

Validated locally without upload/flash/erase/write targets:

- Host evidence harness compiled and ran with
  `src/official_v2002_st7789.cpp`, `src/recovered_app_lifecycle.cpp`,
  `src/recovered_sprice_behavior.cpp`,
  `src/recovered_swarm_behavior.cpp`, and `tools/print_baseline.cpp`.
- Focused SPrice host test compiled and passed with
  `tools/test_sprice_behavior.cpp`.
- Focused swarm host test compiled and passed with
  `tools/test_swarm_behavior.cpp`.
- WatchCoins output: default tokens `BTC ETH BNB LTC EOS ETC XRP BCH ZEC NEO`,
  default count `10`, overflow sample cap count `20`, token 20 `T`, truncation
  offset `39`, trailing tokens detected.
- SPrice row/cache status: 24-byte row adapter, `+16` price-descending ordering
  key, 16-row rendered trim, and cache-copy seam validated; exact equal-price
  and NaN ordering behavior remains open.
- Swarm writer/publisher status: writer records `4`, publisher records `3`,
  layout offsets recovered, writer quartet recovered, publisher pairing
  recovered, `+0x08` count recovered, `+0x0c/+0x10` exact float meanings still
  unrecovered, and `+0x14` has no direct Miner publisher in the current trio.
- Lifecycle validation remains `8/10` with `flashable=0`; official behavior
  parity and fresh-backup write gate are open.
- `uvx platformio run -d firmware/reconstructed/nmminer-v2-baseline` passed.
  Generated `firmware.bin` is `575616` bytes. PlatformIO reported flash use
  `575253` bytes.
- Generated `partitions.bin` byte-matches the official `v2.0.02` ST7789
  partition table with SHA-256
  `f1882db7f6e352825314f3dcc5db01085e74625655c5f1decf58dd84fa118226`.
- Guarded write targets `upload`, `program`, `erase`, `uploadfs`, and
  `uploadfsota` all failed with the expected reconstructed-firmware guard.

No firmware write was performed in this validation pass.

## Device & artifact facts (locked at plan time)

| Item | Value |
|------|-------|
| Port | `/dev/ttyUSB0` (CH340) |
| Chip | ESP32-D0WD-V3 rev 3.1 |
| esptool | `.venv/bin/python ~/.platformio/packages/tool-esptoolpy/esptool.py` (v4.5.1) |
| pio | `.venv/bin/pio` from the parent repo root |
| Env | `ESP32-2432S028R` (huge_app, upload 921600, monitor 115200) |
| firmware.bin | Current live custom webfix2 artifact from the 2026-06-19T05:36Z app write: `firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/firmware.bin` was 2,840,400 B with SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` at flash/readback time. |
| Offsets | bootloader@0x1000, partitions@0x8000, boot_app0@0xe000, firmware@0x10000 |
| Backup dir | `hardware/backups/` (parent-repo root, gitignored `.gitignore:8`) |

---

## 2026-06-19 recovery portal CSS fix app flashes + readback evidence

The review-05 recovery AP recheck found a real narrow-layout defect in the WiFiManager portal: the 320 px root page and 390 px Configure WiFi form clipped at the right edge. The final flashed source adds a custom WiFiManager head style in `src/wManager.cpp` to remove the default wrapper minimum-width behavior on narrow viewports, force border-box sizing, and allow controls/messages/tables to stay inside the viewport.

Build gate for the final CSS fix:

```text
uvx platformio run -e ESP32-2432S028R
ESP32-2432S028R SUCCESS
firmware.bin payload 2840400 bytes
```

Artifact proof for the final CSS fix:

| Artifact | Size | SHA-256 |
|----------|------|---------|
| `.pio/build/ESP32-2432S028R/firmware.bin` | `2840400` | `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5` |
| `firmware/v1.0.0-54-gb83a12a-dirty/ESP32-2432S028R_factory.bin` | `2908160` | `ec320e3e61edad45b405ba172ae7804d53463551676bfdfe343834ee2b50b5a4` |

No-device source/build recheck during the later GSD continuation:

- `uvx platformio run -e ESP32-2432S028R` passed without upload/flash/erase/write commands.
- Build reported RAM 38.0%, flash 90.1%, and `ESP32-2432S028R SUCCESS` in 00:00:18.134.
- Rebuilt `.pio/build/ESP32-2432S028R/firmware.bin` remained 2,840,400 bytes with SHA-256 `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, matching the already-flashed webfix2 app artifact.
- This is source/build continuity evidence only; it is not a new device write.

Fresh backups before the two web UI writes:

| Backup file | Size | SHA-256 |
|-------------|------|---------|
| `hardware/backups/cyd-backup-20260619-052605-pre-web-ui-fix-app-flash.bin` | `4194304` | `9fab4b71f3e28f142a193a0d5464effd4575c62a7ce2438a1a2ed2b4c21e7a25` |
| `hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin` | `4194304` | `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed` |

Write and readback for the final CSS fix:

- App-only write at `0x10000`; bootloader, partition table, NVS, and LittleFS were not intentionally rewritten.
- `write_flash -z 0x10000 firmware.bin` wrote `2,840,400` bytes and ended with `Hash of data verified`.
- Independent readback from `0x10000` for `2,840,400` bytes produced `/tmp/nmminer-app-readback-20260619-053633-web-ui-fix2.bin`.
- Readback SHA-256 was `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`, matching the built app; `cmp` also matched the built artifact.

Post-flash boot and touch checks:

- The earlier filtered/sanitized post-flash smoke ran 35s, produced twelve lines, and found 0 panic lines.
- Touch still reported no contact: 12 diagnostic observations had `directPressed=0` / `libPressed=0`, and 0 lines showed a pressed state.
- A later no-write reset-only 45s touch window read 49 lines, found 0 panic lines, and counted 31 touch diagnostics with 0 pressed observations and 0 IRQ-low observations. The touch init line reported HSPI pins `clk=25 mosi=32 miso=39 cs=33 irq=36`.
- A later passive no-write serial window on 2026-06-19T17:33Z wrote only to `/tmp/p7-serial-passive-inline-20260619-173344.log`. It captured 5,177 bytes / 51 lines, SHA-256 `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`, reached recovery AP at `192.168.4.1`, and counted 23 touch diagnostics with 0 pressed observations and 0 IRQ-low observations.
- Raw serial output was not stored in tracked files.

Recovery AP web UI recheck after the final CSS fix:

- Wi-Fi scan showed open SSID `nmap-2.4g` with signal 100.
- Host joined `nmap-2.4g` on `wlp0s20f3`.
- GET probe results were recorded in `/tmp/p7-webfix2-ap-get-probe.tsv`:

```text
/                 200  4848 bytes
/info             200  6915 bytes
/wifi             200  6881 bytes
/0wifi            200  6693 bytes
/api/status       404  16 bytes
/api/system/info  404  16 bytes
/sd/list          404  16 bytes
```

Current screenshot evidence, kept only under `/tmp`:

| Screenshot | SHA-256 |
|------------|---------|
| `/tmp/p7-webfix2-ui/root-320x568.png` | `b197bc60189ee3e0284aac0ba5595d8489447fc4f982714688fe591166f5dea0` |
| `/tmp/p7-webfix2-ui/root-390x844.png` | `79b7734d4f38257dec56dd00e51c261784be2147923d8d0f498aefd70a92a7a0` |
| `/tmp/p7-webfix2-ui/root-1440x900.png` | `cdae787b7fcb4f7b3b6e362ad8c0009e81705d47827049260f7f12dda688f137` |
| `/tmp/p7-webfix2-ui/wifi-320x568.png` | `e453060fa5951afc3a7f9d7d09abd31285ae4b55a6397c5d731d1151f098fe1c` |
| `/tmp/p7-webfix2-ui/wifi-390x844.png` | `a44ca57f584a674a73c8a335a87343fe092e05c3c0aa564517875a8655f1a658` |
| `/tmp/p7-webfix2-ui/info-390x844.png` | `66a759d2f057e4951d19fcf92b12d679a90a2f3e4b9254330859f95cdade9b0d` |

Visual result:

- Root portal renders without the previous right-edge clipping at 320x568, 390x844, and 1440x900.
- Configure WiFi renders the SSID/password and custom primary/fallback pool fields without the previous right-edge clipping at 320x568 and 390x844.
- `/info` confirms the current sketch size as `2840400 / 5986128`, matching the webfix2 app payload size.
- Custom JSON/SD routes still return 404 in recovery portal mode. This remains an API exposure decision/gap, not a portal reachability failure.

This closes the browser recovery-portal clipping finding. It does not prove the physical CYD panel, touch layer, LED behavior, 2.4 GHz station Wi-Fi, mining, accepted-share behavior, dashboard, or restore round trip.

---

## 2026-06-19 review-05 mining/config hardening app flash + readback evidence

Superseded by the later recovery portal CSS app flash above, but kept as historical evidence for the mining/config hardening implementation.

`07-CURRENT-FIRMWARE-REVIEW-05.md` found three blockers and two warnings after the prior app flash. All five were fixed before the next firmware write, then the app was rebuilt, backed up-before-write, flashed app-only, and independently read back.

Implemented in source:

- fixed `checkValid()` to copy the target bytes correctly, reverse with a non-underflowing loop, and compare target/work lexicographically
- validated compact `nbits` length/hex input and bounds-checked the target buffer write before mining-job construction
- made pool endpoint parsing strict and transactional across portal/API/NVS import paths so invalid `host:abc`, `host:`, and out-of-range ports are rejected instead of partially persisted
- checked NVS mining-stat restore/save failures, wrote stats transactionally, and committed the stats namespace
- widened the stratum subscribe password buffer from 20 to 80 bytes

Build gate:

```text
uvx platformio run -e ESP32-2432S028R
ESP32-2432S028R SUCCESS
RAM 38.0% (124552 / 327680 bytes)
Flash 90.1% (2833213 / 3145728 bytes)
firmware.bin payload 2839504 bytes
```

Artifact proof:

| Artifact | Size | SHA-256 |
|----------|------|---------|
| `.pio/build/ESP32-2432S028R/firmware.bin` | `2839504` | `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee` |
| `firmware/v1.0.0-54-gb83a12a-dirty/ESP32-2432S028R_factory.bin` | `2908160` | `efac2c55a377ea966d7f818f9db6a9fd6e44d6e46faa1c04efc1bf6165d5e383` |

Fresh backup before write:

| Backup file | Size | SHA-256 |
|-------------|------|---------|
| `hardware/backups/cyd-backup-20260619-045710-pre-review05-fix-app-flash.bin` | `4194304` | `99541243892212422fa7071137deb8e00183d3c18dd23de706380dafbd67b5b8` |

Write and readback:

- App-only write at `0x10000`; bootloader, partition table, NVS, and LittleFS were not intentionally rewritten.
- `write_flash -z 0x10000 firmware.bin` wrote `2839504` bytes and ended with `Hash of data verified`.
- Independent readback from `0x10000` for `2839504` bytes produced `/tmp/nmminer-app-readback-20260619-050100-review05-fix.bin`.
- Readback SHA-256 was `d31c5e551940e188fd37fac348b5f1ccbdeb06948047dde73d31e25896c980ee`, matching the built app; `cmp` also matched the built artifact.

Post-flash boot check:

- A filtered/sanitized 26s serial check read 51 lines and found 0 panic/abort/backtrace/brownout lines.
- It showed `NerdMiner v2 starting......`, TFT_eTouch HSPI initialization on CLK=25/MOSI=32/MISO=39/CS=33/IRQ=36, no serial config, no usable NVS config, SD app settings imported without usable Wi-Fi credentials, config mode entered, and `Config IP Address: 192.168.4.1`.
- Touch still reported no contact: 23 diagnostic observations had no pressed state; 0 lines showed a pressed state.
- Raw serial output was not stored in tracked files.

Remaining gap after this section: no hard physical USB unplug/replug, physical panel, touch electrical probe, 2.4 GHz Wi-Fi/pool runtime, primary/fallback outage test, accepted share, audio, LED, dashboard, or restore-round-trip proof has been captured for this live app.

---

## 2026-06-19 review-05 recovery AP web UI recheck

Superseded by the later recovery portal CSS fix and `/tmp/p7-webfix2-ui/` screenshots above. This section is kept to document the defect that triggered the web UI fix.

The host was temporarily joined to the live miner recovery AP after the review-05 app flash. This was web-portal evidence for the older `d31c5e55...` app, not the older `/tmp/p7-custom-ui/` screenshot set and not the final webfix2 app.

Network and HTTP evidence:

- Wi-Fi scan showed open SSID `nmap-2.4g` with signal 100.
- Host joined `nmap-2.4g` on `wlp0s20f3` and received `192.168.4.2/24`.
- `curl http://192.168.4.1/` returned HTTP 200 with 3,992 bytes.
- `curl http://192.168.4.1/info` returned HTTP 200 with 6,060 bytes.
- GET probe results were recorded in `/tmp/p7-current-review05-ap-get-probe.tsv`:

```text
/                 200
/info             200
/wifi             200
/0wifi            200
/api/status       404
/api/system/info  404
/sd/list          404
```

Current screenshot evidence, kept only under `/tmp`:

| Screenshot | Size | SHA-256 |
|------------|------|---------|
| `/tmp/p7-current-review05-ui/root-1440x1000.png` | `19612` | `f79108de1b672037d5be9dab5bf4253c6179b88fd2cfce37a59fb42322c30632` |
| `/tmp/p7-current-review05-ui/root-390x844.png` | `14811` | `8750c7e16725bc6fbe0ddb31743fdf2e589ce80d9350005bbaa39719c353a870` |
| `/tmp/p7-current-review05-ui/root-320x568.png` | `12746` | `e9036545a571cf053741941d8469b153247c881329a9d06e153ed109ca5458d0` |
| `/tmp/p7-current-review05-ui/info-390x844.png` | `33291` | `c1284c05ef7d5ab42a4ff7b6a78dd4bf869b9b7e9db3d43cad37dee3e2001d5d` |
| `/tmp/p7-current-review05-ui/wifi-390x844.png` | `31139` | `864424353c047f801601cd889eae65f64a205d3a289bdac6940802f1642016cd` |

Visual result at that superseded review-05 point:

- Root portal was centered and usable at 1440x1000 and 390x844.
- Root portal had horizontal clipping at 320x568: the right edge of buttons and the message box were cut off.
- Configure WiFi rendered on that review-05 app and included the custom/fallback pool fields, but the 390x844 form also clipped horizontally at the right edge.
- `/info` confirms the current sketch size as `2839504 / 5985232`, matching the review-05 app payload size.
- Custom JSON/SD routes still return 404 in recovery portal mode. This remains an API exposure decision/gap, not a portal reachability failure.

This improves current web-UI evidence, but it does not prove the physical CYD panel, touch layer, LED behavior, 2.4 GHz station Wi-Fi, mining, accepted-share behavior, dashboard, or restore round trip.

---

## 2026-06-19 post-forensics source-blocker fix app flash + readback evidence

Superseded by the later review-05 mining/config hardening app flash above, but kept as historical evidence for the post-forensics source-blocker implementation.

The 2026-06-19 forensics/code-review pass found source blockers that would make later mining/runtime UAT misleading. Those blockers were fixed, rebuilt, backed up-before-write, flashed app-only, and independently read back.

Implemented in source:

- fixed the merkle-root buffer terminator in `src/utils.cpp`
- redacted stratum authorize/submit serial logs and bounded message formatting in `src/stratum.cpp`
- gated SD list/download/log routes behind the config-mode mutation unlock and denied sensitive SD filenames in `src/webapi.cpp`
- made the mutation unlock expire after 10 minutes instead of being unreachable after the API restarts
- migrated legacy LittleFS `/config.json` and WiFiManager-stored credentials before falling back to portal mode in `src/wManager.cpp`
- scrubbed imported SD config after successful promotion, validated pool ports, and set `NM_hashing` once a mining job is accepted

Build gate:

```text
pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
ESP32-2432S028R SUCCESS
RAM 38.0% (124496 / 327680 bytes)
Flash 90.0% (2831549 / 3145728 bytes)
firmware.bin payload 2837840 bytes
```

Artifact proof:

| Artifact | Size | SHA-256 |
|----------|------|---------|
| `.pio/build/ESP32-2432S028R/firmware.bin` | `2837840` | `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6` |
| `firmware/v1.0.0-54-gb83a12a-dirty/ESP32-2432S028R_factory.bin` | `2904064` | `39b7b527ba256ce43fece6bc505641f98d36c914ac79a90ecff306849118d67b` |

Fresh backup before write:

| Backup file | Size | SHA-256 |
|-------------|------|---------|
| `hardware/backups/cyd-backup-20260619-042756-pre-source-blocker-fix-app-flash.bin` | `4194304` | `d3c97e45dad2ebb98d00b089c431be7f0affea4af9f3cee3ed75da3680bcdbf9` |

Write and readback:

- App-only write at `0x10000`; bootloader, partition table, NVS, and LittleFS were not intentionally rewritten.
- `write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x10000 firmware.bin` wrote `2837840` bytes and ended with `Hash of data verified`.
- Independent readback from `0x10000` for `2837840` bytes produced `/tmp/nmminer-app-readback-20260619-043113-source-blocker-fix.bin`.
- Readback SHA-256 was `67a158a063923c19cb762f433c1209e582eae03d4d7de3ecb32fc67e0ae193b6`, matching the built app.

Post-flash boot check:

- A filtered/sanitized 24s serial check read 49 lines and found 0 panic/abort/backtrace/brownout lines.
- It showed SD mounted, no serial config received, no usable NVS Wi-Fi credentials, SD app settings imported without usable Wi-Fi credentials, and config mode entered at `192.168.4.1`.
- Touch still reported no contact: 22 diagnostic lines had `directPressed=0` and `libPressed=0`; 0 lines had a pressed state.
- Raw serial output was not stored in tracked files.

Remaining gap after this section: no hard physical USB unplug/replug, physical panel, touch electrical probe, 2.4 GHz Wi-Fi/pool runtime, primary/fallback outage test, accepted share, audio, LED, dashboard, or restore-round-trip proof has been captured for this live app.

---

## 2026-06-19 fallback-pool app flash + readback evidence

The previous UAT audit found that `HW-04` still required primary/fallback pool behavior, while the source only used the primary pool. That source mismatch is now fixed and the app is live on the board. Runtime mining UAT still remains open.

Implemented in source:

- fallback pool fields in `TSettings` and NVS keys (`pool2`, `pool2_port`, `pool2_pw`, `wallet2`)
- WiFiManager portal, serial `NMCFG`, SD `/sd/config.json`, and API fields for `SecondaryPool`, `SecondaryPort`, `SecondaryAddress`, and `SecondaryPassword`
- bounded/redacted secret handling for secondary wallet/password values
- mining failover from primary to fallback, with primary reachability re-probed every 10s while on fallback and immediate switch-back when primary returns
- full URL parsing for `stratum+tcp://host:port` style pool values

Build gate:

```text
pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
ESP32-2432S028R SUCCESS in 00:00:39.389
RAM 38.0% (124488 / 327680 bytes)
Flash 89.9% (2827837 / 3145728 bytes)
firmware.bin payload 2834128 bytes
```

Artifact proof:

| Artifact | Size | SHA-256 |
|----------|------|---------|
| `.pio/build/ESP32-2432S028R/firmware.bin` | `2834128` | `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9` |
| `firmware/v1.0.0-54-gb83a12a-dirty/ESP32-2432S028R_factory.bin` | `2899968` | `c9ef9f8949902857c90ae3d90febb661ddccc856502fd983c43543c1dde22e3c` |

Fresh backup before write:

| Backup file | Size | SHA-256 |
|-------------|------|---------|
| `hardware/backups/cyd-backup-20260618-235938-pre-fallback-pool-flash.bin` | `4194304` | `11acf9261f0d3ef97c6295522027e8efdda58b3020210176f15634585007309c` |

Write and readback:

- App-only write at `0x10000`; bootloader, partition table, NVS, and LittleFS were not intentionally rewritten.
- `write_flash -z --flash_mode dio --flash_freq 40m --flash_size 4MB 0x10000 firmware.bin` wrote `2834128` bytes and ended with `Hash of data verified`.
- Independent readback from `0x10000` for `2834128` bytes produced `/tmp/nmminer-app-readback-20260619-000220.bin`.
- Readback SHA-256 was `49d318889eaef3f8e9d80a37b3e4a256ede586252475ff35277979521b0bc3a9`, matching the built app.
- Filtered string check found `v2.0.02`, `NMMiner`, `nmap-2.4g`, `192.168.4.1`, `SecondaryPool`, and `SecondaryAddress`; no `V1.8.3` string was found in the filtered scan.

Post-flash boot check:

- A filtered 22s serial check read 46 lines and found 0 panic/abort lines.
- It showed SD mounted, no serial config received, NVS empty, SD app settings imported without usable Wi-Fi credentials, and config mode entered.
- Raw serial output was not stored in tracked files.

Remaining gap after this section: no 2.4 GHz Wi-Fi/pool runtime, primary/fallback outage test, accepted share, physical panel, touch, audio, LED, dashboard, or restore-round-trip proof has been captured for this live app.

---

## 2026-06-18 current code-review fix reflash + recovery web UI evidence

The Phase 7 subagent code review found two remaining blockers after the previous decoded-spec flash: runtime `LittleFS.begin(true)` paths that could still format the decoded asset partition, and legacy `SDCard` ownership that could initialize SD before `SDStore`. Both were fixed in source, rebuilt, backed up-before-write, and reflashed as an app-only update so NVS and decoded LittleFS data were preserved.

Source fixes in this pass:

- `src/ui_nm/nm_screensaver.cpp` and `src/webapi.cpp`: removed runtime `LittleFS.begin(true)` format fallbacks; mount failures now fail closed and preserve the asset partition.
- `src/NerdMinerV2.ino.cpp`: disables the legacy global `SDCard SDCrd` owner on `ESP32_2432S028R` / `ESP32_2432S028_2USB`; `SDStore` owns SD for this target.
- `src/drivers/storage/SDCard.cpp`: redacts legacy config JSON/wallet serial output in case that path is ever reintroduced.

Fresh backup before this write:

| Step | Backup file | Size |
|------|-------------|------|
| Pre current code-review fix app reflash | `hardware/backups/cyd-backup-20260618-231853-pre-current-code-review-fix-reflash.bin` | `4194304` |

Checksum is stored only in the ignored sidecar file beside the backup and is not copied into tracked docs.

Build gate:

```text
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
ESP32-2432S028R SUCCESS in 00:00:36.917
RAM 38.0% (124392 / 327680 bytes)
Flash 89.7% (2820405 / 3145728 bytes)
firmware.bin payload 2826704 bytes
```

Flashed and verified:

- app-only flash: `firmware.bin` at `0x10000`
- `write_flash` reported `Hash of data verified`
- independent `verify_flash 0x10000 .../firmware.bin` passed with digest match
- independent `verify_flash 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed with digest match after the app reflash

Evidence logs:

- `/tmp/p7-readback-20260618-231853-pre-current-code-review-fix-reflash.log`
- `/tmp/p7-flash-current-code-review-fix-app-20260618-232007.log`
- `/tmp/p7-verify-current-code-review-fix-app-20260618-232048.log`
- `/tmp/p7-verify-decoded-littlefs-after-current-app-reflash-20260618-232100.log`

Post-flash serial boot reached the expected recovery path:

```text
NerdMiner v2 starting......
[nm_touch] TFT_eTouch HSPI initialized pins clk=25 mosi=32 miso=39 cs=33 irq=36
[SD] SD card mounted successfully
[CFG] no serial config received
NVS: No stored configuration available.
[SD] Config read: 126 bytes
[CFG] SD config imported app settings but has no usable WiFi credentials
AllDone:
[WEBAPI] Mutating API config-mode gate enabled
*wm:StartAP with SSID:  nmap-2.4g
*wm:AP IP address: 192.168.4.1
Entered Configuration Mode
Config SSID: nmap-2.4g
Config IP Address: 192.168.4.1
*wm:Starting Web Portal
```

Touch remained unresolved after the reflash. Serial still reports no physical press signal:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 directZ1=3 directZ2=4083 directP=15 directPressed=0 ... libPressed=0
```

Recovery web UI review was executed against the current custom firmware:

- Host joined the miner AP and received `192.168.4.2`.
- `curl http://192.168.4.1/` returned HTTP 200 and the WiFiManager root page.
- Browser screenshots were captured at `/tmp/p7-custom-ui/recovery-root-1440x1000.png` and `/tmp/p7-custom-ui/recovery-root-390x844.png`.
- The recovery UI renders coherently at desktop and mobile widths: no visible overlap, readable labels, and the expected `Configure WiFi`, `Info`, `Exit`, and `Update` actions.
- The workstation Wi-Fi was restored after the review.

This proves the current custom recovery web portal, not the physical LVGL panel UI. Physical display/LED/touch review remains open.

Additional safe GET-only AP probe:

```text
/                                 200
/info                             200
/probe                            404
/alive                            404
/api/status                       404
/api/system/info                  404
/api/setting/preference           404
/api/setting/mining               404
/api/sd/list                      404
/api/update/screensaver/preflight 404
```

Evidence log: `/tmp/p7-current-ap-get-probe-20260618-233135.tsv`.

This confirms the WiFiManager recovery portal is reachable in AP mode, while the custom JSON API is not reachable on port 80 during that portal state. No POST, settings mutation, restart, credential, or mining-configuration request was sent.

### 2026-06-18 user-report recheck: custom app still live, touch still no-contact

After the user reported a red LED, an AP join/auth/captive-portal popup, and the miner no longer responding to touches, the board was checked non-destructively. No firmware write was performed in this pass.

Flash identity:

- `verify_flash 0x10000 firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/firmware.bin` passed: the live app region matches the current custom decoded-spec ST7789 build.
- `verify_flash 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed: the decoded LittleFS asset image is still present byte-for-byte.
- At that time, before the later 2026-06-19 identity reflash, `strings firmware.bin` found `V1.8.3` and did not find `v2.0.02`; the custom source used `CURRENT_VERSION` for rebuilt on-panel labels. This was superseded by the 2026-06-19T00:20Z custom app reflash below, which intentionally changed the rebuilt firmware identity to recovered `v2.0.02`.

Evidence logs:

- `/tmp/p7-verify-current-custom-app-after-user-report-20260618-193918.log`
- `/tmp/p7-verify-decoded-littlefs-after-user-report-20260618-193918.log`
- `/tmp/p7-serial-after-user-report-verify-20260618-193931.log`

Fresh serial after the verify/reset still reaches the recovery portal:

```text
[CFG] no serial config received
NVS: No stored configuration available.
[CFG] SD config imported app settings but has no usable WiFi credentials
AllDone:
[WEBAPI] Mutating API config-mode gate enabled
*wm:StartAP with SSID:  nmap-2.4g
*wm:AP IP address: 192.168.4.1
Entered Configuration Mode
Config SSID: nmap-2.4g
Config IP Address: 192.168.4.1
*wm:Starting Web Portal
```

Touch still fails below UI routing:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 directZ1=3 directZ2=4083 directP=15 directPressed=0 ... libPressed=0
```

Interpretation: the board is flashed with the custom decoded-spec firmware and decoded asset image. The recovery AP/captive portal behavior is expected WiFiManager behavior in no-credentials config mode. The unresolved blocker remains physical touch signal verification/re-identification, not another stock restore or blind firmware reflash.

### 2026-06-18 19:58 EDT live recheck: same app/assets verified, touch still no-contact

A second live recheck was run after the follow-up user report. This was non-destructive: no `write_flash`, `erase_region`, `pio upload`, settings POST, or credential mutation was performed.

Flash identity:

- `verify_flash 0x10000 firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/firmware.bin` passed with `-- verify OK (digest matched)`, byte count `0x2b21d0` / `2,826,704`.
- `verify_flash 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed with `-- verify OK (digest matched)`, byte count `0xe0000` / `917,504`.
- At that time, before the later 2026-06-19 identity reflash, the custom app artifact SHA-256 was `d5db11dbf14633e2a7740f5dc82aeb196c8e13a8587727ca2805ef087501f0b4`; it contained `V1.8.3` and did not contain `v2.0.02`. This was superseded by the app-only identity reflash below.

Evidence logs:

- `/tmp/p7-verify-live-current-custom-app-20260618-195813.log`
- `/tmp/p7-verify-live-current-decoded-littlefs-20260618-195813.log`
- `/tmp/p7-serial-live-current-after-verify-20260618-195813.log`

Fresh raw serial after the verify/reset still reports the active no-contact touch signature:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 directZ1=3 directZ2=4083 directP=15 directPressed=0 ... libPressed=0
```

Interpretation: the device is still running the current custom decoded-spec ST7789 app with the decoded LittleFS image intact. The AP join/auth popup is expected captive-portal behavior from WiFiManager. The red LED is expected while the custom firmware is waiting for configuration or connecting. Touch remains a physical signal/path issue below the UI layer.

### 2026-06-19 00:20 UTC custom app identity reflash: rebuilt firmware now reports v2.0.02

The rebuilt custom fork now intentionally reports the recovered NMMiner `v2.0.02`
identity. This aligns the visible/API version with the reverse-engineered stock
spec while keeping the firmware as a rebuilt custom image, not the proprietary
official stock binary.

Source change:

- `firmware/custom/NerdMiner-CYD-Custom/src/version.h` now defines `CURRENT_VERSION` as `v2.0.02`.
- Loading/config screen comments were updated so the version slot is treated as recovered stock-parity identity, not NerdMiner base identity.

Build and artifact proof:

- Build command: `.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R`
- Result: SUCCESS in 34.05 seconds.
- RAM: 38.0% (`124392 / 327680` bytes).
- Flash: 89.7% (`2820405 / 3145728` bytes).
- App artifact: `firmware/custom/NerdMiner-CYD-Custom/.pio/build/ESP32-2432S028R/firmware.bin`
- Size: `2826704` bytes.
- SHA-256: `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1`.
- String check: contains `v2.0.02`, `NMMiner`, `Make it better`, `nmap-2.4g`, and `192.168.4.1`; no `V1.8.3` / `v1.8.3` string remains.

Fresh backup before write:

- Backup: `hardware/backups/cyd-backup-20260619-001919-pre-v2002-identity-app-reflash.bin`
- Size: `4194304` bytes.
- SHA-256: `a92ead9b2572b057b4ab4e9ffbd3f6cc4864002fefe747ea57335e02822e053b`.

Write and live verification:

- Wrote only the app partition at `0x10000`; bootloader, partition table, NVS, and LittleFS were not intentionally rewritten.
- `write_flash -z 0x10000 firmware.bin` wrote `2826704` bytes and ended with `Hash of data verified`.
- `verify_flash --diff no 0x10000 firmware.bin` passed with `-- verify OK (digest matched)`.
- `verify_flash --diff no 0x310000 /tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` passed with `-- verify OK (digest matched)`.

Interpretation: the live custom decoded-spec firmware now presents `v2.0.02`
where the recovered NMMiner firmware version belongs. Version text alone cannot
prove stock-vs-custom identity anymore; use the app artifact SHA, flash offset,
and source provenance. The unresolved hardware blocker remains touch/panel
physical validation, not binary decoding or app flashing.

### 2026-06-19 00:28 UTC post-identity serial boot/runtime capture

After the `v2.0.02` identity app reflash and verify, a fresh serial capture was
taken without any firmware write or settings mutation.

Evidence log:

- `/tmp/p7-serial-post-v2002-identity-20260619-002803.log`
- Window: `2026-06-19T00:28:03Z` to `2026-06-19T00:29:04Z`
- Size: `6881` bytes / `70` lines
- SHA-256: `40fa9402bc1cd51b9dc0875a80e726de5c17ef491ad6d9aa14257f5dbd765659`

Runtime evidence:

- Boot mode: `POWERON_RESET`, `SPI_FAST_FLASH_BOOT`.
- App starts: `NerdMiner v2 starting......`.
- Touch bus initializes on `clk=25 mosi=32 miso=39 cs=33 irq=36`.
- SD card mounts successfully.
- No stored Wi-Fi configuration is present; SD imports app settings but has no usable Wi-Fi credentials.
- WiFiManager recovery portal starts with SSID `nmap-2.4g` and AP IP `192.168.4.1`.
- Grep found no `Guru Meditation`, `PANIC`, `Stack canary`, `abort(`, `Rebooting`, `boot loop`, or `LoadProhibited` markers in the capture.

Touch evidence remains failing below UI routing:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 ... directPressed=0 ... libPressed=0
```

Interpretation: after the identity reflash, the app boots cleanly to the same
recovery AP state with SD mounted and no crash loop. The no-touch symptom is
unchanged and still requires the physical XPT2046/FPC/PENIRQ probe path before
more touch firmware tuning.

### 2026-06-19 01:19 UTC no-flash touch software review + source cleanup build

After the user reported the board still was not responsive to touches, a focused
GSD touch software review was run without hardware, serial, flash, upload, erase,
or settings mutation.

Review result:

- Report: `.planning/phases/07-on-device-validation/07-TOUCH-SOFTWARE-REVIEW.md`
- Reviewed at: `2026-06-19T01:16:29Z`
- Findings: 0 critical, 1 warning
- Verdict: no critical source/pin/bus bug explains the current no-contact
  signature; physical XPT2046/FPC/PENIRQ probing remains the next gate.

The warning was real but not enough to explain the observed idle/no-pressure
diagnostics: legacy `display.cpp::doLedStuff()` still dispatched to the CYD
display driver's old touch reader from the monitor task. That path could poll
`touch.getXY()` and mutate legacy screen/backlight state outside the rebuilt
LVGL/no-VSPI ownership model.

Source cleanup:

- `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/display.cpp`
  now no-ops `doLedStuff()` for `ESP32_2432S028R` and
  `ESP32_2432S028_2USB`, matching the existing CYD no-ops for legacy draw and
  animation paths.
- No firmware was flashed for this cleanup.

Build gate:

```text
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
ESP32-2432S028R SUCCESS in 00:00:25.42
RAM 38.0% (124392 / 327680 bytes)
Flash 89.7% (2820381 / 3145728 bytes)
firmware.bin payload 2826672 bytes (superseded by the 03:04 UTC source-review-fix build below)
```

Artifact status:

- Superseded local unflashed `firmware.bin` SHA-256:
  `a2452282b428e8fbea9939a84958b4c36b6937896b8bca31526d108e724c92aa`
- Superseded local unflashed `firmware.elf` SHA-256:
  `31b4cfff79b1574cff0d16b2efd59f9b11a5f62381744e4b66465fabaa4bdc6a`
- This section is historical. The then-live
  `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1`
  app was later superseded by fallback-pool, source-blocker, review-05, and
  webfix2 app flashes. Current live authority is the webfix2 custom decoded-spec
  app: 2,840,400 bytes, SHA-256
  `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

Interpretation: this removes a warning-level software ownership defect from the
next build, but it does not change the current live touch evidence. The next
useful action remains physical probing/re-identification, not another blind
threshold, rotation, calibration, or gesture-routing patch.

### 2026-06-19 03:04/03:20 UTC no-flash firmware review 02/03 fixes + build

A fresh `gsd-code-reviewer` pass wrote
`.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-02.md`.
It found one source blocker for any future reflash: `readCustomAPName()` could
over-read an unterminated raw flash buffer and log raw config payloads. It also
flagged `.ss` upload header validation as a defensive hardening item.

A follow-up review wrote
`.planning/phases/07-on-device-validation/07-CURRENT-FIRMWARE-REVIEW-03.md`.
It confirmed the over-read was fixed, then found two remaining source-side
cleanup items: the custom AP name could still reach Serial through the
WiFiManager config callback, and the upload path still opened the final LittleFS
path before full validation.

Source fixes:

- `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`: `readCustomAPName()`
  now reads into a terminated buffer, parses bounded data, and logs redacted
  status instead of raw flash/config payloads. The config portal callback now
  redacts the AP SSID in serial output, and WiFiManager debug output is disabled.
- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`: screensaver upload now
  buffers and validates the complete 8-byte `.ss` header before writing; only
  320x240 / `0x01,0xA4` panel files are accepted on this path. Uploads are
  staged under `/ss/.upload.tmp`, write byte counts are checked, and the final
  carousel path is only promoted after a complete valid upload.
- No firmware was flashed for this cleanup.

Build gate:

```text
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
ESP32-2432S028R SUCCESS in 00:00:28.665
RAM 38.0% (124392 / 327680 bytes)
Flash 89.7% (2821205 / 3145728 bytes)
firmware.bin payload 2827504 bytes
```

Artifact status:

- Local unflashed `firmware.bin` SHA-256:
  `32f13d063443e2c1c0cc14b4a29b41a61ee2a5a71ce12d2d0c25777171c9d16b`
- Local unflashed `firmware.elf` SHA-256:
  `a2c9006df15333cb2b1d3d0bdb7faf7db3ce584390b86ba5721a0d0433d7f8cf`
- Local unflashed merged factory image SHA-256:
  `8baddbe21a2272fba1cc0fc2f9af9fc4037b7db1497b7636f84388220d032510`
- This source-review section is historical. The then-live
  `6adc7771ef01ce1f3641fe9995875b13b8e9e916d57fa11a270532c706b7cfa1`
  app was later superseded by fallback-pool, source-blocker, review-05, and
  webfix2 app flashes. Current live authority is the webfix2 custom decoded-spec
  app: 2,840,400 bytes, SHA-256
  `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

Interpretation: the source-side flash blocker from the latest firmware review is
closed for future builds, but this does not change live-device touch evidence.
The next no-write gate remains physical hard power-cycle confirmation and
XPT2046/FPC/PENIRQ probing.

### 2026-06-19 17:33 UTC passive no-write serial capture

A passive serial listen was run with DTR/RTS held low. No `write_flash`,
`erase_region`, `read_flash`, `verify_flash`, `pio upload`, settings POST,
credential entry, or firmware rebuild happened in this pass.

Evidence log:

- `/tmp/p7-serial-passive-inline-20260619-173344.log`
- Window: 25s, ending around `2026-06-19T17:33:44Z`
- Size: `5177` bytes / `51` lines
- SHA-256: `a1225796e171ed241031049961d69b8a05e63256e7716d8b07f34d7716ca0bd8`

Runtime evidence:

- Boot mode marker present: `POWERON_RESET`, `SPI_FAST_FLASH_BOOT`.
- WiFiManager recovery portal is active: `Entered Configuration Mode`, `Config IP Address: 192.168.4.1`.
- Touch evidence remains no-contact: `23` touch diagnostics, `0` pressed observations, and `0` IRQ-low observations.

Representative sanitized touch line:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 directZ1=3 directZ2=4087 directP=11 directPressed=0 ... libPressed=0
```

Interpretation: the device is still alive in the custom recovery AP state, and
the active touch path still sees no raw press. This capture does not satisfy the
missing human hard-power-cycle gate because no operator-confirmed USB
unplug/replug evidence was collected.

### 2026-06-19 02:06 UTC read-only continuation serial capture

A fresh serial listen was run with DTR/RTS held low. No `write_flash`,
`erase_region`, `read_flash`, `verify_flash`, `pio upload`, settings POST,
credential entry, or firmware rebuild happened in this pass.

Evidence log:

- `/tmp/p7-serial-readonly-continuation-20260619-020602.log`
- Window: `2026-06-19T02:06:02Z` to `2026-06-19T02:06:22Z`
- Size: `4607` bytes / `55` lines
- SHA-256: `ae240cbed25eb7588dfe05f69615401cc83524c992632ffbab0d48cd03a3a045`

Runtime evidence:

- Boot mode: `POWERON_RESET`, `SPI_FAST_FLASH_BOOT`.
- App starts: `NerdMiner v2 starting......`.
- Touch bus initializes on `clk=25 mosi=32 miso=39 cs=33 irq=36`.
- SD card mounts successfully.
- No stored Wi-Fi configuration is present; SD imports app settings but has no usable Wi-Fi credentials.
- WiFiManager recovery portal starts with SSID `nmap-2.4g` and AP IP `192.168.4.1`.
- Marker scan found `POWERON_RESET`, `nmap-2.4g`, `192.168.4.1`, `directPressed=0`, and `libPressed=0`; it did not find `Guru Meditation` or `PANIC`.

Touch evidence remains the same no-contact class:

```text
[nm_touch] rot=3 irq=1 miso=0 directX=0 directY=4095 ... directPressed=0 ... libPressed=0
```

Interpretation: the board is still alive in the custom recovery AP state and the
same active touch path still sees no raw press. This fresh serial capture does
not satisfy the missing human hard-power-cycle gate because no operator-confirmed
USB unplug/replug evidence was collected.

---

## 2026-06-18 custom decoded-spec reflash + decoded asset preservation evidence

The on-screen `v2.0.02` seen earlier was the official stock recovery baseline, not the custom decoded-spec firmware at that time. As of the 2026-06-19 identity reflash above, the custom rebuild intentionally also reports `v2.0.02`; flash digest and source provenance distinguish it from the stock binary. This earlier pass rebuilt and reflashed the custom `ESP32-2432S028R` ST7789 fork, then reflashed the recovered LittleFS `.ss` asset image produced from the decoded stock binary assets.

Root cause fixed before this reflash: `TFT_eTouch::init()` calls `TFT_eTouchBase::init()`, which mounts/formats SPIFFS on ESP32. On this partition table, the `spiffs` data partition is the same `0x310000..0x3EFFFF` region the custom firmware uses as LittleFS for recovered stock screensaver assets. Runtime evidence from the previous attempt showed this formatter wiping the decoded asset image before LittleFS could mount it. The custom source now initializes the touch CS pin directly instead of calling `touch.init()` on the CYD path:

- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/drivers/displays/esp23_2432s028r.cpp`

Fresh backups preserved before writes:

| Step | Backup file | Size |
|------|-------------|------|
| Pre patched custom app/data reflash | `hardware/backups/cyd-backup-20260618-184025-pre-no-touch-spiffs-format-reflash.bin` | `4194304` |
| Pre NVS clear after final custom proof | `hardware/backups/cyd-backup-20260618-185101-pre-nvs-clear-after-custom-decoded.bin` | `4194304` |

Checksums are stored only in ignored sidecar files beside the backups and are not copied into tracked docs.

Build gate:

```text
/tmp/p7-build-no-touch-spiffs-format-20260618-183904.log
ESP32-2432S028R SUCCESS in 00:00:25.761
RAM 38.0% (124408 / 327680 bytes)
Flash 89.7% (2820981 / 3145728 bytes)
firmware.bin payload 2827280 bytes
```

Flashed and independently verified:

- bootloader at `0x1000`
- partitions at `0x8000`
- boot_app0 at `0xe000`
- custom firmware at `0x10000`
- decoded LittleFS image `/tmp/p7-custom-littlefs-stock-ss-20260618-221540.bin` at `0x310000`

`verify_flash` passed for the app stack and the decoded LittleFS region. The LittleFS region was verified again after runtime boots and again after NVS was cleared:

```text
0x310000 decoded LittleFS image -- verify OK (digest matched)
after runtime boots -- verify OK (digest matched)
after NVS clear + clean boot -- verify OK (digest matched)
```

Clean recovery state was restored by erasing only the NVS partition (`0x9000`, size `0x5000`) after a fresh backup. Final serial evidence shows the custom firmware now has no stored config and starts the documented recovery AP:

```text
[CFG] no serial config received
NVS: Opening config namespace...
NVS: Config namespace ready.
NVS: No stored configuration available.
AllDone:
*wm:StartAP with SSID:  nmap-2.4g
*wm:AP IP address: 192.168.4.1
Entered Configuration Mode
Config SSID: nmap-2.4g
Config IP Address: 192.168.4.1
*wm:Starting Web Portal
```

Temporary evidence logs:

- `/tmp/p7-flash-no-touch-spiffs-format-app-*.log`
- `/tmp/p7-verify-no-touch-spiffs-format-app-*.log`
- `/tmp/p7-flash-decoded-littlefs-no-touch-format-*.log`
- `/tmp/p7-verify-decoded-littlefs-after-runtime-no-touch-format-*.log`
- `/tmp/p7-erase-nvs-after-custom-decoded-*.log`
- `/tmp/p7-serial-clean-nvs-custom-decoded-20260618-185211.log`

Temporary laptop AP testing was stopped and deleted after use; host Wi-Fi was restored to `Homebase5`.

Remaining evidence gaps:

- Physical display/LED confirmation still requires operator/camera observation.
- Touch diagnostics still show no press signal (`directPressed=0`, `libPressed=0`, low pressure values), so touchscreen responsiveness remains unresolved and needs hardware/pin/calibration diagnosis.
- Wi-Fi/pool join, mining-active proof, hard USB unplug/replug proof, and restore-from-backup proof are still open; VALID-02 and VALID-03 remain incomplete.

---

## 2026-06-18 hardware flash evidence

The user explicitly re-authorized flashing in chat after the previous gate. Before any write, a fresh full 4 MB flash backup was preserved. The firmware target was switched from the stale ILI9341 flag to the repo-observed `esp32-2432s028r-st7789` path, rebuilt, flashed, and verified. A follow-up flash fixed the no-config recovery portal so `nmap-2.4g` remains available instead of timing out after 180 seconds.

Final serial boot evidence shows the device reaches configuration mode without a Guru Meditation reboot:

```text
NerdMiner v2 starting......
[CFG] no serial config received
SPIFS: Mounted
SPIFS: No config file available!
[SD] config.json not found
AllDone:
*wm:StartAP with SSID:  nmap-2.4g
*wm:AP IP address: 192.168.4.1
Entered Configuration Mode
Config SSID: nmap-2.4g
Config IP Address: 192.168.4.1
*wm:Starting Web Portal
```

Host Wi-Fi scan immediately after the corrected boot saw the AP:

```text
nmap-2.4g:100:
```

Temporary evidence files are intentionally under `/tmp` and not tracked:

- `/tmp/p7-build-timeout-fix.log`
- `/tmp/p7-upload-timeout-fix.log`
- `/tmp/p7-verify-timeout-fix.log`
- `/tmp/p7-serial-timeout-fix-filtered.log`
- `/tmp/p7-serial-timeout-fix.raw.log`

Remaining evidence gap: physical display/LED confirmation, hard USB unplug/replug, Wi-Fi/pool configuration, mining-active proof, and restore proof.

## 2026-06-18 official ST7789 binary restore/reflash evidence

After touch diagnostics on the custom fork continued to show no touch-contact signature, the device was restored to the official local `v2.0.02` `esp32-2432s028r-st7789` binary set as a recovery/baseline test. This is the binary fallback lane: it proves the board can boot the vendor image and exposes whether failures are custom-firmware-specific or hardware/variant-level.

Before the write, another full 4 MB backup was preserved:

| Field | Value |
|-------|-------|
| Backup file | `hardware/backups/cyd-backup-20260618-172750-pre-official-st7789-restore.bin` |
| Size (bytes) | `4194304` |
| sha256 | Held outside tracked docs; not copied into git-tracked files |
| Git status | Backup path is ignored and not staged |

Official files flashed:

- `firmware/v2.0.02/esp32-2432s028r-st7789/bootloader.bin` at `0x1000`
- `firmware/v2.0.02/esp32-2432s028r-st7789/partitions.bin` at `0x8000`
- `firmware/v2.0.02/esp32-2432s028r-st7789/boot_app0.bin` at `0xe000`
- `firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin` at `0x10000`
- `firmware/v2.0.02/esp32-2432s028r-st7789/littlefs.bin` at `0x390000`

`verify_flash` passed for all five regions:

```text
bootloader.bin -- verify OK (digest matched)
partitions.bin -- verify OK (digest matched)
boot_app0.bin -- verify OK (digest matched)
firmware.bin -- verify OK (digest matched)
littlefs.bin -- verify OK (digest matched)
```

Stock serial boot evidence:

```text
WiFi connection timeout!
WiFi connection failed!
*wm:StartAP with SSID:  nmap-2.4g
*wm:AP IP address: 192.168.4.1
Config timeout: 299/300s remaining, 0 client(s) connected
```

Host recovery/UI evidence:

- Host scan saw `nmap-2.4g` with strong signal.
- Host joined the AP and received `192.168.4.2/24`.
- `curl http://192.168.4.1/` returned HTTP 200 and the NMMiner Wi-Fi setup page.
- Initial browser screenshots were captured under `/tmp/p7-ui-shots/`; the later verified stock reflash screenshots are under `/tmp/p7-stock-ui/`.
- Root portal desktop layout is centered and usable.
- Root portal mobile layout initially showed 390 px horizontal clipping; the later `/tmp/p7-stock-ui/` pass showed the root page usable at desktop, 390 px, and 320 px, with only the 320 px title wrapping.
- Wi-Fi form mobile layout is usable but exposes sensitive configuration fields; screenshots and HTML remain under `/tmp` and are not tracked.

The stock image was reflashed again after the user reported the unit was red/not responding to touch. This second stock write was also preceded by a fresh full backup:

| Field | Value |
|-------|-------|
| Backup file | `hardware/backups/cyd-backup-20260618-174623-pre-stock-st7789-reflash.bin` |
| Size (bytes) | `4194304` |
| sha256 | Held outside tracked docs; not copied into git-tracked files |
| Flash log | `/tmp/p7-flash-official-st7789-20260618-174740.log` |
| Verify log | `/tmp/p7-verify-official-st7789-20260618-174829.log` |

The second stock `verify_flash` pass succeeded for all five regions: bootloader, partitions, boot_app0, app, and LittleFS.

Second stock runtime/UI evidence:

- Serial boot reached WiFiManager config mode and started AP `nmap-2.4g`.
- Host joined the AP as `192.168.4.2/24`.
- `curl http://192.168.4.1/` returned HTTP 200 and a 4,442-byte setup page.
- Browser screenshots were captured under `/tmp/p7-stock-ui/`.
- Root portal desktop and 390 px mobile layouts are usable.
- At 320 px, the root title wraps but remains usable.
- The Wi-Fi form renders at 390 px and 320 px, but long wallet/pool values clip inside inputs; screenshots remain in `/tmp` only because the form displays sensitive/default mining fields.
- Host Wi-Fi was restored to `Homebase5` after the review.

Current device state after this historical stock section has been superseded. Stock `v2.0.02` remains the verified recovery baseline, but the live unit was later reflashed with the patched custom decoded-spec ST7789 fork and decoded LittleFS assets.

Remaining evidence gap: physical panel touch/display confirmation under the current custom firmware, hard USB unplug/replug, Wi-Fi/pool configuration, mining-active proof, and full backup-restore round trip.

## 2026-06-18 CR-03 custom API hardening attempt and re-review

The read-only code review in `07-CODEX-REVIEW.md` left one unresolved blocker after the earlier portal/touch fixes: mutating custom API routes were unauthenticated and not gated by any local physical/config-mode intent. The working tree now adds a physical-presence write gate:

First attempt, later rejected by review:

- `include/webapi.h` exposed `webApiAuthorizeMutationsFor(uint32_t window_ms)`.
- `src/webapi.cpp` rejected mutating routes with HTTP 403 unless the unlock window was active.
- `src/ui_nm/nm_uimanager.cpp` opened a 5 minute write window on physical touch/button navigation or config gestures.
- Read-only status/discovery/info/list/download endpoints remained available without the write window.

Build gate:

```text
/tmp/p7-build-cr03-mutation-gate.log
ESP32-2432S028R SUCCESS in 00:00:39.925
RAM 38.0% (124416 / 327680 bytes)
Flash 90.7% (2851693 / 3145728 bytes)
firmware.bin payload 2857984 bytes
```

Subagent re-review reopened CR-03 as a blocker. The short physical unlock was too broad because ordinary page navigation could open the write window, and it was not scoped strictly to an explicit config/admin mode.

The follow-up software pass replaced that broad unlock in code:

- `include/webapi.h` / `src/webapi.cpp`: replaced the timed physical unlock with `webApiSetConfigMode(bool)` and `403 config mode required`.
- `src/ui_nm/nm_uimanager.cpp`: removed all casual short-tap/page-next API unlock calls.
- `src/wManager.cpp`: WiFiManager portal lifecycle enables the config-mode gate on explicit config entry and locks it before the API listener resumes.

The same review found a second custom-fork blocker: sensitive settings still leaked through serial/SD/API paths. The follow-up software pass addressed that in code:

- `writeConfigToSD()` no longer writes `BtcWallet` or `PoolPassword` to removable SD.
- Serial logging prints redacted markers for wallet/pool password.
- Legacy `SDCard.cpp` wallet logging is redacted.
- `/api/system/info` and `/api/setting/mining` redact wallet-bearing read fields while exposing configured booleans.

Two warning-level target/verification issues were also addressed:

- `platformio.ini` `default_envs` is narrowed to `ESP32-2432S028R`.
- The stale `NM_TOUCH_SOFTSPI` contract is renamed to `NM_TOUCH_NO_VSPI`; comments now document the active HSPI/TFT_eTouch plus direct-probe path.

Build gate:

```text
/tmp/p7-build-config-gate-secret-redaction.log
ESP32-2432S028R SUCCESS in 00:01:37.510
RAM 38.0% (124416 / 327680 bytes)
Flash 90.6% (2851553 / 3145728 bytes)
firmware.bin payload 2857856 bytes
```

At the time of this software pass, the code had not yet been reflashed and did not satisfy VALID-01/02/03 by itself. The later 2026-06-18T22:53Z pass supersedes that state: the patched custom ST7789 build is now flashed, and the decoded LittleFS asset region is verified after runtime boots and NVS clear. Physical UI/touch, hard power-cycle, Wi-Fi/pool/mining, and restore proof remain open.

Follow-up source/evidence cleanup on 2026-06-19 rechecked the compiled touch bus contract with:

```bash
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R -t envdump
```

The active `CPPDEFINES` include `TOUCH_ENABLE` and `NM_TOUCH_NO_VSPI`; they do not include `USE_HSPI_PORT` or `CONFIG_TFT_eSPI_ESPIDF`. The live CYD loop calls only `nm_touch_poll_gesture()` and gates out legacy `TouchHandler`. Stale comments that still called the active path SoftSPI were corrected. Historical Phase 4 SoftSPI artifacts are superseded by this Phase 7 no-VSPI/HSPI touch contract and are not evidence of a current flash mismatch.

A later touch software review found one warning-level legacy `doLedStuff()` second-reader defect. That source path is now a CYD no-op. Subsequent firmware reviews fixed the bounded/redacted webflasher config parse, config portal AP serial redaction, WiFiManager debug output, `.ss` upload header/staging validation, primary/fallback-pool support, post-forensics source blockers, review-05 mining/config hardening blockers, and the WiFiManager recovery portal narrow-layout clipping. The 2026-06-19T05:36Z webfix2 app flash supersedes the earlier unflashed cleanup, fallback-pool, source-blocker-fix, and review-05 artifacts; the current live app SHA-256 is `f9547fbb43af286c9adb5833d961dde0fd981e043a9bf0787656c16e44aff9f5`.

## 07-02 pre-flash software repair evidence (2026-06-18)

This section records the software hardening pass before any next hardware action. It does not satisfy VALID-01/02/03 by itself; those remain gated on the T1-T7 hardware runbook below.

**Completed code repairs**

- H1 / CR-01: NVS Wi-Fi config is authoritative; SD `/sd/config.json` is fallback/import-only and is consulted only when NVS has no usable SSID.
- H1 / CR-01: firmware no longer writes `WifiPW` to removable SD config; `writeConfigToSD()` mirrors non-Wi-Fi-secret runtime config only.
- H1 / WR-04: SD config reads now reject zero-length or oversized files before parsing, so a truncated `/sd/config.json` cannot become partial config.
- H1 / CR-02: WiFiManager portal parameters now outlive `init_WifiManager()`, and portal saves copy WiFiManager SSID/password into `Settings` before NVS persistence.
- H1 / WR-01: SD path normalization is idempotent for `/sd` and `/sd/...`, so API SD list/download/log paths do not become `/sd/sd/...`.
- H1 follow-up: `wManager.cpp` no longer falls back to legacy `SDCrd.loadConfigFile()` / `SD2nvMemory()` after `tryConfigFromSD()` fails, so the old 512-byte unvalidated SD import path cannot bypass the new candidate checks.
- H1 follow-up: `wManager.cpp` no longer calls `SDCrd.terminate()` after `sdStore.mount()`, avoiding teardown of the shared Arduino `SD` singleton while `SDStore` still believes the card is mounted.
- H5 / WR-01: Loading and Config LVGL roots set explicit black backgrounds with `LV_OPA_COVER`.
- H2 / CR-02: WiFiManager config mode now routes AP SSID and portal IP to the LVGL Config page through `nm_uimanager_show_config()` without persisting `LastPage`.
- H3 / WR-02: touch mapping is rotation-aware for the landscape display path; rotation 3 inverts both base rotation-1 axes and raw/calibrated diagnostics log `rot`, `rawX`, `rawY`, `px`, and `py` for the next on-device calibration pass.
- H4 / WR-02/WR-03: display settings are re-applied after `init_WifiManager()` loads/promotes config; brightness, invert colors, and supported landscape `RotateScreen` values also apply through the preference API.
- H4 follow-up: `/api/setting/preference` now rejects unsupported `RotateScreen` values before mutating settings and normalizes supported values to the landscape values the LVGL display driver actually applies.

**Build gate**

Command run:

```bash
.venv/bin/pio run -d firmware/custom/NerdMiner-CYD-Custom -e ESP32-2432S028R
```

Result: SUCCESS in 00:00:23.101. RAM 38.0% (124,376 / 327,680 bytes), flash 90.6% (2,849,041 / 3,145,728 bytes). Generated firmware payload at 0x010000 was 2,855,344 bytes.

**Review gate**

`07-REVIEW-02.md` found two blocking WiFiManager save/lifetime issues and one SD path warning after the first repair pass. Those were fixed and re-reviewed in `07-REVIEW-03.md`; its final follow-up note closes the remaining SD import warning. A later read-only subagent review found residual legacy `SDCard` fallback/teardown blockers and an unsupported `RotateScreen` persistence warning. Those are now fixed in `wManager.cpp` and `webapi.cpp`, rechecked by the subagent, and covered by the final build gate above.

**Hardware gate note**

No `read_flash`, `write_flash`, `verify_flash`, `pio upload`, backup, restore, or serial-log capture command was run during the original 07-02 software repair. That note is now superseded by later 07-01 hardware work: the custom ST7789 build was backed up-before-write, flashed, verified, and serial-checked; the official stock ST7789 image was backed up-before-write, flashed, verified, and web-UI checked as a recovery baseline; then the patched custom decoded-spec build and decoded LittleFS assets were backed up-before-write, reflashed, and verified again. Any future firmware write still requires a fresh full backup before write.

**Board-driver note**

The repo-level AGENTS note identifies the observed unit as `esp32-2432s028r-st7789` and says to avoid flashing an `ili9341` build unless the hardware is re-identified. The fork `platformio.ini` for `ESP32-2432S028R` has been switched to `-DST7789_DRIVER=1` with ST7789 color/inversion flags. The current on-device firmware is the patched custom decoded-spec ST7789 target.

---

## T1 — Pre-flight probe + USER GO-AHEAD GATE (D-70, VALID-01)

- [x] `/dev/ttyUSB0` present
- [x] Read-only `chip_id` reports ESP32-D0WD-V3 rev 3.1
- [x] Board-driver decision recorded before flash: switched to ST7789 and rebuilt
- [x] **User go-ahead captured (chat + UTC timestamp):**

```
2026-06-18T20:20Z approx: user explicitly authorized flashing in chat.
```

---

## T2 — Fresh 4 MB backup (D-71, VALID-01)

| Field | Value |
|-------|-------|
| Backup file | `hardware/backups/cyd-backup-20260618-202520.bin` |
| Size (bytes) | `4194304` |
| sha256 | Held outside tracked docs; not copied into git-tracked files |
| Git status | Backup path is ignored and not staged |

---

## T3 — Flash validated build (D-72, VALID-02)

- [x] Rebuild reason recorded: ST7789 board-driver switch, post-flash crash fix, and persistent no-config portal fix
- [x] Upload reports "Hash of data verified" / SUCCESS

```
Final upload log: /tmp/p7-upload-timeout-fix.log
Wrote 2855920 bytes @ 0x00010000.
Hash of data verified.
ESP32-2432S028R SUCCESS in 00:00:46.800.
```

---

## T4 — verify_flash + hard USB power-cycle (D-73, VALID-01)

- [x] verify_flash OK @ 0x1000 (bootloader)
- [x] verify_flash OK @ 0x8000 (partitions)
- [x] verify_flash OK @ 0xe000 (boot_app0)
- [x] verify_flash OK @ 0x10000 (firmware)
- [ ] **HARD physical USB unplug/replug confirmed by user** (soft reset NOT sufficient)

```
Final verify log: /tmp/p7-verify-timeout-fix.log
All four regions reported "-- verify OK (digest matched)".
```

---

## T5 — Serial boot + visual parity + mining-active (D-74, VALID-02)

**Serial boot excerpt** (`/tmp/p7-serial.log`):

```
Final serial logs:
/tmp/p7-serial-timeout-fix-filtered.log
/tmp/p7-serial-timeout-fix.raw.log

Clean boot to WiFiManager config portal:
SSID nmap-2.4g, IP 192.168.4.1, no Guru Meditation, no backtrace.
Host scan saw `nmap-2.4g` immediately after boot.
```

**FLASH_GUIDE §5 checklist** (pass/fail):

| # | Item | Result |
|---|------|--------|
| 1 | 6 screens render, CORRECT (non-inverted) colors | PENDING physical screen check |
| 2 | WiFi join + pool connect (boot chime) | PENDING user config / portal submit |
| 3 | Hashrate > 0 | PENDING mining config |
| 4 | Accepted share → short beep (HW-01) + green LED (HW-03) | PENDING mining |
| 5 | Pull WiFi/pool → alarm tone | PENDING mining |
| 6 | `/api/system/info` stock-shape JSON (DATA-05) + `/api/status` sd/speaker blocks | PENDING LAN join |
| 7 | `POST /api/speaker/test-tone` → audible (HW-01) | PENDING LAN join |
| 8 | microSD → `/api/sd/list` lists + CSV log grows (HW-02) | PENDING LAN join |
| 9 | Dashboard (apps/nmminer-control) SD + speaker panels live | PENDING LAN join |

**Color verdict:** PENDING physical screen confirmation; build is now ST7789/BGR/inversion-off.

---

## T6 - Display Driver Re-Identification Result (D-75, CONDITIONAL)

| Field | Before | After |
|-------|--------|-------|
| Driver flag | `-DILI9341_2_DRIVER=1` | `-DST7789_DRIVER=1` |
| Color/invert flags | stale ILI9341 target | `-DTFT_RGB_ORDER=TFT_BGR`, `-DTFT_INVERSION_OFF` |
| Rebuild | ILI9341 repaired build, not flashed after gate | SUCCESS; later patched ST7789 custom build flashed and verified |
| Reflash (T3) re-run | pending | SUCCESS, hash verified |
| T5 re-verify colors | pending | PENDING physical confirmation; serial portal/AP visibility pass |

---

## T7 — Restore reversibility (D-76, VALID-03)

- [x] Backup integrity re-proven: latest full pre-current-app-reflash backup is 4194304 bytes; checksum recorded below
- [x] One-command restore documented for the latest full backup before the current app-only reflash:

```bash
/home/broklein/.local/bin/esptool.py --chip esp32 \
  --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 hardware/backups/cyd-backup-20260619-053633-pre-web-ui-fix2-app-flash.bin
```

Backup SHA-256: `b6c6c58ca755b84bcc27e5cc00c1fca0a6ab9411ae5f378ac3a28f192277f2ed`.

- [ ] Full round-trip: not run — default per D-76; requires explicit opt-in because it rewrites the physical device.

---

## Requirement verdicts

| Req | Verdict | Evidence |
|-----|---------|----------|
| VALID-01 | PARTIAL | Fresh 4 MB backup and app readback passed; hard physical USB unplug/replug still pending |
| VALID-02 | PARTIAL | Current custom decoded-spec ST7789 image with fallback-pool, source-blocker, review-05 mining/config hardening, and recovery portal CSS fixes flashed/readback-verified; serial boot reaches config mode; browser recovery portal works without the prior narrow clipping; decoded LittleFS assets were preserved by app-only write; physical UI/touch, Wi-Fi/pool join, and mining still pending |
| VALID-03 | PENDING | Restore command and backup size recorded; restore round-trip not run |
