#!/usr/bin/env bash
# Regenerate the PanicMiner LVGL fonts in src/ui/fonts/.
# Sources: Fontsource 5.3.0 packages (SIL OFL 1.1), cached in tools/fonts/.cache.
set -euo pipefail
cd "$(dirname "$0")"
CACHE=.cache
OUT=../../src/ui/fonts
CDN=https://cdn.jsdelivr.net/npm/@fontsource
CONV=(npx --yes lv_font_conv@1.5.3)
mkdir -p "$CACHE" "$OUT"

fetch() {  # family weight
  local f="$1-latin-$2-normal.woff"
  [[ -f "$CACHE/$f" ]] || curl -fsSL -o "$CACHE/$f" "$CDN/$1@5.3.0/files/$f"
  echo "$CACHE/$f"
}

gen() {  # name family weight size range-args...
  local name=$1 fam=$2 w=$3 size=$4; shift 4
  "${CONV[@]}" --font "$(fetch "$fam" "$w")" "$@" --size "$size" --bpp 4 \
    --format lvgl --no-compress --lv-include lvgl.h -o "$OUT/$name.c"
}

ASCII=(-r 0x20-0x7E)
gen pm_hero_96  big-shoulders-display 800 96 --symbols '0123456789.,:$#%+- '
gen pm_disp_40  big-shoulders-display 800 40 "${ASCII[@]}"
gen pm_text_20  barlow-semi-condensed 600 20 "${ASCII[@]}" -r 0xB0
gen pm_text_14  barlow-semi-condensed 500 14 "${ASCII[@]}" -r 0xB0
gen pm_mono_12  jetbrains-mono        400 12 "${ASCII[@]}"
gen pm_mono_22  jetbrains-mono        700 22 --symbols '0123456789abcdef.-> '
gen pm_wide_64  unbounded             600 64 --symbols '0123456789.,:$-'
gen pm_wide_18  unbounded             500 18 "${ASCII[@]}"
gen pm_wide_32  unbounded             600 32 --symbols 'panicmer0123456789:'
gen pm_wide_40  unbounded             600 40 --symbols '0123456789.,:$-'
gen pm_wide_14  unbounded             500 14 --symbols 'kMGH/s'
gen pm_text_11  barlow-semi-condensed 500 11 "${ASCII[@]}"
ls -l "$OUT"
