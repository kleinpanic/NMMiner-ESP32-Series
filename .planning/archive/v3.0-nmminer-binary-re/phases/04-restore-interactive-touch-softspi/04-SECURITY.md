---
phase: 04-restore-interactive-touch-softspi
audited: 2026-06-18
asvs_level: 1
block_on: high
threats_total: 8
threats_closed: 8
threats_open: 0
status: SECURED
auditor: gsd-security-auditor
---

# Phase 4: Security Audit — Restore Interactive Touch (SoftSPI)

Verifies each declared threat in the Phase-4 threat models (PLAN 04-01 and 04-02)
by its disposition. Implementation files are READ-ONLY; this audit confirms
mitigations exist in code, not in intent. The load-bearing finding (WR-01: a 10s
gesture wiping user secrets) is the focus.

## Trust surface (Phase 4)

Local physical input device. Attacker must have physical access to the panel/bus.
No new network endpoints introduced — the config portal reached by long-press is
the EXISTING WiFiManager AP, gated behind a deliberate 10s hold.

## Threat Verification

| Threat ID | Category | Disposition | Verdict | Evidence |
|-----------|----------|-------------|---------|----------|
| T-04-01 | Tampering (floating-bus noise) | mitigate | CLOSED | `nm_touch.cpp:124-135` median5 jitter rejection; `:47,116-121` Z threshold; `:165-173` `nm_touch_log_raw` raw-Z diagnosis (D-43); diag seam wired & exercised at `NerdMinerV2.ino.cpp:336-343` |
| T-04-02 | DoS (touch read blocks miner) | mitigate | CLOSED | `nm_touch.cpp:71-93` bit-bang xfer, no delay()/busy-wait; polled from non-mining loop `NerdMinerV2.ino.cpp:381` alongside `nm_uimanager_tick` |
| T-04-SC (01) | Tampering (supply chain) | accept | CLOSED | No new lib_deps; SoftSPI hand-written, `<xpt2046.h>` deliberately NOT included (verified absent in nm_touch). Documented below. |
| T-04-03 | EoP/Tampering (long-press → config-mode entry) | mitigate | CLOSED | **WR-01 closed.** Long-press now calls non-destructive `enter_config_portal()` (`nm_uimanager.cpp:155` touch, `:172` button), NOT `reset_configuration()`. Requires deliberate 10s hold (`NM_TOUCH_LONG_MS=10000`, `nm_uimanager.h:21`) + debounce (`NerdMinerV2.ino.cpp:320-325,344-348`). |
| T-04-04 | DoS (gesture/idle tick blocks loop) | mitigate | CLOSED | Classifier uses millis() deltas only, no delay/while-wait (`NerdMinerV2.ino.cpp:306-363`); idle tick is a single millis delta (`nm_uimanager.cpp:233`); **CR-02 closed** — `nm_display_wake` is now non-blocking (millis-stepped ramp pumped by tick, `nm_display.cpp:136-165`), no delay() in the loop path |
| T-04-05 | Repudiation (silent config action) | accept | CLOSED | Local single-user device; `enter_config_portal()` Serial.println's "[CFG] Entering config portal (non-destructive)" (`wManager.cpp:141`). Logged accepted risk below. |
| T-04-06 | Tampering (dormant HSPI touch object links in binary) | accept | CLOSED | `esp23_2432s028r.cpp` HSPI-bound `touch.init()` reached ONLY via `initDisplay()`→`esp32_2432S028R_Init()`; `initDisplay()` is in the `#else` (non-CYD) branch (`NerdMinerV2.ino.cpp:142`), skipped for CYD which calls `nm_uimanager_init()` at `:139`. Never bound at runtime. Logged accepted risk below. |
| T-04-SC (02) | Tampering (supply chain) | accept | CLOSED | No package-manager installs in Plan 04-02; no new lib_deps. Documented below. |

## WR-01 — destructive-action mitigation (load-bearing) — CLOSED

The real risk for this phase: an accidental 10s hold wiping WiFi creds + wallet via
`reset_configuration()` (which does `nvMem.deleteConfig(); resetStat(); wm.resetSettings();
ESP.restart()` — `wManager.cpp:123-130`).

Verified in current code:
- **Touch long-press** (`nm_uimanager.cpp:152-156`) → `enter_config_portal()`. NOT reset_configuration.
- **Boot-button long-press** (`nm_uimanager_on_button_config`, `nm_uimanager.cpp:168-173`) →
  `enter_config_portal()`. Wired at `NerdMinerV2.ino.cpp:108` for the CYD env.
- `enter_config_portal()` (`wManager.cpp:136-146`) starts the WiFiManager captive portal
  (`wm.startConfigPortal`) WITHOUT deleting NVS. Non-blocking (`setConfigPortalBlocking(false)`).
- The only `reset_configuration` references reachable to the build are in the **non-CYD**
  `#else` branch (`NerdMinerV2.ino.cpp:113`) and the `PIN_BUTTON_2` block (`:127`). For the CYD
  env, PIN_BUTTON_2 is undefined (`esp322432s028r.h:5` commented out) and the CYD inner guard
  takes the `#if` arm — both reset_configuration sites compile out. Grep confirms no
  `reset_configuration` is reachable from any CYD touch/button handler.

An accidental 10s hold can no longer wipe user secrets; worst case is the device entering a
non-destructive provisioning portal it can leave by reboot.

## D-42 bus isolation — CLOSED

`nm_touch.cpp` is pure bit-bang (`digitalWrite`/`digitalRead` on GPIO 25/32/39/33).
Grep confirms NO `SPIClass` instantiation, NO `SPI.begin`, NO `#include <xpt2046.h>` in
nm_touch.{cpp,h} — the only string matches are comments documenting their deliberate absence.
Touch pins are disjoint from the display HSPI pins. VSPI is left free for SD (Phase 6).
Caveat (T-04-06): the dormant HSPI-bound legacy touch object still links into the binary but
is unreached at runtime for the CYD env.

## D-11 — no closed-firmware bytes in touch code — CLOSED

No PROGMEM blobs, no embedded byte arrays, no opaque constants in nm_touch. Geometry and
calibration values (`nm_touch.cpp:49-64`) are documented public facts (CYD rotation-1 320x240
ranges with cited provenance), not extracted binary.

## Accepted Risks Log

| ID | Risk | Rationale |
|----|------|-----------|
| T-04-SC | No package-manager installs this phase | SoftSPI is hand-written; no new lib_deps; existing liangyingy lib untouched and unused by the CYD path — nothing new to vet. |
| T-04-05 | Config-portal entry has no audit trail beyond serial | Local single-user device; `enter_config_portal()` serial-logs entry; sufficient for this surface. Note: now NON-destructive, so the original repudiation concern (silent wipe) no longer applies. |
| T-04-06 | Dormant HSPI-bound legacy touch object links into the binary | Unreached for CYD at runtime (`initDisplay()` skipped). A future caller of `esp32_2432S028R_Init()` would double-bind HSPI — flagged for the SD phase (Phase 6) to reconcile before adding an SD path. Out of scope to edit here. |

## Unregistered Flags

None. SUMMARY 04-01 / 04-02 contain no `## Threat Flags` section; no new attack
surface appeared during implementation beyond the registered threats. The dormant
HSPI object (T-04-06) and the config-mode semantics (T-04-03) are both registered.

## Notes on stale review/validation docs

- `04-REVIEW.md` WR-01/CR-02 were written against an EARLIER state. Current code has
  remediated both: long-press → `enter_config_portal()` (not reset_configuration), and
  `nm_display_wake()` is non-blocking. Verdicts above reflect the SHIPPED code, not the
  stale review snapshot.
- `04-VALIDATION.md` Gate B (lines 61/80/83) still names `reset_configuration` as the
  long-press sink — this is stale documentation; the implementation uses
  `enter_config_portal()`. Documentation lag only; no code gap. Recommend correcting
  04-VALIDATION.md to match the shipped code.

## Verdict

All 8 threats CLOSED (5 mitigate verified in code, 3 accept logged). The load-bearing
destructive-wipe-on-accidental-gesture risk (WR-01) is closed: no `reset_configuration`
is reachable from any CYD long-press path. No high-severity gaps. Phase may ship.
