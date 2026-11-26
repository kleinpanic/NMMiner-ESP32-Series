# Roadmap: PanicMiner

## v4.0 — PanicMiner: owned open firmware

**Created:** 2026-11-24. **Granularity:** coarse.
Prior milestones: `archive/v2.0.02-fork-milestone/`, `archive/v3.0-nmminer-binary-re/`.

Every phase closes on hardware evidence: a serial log, a framebuffer dump or an
operator photo. Every firmware write follows a fresh full 4 MB backup.

## Phases

- [x] **Phase 1: Base + See** - Open base mines on hardware; host simulator and serial framebuffer dump work; panel settings confirmed.
- [x] **Phase 2: PanicMiner UI** - Own design system, five interactive pages and matching Boot/Setup, simulator-first, then on hardware.
- [x] **Phase 3: De-brand + Egress** - No vendor strings; known egress; setup portal for primary and fallback pool.
- [ ] **Phase 4: Hardware extras** - Speaker, SD, RGB LED, touch identification.
- [x] **Phase 5: Hashrate + Soak** - Measure against stock, optimize where the core allows, 1-hour soak.

## Phase Details

### Phase 1: Base + See
**Goal**: The open core mines on this unit, and the agent can see both simulated and real pixels.
**Requirements**: BASE-01, BASE-02, BASE-03, SEE-01, SEE-02, SEE-03
**Success Criteria**:
  1. `v4/panicminer` builds; flash script backs up, writes and verifies.
  2. Serial log shows pool subscribed, nonzero hashrate, an accepted share.
  3. `tools/uisim` writes a PNG per screen; a serial dump of the panel decodes to PNG.
  4. One operator photo confirms driver, inversion, colour order and rotation.

### Phase 2: PanicMiner UI
**Goal**: The panel shows our own design on every screen.
**Depends on**: Phase 1
**Requirements**: UI-01..UI-05
**Success Criteria**:
  1. `docs/design/panicminer-design.md` defines palette, type, grid and motion.
  2. Boot, Mine, Hash, Chain, Clock, Stats and Setup render in the simulator and match on hardware dumps.
  3. `BOOT` pages screens; the hashrate drop with the UI running is under 5 %.
**UI hint**: yes

### Phase 3: De-brand + Egress
**Goal**: Nothing vendor-branded shows, and the device talks only to known hosts.
**Depends on**: Phase 2
**Requirements**: PRIV-01..PRIV-03, MINE-03
**Success Criteria**:
  1. `rg -i 'nmminer|nerdminer'` over UI, AP, hostname and API output finds nothing.
  2. A native header trace shows configured pool, NTP and mempool.space plus documented local network infrastructure; audit bounds are recorded.
  3. Primary and fallback pool are set from the portal and survive reboot.

### Phase 4: Hardware extras
**Goal**: Speaker, SD and LED work on hardware; touch is identified.
**Depends on**: Phase 2
**Requirements**: HW-01..HW-05
**Success Criteria**:
  1. Share beep heard, silent boot, SD statistics include accepted-share counts, LED output states checked (cyan event timing unit-tested).
  2. Touch works, or a probe report names the controller as absent or different.
  3. Actual charger power removal/reinsertion with SD resumes saved mining (HW-05).

### Phase 5: Hashrate + Soak
**Goal**: The hashrate gap to stock is known and narrowed where the core allows.
**Depends on**: Phase 3
**Requirements**: MINE-01, MINE-02
**Success Criteria**:
  1. Hashrate table: stock about 1.04 MH/s against PanicMiner, same unit.
  2. 1-hour run with accepted shares and no reboot.

## Progress

| Phase | Status |
|---|---|
| 1. Base + See | Verified: build, backup/flash, native mining, simulator/dumps and owner panel confirmation |
| 2. PanicMiner UI | Verified: matching screens, owner-confirmed manual touch and exact-image in-band Mine retest |
| 3. De-brand + Egress | Verified: native steady-window IPv4 audit and portal/fallback persistence/recovery |
| 4. Hardware extras | Audio owner-confirmed; SD, LED outputs and touch verified. HW-05 charger/SD cold power requires physical access |
| 5. Hashrate + Soak | Verified on USB with SD: 60.24 minutes, new accepted share, 967.925 kH/s; exact follow-up image also passed the fresh strict 20-minute test |

Only HW-05 remains for full milestone closeout. The earlier hour and fresh
follow-up Mine retest keep separate image identities in the validation record.
