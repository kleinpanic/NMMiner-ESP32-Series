"""End-to-end: the REAL dashboard proxy making REAL HTTP calls to a mock
firmware that implements the custom API contract (docs/API.md). Exercises the
full chain proxy -> HTTP -> firmware -> response, including error branches.

The on-device E2E against the flashed firmware is the hardware-gated plan 08-08;
this validates the wire contract between dashboard and firmware without hardware.
"""

import importlib
import json
import os
import sys
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer

import pytest

APP_DIR = os.path.join(os.path.dirname(__file__), "..", "apps", "nmminer-control")
sys.path.insert(0, os.path.abspath(APP_DIR))
os.environ.setdefault("NMMINER_URL", "http://miner.test")
server = importlib.import_module("server")


class MockFirmware(BaseHTTPRequestHandler):
    """Mirrors src/webapi.cpp behavior closely enough to test the contract."""

    def log_message(self, *a):  # silence
        pass

    def _send(self, code, body, ctype="application/json"):
        data = body if isinstance(body, bytes) else json.dumps(body).encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        path = self.path.split("?")[0]
        if path in ("/api/system/info", "/api/status"):
            self._send(200, {"miner": {"sharesAccepted": 3, "totalMHashes": 10, "uptimeSeconds": 99},
                             "sd": {"mounted": True, "total": 100, "used": 40, "free": 60},
                             "speaker": {"volume": 80, "mute": False}})
        elif path == "/probe":
            self._send(200, {"model": "NMMiner"})
        elif path == "/api/speaker":
            self._send(200, {"volume": 80, "mute": False})
        elif path == "/api/sd/list":
            self._send(200, {"ok": True, "path": "/sd", "entries": ["mining_stats.csv"]})
        elif path == "/api/sd/download":
            if "missing" in self.path:
                self._send(404, {"ok": False, "error": "not found"})
            else:
                self._send(200, b"col1,col2\n1,2\n", ctype="application/octet-stream")
        elif path == "/api/sd/log":
            self._send(200, b"ts,hr,sh,up\n1,2,3,4\n", ctype="text/csv")
        else:
            self._send(404, {"ok": False, "error": "not found"})

    def do_POST(self):
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length) if length else b"{}"
        try:
            body = json.loads(raw or b"{}")
        except ValueError:
            body = {}
        path = self.path.split("?")[0]
        if path == "/api/speaker/volume":
            v = body.get("volume_percent", -1)
            self._send(200 if 0 <= v <= 100 else 400, {"ok": 0 <= v <= 100})
        elif path == "/api/speaker/mute":
            self._send(200, {"ok": True})
        elif path == "/api/speaker/test-tone":
            self._send(200, {"ok": True})
        elif path == "/api/sd/delete":
            self._send(200, {"ok": True})
        else:
            self._send(404, {"ok": False})


@pytest.fixture(scope="module")
def mock_miner():
    httpd = HTTPServer(("127.0.0.1", 0), MockFirmware)
    port = httpd.server_address[1]
    t = threading.Thread(target=httpd.serve_forever, daemon=True)
    t.start()
    prev = server.MINER_BASE_URL
    server.MINER_BASE_URL = f"http://127.0.0.1:{port}"
    yield
    server.MINER_BASE_URL = prev
    httpd.shutdown()


@pytest.fixture
def client():
    server.app.config["TESTING"] = True
    server.dashboard_cache.update({"time": 0.0, "payload": None})
    return server.app.test_client()


def test_e2e_dashboard_status(client, mock_miner):
    r = client.get("/api/dashboard")
    assert r.status_code == 200
    body = r.get_json()
    assert body["ok"] is True
    assert body["system"]["sd"]["mounted"] is True
    assert body["system"]["speaker"]["volume"] == 80


def test_e2e_sd_list(client, mock_miner):
    r = client.get("/api/sd/list?path=/sd")
    assert r.status_code == 200
    assert "mining_stats.csv" in json.dumps(r.get_json()["data"])


def test_e2e_sd_download_forces_attachment(client, mock_miner):
    r = client.get("/api/sd/download?path=/sd/mining_stats.csv")
    assert r.status_code == 200
    assert r.data == b"col1,col2\n1,2\n"
    assert r.headers["Content-Type"] == "application/octet-stream"
    assert r.headers["X-Content-Type-Options"] == "nosniff"


def test_e2e_sd_download_missing_propagates_404(client, mock_miner):
    r = client.get("/api/sd/download?path=/sd/missing.bin")
    assert r.status_code == 404


def test_e2e_sd_delete(client, mock_miner):
    r = client.post("/api/sd/delete", json={"path": "/sd/old.csv"})
    assert r.status_code == 200 and r.get_json()["ok"] is True


def test_e2e_sd_log(client, mock_miner):
    r = client.get("/api/sd/log")
    assert r.status_code == 200


def test_e2e_speaker_roundtrip(client, mock_miner):
    assert client.get("/api/speaker").status_code == 200
    assert client.post("/api/speaker/volume", json={"volume_percent": 60}).status_code == 200
    assert client.post("/api/speaker/mute", json={"muted": True}).status_code == 200
    assert client.post("/api/speaker/test-tone", json={}).status_code == 200


def test_e2e_traversal_blocked_before_firmware(client, mock_miner):
    # proxy rejects traversal at the boundary — never reaches the firmware
    r = client.get("/api/sd/list?path=/sd/../etc")
    assert r.status_code == 400
