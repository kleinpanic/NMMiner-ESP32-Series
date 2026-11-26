"""Native JSON/SD source-port reuse and bounded HTTP close regression check."""

import argparse
import csv
import http.client
import io
import ipaddress
import json
import os
from pathlib import Path
import socket
import time

from sample_miner_api import read_response


def request(host, source_ip, port, path, wait_eof=False, *, server_port=80):
    """Record response shape/timing, never wallet amounts or downloaded rows."""
    began = time.monotonic()
    with socket.socket() as client:
        client.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        client.settimeout(10)
        client.bind((source_ip, port))
        client.connect((host, server_port))
        client.sendall(f"GET {path} HTTP/1.0\r\nHost: {host}\r\n\r\n".encode("ascii"))
        response = http.client.HTTPResponse(client)
        try:
            response.begin()
            if response.getheader("Content-Length") is None:
                raise ValueError("Content-Length required for this regression")
            body = read_response(response)
            if path == "/api/status":
                if not json.loads(body)["miner"]["poolAuthorized"]:
                    raise ValueError("Pool is not authorized")
                rows = None
            else:
                records = list(csv.reader(io.StringIO(body.decode("utf-8"))))
                if len(records) < 2 or any(len(row) != 9 for row in records):
                    raise ValueError("Invalid nine-column SD records")
                rows = len(records) - 1
            eof_seconds = None
            if wait_eof:
                closing = time.monotonic()
                if client.recv(1) != b"":
                    raise ValueError("Unexpected bytes beyond Content-Length")
                eof_seconds = time.monotonic() - closing
                if eof_seconds >= 4:
                    raise ValueError("Server close exceeded the bounded grace period")
            else:
                # Makes the previous active-close firmware a reliable negative
                # control while allowing the repaired peer-close policy to run.
                time.sleep(0.1)
            return {"path": path, "source_port": client.getsockname()[1], "bytes": len(body),
                    "rows": rows, "seconds": time.monotonic() - began,
                    "eof_wait_seconds": eof_seconds, "status": "PASS"}
        finally:
            response.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, type=ipaddress.IPv4Address)
    parser.add_argument("--out", required=True, type=Path,
                        help="existing native capture directory inside ignored logs/")
    parser.add_argument("--local-port", default=47123, type=int,
                        help="two consecutive unused source ports")
    parser.add_argument("--eof-port", default=61001, type=int)
    parser.add_argument("--reuse-delay", default=65, type=float)
    args = parser.parse_args()
    if not 1024 <= args.local_port <= 65534 or not 1024 <= args.eof_port <= 65535:
        parser.error("Source ports must be unprivileged valid TCP ports")
    if args.eof_port in (args.local_port, args.local_port + 1):
        parser.error("--eof-port must differ from both reuse ports")
    if not 65 <= args.reuse_delay <= 90:
        parser.error("--reuse-delay must be 65–90 seconds to exercise the native failure")
    root = Path(__file__).resolve().parents[1]
    out = args.out.resolve()
    if not out.is_relative_to(root / "logs") or not (out / "serial.log").is_file():
        parser.error("Start native capture first; --out must be its directory inside logs/")
    destination = out / "tcp-port-reuse.json"
    if destination.exists():
        parser.error("Existing results are preserved; use a fresh capture directory")
    os.umask(0o077)
    report = {"status": "measuring", "started_epoch": time.time(),
              "reuse_delay_seconds": args.reuse_delay, "requests": []}

    def save():
        temporary = destination.with_suffix(".new")
        temporary.write_text(json.dumps(report, indent=2) + "\n")
        temporary.replace(destination)

    save()
    try:
        host = str(args.host)
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as route:
            route.connect((host, 80))  # Select the source interface; send no UDP.
            source_ip = route.getsockname()[0]
        for stage in ("initial", "same_port_after_delay"):
            if stage != "initial":
                time.sleep(args.reuse_delay)
            for path, port in (("/api/status", args.local_port),
                               ("/api/sd/log", args.local_port + 1)):
                report["requests"].append({"stage": stage,
                    **request(host, source_ip, port, path)})
                save()
                print(f"{stage} {path} PASS", flush=True)
        report["requests"].append({"stage": "bounded_server_eof",
            **request(host, source_ip, args.eof_port, "/api/status", True)})
        report["status"] = "PASS"
    except (OSError, http.client.HTTPException, ValueError, KeyError, TypeError, csv.Error) as error:
        report.update(status="FAIL", failure=type(error).__name__)
    except KeyboardInterrupt:
        report["status"] = "INTERRUPTED"
    finally:
        report["completed_epoch"] = time.time()
        save()
    print(json.dumps(report), flush=True)
    raise SystemExit(report["status"] != "PASS")


if __name__ == "__main__":
    main()
