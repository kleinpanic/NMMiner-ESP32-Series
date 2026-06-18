#!/usr/bin/env python3
"""Decode NMMiner `.ss` screensaver files back to images.

Reverses scripts/nmminer_screensaver.py (the encoder): an `.ss` is an 8-byte
header `"SN" + width(LE) + height(LE) + flags(0x01,0xA4)` followed by a body of
RGB565 pixels packed as RLE then an LZSS bit-stream (10-bit offset, 4-bit len).

Usage: nmminer_ss_decode.py <file.ss|dir> [out_dir]
"""
import sys, os, glob
from PIL import Image

MAGIC = (0x53, 0x4E)  # "SN"


def rgb565_to_rgb888(v: int) -> tuple[int, int, int]:
    # Bit-replication, not a bare left-shift: shifting a 5-bit channel up by 3
    # leaves the low 3 bits zero, so the max value (31<<3 == 248) never reaches
    # 255 -> high values read too dark / the image looks over-saturated (D-26c,
    # user-observed). Replicating the high bits down into the gap fills them so
    # 31 -> 255 and intermediate values scale evenly.
    r5 = (v >> 11) & 0x1F
    g6 = (v >> 5) & 0x3F
    b5 = v & 0x1F
    r = (r5 << 3) | (r5 >> 2)
    g = (g6 << 2) | (g6 >> 4)
    b = (b5 << 3) | (b5 >> 2)
    return (r, g, b)


def hs_decode(body: bytes, w_bits: int = 10, l_bits: int = 4) -> bytes:
    bits = []
    for byte in body:
        for i in range(7, -1, -1):
            bits.append((byte >> i) & 1)
    out = bytearray()
    p, n = 0, len(bits)

    def rd(k: int) -> int:
        nonlocal p
        v = 0
        for _ in range(k):
            v = (v << 1) | bits[p]
            p += 1
        return v

    while p < n:
        flag = bits[p]; p += 1
        if flag == 1:
            if p + w_bits + l_bits > n:
                break
            off = rd(w_bits)
            length = rd(l_bits) + 1
            start = len(out) - off - 1
            if start < 0:
                break
            for k in range(length):
                out.append(out[start + k])
        else:
            if p + 8 > n:
                break
            out.append(rd(8))
    return bytes(out)


def rle_decode(data: bytes, npix: int) -> list:
    px, i = [], 0
    while i < len(data) and len(px) < npix:
        b = data[i]; i += 1
        if b & 0x80:                       # literal run
            cnt = (b & 0x7F) + 1
            for _ in range(cnt):
                if i + 1 >= len(data):
                    break
                px.append(data[i] | (data[i + 1] << 8)); i += 2
        else:                              # repeat run
            run = b + 1
            if i + 1 >= len(data):
                break
            v = data[i] | (data[i + 1] << 8); i += 2
            px.extend([v] * run)
    return px[:npix]


def decode_ss(path: str) -> Image.Image:
    d = open(path, "rb").read()
    if (d[0], d[1]) != MAGIC:
        raise ValueError(f"bad magic {d[0]:#x}{d[1]:#x} (expected 'SN')")
    w = d[2] | (d[3] << 8)
    h = d[4] | (d[5] << 8)
    px = rle_decode(hs_decode(d[8:]), w * h)
    px += [0] * (w * h - len(px))
    img = bytearray(w * h * 3)
    for idx, v in enumerate(px):
        r, g, b = rgb565_to_rgb888(v)
        img[idx * 3] = r
        img[idx * 3 + 1] = g
        img[idx * 3 + 2] = b
    return Image.frombytes("RGB", (w, h), bytes(img))


def main():
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(1)
    target = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(out_dir, exist_ok=True)
    files = sorted(glob.glob(os.path.join(target, "*.ss"))) if os.path.isdir(target) else [target]
    for f in files:
        out = os.path.join(out_dir, os.path.splitext(os.path.basename(f))[0] + ".png")
        decode_ss(f).save(out)
        print(f"{f} -> {out}")


if __name__ == "__main__":
    main()
