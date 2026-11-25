#!/usr/bin/env python3
"""NMMiner String Xref Resolver

Resolves ASCII strings in drom.bin to their corresponding l32r literal addresses
in irom.bin using the established VMA mapping (DROM: 0x3f400020, IROM: 0x400d0020).

Strict Invariant (D-11): This script contains only functional logic and generic
identifiers. Verified firmware literals (e.g., share-accept format strings) must
be supplied at runtime via CLI arguments or gitignored extra-strings files.

Usage:
  nmminer_str_xref.py <drom.bin> <irom.bin> <ascii-string>
  nmminer_str_xref.py <drom.bin> <irom.bin> --all-occurrences <ascii-string>
  nmminer_str_xref.py <drom.bin> <irom.bin> --all
  nmminer_str_xref.py <drom.bin> <irom.bin> --extra-from <file>
"""
import sys
import os
import struct

DROM_VMA = 0x3f400020
IROM_VMA = 0x400d0020

# Generic class-name identifiers allowed per D-11
CORE_IDENTIFIERS = [
    "Loading320x240",
    "Miner320x240",
    "Clock320x240",
    "SPrice320x240",
    "Weather320x240",
    "Config320x240",
    "ss_render"
]

def _all_positions(blob, needle):
    out, i = [], blob.find(needle)
    while i != -1:
        out.append(i)
        i = blob.find(needle, i + 1)
    return out


def resolve_string(drom, irom, search_str, all_occurrences=False):
    """Return list of (drom_addr, irom_addr, delta) hits, or [] if none.

    A 4-byte LE literal can legitimately appear in more than one literal pool, and
    an l32r literal may point a few bytes into the string (Pitfall 3, e.g. SPrice).
    Surface ALL hits across a small delta window rather than hiding multiplicity.
    """
    if isinstance(search_str, str):
        search_bytes = search_str.encode('ascii')
    else:
        search_bytes = search_str

    offsets = _all_positions(drom, search_bytes) if all_occurrences else [drom.find(search_bytes)]
    offsets = [off for off in offsets if off != -1]
    if not offsets:
        return []

    hits = []
    for off in offsets:
        for delta in range(0, 4):  # exact, then up to +3 bytes into the string
            drom_addr = DROM_VMA + off + delta
            for pos in _all_positions(irom, struct.pack('<I', drom_addr)):
                hits.append((drom_addr, IROM_VMA + pos, delta))
    return hits

def main():
    if len(sys.argv) < 4:
        print(__doc__)
        sys.exit(1)

    drom_path = sys.argv[1]
    irom_path = sys.argv[2]
    cmd = sys.argv[3]

    if not os.path.exists(drom_path) or not os.path.exists(irom_path):
        print(f"Error: files not found at {drom_path} or {irom_path}")
        sys.exit(1)

    drom = open(drom_path, "rb").read()
    irom = open(irom_path, "rb").read()

    to_search = []
    all_occurrences = False
    if cmd == "--all":
        to_search = CORE_IDENTIFIERS
    elif cmd == "--all-occurrences":
        if len(sys.argv) < 5:
            print("Error: --all-occurrences requires an ASCII string")
            sys.exit(1)
        to_search = [sys.argv[4]]
        all_occurrences = True
    elif cmd == "--extra-from":
        if len(sys.argv) < 5:
            print("Error: --extra-from requires a file path")
            sys.exit(1)
        extra_path = sys.argv[4]
        if os.path.exists(extra_path):
            with open(extra_path, "r") as f:
                to_search = [line.strip() for line in f if line.strip()]
        else:
            print(f"Error: extra strings file not found at {extra_path}")
            sys.exit(1)
    else:
        to_search = [cmd]

    for s in to_search:
        hits = resolve_string(drom, irom, s, all_occurrences=all_occurrences)
        if not hits:
            print(f"{s} -> NOT FOUND")
            continue
        d_addr, i_addr, delta = hits[0]
        note = "" if delta == 0 else f" (+{delta}-byte offset)"
        print(f"{s} -> DROM 0x{d_addr:08x} -> IROM literal 0x{i_addr:08x}{note}")
        if len(hits) > 1:
            extra = ", ".join(f"0x{i:08x}" for _, i, _ in hits[1:])
            print(f"  WARNING: {len(hits)} literal hits (ambiguous) — others: {extra}")

if __name__ == "__main__":
    main()
