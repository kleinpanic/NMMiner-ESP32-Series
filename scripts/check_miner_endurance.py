"""Validate captured Mine API and native serial evidence without touching hardware.

Run with the root .venv Python. Evidence stays under ignored logs/; the JSON
schemas are the samples/events emitted by the native qualification observer.
"""
import argparse
import json
import math
import re
from pathlib import Path


FATAL = re.compile(
    r"Guru Meditation|Brownout|abort\(\) was called|Rebooting\.\.\.|"
    r"assert failed|CORRUPT HEAP|LV_ASSERT|No enough(?:t)? memory|"
    r"Memory allocation failed|allocation failed:|reservation failed|hashrate nan|\[CHAIN TLS\]",
    re.IGNORECASE,
)


def native_state(started_epoch):
    return {"status": "observing", "started_epoch": started_epoch, "elapsed_seconds": 0,
            "events": [], "rates": [], "sha": [], "feeds": [], "faults": [], "boots": []}


def record_native_line(state, line, seconds):
    event = {"seconds": round(seconds, 3), "line": line.strip()}
    line = event["line"]
    if re.search(r"panicminer [^\r\n]* starting", line):
        state["boots"].append(event)
        if len(state["boots"]) > 1:
            state["faults"].append({**event, "line": "UNEXPECTED RESTART: " + line})
    if line.startswith(("[SD]", "[STATS]", "[POOL]", "[TOUCH]", "[UI]", "[STATE]",
                        "[WIFI]", "[STACK]", "[MINER] HW SHA", "[CFG] Opening")):
        state["events"].append(event)
    if match := re.search(r"avg\. hashrate (\S+) KH/s", line):
        try:
            rate = float(match[1])
            if math.isfinite(rate):
                state["rates"].append({"seconds": event["seconds"], "khs": rate})
            else:
                state["faults"].append({**event, "line": "NONFINITE RATE: " + line})
        except ValueError:
            state["faults"].append({**event, "line": "INVALID RATE: " + line})
    if line.startswith("[SHA] prefilter"):
        state["sha"].append(event)
    if line.startswith("[CHAIN]") and any(x in line for x in ("/5 ok", "stack minimum", "record", "scratch")):
        state["feeds"].append(event)
    if FATAL.search(line):
        state["faults"].append(event)


def evaluate(report, native, serial_text, baseline=994.0):
    samples = report.get("samples", [])
    low, high = baseline * .95, baseline * 1.05

    def band(value):
        return isinstance(value, (int, float)) and math.isfinite(value) and low <= value <= high

    def monotonic(key):
        return all(b[key] >= a[key] for a, b in zip(samples, samples[1:]))

    duration = samples[-1]["seconds"] - samples[0]["seconds"] if len(samples) > 1 else 0
    work_rate = ((samples[-1]["total_mhashes"] - samples[0]["total_mhashes"]) * 1000 / duration
                 if duration > 0 else None)
    boot_count = len(re.findall(r"panicminer [^\r\n]* starting", serial_text))
    feed_lines = [x["line"] for x in native.get("feeds", []) if "/5 ok" in x["line"]]
    sha = [tuple(map(int, match.groups())) for event in native.get("sha", [])
           if (match := re.search(r"checked (\d+), mismatched (\d+),.*dropped (\d+)", event["line"]))]
    native_start = native.get("started_epoch", math.inf)
    native_end = native_start + native.get("elapsed_seconds", 0)
    sample_start = samples[0].get("epoch", -math.inf) if samples else -math.inf
    sample_end = samples[-1].get("epoch", math.inf) if samples else math.inf
    last_sha = native_start + native.get("sha", [{}])[-1].get("seconds", -math.inf) if native.get("sha") else -math.inf
    checks = {
        "full_20_minutes": duration >= 1200,
        "reported_10s_in_band": bool(samples) and all(band(x["rate10_khs"]) for x in samples),
        "reported_60s_in_band": bool(samples) and all(band(x["rate60_khs"]) for x in samples),
        "independent_work_mean_in_band": band(work_rate),
        "pool_authorized": bool(samples) and all(x["pool_authorized"] for x in samples),
        "sd_mounted": bool(samples) and all(x["sd_mounted"] for x in samples),
        "sd_record_progress": len(samples) > 1 and samples[-1]["sd_rows_saved"] > samples[0]["sd_rows_saved"],
        "counters_never_reset": bool(samples) and monotonic("total_mhashes") and monotonic("sd_rows_saved"),
        "wallet_current": bool(samples) and all(x["wallet_valid"] and 0 <= x["wallet_age_seconds"] <= 180 for x in samples),
        "wallet_refreshes": sum(b["wallet_age_seconds"] < a["wallet_age_seconds"] for a, b in zip(samples, samples[1:])) >= 5,
        "continuous_sampling": len(samples) > 1 and all(0 < b["seconds"] - a["seconds"] <= 20 for a, b in zip(samples, samples[1:])),
        "no_api_errors": not report.get("api_errors"),
        "no_native_faults": not native.get("faults") and not FATAL.search(serial_text),
        "one_observed_boot": boot_count == 1,
        "native_covers_run": native_start <= sample_start and native_end >= sample_end - 2 and sample_end - 20 <= last_sha <= native_end,
        "all_feed_cycles_complete": len(feed_lines) >= 5 and all("[CHAIN] 5/5 ok" in x for x in feed_lines),
        "sha_checks_clean": bool(sha) and sha[-1][0] > 0 and all(x[1] == 0 and x[2] == 0 for x in sha)
            and all(b[0] >= a[0] for a, b in zip(sha, sha[1:])),
    }
    return {"status": "PASS" if all(checks.values()) else "FAIL", "checks": checks,
            "duration_seconds": duration, "work_khs": work_rate, "sample_count": len(samples),
            "observed_boots": boot_count, "native_feed_cycles": len(feed_lines)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("native", type=Path)
    parser.add_argument("serial_log", type=Path)
    args = parser.parse_args()
    try:
        result = evaluate(json.loads(args.report.read_text()), json.loads(args.native.read_text()),
                          args.serial_log.read_text(errors="replace"))
    except (KeyError, TypeError, ValueError, OSError) as error:
        parser.exit(2, f"Invalid or incomplete evidence: {type(error).__name__}\n")
    print(json.dumps(result, indent=2))
    raise SystemExit(result["status"] != "PASS")


if __name__ == "__main__":
    main()
