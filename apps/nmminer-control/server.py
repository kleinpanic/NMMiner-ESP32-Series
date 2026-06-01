#!/usr/bin/env python3
"""Local NMMiner control dashboard.

This app intentionally talks to the miner from the local server process instead
of making browser-to-miner requests. That avoids CORS issues and keeps this repo
as the canonical controller surface.
"""

from __future__ import annotations

import os
import threading
import time
from typing import Any

import requests
from flask import Flask, jsonify, request, send_from_directory


APP_DIR = os.path.dirname(os.path.abspath(__file__))
STATIC_DIR = os.path.join(APP_DIR, "static")

MINER_BASE_URL = os.environ.get("NMMINER_URL", "http://192.168.1.136").rstrip("/")
REQUEST_TIMEOUT = float(os.environ.get("NMMINER_TIMEOUT", "5"))
MAX_SCREENSAVER_BYTES = 200 * 1024

SETTINGS_ENDPOINTS = {
    "network": "/api/setting/network",
    "mining": "/api/setting/mining",
    "time": "/api/setting/time",
    "preference": "/api/setting/preference",
    "market": "/api/setting/market",
    "weather": "/api/setting/weather",
}

SETTING_FIELDS = {
    "network": {"Hostname", "WiFiSSID", "WiFiPWD"},
    "mining": {
        "PrimaryPool",
        "PrimaryAddress",
        "PrimaryPassword",
        "SecondaryPool",
        "SecondaryAddress",
        "SecondaryPassword",
    },
    "time": {"Timezone", "TimeFormat", "DateFormat"},
    "preference": {
        "Brightness",
        "RotateScreen",
        "LedEnable",
        "ScreenSaver",
        "ScreenSaverMode",
    },
    "market": {"MainCoin", "WatchCoins", "KlineRotate", "KlineInterval", "PricePageMode"},
    "weather": {
        "WeatherCity",
        "WeatherLat",
        "WeatherLon",
        "WeatherTempUnit",
        "WeatherSpeedUnit",
        "WeatherAltMode",
    },
}

MANUAL_POOLS = [
    {
        "name": "pool.nerdminers.org",
        "url": "stratum+tcp://pool.nerdminers.org:3333",
        "port": 3333,
        "diff": "0.001",
        "status": "live-tested",
    },
    {
        "name": "public-pool.io",
        "url": "stratum+tcp://public-pool.io:21496",
        "port": 21496,
        "diff": "0.001",
        "status": "tcp-only",
    },
    {
        "name": "pool.tazmining.ch",
        "url": "stratum+tcp://pool.tazmining.ch:33333",
        "port": 33333,
        "diff": "0.001",
        "status": "timed-out",
    },
]

app = Flask(__name__, static_folder=STATIC_DIR, static_url_path="")

dashboard_cache_lock = threading.Lock()
dashboard_cache: dict[str, Any] = {"time": 0.0, "payload": None}
settings_cache_lock = threading.Lock()
settings_cache: dict[str, Any] = {"time": 0.0, "payload": None}


class MinerError(RuntimeError):
    def __init__(self, message: str, status_code: int = 502, detail: Any | None = None):
        super().__init__(message)
        self.status_code = status_code
        self.detail = detail


def miner_url(path: str) -> str:
    if not path.startswith("/"):
        path = f"/{path}"
    return f"{MINER_BASE_URL}{path}"


def response_payload(response: requests.Response) -> Any:
    content_type = response.headers.get("content-type", "")
    if "application/json" in content_type:
        return response.json()
    text = response.text.strip()
    try:
        return response.json()
    except ValueError:
        return text


def miner_request(method: str, path: str, timeout: float | None = None, **kwargs: Any) -> Any:
    try:
        response = requests.request(
            method,
            miner_url(path),
            timeout=timeout or REQUEST_TIMEOUT,
            **kwargs,
        )
    except requests.RequestException as exc:
        raise MinerError("miner unreachable", detail=str(exc)) from exc

    if response.status_code >= 400:
        raise MinerError(
            "miner request failed",
            status_code=response.status_code,
            detail=response.text.strip(),
        )
    return response_payload(response)


def read_optional(path: str, timeout: float | None = None) -> tuple[Any | None, str | None]:
    try:
        return miner_request("GET", path, timeout=timeout), None
    except MinerError as exc:
        return None, f"{path}: {exc.detail or exc}"


def coerce_setting(section: str, key: str, value: Any) -> Any:
    if value is None:
        return None

    if isinstance(value, str):
        value = value.strip()
        if value == "":
            return None

    if section == "preference" and key in {"Brightness", "RotateScreen", "LedEnable"}:
        return int(value)
    if section == "time" and key == "TimeFormat":
        return int(value)

    return value


def clean_settings_payload(section: str, payload: dict[str, Any]) -> dict[str, Any]:
    allowed = SETTING_FIELDS[section]
    cleaned: dict[str, Any] = {}

    for key, raw_value in payload.items():
        if key not in allowed:
            continue
        value = coerce_setting(section, key, raw_value)
        if value is None:
            continue
        cleaned[key] = value

    if section == "preference":
        if "Brightness" in cleaned and not 1 <= cleaned["Brightness"] <= 100:
            raise MinerError("brightness must be 1-100", status_code=400)
        if "RotateScreen" in cleaned and cleaned["RotateScreen"] not in {0, 90, 180, 270}:
            raise MinerError("rotation must be 0, 90, 180, or 270", status_code=400)
        if "LedEnable" in cleaned and cleaned["LedEnable"] not in {0, 1}:
            raise MinerError("LED must be 0 or 1", status_code=400)

    if section == "time" and "TimeFormat" in cleaned and cleaned["TimeFormat"] not in {12, 24}:
        raise MinerError("time format must be 12 or 24", status_code=400)

    return cleaned


@app.errorhandler(MinerError)
def handle_miner_error(exc: MinerError):
    return jsonify({"ok": False, "error": str(exc), "detail": exc.detail}), exc.status_code


@app.get("/")
def index():
    return send_from_directory(STATIC_DIR, "index.html")


@app.get("/api/dashboard")
def dashboard():
    now = time.time()
    with dashboard_cache_lock:
        if dashboard_cache["payload"] and now - dashboard_cache["time"] < 3:
            return jsonify(dashboard_cache["payload"])

        payload = build_dashboard_payload()
        dashboard_cache["time"] = time.time()
        dashboard_cache["payload"] = payload
        return jsonify(payload)


def build_dashboard_payload() -> dict[str, Any]:
    probe, probe_error = read_optional("/probe", timeout=2.0)
    system, system_error = read_optional("/api/system/info", timeout=2.5)
    errors = [error for error in [probe_error, system_error] if error]

    return {
        "ok": not errors,
        "checkedAt": int(time.time()),
        "minerBaseUrl": MINER_BASE_URL,
        "probe": probe,
        "system": system,
        "settings": {},
        "manualPools": MANUAL_POOLS,
        "errors": errors,
    }


@app.get("/api/settings")
def get_all_settings():
    now = time.time()
    with settings_cache_lock:
        if settings_cache["payload"] and now - settings_cache["time"] < 8:
            return jsonify(settings_cache["payload"])

        payload = build_settings_payload()
        settings_cache["time"] = time.time()
        settings_cache["payload"] = payload
        return jsonify(payload)


def build_settings_payload() -> dict[str, Any]:
    settings: dict[str, Any] = {}
    errors: list[str] = []

    for section, path in SETTINGS_ENDPOINTS.items():
        data, error = read_optional(path, timeout=1.5)
        if error:
            errors.append(error)
        else:
            settings[section] = data

    return {
        "ok": not errors,
        "checkedAt": int(time.time()),
        "settings": settings,
        "errors": errors,
    }


@app.get("/api/settings/<section>")
def get_settings(section: str):
    if section not in SETTINGS_ENDPOINTS:
        return jsonify({"ok": False, "error": "unknown settings section"}), 404
    return jsonify({"ok": True, "data": miner_request("GET", SETTINGS_ENDPOINTS[section])})


@app.post("/api/settings/<section>")
def post_settings(section: str):
    if section not in SETTINGS_ENDPOINTS:
        return jsonify({"ok": False, "error": "unknown settings section"}), 404

    raw_payload = request.get_json(silent=True) or {}
    if not isinstance(raw_payload, dict):
        return jsonify({"ok": False, "error": "JSON object required"}), 400

    payload = clean_settings_payload(section, raw_payload)
    if not payload:
        return jsonify({"ok": False, "error": "no allowed settings supplied"}), 400

    result = miner_request("POST", SETTINGS_ENDPOINTS[section], json=payload)
    with settings_cache_lock:
        settings_cache["time"] = 0.0
        settings_cache["payload"] = None
    return jsonify({"ok": True, "section": section, "sent": sorted(payload), "minerResponse": result})


@app.post("/api/action/<action>")
def action(action: str):
    paths = {
        "find": "/api/swarm/find",
        "restart": "/api/system/restart",
        "weather-refresh": "/api/weather/refresh",
    }
    if action not in paths:
        return jsonify({"ok": False, "error": "unknown action"}), 404

    result = miner_request("POST", paths[action], json={})
    return jsonify({"ok": True, "action": action, "minerResponse": result})


@app.get("/api/screensaver/preflight")
def screensaver_preflight():
    size = request.args.get("size", type=int)
    if not size or size < 1:
        return jsonify({"ok": False, "error": "positive size query parameter required"}), 400
    result = miner_request("GET", "/api/update/screensaver/preflight", params={"size": size})
    return jsonify({"ok": True, "data": result})


@app.post("/api/screensaver/upload")
def screensaver_upload():
    if not request.files:
        return jsonify({"ok": False, "error": "multipart file required"}), 400

    upload = next(iter(request.files.values()))
    filename = upload.filename or "screensaver.ss"
    if not filename.endswith(".ss"):
        return jsonify({"ok": False, "error": "only .ss files are accepted"}), 400

    data = upload.read()
    if len(data) > MAX_SCREENSAVER_BYTES:
        return jsonify({"ok": False, "error": "file exceeds 200 KB"}), 413

    result = miner_request(
        "POST",
        "/api/update/screensaver",
        files={"file": (filename, data, "application/octet-stream")},
    )
    return jsonify({"ok": True, "data": result})


def main() -> None:
    host = os.environ.get("NMMINER_CONTROL_HOST", "127.0.0.1")
    port = int(os.environ.get("NMMINER_CONTROL_PORT", "8788"))
    app.run(host=host, port=port, debug=False, threaded=True)


if __name__ == "__main__":
    main()
