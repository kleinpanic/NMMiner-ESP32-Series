"""Unit tests for the NMMiner control dashboard proxy (server.py).

Focus: full branch coverage of the security boundary (safe_sd_path,
safe_download_name) and every error/success branch of the SD + speaker
endpoints. The miner is mocked — these tests never touch the network.
"""

import importlib
import os
import sys

import pytest

# Import the app module (apps/nmminer-control/server.py) without installing it.
APP_DIR = os.path.join(os.path.dirname(__file__), "..", "apps", "nmminer-control")
sys.path.insert(0, os.path.abspath(APP_DIR))
os.environ.setdefault("NMMINER_URL", "http://miner.test")
server = importlib.import_module("server")


@pytest.fixture
def client():
    server.app.config["TESTING"] = True
    return server.app.test_client()


# --------------------------------------------------------------------------
# safe_sd_path — every branch
# --------------------------------------------------------------------------

@pytest.mark.parametrize("raw", [
    "",                 # empty
    "no/leading/slash", # missing leading /
    "/etc/\x00passwd",  # null byte
    "/sd/../secret",    # .. traversal segment
    "/sd/..",           # trailing ..
    "../escape",        # leading .. + no slash
])
def test_safe_sd_path_rejected(raw):
    with pytest.raises(server.MinerError) as exc:
        server.safe_sd_path(raw)
    assert exc.value.status_code == 400


@pytest.mark.parametrize("raw,expected", [
    ("/", "/"),
    ("/sd", "/sd"),
    ("/sd/logs/x.csv", "/sd/logs/x.csv"),
    ("/sd//double", "/sd/double"),       # normpath collapses //
    ("/sd/./here", "/sd/here"),          # normpath collapses /./
])
def test_safe_sd_path_accepted(raw, expected):
    assert server.safe_sd_path(raw) == expected


# --------------------------------------------------------------------------
# safe_download_name — every branch
# --------------------------------------------------------------------------

@pytest.mark.parametrize("raw,expected", [
    ("/sd/stats.csv", "stats.csv"),
    ("/sd/we ird name!.bin", "we_ird_name_.bin"),
    ("/sd/", "download"),                 # empty basename -> fallback
    ("/sd/\r\ninject", "__inject"),       # CRLF stripped
])
def test_safe_download_name(raw, expected):
    assert server.safe_download_name(raw) == expected


# --------------------------------------------------------------------------
# /api/sd/list
# --------------------------------------------------------------------------

def test_sd_list_invalid_path(client):
    r = client.get("/api/sd/list?path=/sd/../x")
    assert r.status_code == 400
    assert r.get_json()["ok"] is False


def test_sd_list_success(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request",
                        lambda *a, **k: {"entries": ["a.csv", "b.bin"]})
    r = client.get("/api/sd/list?path=/sd")
    assert r.status_code == 200
    body = r.get_json()
    assert body["ok"] is True and body["data"]["entries"] == ["a.csv", "b.bin"]


def test_sd_list_miner_error(client, monkeypatch):
    def boom(*a, **k):
        raise server.MinerError("sd unavailable", status_code=503)
    monkeypatch.setattr(server, "miner_request", boom)
    r = client.get("/api/sd/list?path=/sd")
    assert r.status_code == 503
    assert r.get_json()["ok"] is False


# --------------------------------------------------------------------------
# /api/sd/download
# --------------------------------------------------------------------------

def test_sd_download_missing_path(client):
    r = client.get("/api/sd/download")
    assert r.status_code == 400


def test_sd_download_invalid_path(client):
    r = client.get("/api/sd/download?path=/sd/../etc")
    assert r.status_code == 400


def test_sd_download_success_forces_safe_headers(client, monkeypatch):
    class Resp:
        status_code = 200
        content = b"hello-bytes"
        headers = {"content-type": "text/html", "content-length": "11"}
    monkeypatch.setattr(server.requests, "get", lambda *a, **k: Resp())
    r = client.get("/api/sd/download?path=/sd/report.csv")
    assert r.status_code == 200
    assert r.data == b"hello-bytes"
    # XSS hardening: never echo upstream content-type; force opaque attachment.
    assert r.headers["Content-Type"] == "application/octet-stream"
    assert r.headers["X-Content-Type-Options"] == "nosniff"
    assert r.headers["Content-Disposition"] == 'attachment; filename="report.csv"'


def test_sd_download_upstream_error(client, monkeypatch):
    class Resp:
        status_code = 404
        content = b""
        headers = {}
        text = "missing"
    monkeypatch.setattr(server.requests, "get", lambda *a, **k: Resp())
    r = client.get("/api/sd/download?path=/sd/missing.bin")
    assert r.status_code == 404


# --------------------------------------------------------------------------
# /api/sd/delete
# --------------------------------------------------------------------------

def test_sd_delete_missing_path(client):
    r = client.post("/api/sd/delete", json={})
    assert r.status_code == 400


def test_sd_delete_invalid_path(client):
    r = client.post("/api/sd/delete", json={"path": "/sd/../boom"})
    assert r.status_code == 400


def test_sd_delete_success(client, monkeypatch):
    captured = {}
    def fake(method, path, **k):
        captured["json"] = k.get("json")
        return {"deleted": True}
    monkeypatch.setattr(server, "miner_request", fake)
    r = client.post("/api/sd/delete", json={"path": "/sd/old.csv"})
    assert r.status_code == 200
    # sanitized path is what gets proxied
    assert captured["json"]["path"] == "/sd/old.csv"


# --------------------------------------------------------------------------
# /api/speaker/*
# --------------------------------------------------------------------------

def test_speaker_volume_missing_field(client):
    # proxy requires volume_percent (bounds enforced firmware-side)
    r = client.post("/api/speaker/volume", json={})
    assert r.status_code == 400
    assert r.get_json()["ok"] is False


def test_speaker_volume_forwarded(client, monkeypatch):
    captured = {}
    monkeypatch.setattr(server, "miner_request",
                        lambda m, p, **k: captured.update(k.get("json") or {}) or "OK")
    r = client.post("/api/speaker/volume", json={"volume_percent": 75})
    assert r.status_code == 200
    assert captured["volume_percent"] == 75


def test_speaker_mute_missing_field(client):
    r = client.post("/api/speaker/mute", json={})
    assert r.status_code == 400


def test_speaker_mute_forwarded(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: "OK")
    r = client.post("/api/speaker/mute", json={"muted": True})
    assert r.status_code == 200


def test_speaker_get(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: {"volume": 80, "mute": False})
    r = client.get("/api/speaker")
    assert r.status_code == 200
    assert r.get_json()["data"]["volume"] == 80


def test_speaker_test_tone_defaults(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: "OK")
    r = client.post("/api/speaker/test-tone", json={})
    assert r.status_code == 200


@pytest.mark.parametrize("method,path,body", [
    ("get", "/api/speaker", None),
    ("post", "/api/speaker/volume", {"volume_percent": 50}),
    ("post", "/api/speaker/mute", {"muted": True}),
    ("post", "/api/speaker/test-tone", {}),
])
def test_speaker_endpoints_miner_error_branch(client, monkeypatch, method, path, body):
    def boom(*a, **k):
        raise server.MinerError("down", status_code=502, detail="d")
    monkeypatch.setattr(server, "miner_request", boom)
    r = client.get(path) if method == "get" else client.post(path, json=body)
    assert r.status_code == 502 and r.get_json()["ok"] is False
