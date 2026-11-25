---
phase: 07-on-device-validation
reviewed: 2026-06-18T19:03:36Z
depth: deep
files_reviewed: 13
files_reviewed_list:
  - firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/sdstore.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_loading.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_ui.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_touch.h
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.h
  - firmware/custom/NerdMiner-CYD-Custom/src/NerdMinerV2.ino.cpp
  - firmware/custom/NerdMiner-CYD-Custom/src/webapi.cpp
findings:
  critical: 2
  warning: 4
  info: 0
  total: 6
status: issues_found
---

# Phase 7: Code Review Report

**Reviewed:** 2026-06-18T19:03:36Z
**Depth:** deep
**Files Reviewed:** 13
**Status:** issues_found

## Summary

Reviewed only the requested nested-firmware files for the Phase 7 H1-H5 gap-closure areas:
NVS-first WiFi config, no `WifiPW` writes to SD, LVGL config feedback, opaque loading/config
screens, rotation-aware touch mapping, runtime brightness/orientation apply, and LVGL pumping
during blocking waits.

No flash, upload, read_flash, write_flash, or backup commands were run. `git diff --check`
passed for the scoped files.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Blocking WiFiManager portal paths still starve LVGL

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:454`

**Issue:** The new `configWaitDelay()` pump only helps the hand-written connect/delay loops.
The forced-config path sets `wm.setConfigPortalBlocking(true)` and then calls
`wm.startConfigPortal()` at lines 454-457; the normal fallback path does the same with
`wm.autoConnect()` at lines 509-513. The local WiFiManager implementation enters its own
blocking loop and only calls `yield()`, not `nm_uimanager_tick()`. During those portal waits
LVGL, screen updates, touch polling, and the countdown/config feedback all freeze for up to
the portal timeout.

**Fix:** Keep the CYD portal path non-blocking and own the wait loop so both WiFiManager and
LVGL are pumped:

```cpp
wm.setConfigPortalBlocking(false);
wm.startConfigPortal(apName, DEFAULT_WIFIPW);

uint32_t start = millis();
while (WiFi.status() != WL_CONNECTED &&
       (uint32_t)(millis() - start) < 180000) {
    wm.process();
    nm_uimanager_tick();
    delay(10);
}
```

Apply the same pattern to the `autoConnect()` fallback, or avoid `autoConnect()` on CYD and
explicitly run the non-blocking portal loop after direct-connect failure.

### CR-02: Runtime orientation apply changes TFT rotation without repainting LVGL

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_display.cpp:170`

**Issue:** `nm_display_apply_runtime_settings()` calls `tft.setRotation(rotation)` and updates
touch mapping at lines 172-177, but it does not invalidate or redraw the active LVGL screen.
`setRotation()` changes how future writes map to panel memory; it does not rotate pixels that
are already in GRAM. This is called after WiFi/config load from `NerdMinerV2.ino.cpp:189` and
after `/api/settings/preference` writes from `webapi.cpp:493`, so a saved or runtime
`RotateScreen` change can leave stale/mixed-orientation pixels until a later page rebuild,
while touch coordinates have already moved to the new mapping.

**Fix:** After changing rotation, invalidate and flush the active screen, preferably through a
UIManager repaint helper:

```cpp
tft.setRotation(rotation);
nm_touch_set_display_rotation(rotation);

lv_obj_invalidate(lv_scr_act());
lv_refr_now(NULL);
```

If full-frame flushing still needs multiple passes with the partial draw buffer, expose the
existing `nm_uimanager_pump_full_frames()` behavior as a safe repaint helper and call it after
runtime rotation changes.

## Warnings

### WR-01: SD fallback can promote an unusable WiFi config into NVS

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:267`

**Issue:** `tryConfigFromSD()` accepts the fallback when `settings->WifiSSID.length() > 0` at
line 284, but `Settings` starts with default SSID/password values before this function is
called. It does not require that `/sd/config.json` actually contains `SSID`, nor does it
distinguish a WiFi-usable legacy file containing `WifiPW` from a newly mirrored SD file where
`writeConfigToSD()` intentionally omits `WifiPW` at lines 296-303. When NVS is empty,
`init_WifiManager()` promotes that SD result into NVS at lines 355-357, which can save a real
SSID with the default AP password or save all-default WiFi credentials, causing repeated
failed direct-connect attempts before the portal.

**Fix:** Split SD import into app settings and WiFi credentials. Only return "usable WiFi
config" when the JSON itself contains a non-empty `SSID` and either a deliberate legacy
`WifiPW` or another explicit marker for open networks. Otherwise import non-secret settings
but force the captive portal instead of saving fake credentials to NVS.

### WR-02: Config screen can stay dimmed or covered by screensaver when entered from idle

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_uimanager.cpp:161`

**Issue:** `nm_uimanager_show_config()` switches to the Config page and pumps two frames, but
it does not stop an active screensaver, clear dim state, reset idle state, or wake the
backlight. If config mode is entered from an idle/dimmed state via the boot button, the portal
feedback can be present but too dim to see. If the screensaver is active, the next tick can
continue driving the screensaver state over the config page.

**Fix:** Treat config entry as an explicit foreground interaction: stop `nm_ss`, clear
`s_ss_active`, clear `s_dimmed`, update `s_last_input_ms`, call `nm_display_wake()`, then load
and flush the Config page.

### WR-03: Config countdown is hardcoded to 300s while the portal timeout is 180s

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/ui_nm/nm_screen_config.cpp:79`

**Issue:** The Config page seeds `"300s"` at lines 79-83 and `nm_config_update()` never changes
it at lines 104-108. The WiFiManager timeout is set to 180 seconds in `wManager.cpp:384`, so
the LVGL config feedback is factually wrong during the portal window.

**Fix:** Either pass the actual timeout/remaining time into the Config screen and update it
from `nm_config_update()`, or remove the countdown label until real state is wired.

### WR-04: Captive portal and serial provisioning can save brightness 0 and black out the panel

**File:** `firmware/custom/NerdMiner-CYD-Custom/src/wManager.cpp:474`

**Issue:** The web API only accepts brightness 1-100 percent, but serial/portal paths clamp
raw PWM brightness to `0..255` at lines 222, 283, 474, 529, and 585. `nm_display_resolve_brightness()`
then treats 0 as valid at `nm_display.cpp:90-96` and `nm_display_apply_runtime_settings()`
writes it immediately. A user-entered `0` can make the screen appear dead after config save,
which reintroduces the black-screen recovery problem this phase is trying to close.

**Fix:** Use a nonzero lower bound for user-facing brightness, or reserve 0 only for an
explicit "screen off" mode with a guaranteed wake/reset path. For ordinary config, clamp to
`1..255` or default invalid/zero values to `DEFAULT_BRIGHTNESS`.

---

_Reviewed: 2026-06-18T19:03:36Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_
