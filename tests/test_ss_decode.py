#!/usr/bin/env -S ./.venv/bin/python
"""Regression tests for scripts/nmminer_ss_decode.py.

Covers RGB565 conversion and the complete format using independently constructed
public fixtures. An additional local check decodes all 17 recovered stock images;
these private inputs are excluded from the public checkout. Set
PANICMINER_STOCK_SS_DIR to require a particular private fixture directory.
"""
import os
import glob
import sys

import pytest
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
from nmminer_ss_decode import decode_ss, hs_decode, rgb565_to_rgb888  # noqa: E402

LITTLEFS_DIR = os.environ.get("PANICMINER_STOCK_SS_DIR") or os.path.join(
    os.path.dirname(__file__), "..", "hardware", "recovered-assets", "littlefs"
)
SS_GLOB = os.path.join(LITTLEFS_DIR, "ss_saver_320_240_*.ss")


def _literal_stream(data):
    """Encode literal tokens directly from the documented format, MSB first."""
    bits = "".join(f"0{byte:08b}" for byte in data)
    bits += "0" * (-len(bits) % 8)
    return bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))


def test_public_full_size_repeat_image(tmp_path):
    # 600 runs of 128 red pixels cover exactly 320 x 240, including run limits.
    path = tmp_path / "red.ss"
    path.write_bytes(b"SN\x40\x01\xf0\x00\x01\xa4" +
                     _literal_stream(b"\x7f\x00\xf8" * 600))
    image = decode_ss(str(path))
    assert image.size == (320, 240)
    assert image.mode == "RGB"
    assert image.tobytes() == bytes((255, 0, 0)) * (320 * 240)


def test_public_literal_pixels(tmp_path):
    path = tmp_path / "colors.ss"
    # Literal-run marker, then independently specified LE RGB565 color values.
    path.write_bytes(b"SN\x03\x00\x02\x00\x01\xa4" + _literal_stream(
        b"\x85\x00\x00\xff\xff\x00\xf8\xe0\x07\x1f\x00\x01\x08"))
    image = decode_ss(str(path))
    assert image.size == (3, 2)
    expected = [
        (0, 0, 0), (255, 255, 255), (255, 0, 0),
        (0, 255, 0), (0, 0, 255), (8, 0, 8),
    ]
    assert image.tobytes() == bytes(channel for pixel in expected for channel in pixel)


def test_public_overlapping_backreference():
    # Literal A, then offset 0 (previous byte), length 4 (stored as 3).
    bits = "001000001" + "1" + "0000000000" + "0011"
    body = bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
    assert hs_decode(body) == b"AAAAA"


def test_rgb565_endpoints():
    assert rgb565_to_rgb888(0xFFFF) == (255, 255, 255)
    assert rgb565_to_rgb888(0x0000) == (0, 0, 0)


def test_rgb565_bit_replication_not_bare_shift():
    # Pure red max: bit-replication fills the low bits so the channel reaches
    # 255, NOT 248 (which a bare `<<3` would leave).
    assert rgb565_to_rgb888(0xF800)[0] == 255
    # Pure green max and blue max likewise reach 255.
    assert rgb565_to_rgb888(0x07E0)[1] == 255
    assert rgb565_to_rgb888(0x001F)[2] == 255


def test_rgb565_mid_value_replication():
    # 5-bit channel value 1 -> (1<<3)|(1>>2) == 8, not 32.
    r, g, b = rgb565_to_rgb888(0x0800)  # red channel == 1
    assert r == 8
    # blue channel value 1 -> 8.
    r, g, b = rgb565_to_rgb888(0x0001)
    assert b == 8


def test_all_17_ss_decode():
    files = sorted(glob.glob(SS_GLOB))
    if not files and not os.environ.get("PANICMINER_STOCK_SS_DIR"):
        pytest.skip("private stock images absent; public format fixtures run above")
    # A requested or partially present private set must still fail loudly.
    assert len(files) == 17, f"expected 17 .ss fixtures, found {len(files)} via {SS_GLOB}"
    for f in files:
        img = decode_ss(f)
        assert isinstance(img, Image.Image), f"{f} did not decode to a PIL Image"
        assert img.size == (320, 240), f"{f} decoded to {img.size}, expected (320, 240)"
