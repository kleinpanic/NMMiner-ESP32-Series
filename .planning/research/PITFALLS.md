# Pitfalls Research

**Domain:** Reverse-engineering + binary-patching a closed ESP32 (Xtensa LX6) / Arduino-ESP32 / LVGL 8.x firmware on a CYD ESP32-2432S028R
**Researched:** 2026-06-17
**Confidence:** HIGH for technical pitfalls 1–3 (verified against esptool/LVGL/CYD sources + project boot-log facts); MEDIUM for legal (4); HIGH for RF fragility (5, project-observed).

> Scope note: this milestone is Path B (reverse-engineer the closed `v2.0.02` image). The asset/format facts below are grounded in the project's own findings (DROM `0x3f400020`, IROM `0x400d0020`, confirmed `lvgl_color_buf`, `Clock320x240`, the already-failed `lv_img_dsc_t` v8 scan) plus verified upstream LVGL/esptool/CYD sources.

---

## Critical Pitfalls

### Pitfall 1: LVGL image extraction scans for the wrong header format (v9 magic byte vs v8 packed bitfield)

**What goes wrong:**
A scan for `lv_img_dsc_t` images in DROM returns zero matches (already observed in this project). You conclude the assets aren't stored as LVGL images, or that they're compressed, and waste time chasing a non-existent decode step.

**Why it happens:**
There are two incompatible on-flash layouts and the scanner was almost certainly built for the wrong one.

- **LVGL v9** `lv_image_dsc_t` begins with a `lv_image_header_t` whose first byte is a **magic byte** (`LV_IMAGE_HEADER_MAGIC` = `0x19`, "must not be a valid ASCII char nor > 0x80"). A v9-style scan greps for that magic. [REF: https://lvgl.io/docs/open/9.1/API/draw/lv_image_dsc]
- **LVGL v8** (what ships with the ESP-IDF v4.4.6 / Arduino-ESP32 generation of NMMiner) has **no magic byte at all**. `lv_img_header_t` is a single packed 32-bit bitfield: [REF: lvgl/lvgl release/v8.3 `src/draw/lv_img_buf.h`]

  ```c
  typedef struct {
      uint32_t cf : 5;          // color format
      uint32_t always_zero : 3; // bits 5..7 of byte0 = 0
      uint32_t reserved : 2;
      uint32_t w : 11;          // width
      uint32_t h : 11;          // height
  } lv_img_header_t;            // 4 bytes, little-endian
  ```

  In flash the full descriptor is:

  ```c
  typedef struct {
      lv_img_header_t header;   // 4 bytes
      uint32_t data_size;       // 4 bytes
      const uint8_t * data;     // 4 bytes -> a DROM pointer (0x3F4xxxxx)
  } lv_img_dsc_t;               // 12 bytes total
  ```

  A v8 descriptor is recognized by structure, not a magic byte: byte0 high 3 bits (`always_zero`) == 0; `cf` in the valid set {4=TRUE_COLOR, 5=TRUE_COLOR_ALPHA, 16=RGB888, 17=RGBA8888, 19=RGB565, 8/9/10/11 indexed, 12–15 alpha}; plausible `w`,`h` (e.g. ≤ 320×240 for this panel); `data_size` consistent with `w*h*bpp`; and `data` pointing into the DROM window (`0x3F4xxxxx`). [REF: lv_img_buf.h enum LV_IMG_CF_*]

**How to avoid:**
- Build the extractor against the **v8 12-byte descriptor**, validating by the constraint set above (not a magic byte). The `data` pointer is the strongest signal: it must land inside `[0x3f400020, 0x3f400020 + len(drom.bin))`. Convert pointer→file-offset with `offset = ptr - 0x3f400020`.
- Confirm the LVGL version first by string/symbol scan of the image: look for `lv_init`, `LVGL`, version strings like `8.3`, or symbols already found (`lvgl_color_buf`). Decide v8 vs v9 before writing any parser.
- For RGB565 (`cf=19`, the overwhelmingly likely format on a 16-bit ST7789 panel), pixel stride = `w*2` bytes; for TRUE_COLOR_ALPHA add the alpha plane. Render extracted bytes as raw RGB565 320×240 to eyeball-verify before trusting the parser.
- Cross-check against the named resource already located (`Clock320x240`) — that symbol's address region is a seed for where the descriptor table lives.

**Warning signs:**
- Zero matches on a magic-byte scan (already hit — this is the tell, not a dead end).
- Extracted "images" whose `data` pointer is outside the DROM window, or whose `data_size != w*h*bpp` → your field offsets are wrong.
- Garbage/sheared render → wrong color format or wrong stride (classic v8 RGB565-vs-TRUE_COLOR_ALPHA mixup).

**Phase to address:** Asset-recovery phase (the first RE phase). This is the gate for everything visual downstream.

---

### Pitfall 2: Binary patching breaks the appended SHA-256 image integrity check ("Image hash failed - image is corrupt")

**What goes wrong:**
You patch bytes in-place (rename a string, redirect a branch, splice a new asset), reflash, and the device refuses to boot or logs `Image hash failed - image is corrupt`. The string the project already found in the binary is the bootloader's integrity verdict.

**Why it happens:**
ESP-IDF images can carry a **simple SHA-256 of the whole image appended after the checksum** (`hash_appended = 0x01` in the extended header). It is *not* secure-boot and *not* flash-encryption — it is a plain corruption check, and the second-stage bootloader validates it on load. Any byte you change invalidates that digest, and the stored digest no longer matches the recomputed one. [REF: https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/firmware-image-format.html]

This is distinct from (and far more likely than) secure boot / flash encryption — see the note below.

**How to avoid:**
1. **Determine first whether the app image actually has `hash_appended` set, and whether secure boot/flash-encryption are on.** Run `esptool.py image_info --version 2 app.bin` on the extracted app partition and `esptool.py get_security_info` / read eFuses (`espefuse.py summary`). Three cases:
   - **No hash, no secure boot:** patch freely, reflash, done.
   - **Hash appended, no secure boot (most likely here):** after patching, **recompute and re-append the SHA-256** so the digest matches. Do not edit and reflash without re-stamping.
   - **Secure boot enabled:** you cannot re-sign without the private key → in-place patching of the app is effectively dead; pivot to a transplant approach (see below).
2. **Prefer the rebuild-digest path over hand-fixing bytes.** The clean way to re-stamp is to round-trip through esptool: `esptool.py image_info` to understand layout, patch, then regenerate the image so the digest+checksum are recomputed. If you patch the raw `.bin` by hand, you must also (a) fix the 1-byte XOR checksum at the end of the last segment and (b) recompute the trailing 32-byte SHA-256 over the image. Both, or the bootloader rejects it.
3. **Confirm flash-encryption is OFF before assuming you can read DROM/IROM meaningfully.** The fact that the project already extracted readable `drom.bin`/`irom.bin` and found plaintext strings (`Clock320x240`, API paths) is strong evidence flash encryption is **disabled** — otherwise those reads would be ciphertext. Document this as a checked assumption, not a hope.

**Xtensa-specific patch hazards (when you do redirect code, not just assets):**
- **Variable-length instructions.** Xtensa LX6 mixes 24-bit (3-byte) and 16-bit (2-byte, "narrow") encodings. You cannot assume 4-byte alignment of instructions; over/under-writing by one byte desyncs the entire downstream disassembly. Always re-disassemble around the patch site after editing.
- **Windowed register ABI / CALL range.** The standard ABI uses register windows; `CALL8`/`CALL4`/`CALL12` and `CALLX` have limited PC-relative range and rotate the window. A naive "redirect this call to my function" can land out of range or corrupt the window-rotation assumptions. Prefer patching a `CALLX*` through a register you load with a literal, or trampoline via the literal pool.
- **Literal pools (L32R).** Constants/pointers are loaded with `L32R`, which reads from a negative-offset literal pool. If you relocate code you may break the L32R back-references. Keep patched code near its literals or add a new literal.
- **No genuinely-free flash inside a segment.** "Free" padding between segments is illusory — segment load addresses are fixed in the header. Net-new code must go in actual free flash (an unused partition, or appended space) and be reached by a trampoline, with the image digest re-stamped afterward.

**Warning signs:**
- `Image hash failed - image is corrupt` or boot loop after a reflash that flashed a hand-edited `.bin`.
- `esptool image_info` reporting "checksum/hash: invalid" on your patched image.
- Disassembly that "goes sideways" (nonsense ops) a few bytes after your edit → instruction-length desync.

**Phase to address:** A dedicated "image integrity & patch-safety recon" phase *before* the first real patch. Verdict (hash? secure boot? encryption?) decides whether the whole milestone is in-place-patch or transplant.

---

### Pitfall 3: XPT2046 touch doesn't register because of the CYD dual-SPI-bus gotcha — and the SD plan collides with it on VSPI

**What goes wrong:**
The open fork's touch "doesn't register at all" on this exact unit (project-observed). Naive ports init the touchscreen on the same SPI instance as the display, or with default-init that silently never reads the controller. Then, separately, the planned SD card on VSPI (GPIO5/18/19/23) **collides with touch**, because on the CYD the XPT2046 also lives on the VSPI bus — so "fix touch" and "add SD" interact.

**Why it happens (CYD-specific wiring):**
- The ST7789 **display** is on **HSPI**. The **XPT2046 touch** controller is on **VSPI** with non-default pins: **CLK GPIO25, MOSI GPIO32, MISO GPIO39, CS GPIO33, IRQ GPIO36**. [REF: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/]
- The **microSD** slot uses the **default VSPI hardware pins**: **CS GPIO5, SCK GPIO18, MISO GPIO19, MOSI GPIO23**. [REF: https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/]
- Touch and SD therefore both want VSPI. Because the bus is shared but the CS/clock pin *sets* differ, the common failure is: bring up SD on hardware VSPI and touch stops responding (or vice-versa), because the second `SPIClass(VSPI).begin(...)` reconfigures the bus pins out from under the first peripheral. RNT's own "use all three simultaneously" guide exists precisely because the naive setup fails. [REF: https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/]

**How to avoid:**
- **Touch first, in isolation.** Bring touch up with an explicit `SPIClass vspi = SPIClass(VSPI); vspi.begin(25, 39, 32, 33);` and pass that bus into the XPT2046 driver — do **not** rely on default pins. Use the up-to-date `XPT2046_Touchscreen` library, not PlatformIO's bundled stale copy. [REF: https://github.com/marcelrv/Cheap-Yellow-Display-ESP32-2432S028R]
- **Calibrate before declaring it broken.** "Touch doesn't register" frequently means raw reads *are* coming in but the calibration maps them off-screen, or the IRQ pin (GPIO36, input-only, no pull-up) is misread. Print raw `getPoint()` Z-pressure first; if Z changes on press, the controller works and it's a calibration/IRQ issue, not a bus issue.
- **For the SD + touch coexistence, pick one strategy and commit:**
  - (a) **Software-SPI touch:** use a SoftSPI build of the XPT2046 driver so touch leaves the VSPI hardware peripheral entirely free for SD. This is the documented CYD fix for the three-peripheral conflict. [REF: RNT CYD display+touch+SD guide]
  - (b) **Strict mutual exclusion + re-begin:** keep both on hardware VSPI but never have both transactions in flight; re-`begin()` the bus with the right pin set around each peripheral's access. More fragile; only if SoftSPI is unavailable.
- **In the RE context specifically:** because this milestone may transplant touch behavior back into / alongside the stock UI, decide early whether touch comes from (i) the recovered stock code paths (`/api/update/screensaver`, the stock input handler) or (ii) a fresh Arduino-side driver. Mixing both bus owners is the surest way to get "no touch."

**Warning signs:**
- Touch dead **and** SD works (or the reverse) → classic VSPI ownership collision.
- Raw Z-pressure flat across presses → wiring/CS wrong (bus problem). Raw Z changes but no UI response → calibration/mapping problem (not a bus problem). Diagnose which before "fixing" the wrong layer.
- IRQ (GPIO36) wired but never firing → it's input-only with no internal pull; needs the driver's polling mode or an external pull-up.

**Phase to address:** Touch-restoration phase, sequenced **before** the SD-integration phase so the bus-ownership decision (SoftSPI touch) is locked in before SD wiring is committed.

---

### Pitfall 4: Modifying / redistributing a closed, license-activated binary without rights

**What goes wrong:**
NMMiner is distributed binary-only with a **per-device license activation** (the project's own boot log shows "License verification passes"). The GitHub repo ships compiled firmware, has **no LICENSE file**, and gates use behind an activation string. Modifying and especially *redistributing* the patched image can (a) violate the implicit all-rights-reserved copyright (no license granted = no modification/redistribution right by default), and (b) break or tamper with the license-verification path inside the binary. [REF: https://github.com/NMminer1024/NMMiner] [REF: https://www.nmminer.com/product/nm-cyd/]

**Why it happens:**
"No LICENSE file" is misread as "public domain / do whatever." The opposite is true: absent an explicit grant, default copyright reserves all rights to the author. The activation system also means the binary likely checks an on-device/online license; a patch that shifts code or strings can trip that check even if you weren't targeting it.

**How to avoid:**
- **Scope the work to personal, private modification of a device you own.** Reverse-engineering for interoperability/personal use is broadly defensible in many jurisdictions, but **redistribution of a modified binary is the bright line** — do not publish or share the patched `v2.0.02` image. Keep modified images out of the repo (the project already keeps backups/secrets out of git — extend that to patched firmware).
- **Do not target, disable, or circumvent the license-verification code.** Treat the license path as read-only. If a patch *incidentally* breaks license verification, that's a signal you've shifted something you shouldn't have — revert and re-place your patch elsewhere.
- **Prefer the additive/transplant model over editing the stock binary** where feasible: re-implement the recovered look/behavior in the open fork (`NerdMiner-CYD-Custom`) rather than mutating NMMiner's proprietary image. This sidesteps both the copyright and the license-check exposure, and aligns with the existing fallback fork already in the repo.
- **Document the personal-use/no-redistribution intent in the requirements** so downstream phases don't accidentally publish artifacts.
- If wider distribution ever becomes a goal, **email the maintainer** (`nmminer1024@gmail.com`) for explicit permission — they already offer free re-licensing on reflash, so they may be amenable.

**Warning signs:**
- Any plan step that says "share/publish the patched image," "host the modified firmware," or "bypass activation" → stop.
- License verification starts failing after an unrelated patch → you've moved license-relevant bytes; revert.

**Phase to address:** Requirements / project-charter phase (set the no-redistribution + don't-touch-license-path constraint up front), re-checked at each patch phase.

---

### Pitfall 5: WiFi/RF and flash fragility under iterative reflash cycles

**What goes wrong:**
The unit's WiFi radio is known-fragile under rapid reflash cycles (project-observed: needs a full power-cycle to recover). Tight edit→flash→test loops produce phantom failures — WiFi won't join, brownout resets, or the flash write itself fails partway — that look like firmware bugs but are really RF/power state not being reset.

**Why it happens:**
- ESP32 WiFi calibration and PHY state persist in RTC memory across soft resets; a soft reset after a botched flash can leave the radio in a bad calibration state until a hard power-cycle clears RTC. USB-bus-powered CYD boards also brown out under WiFi TX current spikes, and a marginal USB port makes both flashing and WiFi flaky.
- Rapid reflash with the bootloader entered via auto-DTR/RTS can desync; a partially written flash leaves a corrupt image that then fails the SHA check from Pitfall 2 — compounding the two failure modes.

**How to avoid:**
- **Hard power-cycle between flash cycles**, not just a soft reset — pull and re-seat USB (or use a switched hub) so RTC/PHY state and the radio fully reset. Make this a literal step in the flash runbook, not an afterthought.
- **Power the board from a quality powered USB port/hub**, not a marginal laptop port, to avoid brownout during WiFi bring-up and during flash writes. A brownout mid-write is how you get a corrupt image that then trips the integrity check.
- **Slow the loop down deliberately:** flash → full power-cycle → wait for WiFi join → test. Batch changes so you reflash less often; iterate logic in the open fork on a *second* board or via the LAN/HTTP API where possible, reserving the real device for validated changes.
- **Always have the verified 4 MB backup + esptool restore command ready** (the project already has this). Treat every flash as potentially needing the restore.
- **Verify the write before trusting it:** `esptool.py verify_flash` (or read-back + checksum against the intended image) after each write, so a partial/corrupt write is caught immediately rather than diagnosed as a WiFi bug three steps later.

**Warning signs:**
- WiFi joins fine after a power-cycle but never after a soft reset → RF state, not firmware.
- Intermittent flash-write failures or brownout/`rst:0x...(BROWNOUT)` in the boot log → power/USB problem.
- "Image is corrupt" appearing only sometimes after flashing → partial writes from an unstable USB/power path (links to Pitfall 2).

**Phase to address:** Bake into the flash/validation runbook used by *every* phase that writes to the device; codify in the project's flashing procedure doc.

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Hand-edit `.bin` bytes and reflash without re-stamping checksum/SHA | Fast first try | Boot loops, "image is corrupt", hours lost re-confirming whether it's the patch or the digest | Never — always re-stamp or rebuild the image |
| Bring up touch on default SPI pins / bundled XPT2046 lib | Less wiring code | "Touch dead" that's actually a bus/lib issue; collides with SD later | Only for a 5-min raw-Z sanity probe, then replace |
| Iterate logic directly on the real device with soft resets | No second board needed | RF/flash fragility produces phantom bugs; risk of corrupt-write brick loop | Only for changes already validated elsewhere |
| Scan firmware for v9-magic LVGL images on a v8 binary | Reuses a generic tool | Zero matches → false "assets are compressed/missing" conclusion (already hit) | Never on this image — it's v8 |
| Treat "no LICENSE file" as permission to redistribute | Simpler mental model | Copyright/EULA exposure; broken activation | Never for redistribution; fine for private personal use |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| XPT2046 touch (VSPI) | Init on display's HSPI / default pins / stale lib | Dedicated `SPIClass(VSPI).begin(25,39,32,33)` + CS33/IRQ36, current XPT2046 lib, calibrate |
| microSD (VSPI) | Bring up on VSPI without resolving touch's VSPI ownership | Move touch to SoftSPI so SD owns hardware VSPI cleanly |
| ESP32 app image | Reflash hand-patched bin as-is | Recompute trailing SHA-256 + last-segment checksum, or rebuild via esptool |
| LVGL asset table | Parse as v9 magic-byte descriptors | Parse v8 12-byte `lv_img_dsc_t`; validate via DROM-pointer + cf + w*h*bpp |
| Stock license verification | Patch near/over license-check code | Treat license path as read-only; place patches elsewhere |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Polling touch on shared VSPI while SD transaction open | Dropped touches / SD read errors | Mutual exclusion or SoftSPI touch | As soon as SD + touch are both active |
| Large extracted assets re-flashed into a too-small free region | Overruns next segment / partition | Map partition table; only write into genuinely free flash | When transplanted asset > available gap |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| Assuming flash encryption is off without checking | Misreading ciphertext as "garbage assets" | `espefuse.py summary` + the fact plaintext strings were already recovered (encryption is off) — document it |
| Disabling/altering license verification | EULA violation, bricked activation | Don't touch the license code path |
| Publishing the modified proprietary image | Copyright/redistribution exposure | Keep patched firmware out of git; personal use only |

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| "Restored" UI with dead touch | Looks stock, feels broken | Gate the UI-restore phase on a passing touch smoke test |
| Speaker beep-on-share with no disconnect alarm | User misses pool drops | Implement both event hooks together (LEDC GPIO26) |
| Branding splice that shifts asset table | Garbled stock screens | Add branding as new assets/trampoline, not by overwriting in-place |

## "Looks Done But Isn't" Checklist

- [ ] **Asset extraction:** Often missing v8-vs-v9 confirmation — verify the extracted `data` pointer lands in `[0x3f400020, +len(drom))` and `data_size == w*h*bpp`.
- [ ] **Patched image:** Often missing the re-stamped SHA-256/checksum — verify with `esptool image_info` reporting valid hash before flashing.
- [ ] **Touch:** Often missing calibration — verify raw Z-pressure changes on press AND on-screen point lands under the finger.
- [ ] **SD + touch together:** Often missing the bus-ownership decision — verify both work *simultaneously*, not just individually.
- [ ] **Flash write:** Often missing read-back verification — verify with `esptool verify_flash` after every write.
- [ ] **Legal:** Often missing the no-redistribution constraint — verify patched image is gitignored and not published.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Corrupt/"image is corrupt" after patch | LOW | Restore the verified 4 MB backup via esptool; re-do patch with digest re-stamped |
| Touch dead | LOW | Probe raw Z first; isolate bus vs calibration; switch touch to SoftSPI |
| WiFi won't join after reflash | LOW | Hard power-cycle (pull USB) to clear RTC/PHY; re-test |
| Secure boot discovered enabled | HIGH | Abandon in-place app patching; pivot to transplant into the open fork |
| Accidentally published modified image | MEDIUM | Remove artifact; rely on personal-use scope; contact maintainer if needed |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| LVGL v8 vs v9 extraction | Asset-recovery phase | Extracted descriptors validate (DROM pointer + cf + w*h*bpp); render matches a known screen |
| Image SHA / patch safety | Integrity-recon phase (before first patch) | `esptool image_info` reports valid hash; eFuse summary confirms no secure boot/encryption |
| Touch + SD VSPI collision | Touch-restoration phase (before SD phase) | Touch + SD both work simultaneously; touch on SoftSPI |
| Closed-binary licensing | Requirements/charter phase | No-redistribution constraint recorded; patched image gitignored |
| RF/flash fragility | Flashing-runbook (all device-write phases) | Hard power-cycle step in runbook; `verify_flash` after each write |

## Sources

- esptool firmware image format (SHA-256 `hash_appended`, checksum, `--dont-append-digest`): https://docs.espressif.com/projects/esptool/en/latest/esp32/advanced-topics/firmware-image-format.html — HIGH
- LVGL v8.3 `lv_img_buf.h` (`lv_img_header_t` bitfield layout, `LV_IMG_CF_*` enum, 12-byte `lv_img_dsc_t`): lvgl/lvgl `release/v8.3` `src/draw/lv_img_buf.h` — HIGH
- LVGL v9 image header magic byte (contrast with v8): https://lvgl.io/docs/open/9.1/API/draw/lv_image_dsc — HIGH
- CYD pinout (display HSPI, touch VSPI on GPIO25/32/33/36/39): https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r/ — HIGH
- CYD display + touch + microSD simultaneous (VSPI collision, SoftSPI fix; SD pins GPIO5/18/19/23): https://randomnerdtutorials.com/esp32-cyd-display-touchscreen-microsd-card/ — HIGH
- CYD dual-SPI touch working example + current XPT2046 lib note: https://github.com/marcelrv/Cheap-Yellow-Display-ESP32-2432S028R — HIGH
- NMMiner repo (binary-only distribution, no LICENSE, per-device activation): https://github.com/NMminer1024/NMMiner — MEDIUM (license terms not explicitly published)
- NMMiner product page (license activation / reflash re-licensing): https://www.nmminer.com/product/nm-cyd/ — MEDIUM
- Project boot-log + foundation facts (DROM 0x3f400020, IROM 0x400d0020, `Clock320x240`, `lvgl_color_buf`, "License verification passes", v8 zero-match, RF fragility): `.planning/PROJECT.md`, `.planning/REQUIREMENTS.md` — HIGH (project-observed)

---
*Pitfalls research for: closed ESP32/Xtensa/LVGL firmware reverse-engineering + patching*
*Researched: 2026-06-17*
