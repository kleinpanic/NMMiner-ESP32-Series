# PanicMiner (NMMiner ESP32 Series repo)

## What This Is

Our own open firmware for the ESP32-2432S028R "Cheap Yellow Display" lottery
miner that shipped with NMMiner. It does the stock job: solo BTC mining over
stratum, live stats on the panel, a LAN API. It carries no NMMiner branding or
licence gate. It contacts no vendor host, and the UI is our own design.

## Core Value

The device mines on a pool we choose, with firmware we can read, build and
change. Every screen has a look nobody else ships.

## Shipped Milestone: v4.0 — PanicMiner: owned open firmware

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

Current firmware source `07c3931cbcb297f330eabe68e7feabbc2f330c2f`, application `0e32d00af0c9cb8d996f1a284cc9a04768707402d336ae5ec8f9e64c4c27b835`, passed
fresh verified full-backup/four-image installation and **1,201.807 seconds, 220 samples, 968.720 kH/s (-2.54% against 994); all 17 strict checks passed**.
Portal save/reopen persistence, native forced failover with advancing work,
restored primary/wallet, LED output states, Wi-Fi recovery, steady-window IPv4
egress, three 5/5 feed cycles and private-path guards passed. The owner confirmed
audio. Public CI 45897395376, sanitizers, 108 local Python tests / 29 subtests and
binary/history secret scans passed. The previous rc.1 image separately passed
its full hour with accepted work; its identity is not replaced by this retest.

The canonical repository is public. Stable v4.0.0 is published as the latest release; all six assets were anonymously downloaded and checksum-verified. It promotes the exact rc.2
application bytes. The owner transferred the SD-equipped miner to a charger;
read-only LAN checks passed for 70.234 seconds at 969.003 kH/s,
with primary authorization, new SD writes, advancing work and current wallet
data. HW-05 is complete. The historical fork is private and archived.
Application source is open; Espressif radio binaries/ROM and IPv6 destination
attribution remain documented platform/audit boundaries, not decoded claims.
See [validation](../docs/panicminer-validation-2026-11-25.md).

## Requirements

### Validated

- Open mining, verified backup/restore and exact-image provenance — v4.0.
- All matching pages, measured statistics and deliberate manual touch — v4.0.
- Configurable primary/fallback portal, recovery and bounded IPv4 audit — v4.0.
- Owner-confirmed audio, SD records, RGB outputs and charger/SD cold boot — v4.0.
- Independent strict Mine measurement and prior mining-core one-hour soak — v4.0.

### Active

None. Project acceptance complete; no next milestone has been requested.

### Out of scope

Closed NMMiner binary redistribution, activation bypass, private signing keys,
and a claim that Espressif radio/ROM or all IPv6 traffic has been decoded.

## Next Milestone Goals

None requested. Preserve verified operation and take a full backup before writes.

## Constraints

- Full 4 MB flash backup before every write; `verify_flash` after it.
- No secrets in git: Wi-Fi passwords, wallets, licences, flash backups, serial logs.
- Recovery: long-press `BOOT`, join the setup AP, browse to `192.168.4.1`.
- Closed NMMiner binaries and recovered assets stay local and gitignored.

## History

- v2.0.02 bring-up + first fork: `archive/v2.0.02-fork-milestone/`.
- v3.0 NMMiner RE + binary customization, closed as superseded:
  `archive/v3.0-nmminer-binary-re/README.md`.

---
*Last updated: 2026-11-26 after v4.0 hardware acceptance.*
