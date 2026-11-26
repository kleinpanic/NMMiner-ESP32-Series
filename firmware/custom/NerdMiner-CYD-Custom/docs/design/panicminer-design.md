# PanicMiner display design

This is the design of the 320x240 panel on the ESP32-2432S028R. The source is `src/ui/`.
`make -C tools/uisim THEME=1` renders every page to `tools/uisim/out-t1/`.

## Idea

The pages show the block being mined as a physical thing. On the Mine page, the block the
unit is hashing is an isometric cube. A scan line sweeps down it once per 2^22 nonces, and
the hashes being tried drift up behind it as faint hex. Under it, a strip of blocks shows
the next three pending blocks by fee, a dashed line, and the newest blocks mined.

The other signature element is the best hash so far. It sits on the Hash page with its leading
zeros lit, and a rule under it runs to the zero count a block needs. A lottery miner's whole
story is that distance: how close the best hash came, and how far a block still is. A live
line under it shows the newest hash past the 16-bit prefilter. This tells you the miner is
working on real hashes. The device verifies each of these hashes in software before it
shows it.

Every page uses the same frame. A header shows context on the left, and a glowing status
dot with the miner state on the right. Content splits at x 160, with the right column at
x 170. Wide 40 px numerals and a small scope match Mine's hashrate. Hash, Chain and Clock
keep the same four-stat footer at y 200. Boot and Setup use the same header and columns.

## Themes

`PM_THEME` selects the theme at build time. `1` is the default.

| Token | 1 Afterglow | 2 Hazard | Use |
|---|---|---|---|
| bg top | `#230B30` | `#000000` | top of the vertical background gradient, which fades to black |
| accent | `#FF3D7F` | `#FFD21F` | status dot, charts, lit zeros, track fills, setup QR tile |
| accent 2 | `#FFA24A` | `#FFFFFF` | the block line, zeros in the live hash, a negative retarget |
| hero | `#F7F3F8` | `#FFD21F` | the hero number |
| dim | `#8C8293` | `#8A8D90` | captions |
| faint | `#3B3342` | `#34363A` | unlit hash digits, unfilled tracks, old boot lines |
| alarm | `#FF3B2F` | `#FF3B2F` | dot, charts and unit when there is no pool; Hazard also turns the tape red |

Hazard adds a strip of diagonal warning tape, 8 px tall, across the top of every page.

## Type

All fonts are SIL OFL 1.1. The licences are in `src/ui/fonts/licenses/`. To convert them,
run `tools/fonts/build_fonts.sh`.

| Font | Face | Role |
|---|---|---|
| `pm_wide_32` | Unbounded 600, 32 px, `panicmer`, digits and `:` | Clock HH:MM, fits even at 00:00 |
| `pm_wide_40` | Unbounded 600, 40 px, digits and `.,:$-` | Mine/Hash hashrate, best proof, Chain price, seconds |
| `pm_wide_14` | Unbounded 500, 14 px, `kMGH/s` | hashrate unit |
| `pm_disp_40` | Big Shoulders Display 800, 40 px | difficulty change |
| `pm_mono_22` | JetBrains Mono 700, 22 px, hex digits | the best hash |
| `pm_text_20` | Barlow Semi Condensed 600, 20 px | setup values, stat values, boot wordmark |
| `pm_text_14` | Barlow Semi Condensed 500, 14 px | header, captions, key and value rows |
| `pm_text_11` | Barlow Semi Condensed 500, 11 px | block strip, price units, small captions |
| `pm_mono_12` | JetBrains Mono 400, 12 px | the live hash, the boot log, the hash stream |

## Layout (Afterglow)

Hash, Chain and Clock share this compact frame in both themes:

```
y 10   context                          * state     header
y 28   left caption                 right caption
y 44   proof / retarget / time      rate / price / seconds
y 88   caption or track             small scope
y 110  secondary content across the page
y 200  value     value     value     value          stats at x 12 / 92 / 176 / 248
       caption   caption   caption   caption
```

The Mine page splits at x 160:

```
y 10   pool                             * state     header
y 34   /\ cube, hash stream behind  |  hashrate     right column at x 170
       ||                            |  991 kH/s    rolling digits
y 88   ||                            |  ~~scope~~*  last 60 s, dot at now
y 114  \/                            |  best hash   6 of 20 zeros
y 134  hashing 977,565              |  [######--------------]
y 147                                |  shares blocks / best up
y 182  [] [] [] :  [] [] []                         3 pending by fee : 3 newest mined
y 216  fee  fee  :  height  height  height
```

To cycle the pages Mine, Hash, Chain and Clock, press BOOT.

- **Mine**: the block being hashed as a cube, with the hash stream behind it and its height
  under it. The right column shows the hashrate, a scope of its last 60 s, the best hash's
  zeros against the zeros a block needs, and shares, blocks, best share and uptime. The
  strip at the foot shows the fast, hour and economy fees on three pending blocks, then the
  three newest blocks.
- **Hash**: the left column shows the best proof's zero count against the block target,
  with a segmented track. The right column repeats Mine's hashrate and 60-second scope.
  The full best hash is at y 160, with its leading zeros lit and a target rule below it.
  The live line at y 182 shows a new verified hash up to 8 times a second. The stats row
  shows shares, best share, blocks and uptime.
- **Chain**: the left column shows the next difficulty change and blocks left. The right
  column shows a compact price in thousands (`k USD`) or millions (`M USD`) with three
  significant digits, plus a chart of the last 60 price updates. Under them is the period
  progress. The 2016 blocks of the period are a grid of 96 x 21 dots: mined dots glow
  brighter toward the newest, and the next block blinks. The stats row shows difficulty,
  network hashrate, and fast and slow fees. Until mempool.space answers, the page shows
  `-`. If the feed is off, the page says so.
- **Clock**: local time in the left column, seconds in the right, with the hash stream behind HH:MM.
  Under it, the minute fills a track. Two rows show the pool, Wi-Fi signal and chip
  temperature. The stats row shows hashrate, shares, best share and uptime.
- **Boot**: a cube fills on the left as boot steps finish, with the wordmark and progress
  track on the right. The six newest startup lines appear below in mono, newest bright.
- **Setup**: a Wi-Fi QR code on a glowing accent tile, beside the three steps to join the
  configuration network. A setup status dot shares the header; a scan caption and divider
  separate the QR tile from the footer instructions. Reopening Setup loads the replacement
  before deleting the old screen so LVGL always has an active screen.

## Motion

One LVGL timer runs every 66 ms. It redraws only what moves on the page on screen:

- Mine: the cube and the hash stream every frame.
- Hash and Clock: the hash stream every other frame.
- Chain: the blinking next-block dot.

Everything else changes once a second, and only when it changes.

| Moment | What moves |
|---|---|
| a value changes | its digits roll to the new value (600 ms); the first value rolls up from zeros |
| a share is accepted | a pink toast on Mine and Hash (2.4 s), the cube glows (1.4 s), the shares values flash |
| a new block | an orange toast on Chain, the strip slides the block across the divider (900 ms), a ripple on its dot (1 s) |

`src/ui/pm_px.cpp` draws the cubes, the hash stream and the dot grid straight into the LVGL
draw buffer: no images are stored. `src/ui/pm_odo.cpp` has the rolling digits, the toast
and the colour flash.
