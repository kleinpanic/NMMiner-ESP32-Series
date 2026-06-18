# Phase 1: RE-Spec the UI — Discussion Log

**Mode:** `--auto` (autonomous; standing authorization from the milestone `/goal`)
**Date:** 2026-06-18

Human-reference audit log only — not consumed by downstream agents.

## Why auto mode

The milestone goal granted standing authorization to drive autonomously and
"stop to ask only when a decision genuinely needs my input." Phase 1's technical
approach is already locked by `.planning/research/` (4 dimensions + synthesis).
The remaining choices are recommended defaults derived from that research — all
reversible, all within the locked phase boundary — so none required live user
input.

## Gray areas resolved (recommended option taken)

| Area | Decision | Rationale |
|------|----------|-----------|
| SPEC.md coverage breadth | Locate all 6 screen funcs; deep-spec Miner+Clock first (D-01/02/03) | Most-used screens already located; others back-fillable per their phases |
| FIDB build effort | Attempt v4.4.6 FIDB, time-boxed, with fallback to existing xref pipeline (D-04/05) | Screen funcs were located WITHOUT a FIDB — accelerator, not blocker |
| Tooling install | Ghidra 11.4.x official release + dynacylabs loader v12.0 (D-06) | JDK 21 present; Ghidra not yet installed |
| LVGL version | Confirm v8/v9 with two signals before any extractor (D-07) | Research gap flagged MEDIUM; hypothesis v8 |
| Validation oracle | Cross-check field formatting vs live LAN API 192.168.1.136 (D-08) | Device reachable over WiFi even without /dev/ttyUSB0 |
| Anti-patterns | mis-carved PNGs are noise; no v9 magic-byte scan (D-09/10) | Directly from PITFALLS.md |

## Deferred (captured in CONTEXT.md `<deferred>`)

Font/asset extraction → Phase 2; screen rebuild → Phase 3; full ss_render +
screensaver state machine → Phase 5; Path-A integrity recon → Phase 8.

No scope creep raised.
