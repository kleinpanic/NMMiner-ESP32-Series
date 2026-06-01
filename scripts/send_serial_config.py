#!/usr/bin/env python3
import argparse
import json
import sys
import time

import serial


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Send NMMiner Wi-Fi/mining config JSON over USB serial."
    )
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--ssid", required=True)
    parser.add_argument("--password", required=True)
    parser.add_argument("--wallet", required=True)
    parser.add_argument("--fallback-wallet")
    parser.add_argument(
        "--primary-pool",
        default="stratum+tcp://public-pool.io:21496",
    )
    parser.add_argument(
        "--fallback-pool",
        default="stratum+tcp://pool.tazmining.ch:33333",
    )
    parser.add_argument("--primary-password", default="x")
    parser.add_argument("--fallback-password", default="x")
    parser.add_argument(
        "--delay",
        type=float,
        default=1.0,
        help="Seconds to wait after opening the serial port before first write.",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=1,
        help="Number of times to write the JSON payload.",
    )
    parser.add_argument(
        "--repeat-interval",
        type=float,
        default=0.5,
        help="Seconds between repeated writes.",
    )
    args = parser.parse_args()

    payload = {
        "ssid": args.ssid,
        "password": args.password,
        "address": args.wallet,
        "address2": args.fallback_wallet or args.wallet,
        "primary_pool_url": args.primary_pool,
        "primary_pool_password": args.primary_password,
        "fallback_pool_url": args.fallback_pool,
        "fallback_pool_password": args.fallback_password,
    }

    encoded = json.dumps(payload, separators=(",", ":")).encode("utf-8")

    with serial.Serial(args.port, args.baud, timeout=2) as ser:
        ser.dtr = False
        ser.rts = False
        time.sleep(args.delay)
        for idx in range(args.repeat):
            ser.write(encoded)
            ser.flush()
            if idx + 1 < args.repeat:
                time.sleep(args.repeat_interval)

    print(f"Sent {len(encoded)} bytes to {args.port} ({args.repeat} write(s))")
    return 0


if __name__ == "__main__":
    sys.exit(main())
