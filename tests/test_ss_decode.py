#!/usr/bin/env python3
"""Regression tests for scripts/nmminer_ss_decode.py.

Covers (D-26c) the RGB565->RGB888 bit-replication fix and (D-26a) that all 17
stock `.ss` screensavers decode to 320x240 without raising. The `.ss` inputs are
gitignored fixtures present on this machine (hardware/recovered-assets/littlefs/);
a missing fixture set FAILS loudly rather than silently skipping (threat T-02-03).
"""
import os
import glob
import sys

from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))
from nmminer_ss_decode import decode_ss, rgb565_to_rgb888  # noqa: E402

LITTLEFS_DIR = os.path.join(
    os.path.dirname(__file__), "..", "hardware", "recovered-assets", "littlefs"
)
SS_GLOB = os.path.join(LITTLEFS_DIR, "ss_saver_320_240_*.ss")


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
    # Fail loudly if fixtures are absent (do not silently pass on 0 matches).
    assert len(files) == 17, f"expected 17 .ss fixtures, found {len(files)} via {SS_GLOB}"
    for f in files:
        img = decode_ss(f)
        assert isinstance(img, Image.Image), f"{f} did not decode to a PIL Image"
        assert img.size == (320, 240), f"{f} decoded to {img.size}, expected (320, 240)"
