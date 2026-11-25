# Phase 7 Binary Baseline Route

**Updated:** 2026-06-20T05:22:34Z
**Status:** active route correction.

## Decision

The active decompilation/custom-edit route is the official NMMiner `v2.0.02`
ST7789 firmware already backed up and extracted in this repo:

- Full flash backup: `hardware/backups/cyd-backup-20260617-172106.bin`
- Official local bundle: `firmware/v2.0.02/esp32-2432s028r-st7789/`
- Extracted app/decomp workbench: `hardware/recovered-assets/app_factory.bin`,
  `drom.bin`, `irom.bin`, `irom.asm`, and `decompiled/`

The earlier device/factory dump
`hardware/backups/nmminer-esp32-d0wd-v3-full-flash-2026-06-01.bin` identifies as
`v1.8.28`. Keep it as a historical factory/setup snapshot and recovery artifact,
not the primary baseline for the current custom-edit effort.

The public flasher manifest now also lists `v2.0.03` for
`esp32-2432s028r-st7789`, but this repo's existing RE/spec/custom work is based on
the verified local `v2.0.02` official backup. Do not silently switch to `v2.0.03`
without a fresh provenance record and a new extraction/decomp pass.

## Evidence

Official `v2.0.02` ST7789 bundle hashes already present locally:

| File | SHA-256 |
|------|---------|
| `boot_app0.bin` | `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0` |
| `bootloader.bin` | `0f65515a7e0dfe9cc4bc78c01bef79c941af1b2deb34d01ddd500ee3d465eedf` |
| `firmware.bin` | `fc7ff6164f8c9135f7fd0085da4c209b0ab9c6bdd38efc7648af04511919f524` |
| `littlefs.bin` | `700681e91194ca233dfdef62725541f917270604279253c4233e5551bf93a2cf` |
| `partitions.bin` | `f1882db7f6e352825314f3dcc5db01085e74625655c5f1decf58dd84fa118226` |

`cyd-backup-20260617-172106.bin` matches the official `v2.0.02` ST7789 bundle at
the flasher offsets for bootloader, partition table, boot app, and app firmware:

| Offset | Component | Result |
|--------|-----------|--------|
| `0x1000` | `bootloader.bin` | byte match |
| `0x8000` | `partitions.bin` | byte match |
| `0xe000` | `boot_app0.bin` | byte match |
| `0x10000` | `firmware.bin` | byte match |

The filesystem partition in the full backup is not byte-identical to the pristine
official `littlefs.bin`; it matches `hardware/recovered-assets/spiffs.bin`
instead. Treat it as the live device filesystem captured after setup/use. The
official bundled `littlefs.bin` contains two `/ss` assets (`001`, `002`); the
captured/recovered expanded image contains 17 assets (`002` through `018`).
Use `07-LITTLEFS-SS-ASSET-REPLACEMENT.md` for the replacement proof and do not
mix this filesystem distinction into the app binary identity.

`v2.0.02` app facts:

- Image size: `2794160`
- Entry point: `0x40086aa4`
- DROM: load `0x3f400020`, app file offset `0x00000018`, payload
  `0x00000020`, length `0x157de8`
- DRAM: load `0x3ffbdb60`, app file offset `0x00157e08`, payload
  `0x00157e10`, length `0x0065a0`
- IRAM0: load `0x40080000`, app file offset `0x0015e3b0`, payload
  `0x0015e3b8`, length `0x001c60`
- IROM: load `0x400d0020`, app file offset `0x00160018`, payload
  `0x00160020`, length `0x13227c`
- IRAM1: load `0x40081c60`, app file offset `0x0029229c`, payload
  `0x002922a4`, length `0x017fe4`
- ESP-IDF: `v4.4.6-dirty`
- Validation hash: `df782f8b1f66aa01a39ad4fdf4c55addd062caa45e287af149301b7071bc5638`

String evidence separates the two baselines:

- `hardware/recovered-assets/drom.bin` contains `v2.0.02` and the class-name
  anchors used by the current RE work, including `Clock320x240`,
  `Config320x240`, `Loading320x240`, `Miner320x240`, and `Weather320x240`.
- `hardware/recovered-assets/original-20260601/drom.bin` contains `v1.8.28` and
  `NMMiner version [%s] starting...`, but not those `v2.0.02` screen-class
  anchors.

## Active Work Route

1. Continue decompilation from the official `v2.0.02` ST7789 app extraction.
2. Use the existing Ghidra/string-xref/decompiled artifacts as the source of truth
   for custom edits.
3. Use `07-BINARY-DECOMP-TASKS.md` for the immediate decompilation queue and
   `07-BINARY-EDIT-TARGET-MAP.md` for candidate edit sites and patch
   preconditions.
4. Treat the current fork/UI/touch diagnostic path as historical unless it is
   needed only to restore device state or preserve reversibility.
5. Before any future firmware write, preserve a fresh full 4 MB flash backup.
6. Do not commit secrets, raw flash backups, raw serial logs, wallet values,
   Wi-Fi credentials, or license values.

## Public Metadata Checked

Official sources checked on 2026-06-19:

- GitHub releases:
  `https://github.com/NMminer1024/NMMiner/releases`
- Live flasher manifest:
  `https://flash.nmminer.com/firmware/devices_version.json`
- Official flasher:
  `https://flash.nmminer.com/`
- Official CYD/ST7789 guide:
  `https://www.nmminer.com/2025/12/01/flash-correct-nmminer-to-cyd/`
- Official start/troubleshooting guide:
  `https://www.nmminer.com/2025/11/22/how-to-start-your-nmminer-journey/`

Current source conclusions:

- Official GitHub releases list `v2.0.02` as 2026-05-08/09 and `v2.0.03` as
  2026-06-08.
- The official release notes for `v2.0.02` include the ST7789 boot black-screen
  SPI-mode fix.
- Rechecked on 2026-06-20: the public `v2.0.02` tag tree contains README,
  docs, CH340 driver assets, figures, per-board firmware binaries, and tool
  binaries. It does not contain a firmware `src/`, `platformio.ini`, Arduino
  sketch, ESP-IDF project, or other buildable firmware source tree. The
  `v2.0.02` release asset is `firmware.rar` with GitHub API digest
  `sha256:daffa84c3d28c19c50bb23b605260fc00cf3bcfdcd7809715535fe2d5efa1128`.
- The live public flasher manifest currently lists both `v2.0.03` and
  `v2.0.02` for `esp32-2432s028r-st7789`; this repo remains pinned to the
  already extracted `v2.0.02` baseline.
- The official CYD/ST7789 guide says the NMMiner-logo ESP32-2432S028 uses the
  ST7789 LCD driver, matching this repo's target selection.
- Official setup says first-boot config uses AP `nmap-2.4g`, and the
  troubleshooting guide says most ESP32 NMMiner devices support only 2.4 GHz
  Wi-Fi. It also treats red-light/no-screen as a wrong-firmware symptom and
  points users back to the official flasher. This project still requires a fresh
  full backup and the locked ST7789 route before any write.

## Current Decompilation Artifacts

- `scripts/nmminer_str_xref.py --all` resolves the six screen-class anchors plus
  `ss_render` from DROM strings to IROM literal refs.
- `scripts/nmminer_addr_map.py` maps DROM, DRAM, IRAM0, IROM, and IRAM1 VM
  addresses to app offsets and full-flash offsets for the verified `v2.0.02`
  app image.
- `07-BINARY-EDIT-TARGET-MAP.md` now has concrete literal and function offset
  records. Literal records are patch candidates; function records are
  decompilation/navigation records only until runtime update paths are proven.
- `scripts/nmminer_patch_literal.py` is the first local-only patch tool for
  approved fixed-length DROM literals. It has validated a throwaway `/tmp`
  tagline patch with a valid ESP32 checksum/hash footer. It does not flash.
- `firmware/reconstructed/nmminer-v2-baseline/` is now the first tracked
  source-recovered baseline scaffold derived from the official binary artifacts.
  It captures the verified full segment address map, the official IRAM1
  entrypoint constant `0x40086aa4`, screen widget specs, publisher
  callsites, runtime evidence records, SPrice market config/default evidence,
  executable WatchCoins tokenizer behavior, SPrice `runtime+0x74`
  setter/task/batch-parser evidence, the `0x400e3a80` 24-byte row producer
  evidence, the proven SPrice row cache writer model, official app
  lifecycle/task evidence including the
  `0x400de1e4` begin-style route and corrected task/name helper records, and
  official/recovered LittleFS screensaver asset records. It now also exposes the
  raw official startup literal table and ordered startup-step table from
  `Official_startup.recovered.c`, including the BSS clear range
  `0x3ffc4100..0x3ffd6b30`, constructor predicate/sweep
  `0x40130cd0 -> 0x40086910`, framework hooks, scheduler handoff
  `0x4008766c`, runtime service slots `+0x74/+0x80/+0x84/+0x88`, and a
  source-baseline gate summary that keeps behavior parity, fresh-backup write
  gate, and flash approval open. The scaffold also now has
  `src/recovered_app_lifecycle.cpp`, a lifecycle validation layer that checks
  the decoded entrypoint, BSS range, constructor route, scheduler handoff, begin
  route, required task registrations, runtime service slots, behavior-parity
  gate, fresh-backup gate, and flash guard. Current host output is
  `Lifecycle validation=8/10 ... flashable=0`; the two open lifecycle gates are
  official behavior parity and the fresh-backup write gate. It also now has
  `src/recovered_sprice_behavior.cpp`, which implements and host-tests the
  official WatchCoins default parsing, null versus empty-string behavior,
  exactly-20-token cap, truncation before the 21st token, 16-byte row payload
  adapter, recovered `+16` price-descending ordering key, 16-row rendered trim,
  and null ArrayView guards. Exact equal-price and NaN row ordering behavior
  remains open.
  It now also contains a compile-only PlatformIO ESP32/Arduino/LVGL harness
  using the recovered official partition geometry and ST7789 display flags.
  The harness renders recovered baseline evidence and proves the reconstructed
  tree can build under the ESP32 toolchain, but it is not official behavior
  parity yet and is not flash-approved.
  It host-compiles with:
  `g++ -std=c++17 -Wall -Wextra -pedantic -Ifirmware/reconstructed/nmminer-v2-baseline/include firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/tools/print_baseline.cpp -o /tmp/nmminer-v2002-baseline`.
  The SPrice slice also host-tests with:
  `g++ -std=c++17 -Wall -Wextra -pedantic -Ifirmware/reconstructed/nmminer-v2-baseline/include firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/tools/test_sprice_behavior.cpp -o /tmp/nmminer-sprice-test`.
  The swarm slice also host-tests with:
  `g++ -std=c++17 -Wall -Wextra -pedantic -Ifirmware/reconstructed/nmminer-v2-baseline/include firmware/reconstructed/nmminer-v2-baseline/src/official_v2002_st7789.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_app_lifecycle.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_sprice_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/src/recovered_swarm_behavior.cpp firmware/reconstructed/nmminer-v2-baseline/tools/test_swarm_behavior.cpp -o /tmp/nmminer-swarm-test`.
  It also builds with:
  `uvx platformio run -d firmware/reconstructed/nmminer-v2-baseline`.
  Revalidation on 2026-06-20 produced `firmware.bin` size `575616` bytes
  (`575253` bytes PlatformIO flash use), still under the official `0x380000`
  factory partition; the generated partition table
  hash remained identical to the official bundle
  (`f1882db7f6e352825314f3dcc5db01085e74625655c5f1decf58dd84fa118226`).
  The PlatformIO target includes a reconstructed-upload guard that refuses
  upload/program/erase/uploadfs/uploadfsota targets unless
  `NMMINER_ALLOW_RECONSTRUCTED_UPLOAD=1` is explicitly set after the project
  write gate is satisfied. On 2026-06-20, `upload`, `program`, `erase`,
  `uploadfs`, and `uploadfsota` all failed at that guard before any device
  write path.
- `hardware/recovered-assets/decompiled/source/Official_startup.recovered.c`
  records the official startup evidence recovered from the newly extracted IRAM
  segments: app entrypoint `0x40086aa4`, IRAM0 literal pool values, BSS clear
  range `0x3ffc4100..0x3ffd6b30`, constructor-sweep gate `0x40130cd0 ->
  0x40086910`, framework/IROM startup hook callsites, app lifecycle logs,
  task/name helper evidence for `(button)`, `(scan)`, `(swarm)`, `(stratum)`,
  `(tMiner)`, and split `(nMiner)` setup/helper callsites, and the
  `0x400de1e4` begin-style bring-up path through the
  `NMMiner::begin: all tasks started` log at `0x400de25d`.
  Exact IDF/Arduino helper names remain unresolved.

## Source-Recovery Meaning

Because official public `v2.0.02` firmware distribution is binary-only, the
active "decompile source" goal is a source-equivalent reconstruction from the
verified official binary plus LittleFS assets. Decompiled artifacts are not
expected to become the vendor's original source tree byte-for-byte. The baseline
success target is instead:

1. recover the screen/task/data-path semantics from `nmminer-app.elf` and
   `irom.asm`;
2. grow `firmware/reconstructed/nmminer-v2-baseline/` from the compile-only
   ESP32/LVGL harness into source-equivalent firmware that matches
   the official `v2.0.02` ST7789 behavior at the UI/API/mining/config
   boundaries we need;
3. validate that rebuilt baseline against the official binary behavior before
   adding SD-card and speaker features;
4. only then apply custom hardware edits and open the future flash gate after a
   fresh full backup.
