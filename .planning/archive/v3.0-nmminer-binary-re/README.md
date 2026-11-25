# Archived milestone v3.0 — NMMiner RE + binary customization

Closed 2026-11-24 as superseded, not complete.

## Why it closed

Klein's goal is an owned firmware: same mining job, no NMMiner telemetry,
no NMMiner branding, a new look. This milestone drifted from that goal in two steps.

1. Phases 1-6 rebuilt a pixel clone of the stock NMMiner UI on the NerdMiner
   fork. They passed compile and code-review gates only. On hardware the panel
   looked wrong and touch reported no presses.
2. On 2026-06-19 a Codex session re-routed Phase 7 to patching the closed
   official `v2.0.02` binary. That route kept the licence gate, the vendor
   branding and the closed code as explicit no-touch zones, so it could not
   reach the goal.

## What stays useful

- `phases/01-*/` and `phases/02-*/`: the stock screen inventory, `.ss` format and fonts.
- `phases/07-*/07-TOUCH-PROBE-RUNBOOK.md` and `debug/touch-no-press.md`: touch evidence.
- Fork branch `archive/nmminer-clone-lvgl` (`9777259`): the LVGL bridge,
  NVS-first Wi-Fi config, fallback pool, SD, speaker and LED modules.

The successor milestone is v4.0 in `../../ROADMAP.md`.
