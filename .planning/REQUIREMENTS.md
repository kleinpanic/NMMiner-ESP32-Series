# Requirements: v4.0 — PanicMiner: owned open firmware

**Defined:** 2026-11-24. Each requirement maps to one phase in `ROADMAP.md`.
"On hardware" means a serial log, a framebuffer dump or an operator photo.

## Base and tooling (BASE)

- [x] **BASE-01**: Branch `v4/panicminer` from `61aae97` builds for `ESP32-2432S028R` with the repo's PlatformIO.
- [x] **BASE-02**: One script backs up the full 4 MB flash, flashes, and runs `verify_flash`. It refuses to flash without a fresh backup.
- [x] **BASE-03**: The open mining core mines on hardware: pool subscribed, nonzero hashrate, at least one accepted share. Current optimized-core evidence is in the 2026-11-26 qualification.

## See what we make (SEE)

- [x] **SEE-01**: A host simulator compiles the firmware's UI source with the same LVGL version and writes each screen to PNG.
- [x] **SEE-02**: The device dumps its framebuffer over serial on request; a host script turns the dump into PNG.
- [x] **SEE-03**: Panel driver, colour inversion, colour order and rotation are confirmed against one operator photo.

## UI (UI)

- [x] **UI-01**: A written design system: palette, type scale, grid, motion rules.
- [ ] **UI-02**: Screens Boot, Mine, Chain, Clock and Setup render in the simulator and on hardware.
- [x] **UI-03**: The Setup screen shows the AP name, `192.168.4.1` and a Wi-Fi QR code.
- [ ] **UI-04**: Short `BOOT` or a deliberate left/right touch pages through screens; held presses act once. Pages stay selected without automatic timer rotation (owner correction after unwanted paging).
- [ ] **UI-05**: The UI holds 30 fps-class redraws without dropping hashrate more than 5 %.

## Privacy and branding (PRIV)

- [ ] **PRIV-01**: No `NMMiner` or `NerdMiner` string reaches the panel, AP name, hostname or HTTP API.
- [ ] **PRIV-02**: Egress is the pool, NTP and, when the feed toggle is on, mempool.space only.
- [ ] **PRIV-03**: Egress is checked on hardware with DNS or packet capture.

## Mining (MINE)

- [x] **MINE-01**: PanicMiner mines on hardware for 1 hour with accepted shares and no reboot.
- [x] **MINE-02**: Hashrate is measured and compared against stock (about 1.04 MH/s); the gap is recorded.
- [ ] **MINE-03**: Primary and fallback pool, both set from the setup portal.

## Hardware extras (HW)

- [ ] **HW-01**: Speaker beeps once per accepted share and stays silent at boot.
- [x] **HW-02**: SD card mounts and logs shares.
- [ ] **HW-03**: RGB LED shows mining, share and disconnect states.
- [x] **HW-04**: The touch controller is identified (XPT2046, capacitive, or none) and touch works or is declared absent.

## Out of scope

- Patching or redistributing the closed NMMiner binary.
- The NMMiner licence or activation flow.
- Pixel parity with the stock NMMiner screens.

## Traceability

| Requirement | Phase |
|---|---|
| BASE-01..03, SEE-01..03 | 1 |
| UI-01..05 | 2 |
| PRIV-01..03, MINE-03 | 3 |
| HW-01..04 | 4 |
| MINE-01..02 | 5 |

Coverage: 23 of 23 mapped.
