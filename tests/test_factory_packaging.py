"""Exercise real packaging code without SCons, serial access or device data."""
from pathlib import Path
import runpy
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "firmware/custom/NerdMiner-CYD-Custom/post_build_merge.py"


class Environment(dict):
    def subst(self, value):
        for key, replacement in self.items():
            value = value.replace("$" + key, str(replacement))
        return value

    def AddPostAction(self, target, callback):
        self.callback = callback


class FactoryPackagingTests(unittest.TestCase):
    def setUp(self):
        private = ROOT / "logs/host-tests"
        private.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=private)
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.env = Environment(PROJECT_DIR=self.root, BUILD_DIR=self.root / "build",
                               PIOENV="fixture", ESP32_APP_OFFSET="0x10000")
        self.module = runpy.run_path(str(SCRIPT), init_globals={"env": self.env, "Import": lambda _: None})
        self.module["get_firmware_version"].__globals__["get_firmware_version"] = lambda: "fixture"

    def image(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def test_uses_sdk_boot_app0_and_exact_upload_offsets(self):
        images = [("0x1000", self.image("sdk/bootloader.bin", b"BOOT")),
                  ("0x8000", self.image("build/partitions.bin", b"PART")),
                  ("0xe000", self.image("sdk/tools/partitions/boot_app0.bin", b"OTA0"))]
        self.env["FLASH_EXTRA_IMAGES"] = images
        self.image("build/firmware.bin", b"APPLICATION")
        self.env.callback(None, None, self.env)
        output = self.root / "firmware/fixture"
        merged = (output / "fixture_factory.bin").read_bytes()
        for offset, path in images:
            self.assertEqual(merged[int(offset, 0):int(offset, 0) + path.stat().st_size], path.read_bytes())
        self.assertEqual(merged[0x10000:0x1000b], b"APPLICATION")
        self.assertEqual(merged[0x9000:0xa000], b"\xff" * 4096)
        self.assertEqual(len(merged) % 4096, 0)
        self.assertEqual((output / "fixture_firmware.bin").read_bytes(), b"APPLICATION")

    def test_missing_required_input_refuses_both_outputs(self):
        for missing in ("bootloader", "boot_app0"):
            with self.subTest(missing=missing):
                self.env["FLASH_EXTRA_IMAGES"] = [("0xe000", self.root / missing)]
                self.image("build/firmware.bin", b"APPLICATION")
                with self.assertRaises(FileNotFoundError):
                    self.env.callback(None, None, self.env)
                self.assertFalse((self.root / "firmware").exists())

    def test_empty_manifest_refuses_app_only_factory(self):
        with self.assertRaises(ValueError):
            self.env.callback(None, None, self.env)

    def test_overlap_range_and_empty_images_are_rejected(self):
        first = self.image("first.bin", b"1234")
        empty = self.image("empty.bin", b"")
        for entries in ([(0, first), (3, first)], [(-1, first)], [(0x3fffff, first)],
                        [(0, empty)], []):
            with self.subTest(entries=entries):
                with self.assertRaises(ValueError):
                    self.module["merge_images"](entries)

    def test_other_app_offset_is_preserved(self):
        self.env["FLASH_EXTRA_IMAGES"] = [("0x0", self.image("sdk/boot.bin", b"BOOT"))]
        self.env["ESP32_APP_OFFSET"] = "0x20000"
        self.image("build/firmware.bin", b"APPLICATION")
        self.env.callback(None, None, self.env)
        data = (self.root / "firmware/fixture/fixture_factory.bin").read_bytes()
        self.assertEqual(data[0x20000:0x2000b], b"APPLICATION")
