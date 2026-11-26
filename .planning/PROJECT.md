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

Matching pages, manual touch, verified live feeds, configured-address balance,
SD logging and automatic reconnect are implemented and observed on USB. The
current image and exact passing/failed measurements are in
[the validation record](../docs/panicminer-validation-2026-11-25.md). A full strict
20-minute measurement has passed. The first full hour failed on an API timeout,
a partial feed cycle and no new accepted share; a reset-only repeat of the same
image was interrupted for an independently verified SD API private-path alias repair.
The repaired source 6cf06a7 is installed as f9c8fe99 after full verified backup;
fresh native qualification is running. Charger
cold boot with SD remains a separate physical gate. The owner is asleep and authorized continued USB testing; do not prompt
for another move tonight. Publication remains held until hardware acceptance.
The application is open source; Espressif radio binaries and chip ROM are still
part of the platform, as documented in firmware provenance.

## Constraints

- Full 4 MB flash backup before every write; `verify_flash` after it.
- No secrets in git: Wi-Fi passwords, wallets, licences, flash backups, serial logs.
- Recovery: long-press `BOOT`, join the setup AP, browse to `192.168.4.1`.
- Closed NMMiner binaries and recovered assets stay local and gitignored.

## History

- v2.0.02 bring-up + first fork: `archive/v2.0.02-fork-milestone/`.
- v3.0 NMMiner RE + binary customization, closed as superseded:
  `archive/v3.0-nmminer-binary-re/README.md`.
