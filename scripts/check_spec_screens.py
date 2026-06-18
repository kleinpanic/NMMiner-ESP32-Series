#!/usr/bin/env python3
"""Strengthened smoke check for the recovered NMMiner UI design contract (SPEC.md).

Asserts the SPEC is evidence-bearing, not just section headers (RE-03):
  - all 6 screen class identifiers are present, each with a 0x400... IROM address
  - the Miner and Clock geometry tables each have >= MIN_WIDGET_ROWS widget rows
  - the Share-Accept seam and ss_render entries each carry an address + a call-shape
  - the LVGL version section cites at least two distinct acceptable-evidence items
  - every required section/table carries a `Confidence` column header

Identifier-only (D-11): contains screen CLASS-NAME identifiers + structural checks;
NO verbatim closed-firmware literals. Operates on the gitignored SPEC.md.

Usage: check_spec_screens.py [path/to/SPEC.md]
       (default: hardware/recovered-assets/SPEC.md)
Exit 0 = pass; non-zero with the failing assertion = fail.
"""
import sys, re

SCREENS = ["Loading320x240", "Miner320x240", "Clock320x240",
           "SPrice320x240", "Weather320x240", "Config320x240"]
MIN_WIDGET_ROWS = 5
ADDR_RE = re.compile(r"0x4[0-9a-fA-F]{6,7}")


def section(text, title_substr):
    """Return the markdown section whose heading contains title_substr (case-insensitive)."""
    m = re.search(r"(?ims)^#+[ \t]*[^\n]*" + re.escape(title_substr) +
                  r"[^\n]*$.*?(?=^#+[ \t]|\Z)", text)
    return m.group(0) if m else None


def widget_row_count(sec):
    # count markdown table data rows (lines starting with '|' that are not the
    # header or the |---| separator)
    n = 0
    for ln in sec.splitlines():
        s = ln.strip()
        if not s.startswith("|"):
            continue
        if re.match(r"^\|[\s:|-]+\|?$", s):  # separator
            continue
        if re.search(r"(?i)\bslot\b|\bpurpose\b|\bwidget\b", s) and "x" in s.lower() and "y" in s.lower():
            continue  # header row
        n += 1
    return n


def fail(msg):
    print("FAIL: " + msg)
    sys.exit(1)


def main(argv):
    path = argv[1] if len(argv) > 1 else "hardware/recovered-assets/SPEC.md"
    try:
        t = open(path).read()
    except OSError as e:
        fail("cannot read SPEC at %s (%s)" % (path, e))

    # 1. all 6 screens present
    for s in SCREENS:
        if s not in t:
            fail("screen class %s missing from SPEC" % s)

    # 2. each screen section has a 0x400 address
    for s in SCREENS:
        sec = section(t, s) or ""
        if not ADDR_RE.search(sec):
            fail("screen %s section has no 0x400... IROM address" % s)

    # 3. Miner + Clock geometry tables have >= MIN_WIDGET_ROWS rows, with Confidence column
    for s in ("Miner320x240", "Clock320x240"):
        sec = section(t, s)
        if sec is None:
            fail("no section for %s" % s)
        rows = widget_row_count(sec)
        if rows < MIN_WIDGET_ROWS:
            fail("%s geometry table has %d widget rows (< %d)" % (s, rows, MIN_WIDGET_ROWS))
        if "confidence" not in sec.lower():
            fail("%s section lacks a Confidence column" % s)

    # 4. Share-Accept seam + ss_render rows carry address + call-shape
    for key in ("Share-Accept", "ss_render"):
        sec = section(t, key)
        if sec is None:
            fail("no %s section in SPEC" % key)
        if not ADDR_RE.search(sec):
            fail("%s section has no address" % key)
        if not re.search(r"(?i)call[- ]?shape|signature|args?\b|\(.*\)", sec):
            fail("%s section has no call-shape/signature" % key)

    # 5. LVGL version section cites >= 2 distinct evidence items + Confidence
    lv = section(t, "LVGL")
    if lv is None:
        fail("no LVGL version section")
    if not re.search(r"(?i)\bv8\b|\bv9\b", lv):
        fail("LVGL section states no v8/v9 verdict")
    items = set(re.findall(r"(?i)item\s*#?\s*([0-9])", lv))
    if len(items) < 2:
        fail("LVGL section cites < 2 distinct acceptable-evidence items: %s" % sorted(items))
    if "confidence" not in lv.lower():
        fail("LVGL section lacks a Confidence label")

    print("OK: SPEC.md passes strengthened screen/evidence checks "
          "(6 screens+addrs, Miner/Clock>=%d rows, seam+ss_render addr+shape, LVGL 2-evidence, Confidence)" % MIN_WIDGET_ROWS)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
