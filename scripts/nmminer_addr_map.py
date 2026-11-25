#!/usr/bin/env python3
"""Map NMMiner v2.0.02 ESP32 app VM addresses to file offsets.

This helper is intentionally narrow: it documents the verified segment layout for
the official esp32-2432s028r-st7789 v2.0.02 app image used by this repo.
"""

import argparse
import sys


APP_FLASH_BASE = 0x10000
FACTORY_PARTITION_SIZE = 0x380000

SEGMENTS = (
    {
        "name": "DROM",
        "load": 0x3F400020,
        "length": 0x157DE8,
        "segment_header_offset": 0x000018,
        "payload_offset": 0x000020,
    },
    {
        "name": "DRAM",
        "load": 0x3FFBDB60,
        "length": 0x0065A0,
        "segment_header_offset": 0x157E08,
        "payload_offset": 0x157E10,
    },
    {
        "name": "IRAM0",
        "load": 0x40080000,
        "length": 0x001C60,
        "segment_header_offset": 0x15E3B0,
        "payload_offset": 0x15E3B8,
    },
    {
        "name": "IROM",
        "load": 0x400D0020,
        "length": 0x13227C,
        "segment_header_offset": 0x160018,
        "payload_offset": 0x160020,
    },
    {
        "name": "IRAM1",
        "load": 0x40081C60,
        "length": 0x017FE4,
        "segment_header_offset": 0x29229C,
        "payload_offset": 0x2922A4,
    },
)


def parse_int(value):
    try:
        return int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"not an integer address: {value}") from exc


def map_vaddr(vaddr):
    for seg in SEGMENTS:
        rel = vaddr - seg["load"]
        if 0 <= rel < seg["length"]:
            app_off = seg["payload_offset"] + rel
            flash_off = APP_FLASH_BASE + app_off
            return {
                "segment": seg["name"],
                "vaddr": vaddr,
                "relative": rel,
                "app_offset": app_off,
                "flash_offset": flash_off,
                "in_factory": app_off < FACTORY_PARTITION_SIZE,
            }
    return None


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("addresses", nargs="+", type=parse_int)
    args = parser.parse_args(argv)

    ok = True
    print("segment,vaddr,relative,app_offset,flash_offset,in_factory")
    for vaddr in args.addresses:
        row = map_vaddr(vaddr)
        if row is None:
            ok = False
            print(f"UNKNOWN,0x{vaddr:08x},,,,,", file=sys.stderr)
            continue
        print(
            "{segment},0x{vaddr:08x},0x{relative:x},0x{app_offset:x},"
            "0x{flash_offset:x},{in_factory}".format(**row)
        )
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
