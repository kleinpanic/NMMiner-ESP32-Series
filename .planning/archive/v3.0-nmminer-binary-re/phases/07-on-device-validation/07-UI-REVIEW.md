# Phase 7 - UI Review

**Audited:** 2026-06-19
**Baseline:** abstract 6-pillar standards; no Phase 7 `UI-SPEC.md` found
**Screenshots:** reviewed historical stock web portal screenshots under `/tmp/p7-stock-ui/`, older custom recovery root screenshots under `/tmp/p7-custom-ui/`, superseded review-05 screenshots under `/tmp/p7-current-review05-ui/`, and current webfix2 screenshots under `/tmp/p7-webfix2-ui/`; no screenshots copied into the repo. No new CLI screenshots were captured in this re-audit because `localhost:3000` returned 307 and ports 5173/8080 were not serving HTTP 200. No physical panel photo/video has been captured yet.
**Scope:** current NMMiner ESP32 recovery state plus the user's latest physical-panel observation. The live device is running the flashed custom decoded-spec `ESP32-2432S028R` ST7789 fork plus decoded LittleFS assets, fallback-pool support, post-forensics source-blocker fixes, review-05 mining/config hardening fixes, and the WiFiManager recovery portal CSS fix. Stock `v2.0.02` screenshots remain historical recovery-baseline evidence only.

---

## Evidence Boundary

This UI review has four evidence layers:

- Historical stock recovery UI: official stock ST7789 files were flashed and verified earlier. The stock AP and browser portal were proven, and browser screenshots were captured under `/tmp/p7-stock-ui/`.
- Superseded custom review-05 recovery web UI: after the review-05 app flash, `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200, but `/tmp/p7-current-review05-ui/root-320x568.png` and `/tmp/p7-current-review05-ui/wifi-390x844.png` showed right-edge clipping. That defect triggered the CSS fix.
- Current custom webfix2 recovery web UI: after the webfix2 app flash/readback, `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200; `/api/status`, `/api/system/info`, and `/sd/list` returned HTTP 404 in portal mode. Current screenshots were captured under `/tmp/p7-webfix2-ui/`.
- Current physical ST7789/LVGL panel: the user has now physically observed the miner display and reports it "looks like shit and nothing like it should." Treat this as current evidence that the physical-panel visual parity gate failed. No photo, screen name, color/orientation detail, layout detail, or text sample has been captured yet, so the exact defect class remains untriaged.

The current browser recovery portal now renders without the previous narrow right-edge clipping at 320 px, 390 px, and desktop widths. The exact current screenshot evidence is recorded in `07-VALIDATION.md`; screenshots remain in `/tmp` because portal/form captures can expose local SSIDs or default mining fields.

Browser screenshots validate the recovery web path only and must not be used to offset the physical-panel result. The physical CYD panel UI is now a failed parity gate, not merely an unproven gate. The failure is specific enough to block Phase 7 visual acceptance, but not yet detailed enough to identify whether the root is panel driver flags, LVGL layout, decoded asset rendering, screen selection/state, color inversion/order, rotation, or another display path. The user previously reported red LED and touch unresponsive, and current serial diagnostics still show no press signal (`directPressed=0`, `libPressed=0`), so physical touch also cannot be treated as working.

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 2/4 | Browser recovery labels are understandable, but physical panel text/copy cannot be accepted after the user-observed parity failure. |
| 2. Visuals | 1/4 | Current physical ST7789/LVGL panel visual parity failed by user observation; browser portal screenshots do not mitigate this. |
| 3. Color | 1/4 | Physical panel color/orientation/inversion parity is failed or at least materially wrong by observation; exact channel not captured. |
| 4. Typography | 2/4 | Browser text is readable, but physical panel text rendering/readability remains failed for acceptance until photographed or itemized. |
| 5. Spacing | 1/4 | Physical panel layout/geometry parity failed by observation; browser recovery spacing is separate evidence. |
| 6. Experience Design | 1/4 | Recovery AP/root work by serial/browser evidence, but touch is actively no-contact and physical UI/mining/dashboard/restore flows remain unproven. |

**Overall: 8/24**

---

## Top 3 Priority Fixes

1. **Classify the physical panel parity failure with no-secret evidence** - user impact: the current display is visibly wrong on hardware and cannot be accepted as the rebuilt NMMiner UI - concrete fix: capture a photo/video or written checklist naming the visible screen, text, colors, rotation, clipping/overlap, LED state, and whether it matches the recovered six-screen spec.
2. **Debug physical display parity separately from the browser recovery portal** - user impact: the WiFiManager portal can pass while the ST7789/LVGL panel still looks wrong - concrete fix: compare the physical panel observation against `hardware/recovered-assets/SPEC.md`/Phase 1 and Phase 3 screen expectations, then triage ST7789 flags, BGR/inversion, rotation, LVGL root/screen selection, and decoded asset rendering from that evidence.
3. **Probe the touch electrical path before touch/UI interaction claims** - user impact: the display failure is now current and touch still has no raw contact, so the device cannot complete setup or carousel validation from the panel - concrete fix: follow `07-TOUCH-PROBE-RUNBOOK.md` to confirm touch controller/FPC presence, PENIRQ/GPIO36 behavior, and CS/CLK/MOSI/MISO activity before more LVGL/touch tuning.

---

## Detailed Findings

### Pillar 1: Copywriting (2/4)

**WARNING - Recovery web portal:** The current webfix2 root page copy is direct and task-oriented: it identifies WiFiManager, shows the recovery AP name, and presents clear actions for configuring Wi-Fi, viewing info, exiting, and updating. This is sufficient for a browser recovery portal and was verified from `/tmp/p7-webfix2-ui/root-320x568.png`, `root-390x844.png`, and `root-1440x900.png`. `07-VALIDATION.md:78-112` records those browser screenshots and explicitly limits them to recovery-portal evidence.

**WARNING - Configure WiFi form:** The Wi-Fi form labels are understandable for the target operator: network, password, mining address/pool, fallback pool, timezone, and save/refresh actions. Exact form values are intentionally not copied into this tracked review.

**BLOCKER - Physical panel copy/readability:** The user's current physical observation that the miner display looks nothing like it should means on-panel copy cannot be accepted as readable, correctly placed, or even showing the intended screen. This does not identify the exact bad string yet because no no-secret photo/detail has been captured, but it is enough to fail physical copy acceptance for Phase 7. `03-VERIFICATION.md:36-41` already warned that Phase 3 proved code/build structure only, not physical rendering.

### Pillar 2: Visuals (1/4)

**PASS - Browser recovery portal:** The webfix2 root page has a clear visual focal point, centered content, high-visibility action buttons, and a readable status callout at desktop, 390 px, and 320 px.

**PASS - Browser Wi-Fi form:** The current Configure WiFi form renders at 320 px and 390 px with the expected primary/fallback pool fields and no longer shows the prior right-edge clipping.

**BLOCKER - Current physical panel visual parity failed:** The user has directly observed the live miner display and reports it "looks like shit and nothing like it should." This converts the physical UI gate from pending to failed. The failure is not yet photographically characterized, so the next step is evidence capture and classification, not acceptance. `07-VERIFICATION.md:53-61` requires visual parity as part of VALID-02, while `07-AUTHORITY-INDEX.md:57-67` keeps the physical panel/UI gate open. `needs_human_review: true`.

### Pillar 3: Color (1/4)

**WARNING - Recovery web portal:** The browser portal uses a restrained black/white/blue/gray palette. Contrast is strong, blue primary actions stand out, and the gray status callout remains readable.

**WARNING - Source limitation:** Tailwind/class color counts are not applicable because the screenshot-audited UI is firmware-served WiFiManager HTML, not a local React/Tailwind source tree.

**BLOCKER - Physical color/parity failed:** The current physical observation says the panel does not look like the expected NMMiner UI. Without a photo/detail, this cannot yet be narrowed to BGR/RGB order, inversion, rotation, palette constants, decoded asset rendering, or screen state. It still fails the color/visual parity pillar because Phase 7 acceptance depends on the actual ST7789 panel, not the browser portal. `07-01-PLAN.md:223-264` makes correct non-inverted colors part of the physical checklist.

### Pillar 4: Typography (2/4)

**PASS - Root evidence:** The current custom root page text is readable at desktop, 390 px, and 320 px after the CSS fix.

**PASS - Current form:** Labels and inputs remain readable in the current 320 px and 390 px Configure WiFi screenshots. Long form values may still scroll inside inputs as normal browser controls, but the page itself no longer clips at the viewport edge.

**BLOCKER - Physical typography not acceptable for signoff:** The physical panel report invalidates any assumption that LVGL text size, placement, or readability matches the recovered stock UI. `01-VERIFICATION.md:20-29` verifies the recovered design contract and `03-VERIFICATION.md:58-71` verifies source-level screen structure, but neither proves the current panel renders those labels correctly. No photo/detail has been captured to identify whether the issue is font, scaling, rotation, overlap, or wrong screen state.

### Pillar 5: Spacing (1/4)

**PASS - Browser root:** The current custom root layout fits within 320 px, 390 px, and desktop browser viewports after the custom WiFiManager CSS was flashed.

**PASS - Browser form:** The current Configure WiFi form stacks fields predictably and stays inside the tested mobile viewports after webfix2.

**BLOCKER - Physical layout/geometry parity failed:** The user's physical observation is a current failure of the 320x240 panel composition. The browser recovery spacing fix is still valid, but it does not prove LVGL object geometry, screen root opacity, asset placement, rotation, or carousel state on the panel. `07-VALIDATION.md:1109-1124` still has the six-screen render/color checklist pending, and the latest report moves that checklist from pending to failed pending classification.

### Pillar 6: Experience Design (1/4)

**WARNING - Recovery path proven:** The current webfix2 flashed custom recovery path works through the browser: the app readback matches the built app, AP mode started, `/`, `/info`, `/wifi`, and `/0wifi` returned HTTP 200, and current screenshots were reviewed.

**WARNING - Current custom web flow incomplete:** Current custom screenshot evidence covers the recovery root, Info, and Configure WiFi pages. It does not prove save behavior, update behavior, custom JSON API exposure, or interaction outcomes beyond the GET probes recorded in `07-VALIDATION.md`.

**BLOCKER - Physical workflow failed at first visible gate:** Phase 7 is still not complete because the physical panel now has a current user-observed visual parity failure, touch remains no-contact, and hard power-cycle repeatability, Wi-Fi/pool setup, mining-active evidence, speaker/SD/API checks, dashboard checks, and restore proof remain open. `07-CURRENT-FAILURE-STATE.md:46-56` classifies physical LVGL panel UI and touch separately from the proven recovery web UI; the new user observation updates the physical UI row from "not proven" to "failed, details not captured."

**WARNING - Custom fork status:** The custom fork is flashed with config-mode, secret-redaction, no-touch-SPIFFS-format, source-review hardening, fallback-pool fixes, post-forensics source-blocker fixes, review-05 mining/config hardening fixes, and the recovery portal CSS fix. The custom LVGL UI must still be physically validated before any claim that the custom six-screen experience works on hardware.

---

## Files Audited

- `AGENTS.md`
- `.planning/phases/07-on-device-validation/07-AUTHORITY-INDEX.md`
- `.planning/phases/07-on-device-validation/07-CURRENT-FAILURE-STATE.md`
- `.planning/phases/07-on-device-validation/07-VALIDATION.md`
- `.planning/phases/07-on-device-validation/07-VERIFICATION.md`
- `.planning/phases/07-on-device-validation/07-UAT-AUDIT.md`
- `.planning/phases/07-on-device-validation/07-TOUCH-PROBE-RUNBOOK.md`
- `.planning/phases/07-on-device-validation/07-01-PLAN.md`
- `.planning/phases/07-on-device-validation/07-02-PLAN.md`
- `.planning/phases/07-on-device-validation/07-02-SUMMARY.md`
- `.planning/phases/01-re-spec-the-ui-read-only/01-VERIFICATION.md`
- `.planning/phases/03-rebuild-core-screens-uimanager-carousel/03-VERIFICATION.md`
- `.planning/ui-reviews/.gitignore`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_miner.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_clock.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_sprice.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_weather.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp`
- `firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp`
- `/tmp/p7-stock-ui/root-desktop.png`
- `/tmp/p7-stock-ui/root-mobile.png`
- `/tmp/p7-stock-ui/root-320.png`
- `/tmp/p7-stock-ui/wifi-mobile.png`
- `/tmp/p7-stock-ui/wifi-320.png`
- `/tmp/p7-custom-ui/recovery-root-1440x1000.png`
- `/tmp/p7-custom-ui/recovery-root-390x844.png`
- `/tmp/p7-current-review05-ui/root-1440x1000.png`
- `/tmp/p7-current-review05-ui/root-390x844.png`
- `/tmp/p7-current-review05-ui/root-320x568.png`
- `/tmp/p7-current-review05-ui/info-390x844.png`
- `/tmp/p7-current-review05-ui/wifi-390x844.png`
- `/tmp/p7-webfix2-ui/root-1440x900.png`
- `/tmp/p7-webfix2-ui/root-390x844.png`
- `/tmp/p7-webfix2-ui/root-320x568.png`
- `/tmp/p7-webfix2-ui/info-390x844.png`
- `/tmp/p7-webfix2-ui/wifi-390x844.png`
- `/tmp/p7-webfix2-ui/wifi-320x568.png`
