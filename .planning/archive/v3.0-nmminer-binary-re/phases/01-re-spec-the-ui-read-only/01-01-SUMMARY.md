# 01-01 SUMMARY — Ghidra workbench + string→l32r xref helper (RE-01)

**Status:** complete
**Requirements:** RE-01

## What was built
- **Ghidra 11.3.1** (`~/ghidra_11.3.1_PUBLIC`) + **dynacylabs esp32 flash-loader v11.3.1** extension installed; provenance (URL/tag/SHA256/path) in gitignored `hardware/recovered-assets/WORKBENCH.md`. SHAs: Ghidra `bcda0a9d…`, loader `fb9f3822…`.
- JDK blocker solved without sudo: system had only the JRE; installed portable **Temurin JDK 21.0.11** (`~/jdk-21.0.11+10`), pointed Ghidra at it via `JAVA_HOME_OVERRIDE`.
- Image imported via the ESP32 Flash Image loader (Xtensa:LE:32). The loader threw a non-fatal `MemoryConflictException` on a RAM `.data` segment and aborted before mapping IROM, so the **IROM code segment (`0x400d0020`) was injected from `irom.bin`** (D-05 fallback path) and auto-analyzed. DROM (`0x3f400020`) + SVD peripheral registers mapped by the loader.
- **`scripts/nmminer_str_xref.py`** (committed) — the proven string→l32r→function xref pipeline. `--all` resolves all 6 screen class identifiers + `ss_render`; `--extra-from` reads runtime literals from a gitignored file; identifier-only (D-11, grep-guarded).

## Verification
- `nmminer_str_xref.py` reproduces Miner320x240→`0x400d1960`, Clock320x240→`0x400d1864`, SPrice→`0x400d19a8` (+1-byte case), and resolves all 6 + ss_render.
- Memory map confirmed in Ghidra: `app_DROM0_3f400020` present; `app_IROM0` @ `0x400d0020` injected; DROM byte-check at `0x3f4be960` = "Miner320x240".

## RE-01 outcome (per reworded requirement)
FIDB build was **not** required: the screen/render/share-accept functions are fully decompilable via the **dynacylabs loader ROM/SVD annotations + the string→l32r xref pipeline** — symbol recovery sufficient for Plans 02/03 (the D-05 fallback, which IS the requirement). Full ESP-IDF FIDB deferred (accelerator only; LVGL is statically linked and wouldn't be named by an IDF FIDB anyway).

## Notes / deviations
- IROM-injection fallback (vs pure loader load) recorded in WORKBENCH.md as the authoritative import.
- Agent shell is a TTY (no WAYLAND_DISPLAY) → all RE done via `analyzeHeadless`; user's Wayland GUI available but not needed.
