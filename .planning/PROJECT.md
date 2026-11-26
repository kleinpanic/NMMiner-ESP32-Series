# PanicMiner (NMMiner ESP32 Series repo)

## What This Is

Our own open firmware for the ESP32-2432S028R "Cheap Yellow Display" lottery
miner that shipped with NMMiner. It does the stock job: solo BTC mining over
stratum, live stats on the panel, a LAN API. It carries no NMMiner branding or
licence gate. It contacts no vendor host, and the UI is our own design.

## Core Value

The device mines on a pool we choose, with firmware we can read, build and
change. Every screen has a look nobody else ships.

## Current Milestone: v4.0 — PanicMiner: owned open firmware

**Goal:** Replace the closed NMMiner firmware with PanicMiner on this unit. It
mines with accepted shares, shows our own UI, and has a known network egress.

**Base:** the MIT NerdMiner fork is consolidated as ordinary source under
`firmware/custom/NerdMiner-CYD-Custom/` in the canonical
`kleinpanic/NMMiner-ESP32-Series` repository. Its historical open base is
`BitMaker-hub/NerdMiner_v2` `nerdminer-release-V1.8.3`; `61aae97` recorded the
early working board. Source consolidation is root commit `a730def`. The former
nested repository is historical; current source and tests live here.

**Gate rule:** a phase closes only on hardware evidence: a serial log, a
framebuffer dump or an operator photo. Compile and review gates are necessary,
not sufficient. Phases 3-6 of v3.0 failed on exactly that gap.

**See-what-we-make rule:** every screen renders on the laptop through the host
simulator before it is flashed. After a flash, the device dumps its framebuffer
over serial so the agent can view the real pixels.

## Decisions

| Date | Decision | Why |
|---|---|---|
| 2026-11-24 | Own firmware on the open NerdMiner core; the binary-patch route is closed | The patch route kept the licence gate, branding and closed code |
| 2026-11-24 | Name: PanicMiner. AP and hostname `panicminer-XXXX` | Ties to the kleinpanic terminal identity; drops vendor names |
| 2026-11-24 | UI on LVGL 8.3 with a host simulator | One UI source renders on the laptop and the panel |
| 2026-11-24 | Data feeds from one host, mempool.space, behind one toggle | Pool + NTP + one feed is the whole egress |
| 2026-11-24 | Design choices are the agent's call | Klein: "make proper choices to make it look nice and unique" |

## Hardware

- Board: ESP32-2432S028R, ESP32-D0WD-V3, 4 MB flash, CH340 USB serial. Helpers prefer the stable by-id link; ttyUSB numbering may change.
- Panel: 320x240, working `ILI9341_2_DRIVER` with `TFT_INVERSION_ON`. The owner confirmed visibility and the corrected colours; stock calls it ST7789. Preserve the tested configuration.
- Touch: resistive controller on HSPI (CLK25/MISO39/MOSI32/CS33/IRQ36). The owner confirmed left/right navigation. Stable-press, release and cooldown filtering suppress repeated or phantom paging.
- Extras fitted: microSD (FAT32), speaker on GPIO26, RGB LED on GPIO4/16/17.
- Wi-Fi is 2.4 GHz only. Saved settings load automatically; Setup selects a new network after relocation.
- Historical stock API readback was about 1.04 MH/s. Current acceptance uses the agreed 994 kH/s baseline, with a 944.3–1043.7 band. The previous SD-enabled image passed a strict 1,202.960-second run at 967.639 kH/s, with all sampled 10/60-second rates in band and all 17 checks passing.

## Current acceptance

Image 23236906 / source 49afce5 passed USB/SD composite qualification after
fresh full 4 MB backup and verification of all four written images. The strict
20-minute Mine run passed all 17 checks: 1,203.988 seconds, 222 samples and
967.899 kH/s. The same boot passed 3,614.418 seconds at 967.925 kH/s with one
new accepted share, 60 SD records, 78 complete in-window feeds and no reboot,
API gap, radio drop, SHA mismatch or uncommanded navigation. Complete exports,
blocked-client pressure, 65-second source-port reuse and controlled recovery
passed. A separate saved-settings EN reset with SD restored pool authorization,
wallet data and lifetime counters. Full sanitizer suite/build and 108 local Python
tests / 29 subtests pass. Public CI passed both firmware targets, sanitizers,
restore safety, 107 Python tests / 29 subtests (one private-fixture skip),
both UI themes and artifact uploads. Exact identity, snapshots and original failures are
in [the validation record](../docs/panicminer-validation-2026-11-25.md).

The owner authorized publication after these results. The canonical source is
public and [v4.0.0-rc.1](https://github.com/kleinpanic/NMMiner-ESP32-Series/releases/tag/v4.0.0-rc.1)
is published with six downloaded, checksum-verified assets. Release CI at
`8d24cd1` passed and history, private-value and binary secret scans were clean.
Version v4.0.0-rc.1 is a hardware-specific release candidate;
the full milestone remains open. Charger cold boot with SD, complete egress
capture, portal/fallback-pool persistence and observed audio/LED events still
require acceptance. Panel settings were already confirmed (SEE-03). The owned
application is open; Espressif radio binaries and chip ROM remain platform
components, as documented in firmware provenance.

## Constraints

- Full 4 MB flash backup before every write; `verify_flash` after it.
- No secrets in git: Wi-Fi passwords, wallets, licences, flash backups, serial logs.
- Recovery: long-press `BOOT`, join the setup AP, browse to `192.168.4.1`.
- Closed NMMiner binaries and recovered assets stay local and gitignored.

## History

- v2.0.02 bring-up + first fork: `archive/v2.0.02-fork-milestone/`.
- v3.0 NMMiner RE + binary customization, closed as superseded:
  `archive/v3.0-nmminer-binary-re/README.md`.
