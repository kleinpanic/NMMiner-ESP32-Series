#!/usr/bin/env python3
"""Generate NMMiner .ss screensaver files from local images.

The encoder mirrors the official v2.0.02 web UI path:
RGB565 -> RLE body -> HS bit encoding -> 8-byte NS header.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageOps


WIDTH = 320
HEIGHT = 240


def rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def quantize_565(pixel: int, level: int) -> int:
    if level <= 0:
        return pixel
    r = (pixel >> 11) & 0x1F
    g = (pixel >> 5) & 0x3F
    b = pixel & 0x1F
    rb = min(level, 4)
    gb = min(level, 5)
    bb = min(level, 4)
    return ((r >> rb << rb) << 11) | ((g >> gb << gb) << 5) | (b >> bb << bb)


def rle_encode_body(pixels: list[int]) -> bytes:
    parts: list[bytes] = []
    i = 0
    length = len(pixels)
    while i < length:
        cur = pixels[i]
        if i + 1 < length and pixels[i + 1] == cur:
            run = 1
            while i + run < length and pixels[i + run] == cur and run < 128:
                run += 1
            parts.append(bytes((run - 1, cur & 0xFF, (cur >> 8) & 0xFF)))
            i += run
        else:
            lits = [cur]
            i += 1
            while i < length and len(lits) < 128 and (i + 1 >= length or pixels[i] != pixels[i + 1]):
                lits.append(pixels[i])
                i += 1
            out = bytearray([0x80 | (len(lits) - 1)])
            for lit in lits:
                out.extend((lit & 0xFF, (lit >> 8) & 0xFF))
            parts.append(bytes(out))
    return b"".join(parts)


def hs_encode(data: bytes, w_bits: int = 10, l_bits: int = 4) -> bytes:
    w_size = 1 << w_bits
    max_match = 1 << l_bits
    bits: list[int] = []
    pos = 0
    data_len = len(data)

    while pos < data_len:
        best_len = 0
        best_off = 0
        max_len = min(max_match, data_len - pos)
        search_start = max(0, pos - w_size)
        b0 = data[pos]

        for s in range(pos - 1, search_start - 1, -1):
            if data[s] != b0:
                continue
            match_len = 1
            while match_len < max_len and data[s + match_len] == data[pos + match_len]:
                match_len += 1
            if match_len > best_len:
                best_len = match_len
                best_off = pos - s - 1
                if match_len == max_len:
                    break

        if best_len >= 2:
            bits.append(1)
            for bit in range(w_bits - 1, -1, -1):
                bits.append((best_off >> bit) & 1)
            for bit in range(l_bits - 1, -1, -1):
                bits.append(((best_len - 1) >> bit) & 1)
            pos += best_len
        else:
            bits.append(0)
            for bit in range(7, -1, -1):
                bits.append((data[pos] >> bit) & 1)
            pos += 1

    out = bytearray((len(bits) + 7) >> 3)
    for index, bit in enumerate(bits):
        if bit:
            out[index >> 3] |= 0x80 >> (index & 7)
    return bytes(out)


def encode_ss(image: Image.Image, quant_level: int) -> bytes:
    rgba = image.convert("RGBA")
    pixels = []
    for r, g, b, _a in rgba.getdata():
        pixels.append(quantize_565(rgb565(r, g, b), quant_level))
    body = hs_encode(rle_encode_body(pixels), 10, 4)
    header = bytearray()
    header.extend((0x53, 0x4E))
    header.extend((WIDTH & 0xFF, WIDTH >> 8))
    header.extend((HEIGHT & 0xFF, HEIGHT >> 8))
    header.extend((1, 0xA4))
    return bytes(header) + body


def cover_to_screen(image: Image.Image) -> Image.Image:
    image = ImageOps.exif_transpose(image).convert("RGB")
    if image.height > image.width:
        image = image.rotate(90, expand=True)
    return ImageOps.fit(image, (WIDTH, HEIGHT), method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))


def stretch_to_screen(image: Image.Image) -> Image.Image:
    image = ImageOps.exif_transpose(image).convert("RGB")
    if image.height > image.width:
        image = image.rotate(90, expand=True)
    return image.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)


def apply_scanlines(image: Image.Image, alpha: int = 42) -> Image.Image:
    out = image.convert("RGBA")
    overlay = Image.new("RGBA", out.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    for y in range(0, HEIGHT, 3):
        draw.line((0, y, WIDTH, y), fill=(0, 0, 0, alpha))
    return Image.alpha_composite(out, overlay).convert("RGB")


def apply_vignette(image: Image.Image, strength: float = 0.55) -> Image.Image:
    out = image.convert("RGB")
    mask = Image.new("L", out.size, 0)
    center_x = WIDTH / 2
    center_y = HEIGHT / 2
    pix = mask.load()
    max_dist = math.hypot(center_x, center_y)
    for y in range(HEIGHT):
        for x in range(WIDTH):
            dist = math.hypot(x - center_x, y - center_y)
            value = int(255 * min(1.0, (dist / max_dist) ** 1.7 * strength))
            pix[x, y] = value
    dark = Image.new("RGB", out.size, (0, 0, 0))
    return Image.composite(dark, out, mask)


def duotone(image: Image.Image, low: tuple[int, int, int], high: tuple[int, int, int]) -> Image.Image:
    gray = ImageOps.grayscale(image)
    return ImageOps.colorize(gray, black=low, white=high).convert("RGB")


def add_grid(image: Image.Image, color: tuple[int, int, int, int]) -> Image.Image:
    out = image.convert("RGBA")
    overlay = Image.new("RGBA", out.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    for x in range(0, WIDTH, 32):
        draw.line((x, 0, x, HEIGHT), fill=color)
    for y in range(0, HEIGHT, 24):
        draw.line((0, y, WIDTH, y), fill=color)
    return Image.alpha_composite(out, overlay).convert("RGB")


def add_border(image: Image.Image, color: tuple[int, int, int]) -> Image.Image:
    out = image.convert("RGB")
    draw = ImageDraw.Draw(out)
    draw.rectangle((4, 4, WIDTH - 5, HEIGHT - 5), outline=color, width=2)
    draw.rectangle((10, 10, WIDTH - 11, HEIGHT - 11), outline=(32, 42, 30), width=1)
    return out


def variants_for(image: Image.Image) -> list[tuple[str, Image.Image, int]]:
    cover = cover_to_screen(image)
    stretch = stretch_to_screen(image)
    variants: list[tuple[str, Image.Image, int]] = [
        ("cover", cover, 3),
        ("stretch", stretch, 3),
        ("cool", ImageEnhance.Color(cover).enhance(1.25), 3),
        ("dim", ImageEnhance.Brightness(cover).enhance(0.62), 3),
        ("contrast", ImageEnhance.Contrast(cover).enhance(1.45), 3),
        ("poster", ImageOps.posterize(cover, 4), 4),
        ("scan", apply_scanlines(cover), 3),
        ("vignette", apply_vignette(cover), 3),
        ("grid", add_grid(apply_vignette(cover, 0.35), (234, 36, 247, 70)), 3),
        ("border", add_border(apply_scanlines(cover), (234, 36, 247)), 3),
        ("green", duotone(cover, (3, 7, 3), (116, 201, 56)), 4),
        ("magenta", duotone(cover, (7, 3, 8), (234, 36, 247)), 4),
        ("terminal", add_grid(duotone(cover, (2, 4, 2), (116, 201, 56)), (116, 201, 56, 52)), 4),
        ("soft", ImageEnhance.Contrast(cover.filter(ImageFilter.SMOOTH_MORE)).enhance(0.9), 3),
        ("sharp", ImageEnhance.Sharpness(cover).enhance(1.9), 3),
    ]
    return variants


def generate(inputs: list[Path], out_dir: Path, limit: int | None) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    index = 1
    manifest: list[str] = []

    for source in inputs:
        image = Image.open(source)
        stem = source.stem.replace(" ", "_")
        for variant_name, preview, quant in variants_for(image):
            if limit is not None and index > limit:
                break
            png_name = f"saver_320_240_{index:03d}_{stem}_{variant_name}.png"
            ss_name = f"saver_320_240_{index:03d}_{stem}_{variant_name}.ss"
            png_path = out_dir / png_name
            ss_path = out_dir / ss_name
            preview.save(png_path, optimize=True)
            encoded = encode_ss(preview, quant)
            ss_path.write_bytes(encoded)
            manifest.append(f"{ss_name}\t{len(encoded)}\t{png_name}\tquant={quant}\tsource={source}")
            index += 1
        if limit is not None and index > limit:
            break

    (out_dir / "MANIFEST.tsv").write_text("file\tbytes\tpreview\tquant\tsource\n" + "\n".join(manifest) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("images", nargs="+", type=Path)
    parser.add_argument("--out", type=Path, default=Path("hardware/screensavers/generated"))
    parser.add_argument("--limit", type=int, default=None)
    args = parser.parse_args()
    generate(args.images, args.out, args.limit)


if __name__ == "__main__":
    main()
