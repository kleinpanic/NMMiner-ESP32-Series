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

**Base:** fork `kleinpanic/NerdMiner-CYD-Custom` (MIT, upstream
`BitMaker-hub/NerdMiner_v2` `nerdminer-release-V1.8.3`). The work branches from
`61aae97`, the last commit recorded as mining on this hardware. Modules from the
archived LVGL stack (`archive/nmminer-clone-lvgl`) move over one by one, each
with its own on-device check.

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

- Board: ESP32-2432S028R, ESP32-D0WD-V3, 4 MB flash, serial `/dev/ttyUSB0` (CH340).
- Panel: 320x240. Stock firmware identity says ST7789. The base `61aae97` built
  with `ILI9341_2_DRIVER`. Phase 1 settles the driver with a photo.
- Touch: XPT2046 path showed no press and no PENIRQ edge in v3.0. Unresolved.
- Extras fitted: microSD (FAT32), speaker on GPIO26, RGB LED on GPIO4/16/17.
- Wi-Fi is 2.4 GHz only. Target SSID `Homebase`, not `Homebase5`.
- Stock hashrate on this unit: about 1.04 MH/s (2026-06-01 API readback).
  The open core's rate on this unit is not yet measured.

## Constraints

- Full 4 MB flash backup before every write; `verify_flash` after it.
- No secrets in git: Wi-Fi passwords, wallets, licences, flash backups, serial logs.
- Recovery: long-press `BOOT`, join the setup AP, browse to `192.168.4.1`.
- Closed NMMiner binaries and recovered assets stay local and gitignored.

## History

- v2.0.02 bring-up + first fork: `archive/v2.0.02-fork-milestone/`.
- v3.0 NMMiner RE + binary customization, closed as superseded:
  `archive/v3.0-nmminer-binary-re/README.md`.
