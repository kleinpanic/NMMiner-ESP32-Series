"""Collect a strict 20-minute USB qualification alongside native capture.

Run through ./.venv/bin/python. Start capture_miner_runtime.py first, then use
its same ignored --out directory. Only measured, non-wallet-value fields are
recorded. This tool neither changes settings nor flashes/restarts the miner.
"""
import argparse
import csv
import io
import ipaddress
import json
import math
import os
import re
import time
import urllib.request
from pathlib import Path

from check_miner_endurance import evaluate


def status_point(data, epoch, latency):
    miner, sd, wallet = data["miner"], data["sd"], data.get("wallet", {})
    return {"epoch": epoch, "api_response_seconds": latency,
            "rate10_khs": miner["hashrate10sKhs"], "rate60_khs": miner["hashrate60sKhs"],
            "total_mhashes": miner["totalMHashes"], "pool_authorized": miner["poolAuthorized"],
            "heap_free_bytes": miner["heapFreeBytes"], "chip_temp_c": miner["chipTempC"],
            "accepted": miner["sharesAccepted"], "rejected": miner["sharesRejected"],
            "best_difficulty": miner["bestVerifiedDifficulty"], "pool_difficulty": miner["poolDifficulty"],
            "sd_mounted": sd["mounted"], "sd_rows_saved": sd["rowsSaved"],
            "wallet_valid": wallet.get("valid", False), "wallet_age_seconds": wallet.get("ageSeconds", -1)}


def ready(point):
    def band(value):
        return isinstance(value, (int, float)) and math.isfinite(value) and 944.3 <= value <= 1043.7
    return (band(point["rate10_khs"]) and band(point["rate60_khs"]) and
            point["pool_authorized"] and point["sd_mounted"] and point["wallet_valid"] and
            0 <= point["wallet_age_seconds"] <= 180)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, type=ipaddress.IPv4Address, help="miner's LAN IPv4 address")
    parser.add_argument("--out", required=True, type=Path, help="existing native capture directory in logs/")
    parser.add_argument("--image-sha256", help="digest from the separately verified installation manifest")
    args = parser.parse_args()
    if args.image_sha256 and not re.fullmatch(r"[0-9a-fA-F]{64}", args.image_sha256):
        parser.error("--image-sha256 must be a complete SHA-256 digest.")
    root = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    if not out.is_relative_to(root / "logs") or not (out / "serial.log").is_file():
        parser.error("Start native capture first; --out must be its directory inside ignored logs/.")
    destination = out / "usb-endurance.json"
    if destination.exists():
        parser.error("Existing endurance results are preserved; use a fresh native capture.")
    os.umask(0o077)
    report = {"status": "warming up", "baseline_khs": 994, "samples": [], "api_errors": [], "warmup_errors": []}
    if args.image_sha256:
        report["installed_sha256"] = args.image_sha256.lower()

    def save():
        temporary = out / "usb-endurance.new"
        temporary.write_text(json.dumps(report, indent=2) + "\n")
        temporary.replace(destination)

    def fetch(path):
        began = time.monotonic()
        with urllib.request.urlopen(f"http://{args.host}{path}", timeout=10) as response:
            body = response.read(2 * 1024 * 1024 + 1)
            if response.status != 200 or len(body) > 2 * 1024 * 1024:
                raise ValueError("Invalid or oversized API response")
        return body, time.monotonic() - began

    def point():
        body, latency = fetch("/api/status")
        return status_point(json.loads(body), time.time(), latency)

    def native():
        state = json.loads((out / "result.json").read_text())
        if state["status"] != "observing" or time.time() - state["started_epoch"] - state["elapsed_seconds"] > 5:
            raise ValueError("Native capture is not current")
        if state["faults"]:
            raise ValueError("Native firmware fault")
        return state

    save()
    try:
        consecutive = 0
        deadline = time.monotonic() + 480
        while time.monotonic() < deadline:
            native()
            try:
                sample = point()
                consecutive = consecutive + 1 if ready(sample) else 0
                report["latest_warmup"] = sample
            except (OSError, ValueError, KeyError) as error:
                report["warmup_errors"].append({"epoch": time.time(), "error": type(error).__name__})
                consecutive = 0
            save()
            if consecutive >= 6:
                break
            time.sleep(5)
        else:
            raise ValueError("No stable in-band warmup")
        began = time.monotonic()
        sample["seconds"] = 0
        report.update(status="measuring", started_epoch=sample["epoch"], samples=[sample])
        save()
        while True:
            time.sleep(5)
            native()
            try:
                sample = point()
            except (OSError, ValueError, KeyError) as error:
                report["api_errors"].append({"epoch": time.time(), "error": type(error).__name__})
                raise
            sample["seconds"] = time.monotonic() - began
            report["samples"].append(sample)
            report["elapsed_seconds"] = sample["seconds"]
            save()
            if not ready(sample):
                raise ValueError("Rate, pool, SD or wallet outside acceptance")
            if sample["seconds"] >= 1200:
                break
        verdict = evaluate(report, native(), (out / "serial.log").read_text(errors="replace"))
        report["strict_qualification"] = verdict
        # Bulk SD I/O is deliberately outside the timed performance window.
        body, latency = fetch("/api/sd/log")
        rows = list(csv.reader(io.StringIO(body.decode("utf-8"))))
        consistent = len(rows) > 1 and len(rows[0]) == 9 and all(len(row) == 9 for row in rows[1:])
        report["csv_download"] = {"row_count": max(0, len(rows) - 1), "column_count": len(rows[0]) if rows else 0,
                                  "consistent_records": consistent, "response_seconds": latency}
        (out / "mining_stats_v2.csv").write_bytes(body)
        report["status"] = "PASS" if verdict["status"] == "PASS" and consistent else "FAIL"
    except (OSError, ValueError, KeyError, TypeError) as error:
        report.update(status="FAIL", failure=type(error).__name__ + ": " + str(error))
    except KeyboardInterrupt:
        report["status"] = "INTERRUPTED"
    finally:
        report["completed_epoch"] = time.time()
        save()
    print(json.dumps({key: report[key] for key in ("status", "strict_qualification", "csv_download") if key in report}, indent=2))
    raise SystemExit(report["status"] != "PASS")


if __name__ == "__main__":
    main()
