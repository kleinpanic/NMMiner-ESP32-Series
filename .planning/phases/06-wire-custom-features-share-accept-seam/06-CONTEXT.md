# Phase 6: Wire Custom Features at Share-Accept Seam - Context

**Gathered:** 2026-06-18 (auto mode; grounded in fork scout — most custom HW is already built)
**Status:** Ready for planning (research SKIP — research SUMMARY: "already built + tested; integration is confirmation, not discovery")

<domain>
## Phase Boundary

Light up the custom hardware over the recovered NMMiner UI: speaker (LEDC GPIO26) beep
on an accepted share + alarm on pool/WiFi disconnect, SD (VSPI) logging on the
now-free hardware VSPI (freed by Phase-4 SoftSPI touch), custom LED status +
kleinpanic accents — wired at the NerdMiner **share-accept seam** as an in-process
call. Mining stays fully functional. Build SUCCESS is the gate; on-device confirm is
Phase 7. MOST of this is ALREADY BUILT in the fork — this phase is confirmation + the
one missing hook.
</domain>

<decisions>
## Implementation Decisions

### Already built (confirm, don't rewrite) — D-61
- **D-61:** The fork ALREADY has: `src/speaker.cpp` (Speaker class, LEDC IDF v4.4.6, volume/mute/tones; `Speaker::playDisconnectAlarm()` already called at `mining.cpp:263` on WiFi disconnect) and `src/sdstore.cpp` (SDStore on a dedicated `SPIClass vspi(VSPI)`, mounts on SD_CLK/MISO/MOSI/CS). The dashboard API already exposes `/api/speaker/*` + `/api/sd/*` (Phase-5 webapi kept them). So **disconnect alarm = DONE, SD mount = DONE.** Phase 6 confirms these compile/work in the integrated build, not rewrite them.

### The ONE missing piece — beep on accepted share (D-62)
- **D-62 (HW-01, the new work):** there is NO beep wired on an ACCEPTED share. Add a minimal in-process call at the share-accept seam — `mining.cpp:452` (`shares++;` after a pool-accepted submit) — e.g. `Speaker::beepShareAccepted();` (a short non-blocking tone). This is THE one intentional touch to mining.cpp this milestone, justified by HW-01 ("wired at the NerdMiner share-accept seam, in-process call, not an Xtensa hook"). It is ADDITIVE (a tone call) — it must NOT alter the hashing/stratum/share logic (HW-04). Keep it non-blocking (arm a tone, don't delay the mining loop).

### SD on the now-free VSPI (D-63)
- **D-63 (HW-02):** Phase 4 moved touch to SoftSPI, so the hardware VSPI (GPIO5/18/19/23) is now free for SD. Confirm `sdstore.cpp` mounts cleanly with no touch/SD bus contention in the integrated build (the touch-before-SD lock paying off). Optionally log accepted shares / events to SD at the seam (HW-02 "mounts and logs"). Keep SD optional (no card present = graceful, mining unaffected).

### LED + kleinpanic accents (D-64)
- **D-64 (HW-03):** custom LED status behavior (the fork/CYD RGB LED — Phase-4 FindMe already blinks it on pins 4/16/17) + kleinpanic branding accents. kleinpanic splash/accent is already on the Loading screen (Phase 3, D-37). Confirm LED status semantics (mining/share/disconnect) and that the kleinpanic accent is present. Verify against silkscreen pins where the plan touches them.

### Mining stays functional (D-65)
- **D-65 (HW-04, load-bearing):** the ONLY mining.cpp change is the additive beep call at the seam. Stratum core, pool primary/fallback + switch-back ≤10s, hashing all unchanged. The build gate + a git diff confirming mining.cpp's diff is ONLY the beep hook is the evidence (on-device "mining still accepts shares" is Phase 7).

### Claude's Discretion
- Exact beep tone/duration for share-accept (short, distinct from the disconnect alarm).
- Whether to add SD share-logging now or just confirm mount (HW-02 minimal = mount + the log hook).
- LED status state mapping if not already defined.
</decisions>

<canonical_refs>
## Canonical References
- firmware/custom/NerdMiner-CYD-Custom/src/speaker.{cpp,h} (Speaker class — add beepShareAccepted; playDisconnectAlarm exists), src/sdstore.{cpp,h} (SD on VSPI), src/mining.cpp (the share-accept seam @~452 `shares++`; disconnect alarm @263).
- hardware/recovered-assets/SPEC.md §Share-Accept Seam (the stock seam characterization from Phase 1 — informs WHERE/semantics; the fork seam is mining.cpp:452).
- .planning/REQUIREMENTS.md — HW-01..04.
- .planning/research/FEATURES.md (speaker beep-on-share + disconnect-alarm; SD NOT stock = our re-add; LED).
- src/webapi.cpp (/api/speaker/* + /api/sd/* already serve these — dashboard contract).
</canonical_refs>

<code_context>
## Existing Code Insights
- Speaker + SDStore are fully implemented; disconnect alarm already hooked. The gap is ONLY the accepted-share beep.
- mining.cpp:452 `shares++;` is the in-process seam (post-accept counter increment) — the natural hook point.
- VSPI is free for SD because Phase 4 put touch on SoftSPI (the touch-before-SD ordering lock).
</code_context>

<specifics>
## Specific Ideas
- This is the lightest build phase — mostly "confirm it all links + works together" + the one beep hook. Flash is at 90.5% (9.5% headroom) — the beep hook is tiny.
- Keep mining sacrosanct: the seam beep is the only mining.cpp edit, additive + non-blocking.
</specifics>

<deferred>
## Deferred Ideas
- On-device confirm (beep audible on real share, SD card logging, LED visible) → Phase 7.
- Differentiator polish → v2/POLISH.
</deferred>

---
*Phase: 6-wire-custom-features-share-accept-seam*
*Context gathered: 2026-06-18*
