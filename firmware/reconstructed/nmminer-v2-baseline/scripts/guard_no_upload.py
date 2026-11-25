import os

from SCons.Script import COMMAND_LINE_TARGETS


BLOCKED_TARGETS = {
    "erase",
    "erase_flash",
    "program",
    "upload",
    "uploadfs",
    "uploadfsota",
}

requested = set(COMMAND_LINE_TARGETS)
if requested & BLOCKED_TARGETS and os.environ.get("NMMINER_ALLOW_RECONSTRUCTED_UPLOAD") != "1":
    raise RuntimeError(
        "Refusing to upload reconstructed NMMiner firmware. "
        "This target is compile-only until the backup/validation gate passes. "
        "Set NMMINER_ALLOW_RECONSTRUCTED_UPLOAD=1 only after that gate is explicit."
    )
