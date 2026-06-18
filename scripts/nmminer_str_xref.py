#!/usr/bin/env python3
"""NMMiner String Xref Resolver

Resolves ASCII strings in drom.bin to their corresponding l32r literal addresses 
in irom.bin using the established VMA mapping (DROM: 0x3f400020, IROM: 0x400d0020).

Strict Invariant (D-11): This script contains only functional logic and generic 
identifiers. Verified firmware literals (e.g., share-accept format strings) must 
be supplied at runtime via CLI arguments or gitignored extra-strings files.

Usage:
  nmminer_str_xref.py <drom.bin> <irom.bin> <ascii-string>
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

def resolve_string(drom, irom, search_str):
    if isinstance(search_str, str):
        search_bytes = search_str.encode('ascii')
    else:
        search_bytes = search_str

    off = drom.find(search_bytes)
    if off == -1:
        return None

    drom_addr = DROM_VMA + off
    pos = irom.find(struct.pack('<I', drom_addr))

    if pos == -1:
        # Fallback (Pitfall 3, e.g. SPrice): the l32r literal may point one byte
        # past the string start.
        drom_addr += 1
        pos = irom.find(struct.pack('<I', drom_addr))
        if pos == -1:
            return None

    return (drom_addr, IROM_VMA + pos)

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
    if cmd == "--all":
        to_search = CORE_IDENTIFIERS
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
        res = resolve_string(drom, irom, s)
        if res:
            d_addr, i_addr = res
            print(f"{s} -> DROM 0x{d_addr:08x} -> IROM literal 0x{i_addr:08x}")
        else:
            print(f"{s} -> NOT FOUND")

if __name__ == "__main__":
    main()
