---
status: investigating
trigger: "Current custom decoded-spec ST7789 firmware boots recovery AP, but the physical miner display is user-observed as visually wrong and not matching the expected NMMiner UI."
created: 2026-06-19T18:03:38Z
updated: 2026-06-19T18:03:38Z
---

# Debug Session: physical-ui-mismatch

## Symptoms

- Expected behavior: The physical ESP32-2432S028R ST7789 panel should either show a clear expected recovery/config screen while WiFiManager AP mode is active, or show the rebuilt NMMiner screen carousel matching the recovered stock specification when configured/mining.
- Actual behavior: The user physically observed the live miner display on 2026-06-19 and reported that it looks wrong and nothing like expected.
- Error messages: No new display-specific serial error is captured. The latest passive serial summary still shows recovery AP mode and no panic markers. Touch remains a separate no-contact failure with 23 diagnostics, 0 pressed observations, and 0 IRQ-low observations in the latest passive window.
- Timeline: Observed after the current custom decoded-spec ST7789 webfix2 app was flashed/readback-verified and after browser recovery portal screenshots showed the CSS clipping fix.
- Reproduction: Boot the current custom firmware, let it enter recovery AP `nmap-2.4g` / `192.168.4.1`, and observe the physical ST7789 panel.

## Current Focus

- hypothesis: Physical panel mismatch is real, but root cause is not yet classified. Top suspects are expected recovery/config screen versus expected miner screen, ST7789 RGB/BGR/inversion/byte-swap flags, rotation, incomplete rebuilt LVGL screen parity, or LVGL flush/SPI artifacts.
- test: Capture no-secret physical evidence that classifies the visible failure before any display-code tuning.
- expecting: The next useful evidence will say which class is visible: wrong screen/state, wrong colors, inverted colors, mirrored/upside-down rotation, clipped/overlapping layout, stale rectangles/tearing, unreadable text, or incomplete screen content.
- next_action: Capture a no-secret photo/video or written checklist of the panel while AP mode is active: visible screen/text, colors, rotation, clipping/overlap, stale regions/tearing, and LED state. If a firmware diagnostic is chosen later, take a fresh full 4 MB backup first, then test only one variable at a time.
- reasoning_checkpoint: Browser recovery portal screenshots prove only the HTTP WiFiManager pages. They cannot offset the failed physical ST7789/LVGL panel evidence.
- tdd_checkpoint: Hardware-facing visual issue; automated tests cannot prove physical panel parity without a camera or operator observation.

## Evidence

- timestamp: 2026-06-19T17:51:21Z
  source: `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md`
  observation: Live firmware is the custom decoded-spec ST7789 webfix2 app, not stock; recovery AP is `nmap-2.4g` / `192.168.4.1`; browser recovery evidence does not prove physical LVGL panel parity.
- timestamp: 2026-06-19T17:33:44Z
  source: `/tmp/p7-serial-passive-inline-20260619-173344.log` summarized by `scripts/phase7_serial_touch_summary.py`
  observation: Passive no-write serial window reached recovery AP with 23 touch diagnostics, 0 pressed observations, 0 IRQ-low observations, and no panic markers beyond expected reset evidence.
- timestamp: 2026-06-19T18:03:38Z
  source: user physical observation
  observation: User reports the live physical miner display looks wrong and nothing like expected. No photo/detail captured yet.
- timestamp: 2026-06-19T18:03:38Z
  source: `.planning/phases/07-on-device-validation/07-UI-REVIEW.md`
  observation: Phase 7 UI review now scores 8/24 and marks the physical ST7789/LVGL panel as a failed/unclassified visual parity gate. Browser portal screenshots remain separate evidence.
- timestamp: 2026-06-19T18:03:38Z
  source: read-only display-path audit
  observation: Top code/config suspects are: WiFiManager config page state, ST7789 color order/inversion/byte-swap, rotation mismatch, incomplete rebuilt LVGL parity, and LVGL flush/SPI artifacts.

## Eliminated

- hypothesis: Browser recovery portal screenshots prove physical panel parity.
  evidence: Current UI review and authority index explicitly separate browser WiFiManager evidence from physical ST7789/LVGL panel evidence.
- hypothesis: Seeing `v2.0.02` proves official stock firmware is live.
  evidence: The current live app is the custom rebuilt fork and intentionally reports recovered `v2.0.02`.

## Resolution

- root_cause: unresolved.
- fix: none yet. Do not tune display flags blind. Classify the physical failure first; if a diagnostic firmware write is chosen later, take a fresh full 4 MB backup first.
- verification: open.
- files_changed: `.planning/debug/physical-ui-mismatch.md`, `.planning/phases/07-on-device-validation/07-UI-REVIEW.md`, `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md`, `.planning/phases/07-on-device-validation/07-VERIFICATION.md`, `.planning/phases/07-on-device-validation/07-UAT-AUDIT.md`, `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md`.
