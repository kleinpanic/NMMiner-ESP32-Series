# Requirements: v4.0 — PanicMiner: owned open firmware

**Defined:** 2026-11-24. Each requirement maps to one phase in `ROADMAP.md`.
"On hardware" means a serial log, a framebuffer dump or an operator photo.

## Base and tooling (BASE)

- [x] **BASE-01**: The consolidated source (historical `v4/panicminer` from `61aae97`) builds for `ESP32-2432S028R` with the repo's PlatformIO.
- [x] **BASE-02**: One script backs up the full 4 MB flash, flashes, and runs `verify_flash`. It refuses to flash without a fresh backup.
- [x] **BASE-03**: The open mining core mines on hardware: pool subscribed, nonzero hashrate, at least one accepted share. Current optimized-core evidence is in the 2026-11-26 qualification.

## See what we make (SEE)

- [x] **SEE-01**: A host simulator compiles the firmware's UI source with the same LVGL version and writes each screen to PNG.
- [x] **SEE-02**: The device dumps its framebuffer over serial on request; a host script turns the dump into PNG.
- [x] **SEE-03**: Panel driver, colour inversion, colour order and rotation are confirmed against one operator photo.

## UI (UI)

- [x] **UI-01**: A written design system: palette, type scale, grid, motion rules.
- [x] **UI-02**: Screens Boot, Mine, Hash, Chain, Clock, Stats and Setup render in the simulator and on hardware.
- [x] **UI-03**: The Setup screen shows the AP name, `192.168.4.1` and a Wi-Fi QR code.
- [x] **UI-04**: Short `BOOT` or a deliberate left/right touch pages through screens; held presses act once. Pages stay selected without automatic timer rotation (owner correction after unwanted paging).
- [x] **UI-05**: The UI uses a 40 ms (25 fps) redraw target without dropping hashrate more than 5 %.

## Privacy and branding (PRIV)

- [x] **PRIV-01**: No `NMMiner` or `NerdMiner` string reaches the panel, AP name, hostname or HTTP API.
- [x] **PRIV-02**: Application egress is the configured pool, NTP and mempool.space; local DNS/DHCP, ARP and LAN API replies are expected infrastructure. The native audit records its window and protocol bounds; IPv6 frames are counted without destination decoding.
- [x] **PRIV-03**: Egress is checked on hardware with DNS or packet capture.

## Mining (MINE)

- [x] **MINE-01**: PanicMiner mines on hardware for 1 hour with accepted shares and no reboot.
- [x] **MINE-02**: Hashrate is measured and compared against stock (about 1.04 MH/s); the gap is recorded.
- [x] **MINE-03**: Primary and fallback pool, both set from the setup portal.

## Hardware extras (HW)

- [x] **HW-01**: Speaker emits rate-limited confirmed-share audio and stays silent at boot; owner confirmation and production playback tests are recorded.
- [x] **HW-02**: SD card mounts and logs measured statistics, including accepted-share counts, each minute.
- [x] **HW-03**: RGB LED shows mining, share and disconnect states.
- [x] **HW-04**: The touch controller is identified (XPT2046, capacitive, or none) and touch works or is declared absent.

- [x] **HW-05**: Actual standalone charger power removal/reinsertion with SD inserted restores saved Wi-Fi, pool authorization and mining. This makes the already-requested physical acceptance explicit; laptop USB EN resets are separate evidence.

The original 30-fps-class UI wording is made precise as the existing 40 ms
Monitor frame target. Hashrate remains subject to the unchanged 5% limit.
Native LED evidence covers actual output latches; cyan event timing is covered
by the production C++ test. See the November 26 follow-up validation.

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
| HW-01..05 | 4 |
| MINE-01..02 | 5 |

Coverage: 22 of 22 mapped; 22 verified, including owner-confirmed and LAN-verified charger/SD cold boot.
