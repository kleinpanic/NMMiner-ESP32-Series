#!/usr/bin/env python3
"""Provision the fork firmware over USB serial, then log its boot.

Reads the NMCFG JSON from --config (default secrets/device.json, which git
ignores), or from stdin with --config -, so secrets never reach argv or the
shell history. Resets the board, waits for the firmware's serial-config
window, sends one `NMCFG {json}` line, and logs serial output with every
secret value replaced by <redacted>.

Keys: SSID, WifiPW, PoolUrl, PoolPort, BtcWallet, PoolPassword, Timezone.
The board keeps its stored value for any key the file leaves out. The board
is 2.4 GHz only: a 5 GHz SSID never connects.

Usage: provision_fork_serial.py --log out.log [--config secrets/device.json]
"""
import argparse
import json
import pathlib
import sys
import time

import serial


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default="/dev/ttyUSB0")
    ap.add_argument("--log", required=True)
    ap.add_argument("--seconds", type=float, default=120)
    ap.add_argument("--config", default=str(pathlib.Path(__file__).resolve().parent.parent
                                            / "secrets" / "device.json"))
    args = ap.parse_args()

    if args.config == "-":
        cfg = json.load(sys.stdin)
    else:
        with open(args.config) as f:
            cfg = json.load(f)
    secrets = [str(v) for k, v in cfg.items() if k in ("SSID", "WifiPW", "BtcWallet") and v]
    line = ("NMCFG " + json.dumps(cfg, separators=(",", ":")) + "\n").encode()

    s = serial.Serial(args.port, 115200, timeout=0.2)
    s.dtr = False
    s.rts = True          # EN low: hold in reset, BOOT stays high
    time.sleep(0.1)
    s.rts = False

    sent, buf, end = False, b"", time.time() + args.seconds
    with open(args.log, "w") as log:
        while time.time() < end:
            buf += s.read(4096)
            while b"\n" in buf:
                raw, buf = buf.split(b"\n", 1)
                text = raw.decode(errors="replace").rstrip("\r")
                for value in secrets:
                    text = text.replace(value, "<redacted>")
                log.write(text + "\n")
                log.flush()
                if not sent and "[CFG]" in text and "NMCFG" in text:
                    s.write(line)
                    sent = True
                    log.write("# host: NMCFG sent\n")
    print("sent" if sent else "config window not seen")
    return 0 if sent else 1


if __name__ == "__main__":
    sys.exit(main())
