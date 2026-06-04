"""Coverage for the remaining server.py surface: helpers, settings, dashboard,
actions, screensaver — and the MinerError error handler. Miner is mocked."""

import importlib
import os
import sys

import pytest

APP_DIR = os.path.join(os.path.dirname(__file__), "..", "apps", "nmminer-control")
sys.path.insert(0, os.path.abspath(APP_DIR))
os.environ.setdefault("NMMINER_URL", "http://miner.test")
server = importlib.import_module("server")


@pytest.fixture(autouse=True)
def reset_caches():
    server.dashboard_cache.update({"time": 0.0, "payload": None})
    server.settings_cache.update({"time": 0.0, "payload": None})
    yield


@pytest.fixture
def client():
    server.app.config["TESTING"] = True
    return server.app.test_client()


class FakeResp:
    def __init__(self, status=200, json_body=None, text="", ctype="application/json"):
        self.status_code = status
        self._json = json_body
        self.text = text
        self.headers = {"content-type": ctype}
    def json(self):
        if self._json is None:
            raise ValueError("no json")
        return self._json


# --- pure helpers ---------------------------------------------------------

def test_miner_url_adds_leading_slash():
    assert server.miner_url("probe").endswith("/probe")
    assert server.miner_url("/probe").endswith("/probe")


def test_response_payload_json_and_text():
    assert server.response_payload(FakeResp(json_body={"a": 1})) == {"a": 1}
    txt = FakeResp(text="plain", ctype="text/plain")
    assert server.response_payload(txt) == "plain"


def test_miner_request_unreachable(monkeypatch):
    def boom(*a, **k):
        raise server.requests.RequestException("down")
    monkeypatch.setattr(server.requests, "request", boom)
    with pytest.raises(server.MinerError) as e:
        server.miner_request("GET", "/x")
    assert e.value.status_code == 502


def test_miner_request_http_error(monkeypatch):
    monkeypatch.setattr(server.requests, "request",
                        lambda *a, **k: FakeResp(status=500, text="boom"))
    with pytest.raises(server.MinerError) as e:
        server.miner_request("GET", "/x")
    assert e.value.status_code == 500


def test_read_optional_success_and_error(monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: {"ok": 1})
    assert server.read_optional("/p") == ({"ok": 1}, None)
    def boom(*a, **k):
        raise server.MinerError("nope", detail="d")
    monkeypatch.setattr(server, "miner_request", boom)
    data, err = server.read_optional("/p")
    assert data is None and "/p" in err


# --- coerce_setting -------------------------------------------------------

def test_coerce_setting_branches():
    assert server.coerce_setting("preference", "Brightness", None) is None
    assert server.coerce_setting("preference", "Brightness", "   ") is None
    assert server.coerce_setting("preference", "Brightness", "70") == 70
    assert server.coerce_setting("time", "TimeFormat", "12") == 12
    assert server.coerce_setting("network", "Ssid", " Home ") == "Home"


# --- clean_settings_payload ----------------------------------------------

def test_clean_drops_unknown_and_none():
    out = server.clean_settings_payload("preference", {"Brightness": "70", "Bogus": "x", "RotateScreen": ""})
    assert out == {"Brightness": 70}


@pytest.mark.parametrize("payload,msg", [
    ({"Brightness": "150"}, "brightness"),
    ({"RotateScreen": "45"}, "rotation"),
    ({"LedEnable": "5"}, "LED"),
])
def test_clean_preference_validation(payload, msg):
    with pytest.raises(server.MinerError) as e:
        server.clean_settings_payload("preference", payload)
    assert e.value.status_code == 400


def test_clean_time_format_validation():
    with pytest.raises(server.MinerError):
        server.clean_settings_payload("time", {"TimeFormat": "13"})


# --- endpoints ------------------------------------------------------------

def test_index_serves_html(client):
    r = client.get("/")
    assert r.status_code == 200


def test_dashboard(client, monkeypatch):
    monkeypatch.setattr(server, "read_optional", lambda p, timeout=None: ({"v": p}, None))
    r = client.get("/api/dashboard")
    assert r.status_code == 200 and r.get_json()["ok"] is True


def test_dashboard_with_errors(client, monkeypatch):
    monkeypatch.setattr(server, "read_optional", lambda p, timeout=None: (None, f"{p}: err"))
    r = client.get("/api/dashboard")
    assert r.get_json()["ok"] is False and r.get_json()["errors"]


def test_get_all_settings(client, monkeypatch):
    monkeypatch.setattr(server, "read_optional", lambda p, timeout=None: ({"p": p}, None))
    r = client.get("/api/settings")
    assert r.status_code == 200 and r.get_json()["ok"] is True


def test_get_settings_unknown(client):
    assert client.get("/api/settings/nope").status_code == 404


def test_get_settings_valid(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: {"Ssid": "Home"})
    section = next(iter(server.SETTINGS_ENDPOINTS))
    r = client.get(f"/api/settings/{section}")
    assert r.status_code == 200


def test_post_settings_unknown(client):
    assert client.post("/api/settings/nope", json={}).status_code == 404


def test_post_settings_empty_after_clean(client):
    r = client.post("/api/settings/preference", json={"Bogus": "x"})
    assert r.status_code == 400


def test_post_settings_valid(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: "OK")
    r = client.post("/api/settings/preference", json={"Brightness": "70"})
    assert r.status_code == 200 and "Brightness" in r.get_json()["sent"]


def test_miner_error_handler(client, monkeypatch):
    # get_settings lets MinerError propagate to the registered error handler
    def boom(*a, **k):
        raise server.MinerError("boom", status_code=502, detail="d")
    monkeypatch.setattr(server, "miner_request", boom)
    section = next(iter(server.SETTINGS_ENDPOINTS))
    r = client.get(f"/api/settings/{section}")
    assert r.status_code == 502 and r.get_json()["ok"] is False


def test_action_unknown(client):
    assert client.post("/api/action/nope").status_code == 404


def test_action_valid(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: "OK")
    r = client.post("/api/action/restart")
    assert r.status_code == 200 and r.get_json()["action"] == "restart"


def test_screensaver_preflight_bad_size(client):
    assert client.get("/api/screensaver/preflight").status_code == 400


def test_screensaver_preflight_ok(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: {"space": True})
    assert client.get("/api/screensaver/preflight?size=1000").status_code == 200


def test_screensaver_upload_no_file(client):
    assert client.post("/api/screensaver/upload").status_code == 400


def test_response_payload_text_is_json():
    # text/plain content-type but body parses as JSON -> returns parsed
    r = FakeResp(json_body={"k": 1}, ctype="text/plain")
    assert server.response_payload(r) == {"k": 1}


def _upload(client, name, data):
    import io
    return client.post("/api/screensaver/upload",
                       data={"file": (io.BytesIO(data), name)},
                       content_type="multipart/form-data")


def test_screensaver_upload_rejects_non_ss(client):
    assert _upload(client, "x.png", b"abc").status_code == 400


def test_screensaver_upload_too_big(client):
    big = b"x" * (server.MAX_SCREENSAVER_BYTES + 1)
    assert _upload(client, "x.ss", big).status_code == 413


def test_screensaver_upload_valid(client, monkeypatch):
    monkeypatch.setattr(server, "miner_request", lambda *a, **k: {"ok": True})
    assert _upload(client, "logo.ss", b"\x4e\x53data").status_code == 200
