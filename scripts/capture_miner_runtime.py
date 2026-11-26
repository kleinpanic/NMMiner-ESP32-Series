"""Capture private native qualification evidence without flashing firmware.

Use ./.venv/bin/python; only one process may own the serial port. Output must
stay in ignored logs/. --reset pulses EN without erasing saved settings.
"""
import argparse
import json
import os
import signal
import time
from pathlib import Path

from check_miner_endurance import native_state, record_native_line


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--duration", type=float, default=10800)
    parser.add_argument("--reset", action="store_true")
    parser.add_argument("--allow-commands", action="store_true", help="send out/command.txt, then remove it")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    if not out.is_relative_to(root / "logs") or args.duration <= 0:
        parser.error("Use an output directory inside ignored logs/ and a positive duration.")
    os.umask(0o077)
    out.mkdir(parents=True, exist_ok=True, mode=0o700)
    out.chmod(0o700)
    if (out / "serial.log").exists() or (out / "result.json").exists():
        parser.error("Use a fresh evidence directory; existing captures are preserved.")

    import serial
    device = serial.Serial()
    device.port, device.baudrate, device.timeout = args.port, 115200, .15
    device.dtr = device.rts = False
    device.exclusive = True
    device.open()
    stopped = False

    def stop(*_):
        nonlocal stopped
        stopped = True

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    state = native_state(time.time())
    start = time.monotonic()
    pending = bytearray()
    saved_at = -1

    def save():
        state["elapsed_seconds"] = round(time.monotonic() - start, 3)
        temporary = out / "result.new"
        temporary.write_text(json.dumps(state, indent=2) + "\n")
        temporary.replace(out / "result.json")

    try:
        with (out / "serial.log").open("xb", buffering=0) as raw:
            if args.reset:
                device.rts = True
                time.sleep(.15)
                device.rts = False
            save()
            while not stopped and time.monotonic() - start < args.duration:
                command = out / "command.txt"
                if args.allow_commands and command.exists():
                    data = command.read_bytes()
                    if len(data) > 4096:
                        raise ValueError("Diagnostic command exceeds 4096 bytes")
                    command.unlink()
                    device.write(b"\n" + data + b"\n")
                data = device.read(min(device.in_waiting or 1, 65536))
                raw.write(data)
                pending.extend(data)
                if len(pending) > 1024 * 1024:
                    raise ValueError("Serial frame exceeds 1 MB")
                while b"\n" in pending:
                    line, _, pending = pending.partition(b"\n")
                    record_native_line(state, line.decode("ascii", "replace"), time.monotonic() - start)
                if time.monotonic() - saved_at >= 1:
                    save()
                    saved_at = time.monotonic()
            state["status"] = "stopped" if stopped else "capture duration complete"
    except (OSError, ValueError, serial.SerialException) as error:
        state["status"] = "capture failed"
        state["faults"].append({"seconds": round(time.monotonic() - start, 3), "line": type(error).__name__})
        raise
    finally:
        save()
        device.close()
    print(json.dumps({"status": state["status"], "faults": len(state["faults"]), "boots": len(state["boots"])}))


if __name__ == "__main__":
    main()
