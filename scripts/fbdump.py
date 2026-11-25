#!/usr/bin/env python3
"""Grab what the PanicMiner firmware is drawing, as PNGs.

Sends PMDUMP over serial and decodes the PMFB lines from the firmware's
LVGL flush into an image. With --pages N it dumps N pages, sending PMNEXT
(the same path as a BOOT press) between them. Opening the port resets the
board; the script waits for the miner loop, then --settle seconds.
Usage: .venv/bin/python scripts/fbdump.py out.png [--pages 3] [--settle 60]
Pages after the first go to out-1.png, out-2.png.
"""
import argparse
import sys
import time
from pathlib import Path

import serial
from PIL import Image


def rgb565(v: int) -> tuple[int, int, int]:
    r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
    return (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)


def dump(ser: serial.Serial, deadline: float) -> tuple[Image.Image | None, int]:
    ser.reset_input_buffer()
    ser.write(b"\nPMDUMP\n")   # the leading newline flushes boot noise from the command buffer
    img, rows = None, 0
    while time.time() < deadline:
        line = ser.readline().decode("ascii", "replace").strip()
        if not line.startswith("PMFB "):
            continue
        parts = line.split(" ")
        if parts[1] == "BEGIN":
            img = Image.new("RGB", (int(parts[2]), int(parts[3])))
        elif parts[1] == "END":
            break
        elif img is not None and len(parts) == 5:
            y, x0, w, hexs = int(parts[1]), int(parts[2]), int(parts[3]), parts[4]
            if len(hexs) != w * 4:
                continue               # a byte was lost on the wire; skip the row
            for i in range(w):
                img.putpixel((x0 + i, y), rgb565(int(hexs[i * 4:i * 4 + 4], 16)))
            rows += 1
    return img, rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--pages", type=int, default=1)
    ap.add_argument("--settle", type=float, default=0.0, help="seconds to mine before the first dump")
    ap.add_argument("--timeout", type=float, default=90.0, help="seconds allowed per wait")
    args = ap.parse_args()

    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = args.port, 115200, 1
    ser.dtr = ser.rts = False
    ser.open()
    # Opening the port resets this CH340 board even with DTR/RTS low. Wait for
    # the Monitor task's once-a-second line, so the command is not eaten by
    # the boot-time serial config prompt.
    deadline = time.time() + args.timeout
    while time.time() < deadline:
        if ser.readline().startswith(b">>> Completed"):
            break
    time.sleep(args.settle)

    out = Path(args.out)
    status = 0
    for page in range(args.pages):
        if page:
            ser.write(b"PMNEXT\n")
            time.sleep(1.5)            # the page switch lands on the next frame
        img, rows = dump(ser, time.time() + args.timeout)
        path = out if page == 0 else out.with_name(f"{out.stem}-{page}{out.suffix}")
        if img is None:
            print(f"page {page}: no PMFB BEGIN seen; is the PanicMiner build running?", file=sys.stderr)
            status = 1
            continue
        img.save(path)
        print(f"{path}: {rows} row segments")
    ser.close()
    return status


if __name__ == "__main__":
    sys.exit(main())
