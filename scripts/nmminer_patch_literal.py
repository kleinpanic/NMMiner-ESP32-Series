#!/usr/bin/env python3
"""Patch fixed-length DROM literals in the official NMMiner v2.0.02 app image.

This tool is intentionally narrow. It only patches target records that have
already been mapped in Phase 7 and refreshes the ESP32 image checksum/hash after
the byte edit. It does not flash hardware.
"""

import argparse
import hashlib
import shutil
import struct
import subprocess
import sys
from pathlib import Path


DEFAULT_INPUT = Path("firmware/v2.0.02/esp32-2432s028r-st7789/firmware.bin")
ESP_IMAGE_MAGIC = 0xE9
ESP_CHECKSUM_MAGIC = 0xEF
ESP32_HEADER_LEN = 8 + 16
FACTORY_PARTITION_SIZE = 0x380000

TARGETS = {
    "BIN02-LIT-LOAD-TAGLINE": {
        "app_offset": 0x9912A,
        "current": b"Make it better",
        "max_len": 14,
        "status": "best first literal candidate",
        "patchable": True,
    },
    "BIN02-LIT-LOAD-IP-SEED": {
        "app_offset": 0x9911A,
        "current": b"192.168.124.222",
        "max_len": 15,
        "status": "loading seed; may be runtime-overwritten",
        "patchable": True,
    },
    "BIN02-LIT-GLOBAL-VERSION": {
        "app_offset": 0x8EB,
        "current": b"v2.0.02",
        "max_len": 7,
        "status": "shared/width-sensitive; requires --allow-shared",
        "patchable": True,
        "shared": True,
    },
    "BIN02-LIT-CONFIG-COUNTDOWN-WIDTH": {
        "app_offset": 0x738E2,
        "current": b"300s",
        "max_len": 4,
        "status": "width reference, not initial visible text",
        "patchable": True,
    },
    "BIN02-LIT-SPRICE-COIN-SEED": {
        "app_offset": 0x737D4,
        "current": b"---",
        "max_len": 3,
        "status": "cold-start placeholder only",
        "patchable": True,
    },
    "BIN02-LIT-SPRICE-PRICE-SEED": {
        "app_offset": 0xE41B5,
        "current": b"$0",
        "max_len": 2,
        "status": "cold-start placeholder only",
        "patchable": True,
    },
    "BIN02-LIT-SPRICE-WATCHCOINS-DEFAULT": {
        "app_offset": 0xCDA,
        "current": b"BTC,ETH,BNB,LTC,EOS,ETC,XRP,BCH,ZEC,NEO",
        "max_len": 39,
        "status": "default WatchCoins list; existing NVS/config may override",
        "patchable": True,
    },
    "BIN02-LIT-KHS-HS-SHARED": {
        "app_offset": 0xE42D6,
        "current": b"kH/s",
        "max_len": 4,
        "status": "overlapping shared units; not approved for direct patching",
        "patchable": False,
    },
    "BIN02-SEAM-SHARE-ACCEPT": {
        "app_offset": 0x736C8,
        "current": b"#%d share accepted, %ldms",
        "max_len": 25,
        "status": "seam format; caller and args must be decompiled first",
        "patchable": False,
    },
}


class PatchError(Exception):
    pass


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--input",
        type=Path,
        default=DEFAULT_INPUT,
        help=f"source app image, default: {DEFAULT_INPUT}",
    )
    parser.add_argument("--output", type=Path, help="patched app image output path")
    parser.add_argument("--target-id", choices=sorted(TARGETS), help="target record id")
    parser.add_argument("--replacement", help="ASCII replacement text")
    parser.add_argument(
        "--allow-shared",
        action="store_true",
        help="allow targets marked shared/width-sensitive",
    )
    parser.add_argument(
        "--list-targets",
        action="store_true",
        help="print known target records and exit",
    )
    parser.add_argument(
        "--validate",
        action="store_true",
        help="run python3 -m esptool image-info on the output after writing",
    )
    return parser.parse_args()


def list_targets():
    for target_id, target in TARGETS.items():
        flag = "patchable" if target["patchable"] else "blocked"
        print(
            f"{target_id}: offset=0x{target['app_offset']:x} "
            f"max={target['max_len']} current={target['current']!r} "
            f"{flag}; {target['status']}"
        )


def checksum_offset(pos):
    return pos + (15 - (pos % 16))


def refresh_esp32_checksum_and_hash(data):
    if not data or data[0] != ESP_IMAGE_MAGIC:
        raise PatchError("not an ESP32 app image: bad magic")

    segment_count = data[1]
    pos = ESP32_HEADER_LEN
    checksum = ESP_CHECKSUM_MAGIC

    for index in range(segment_count):
        if pos + 8 > len(data):
            raise PatchError(f"segment {index} header exceeds image length")
        _load_addr, size = struct.unpack_from("<II", data, pos)
        payload = pos + 8
        end = payload + size
        if end > len(data):
            raise PatchError(f"segment {index} payload exceeds image length")
        for byte in data[payload:end]:
            checksum ^= byte
        pos = end

    footer = checksum_offset(pos)
    digest_start = footer + 1
    digest_end = digest_start + 32
    if digest_end > len(data):
        raise PatchError("image footer is too short for checksum/hash")

    data[footer] = checksum
    digest = hashlib.sha256(data[:digest_start]).digest()
    data[digest_start:digest_end] = digest
    return {
        "checksum_offset": footer,
        "checksum": checksum,
        "digest_offset": digest_start,
        "digest": digest.hex(),
    }


def apply_literal_patch(data, target, replacement):
    if "\x00" in replacement:
        raise PatchError("replacement must not contain NUL bytes")
    try:
        replacement_bytes = replacement.encode("ascii")
    except UnicodeEncodeError as exc:
        raise PatchError("replacement must be ASCII") from exc

    max_len = target["max_len"]
    if len(replacement_bytes) > max_len:
        raise PatchError(
            f"replacement is {len(replacement_bytes)} bytes; max is {max_len}"
        )

    offset = target["app_offset"]
    if offset + max_len > FACTORY_PARTITION_SIZE:
        raise PatchError("target exceeds factory partition length")

    expected = target["current"]
    found = bytes(data[offset : offset + len(expected)])
    if found != expected:
        raise PatchError(
            f"target bytes mismatch at 0x{offset:x}: "
            f"expected {expected!r}, found {found!r}"
        )

    data[offset : offset + max_len] = replacement_bytes.ljust(max_len, b"\x00")
    return {
        "app_offset": offset,
        "old": expected.decode("ascii"),
        "new": replacement,
        "max_len": max_len,
    }


def validate_with_esptool(path):
    commands = [[sys.executable, "-m", "esptool", "image-info", str(path)]]
    python3 = shutil.which("python3")
    if python3 and python3 != sys.executable:
        commands.append([python3, "-m", "esptool", "image-info", str(path)])

    result = None
    for cmd in commands:
        result = subprocess.run(cmd, check=False, text=True, capture_output=True)
        if result.returncode == 0:
            return result
    return result


def main():
    args = parse_args()

    if args.list_targets:
        list_targets()
        return 0

    if not args.target_id or args.replacement is None:
        raise PatchError("--target-id and --replacement are required")
    if args.output is None:
        raise PatchError("--output is required")
    if args.input.resolve() == args.output.resolve():
        raise PatchError("output must not overwrite input")

    target = TARGETS[args.target_id]
    if not target["patchable"]:
        raise PatchError(f"{args.target_id} is mapped but not approved for patching")
    if target.get("shared") and not args.allow_shared:
        raise PatchError(f"{args.target_id} is shared; rerun with --allow-shared")

    data = bytearray(args.input.read_bytes())
    patch_info = apply_literal_patch(data, target, args.replacement)
    footer_info = refresh_esp32_checksum_and_hash(data)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)

    print(
        f"patched {args.target_id} at app offset 0x{patch_info['app_offset']:x}: "
        f"{patch_info['old']!r} -> {patch_info['new']!r}"
    )
    print(
        f"checksum 0x{footer_info['checksum']:02x} at "
        f"0x{footer_info['checksum_offset']:x}"
    )
    print(f"validation hash {footer_info['digest']}")
    print(f"wrote {args.output}")

    if args.validate:
        result = validate_with_esptool(args.output)
        print(result.stdout.rstrip())
        if result.returncode != 0:
            print(result.stderr.rstrip(), file=sys.stderr)
            return result.returncode

    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except PatchError as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)
