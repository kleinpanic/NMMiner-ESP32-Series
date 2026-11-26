#!/usr/bin/env python3
"""
Post-build script for NerdMiner_v2 firmware merging
Uses PlatformIO upload offsets and SDK images; refuses incomplete factory images.
Generates factory (0x0) and update (0x10000) files automatically after build

Usage: Add to platformio.ini environments:
extra_scripts =
    pre:auto_firmware_version.py
    post:post_build_merge.py
"""

import subprocess
from pathlib import Path

Import("env")

def get_firmware_version():
    """Get firmware version from git"""
    try:
        result = subprocess.run(["git", "describe", "--tags", "--dirty"], 
                              stdout=subprocess.PIPE, text=True, 
                              cwd=env.subst("$PROJECT_DIR"))
        if result.returncode == 0:
            version = result.stdout.strip()
            # Clean up version string
            version = version.replace('Release', '').replace('release', '')
            return version
    except:
        pass
    return "dev"

def merge_images(images, capacity=0x400000):
    """Read every required image and reject incomplete, overlapping layouts."""
    result = bytearray(b"\xff" * capacity)
    end = 0
    for address, path in sorted(images, key=lambda entry: entry[0]):
        data = Path(path).read_bytes()
        if not data:
            raise ValueError(f"Empty flash image: {path}")
        if address < end or address < 0 or address + len(data) > capacity:
            raise ValueError(f"Overlapping or out-of-range flash image: {path}")
        result[address:address + len(data)] = data
        end = address + len(data)
    if not end:
        raise ValueError("No flash images supplied")
    return result[:((end + 4095) // 4096) * 4096]


def create_merged_firmware(source, target, env):
    """Package the exact extra images and offsets selected by PlatformIO."""
    project_dir = Path(env.subst("$PROJECT_DIR"))
    build_dir = Path(env.subst("$BUILD_DIR"))
    env_name = env.subst("$PIOENV")
    firmware_file = build_dir / "firmware.bin"
    # Arduino's uploader supplies the SDK boot_app0 path here. It is not
    # normally copied to BUILD_DIR. Reuse that resolved upload manifest rather
    # than guessing a chip from bootloader size or silently omitting an image.
    extras = env.get("FLASH_EXTRA_IMAGES", [])
    if not extras:
        raise ValueError("PlatformIO FLASH_EXTRA_IMAGES is empty; factory image refused")
    images = [(int(env.subst(str(address)), 0), Path(env.subst(str(path))))
              for address, path in extras]
    app_address = int(env.subst(str(env.get("ESP32_APP_OFFSET", "0x10000"))), 0)
    images.append((app_address, firmware_file))
    merged = merge_images(images)
    version_dir = project_dir / "firmware" / get_firmware_version()
    version_dir.mkdir(parents=True, exist_ok=True)
    factory_file = version_dir / f"{env_name}_factory.bin"
    update_file = version_dir / f"{env_name}_firmware.bin"
    # Validate the complete layout before writing either distributable.
    for output, data in ((factory_file, merged), (update_file, firmware_file.read_bytes())):
        pending = output.with_suffix(".pending")
        pending.write_bytes(data)
        pending.replace(output)
    print(f"Factory: {factory_file.name} ({len(merged)} bytes; {len(images)} required inputs)")
    print(f"Firmware: {update_file.name}")


env.AddPostAction("$BUILD_DIR/firmware.bin", create_merged_firmware)
