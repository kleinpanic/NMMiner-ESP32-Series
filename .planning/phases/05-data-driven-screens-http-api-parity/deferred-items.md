# Phase 5 — Deferred / Out-of-Scope Items

Items discovered during execution that are outside the current plan's scope
(SCOPE BOUNDARY rule). Not fixed by the discovering plan.

## From 05-03 (Weather)

### [out-of-scope] nm_market.cpp ArduinoJson Filter() ambiguity blocks full build

- **Discovered:** 05-03 Task 2 (`pio run -e ESP32-2432S028R`).
- **File:** `firmware/custom/NerdMiner-CYD-Custom/src/nm_market.cpp:178` (owned by 05-02, committed f7460c2).
- **Error:** `call of overloaded 'Filter(...ElementProxy...)' is ambiguous` — ArduinoJson v6.21
  `DeserializationOption::Filter(filter[0])` where `filter[0]` is an `ElementProxy` matches both
  the `JsonVariantConst` ctor and the copy/move ctors. Fix: wrap as
  `Filter(filter[0].as<JsonVariantConst>())` or `Filter(filter.as<JsonVariant>()[0])`.
- **Why not fixed here:** nm_market.cpp is explicitly outside 05-03's ownership (sibling W2 plan).
  05-03's own files (`nm_weather.{h,cpp}`, `nm_screen_weather.cpp`) pass `-fsyntax-only` cleanly
  under the full ESP32 toolchain + build flags; the failure is entirely in the sibling file.
- **Owner:** 05-02 (or the 05-06 integration build, which owns the full-build gate).
- **[05-02] Full-build link blocked by sibling W2 file `src/ui_nm/nm_screensaver.cpp`** (plan 05-04, mid-flight). Compile errors: `File`/`f`/`dir`/`e` not declared (missing `#include <FS.h>`/`<LittleFS.h>` or scope issue). Out of scope for 05-02 (do not touch sibling-owned files). 05-02 files (`nm_market.cpp`, `nm_screen_sprice.cpp`) both compile to clean `.o` objects; full `pio run` SUCCESS deferred until 05-04 completes / 05-06 integrates.
