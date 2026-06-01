const state = {
  history: [],
  lastData: null,
  refreshPromise: null,
  formsPrimed: new Set(),
  dirtyForms: new Set(),
};

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => Array.from(document.querySelectorAll(selector));

function text(selector, value) {
  const node = $(selector);
  if (node) node.textContent = value ?? "-";
}

function showToast(message, type = "ok") {
  const toast = $("#toast");
  toast.textContent = message;
  toast.className = type === "error" ? "show error" : "show";
  window.clearTimeout(showToast.timer);
  showToast.timer = window.setTimeout(() => {
    toast.className = "";
  }, 3600);
}

async function fetchJson(url, options = {}) {
  const response = await fetch(url, {
    headers: { "Accept": "application/json", ...(options.headers || {}) },
    ...options,
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok || data.ok === false) {
    throw new Error(data.detail || data.error || `HTTP ${response.status}`);
  }
  return data;
}

function formatHashRate(ghs) {
  const value = Number(ghs);
  if (!Number.isFinite(value)) return "-";
  if (value >= 1) return `${value.toFixed(3)} GH/s`;
  if (value >= 0.001) return `${(value * 1000).toFixed(3)} MH/s`;
  return `${(value * 1000000).toFixed(0)} KH/s`;
}

function formatSeconds(total) {
  const seconds = Number(total);
  if (!Number.isFinite(seconds)) return "-";
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  if (days) return `${days}d ${hours}h ${minutes}m`;
  if (hours) return `${hours}h ${minutes}m`;
  return `${minutes}m`;
}

function formatBytes(bytes) {
  const value = Number(bytes);
  if (!Number.isFinite(value)) return "-";
  if (value > 1024 * 1024) return `${(value / 1024 / 1024).toFixed(2)} MB`;
  return `${(value / 1024).toFixed(1)} KB`;
}

function masked(value) {
  const textValue = String(value || "");
  if (textValue.length <= 16) return textValue || "-";
  return `${textValue.slice(0, 8)}...${textValue.slice(-6)}`;
}

function updateManualPools(pools = []) {
  const target = $("#manual-pools");
  if (!target) return;
  target.innerHTML = pools.map((pool) => `
    <div class="pool-item">
      <strong>${pool.name}</strong>
      <span>${pool.port} · diff ${pool.diff} · ${pool.status}</span>
    </div>
  `).join("");
}

function setFormValues(formId, values) {
  const form = document.getElementById(formId);
  if (!form || !values || state.dirtyForms.has(formId)) return;

  Array.from(form.elements).forEach((element) => {
    if (!element.name || values[element.name] === undefined) return;
    element.value = values[element.name];
  });
  state.formsPrimed.add(formId);
}

function updateForms(settings = {}) {
  setFormValues("preference-form", settings.preference);
  setFormValues("market-form", settings.market);
  setFormValues("weather-form", settings.weather);
  setFormValues("mining-form", settings.mining);
}

function updateScreenPreview(data) {
  const canvas = $("#screen-preview");
  if (!canvas || !data?.system) return;

  const ctx = canvas.getContext("2d");
  const info = data.system;
  const miner = info.miner || {};
  const identity = info.identity || {};
  const stratum = info.stratum || {};

  ctx.fillStyle = "#050705";
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.strokeStyle = "#2f342d";
  ctx.strokeRect(12, 12, 296, 216);

  ctx.fillStyle = "#f4f1ee";
  ctx.font = "16px ui-monospace, monospace";
  ctx.fillText(identity.hostName || "NMMiner", 24, 38);

  ctx.fillStyle = "#74c938";
  ctx.font = "13px ui-monospace, monospace";
  ctx.fillText(`${identity.rssi ?? "-"} dBm`, 226, 38);

  ctx.fillStyle = "#ea24f7";
  ctx.font = "34px ui-monospace, monospace";
  ctx.fillText(formatHashRate(miner.hashRate), 24, 86);

  ctx.fillStyle = "#767168";
  ctx.fillRect(24, 104, 272, 1);

  ctx.fillStyle = "#74c938";
  ctx.font = "17px ui-monospace, monospace";
  ctx.fillText(`ACCEPT ${miner.sAccepted ?? "-"}`, 24, 132);
  ctx.fillText(`REJECT ${miner.sRejected ?? "-"}`, 176, 132);

  ctx.fillStyle = "#f4f1ee";
  ctx.font = "13px ui-monospace, monospace";
  ctx.fillText(`POOL ${stratum.url || "-"}`, 24, 166);
  ctx.fillText(`BEST ${miner.bestDiffEver || "-"}`, 24, 190);
  ctx.fillText(`UP ${formatSeconds(miner.uptimeSeconds)}`, 176, 190);
}

function updateStatus(data) {
  const system = data.system || {};
  const probe = data.probe || {};
  const identity = system.identity || {};
  const miner = system.miner || {};
  const stratum = system.stratum || {};
  const storage = system.storage || {};

  state.lastData = data;
  state.history.push({
    t: Date.now(),
    hashRate: Number(miner.hashRate || 0),
    accepted: Number(miner.sAccepted || 0),
    rejected: Number(miner.sRejected || 0),
  });
  state.history = state.history.slice(-90);

  text("#miner-url", data.minerBaseUrl);
  text("#host-name", identity.hostName || probe.hostname || "NMMiner");
  text("#hw-model", identity.hwModel || probe.model || "-");
  text("#self-ip", data.minerBaseUrl?.replace(/^https?:\/\//, "") || "-");
  text("#fw-version", identity.fwVersion || probe.ver || "-");
  text("#firmware-chip", identity.fwVersion || probe.ver || "firmware");
  text("#rssi-side", `${identity.rssi ?? "-"} dBm`);
  text("#last-seen", new Date().toLocaleTimeString());

  text("#hash-rate", formatHashRate(miner.hashRate));
  text("#hash-rate-raw", `${miner.hashRate ?? "-"} GH/s`);
  text("#accepted", miner.sAccepted ?? "-");
  text("#rejected", miner.sRejected ?? "-");
  const totalShares = Number(miner.sAccepted || 0) + Number(miner.sRejected || 0);
  text("#reject-rate", totalShares ? `${((Number(miner.sRejected || 0) / totalShares) * 100).toFixed(2)}%` : "0.00%");
  text("#pool-diff", miner.poolDiff || "-");
  text("#last-diff", `last ${miner.lastDiff || "-"}`);
  text("#uptime", formatSeconds(miner.uptimeSeconds));
  text("#uptime-ever", `ever ${formatSeconds(miner.uptimeEver)}`);

  text("#pool-url", stratum.url || "-");
  text("#pool-user", masked(stratum.user));
  text("#network-diff", miner.networkDiff || "-");
  text("#best-session", miner.bestDiffSession || "-");
  text("#best-ever", miner.bestDiffEver || "-");
  text("#heap", `${formatBytes(miner.freeHeap)} free / ${formatBytes(miner.minFreeHeap)} min`);
  text("#storage", `${formatBytes(storage.fsUsed)} used / ${formatBytes(storage.fsTotal)} total`);

  updateManualPools(data.manualPools);
  updateForms(data.settings);
  updateScreenPreview(data);
  drawChart();

  if (data.errors?.length) {
    text("#poll-state", "partial");
  } else {
    text("#poll-state", "live");
  }
}

function drawChart() {
  const canvas = $("#hash-chart");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  const width = canvas.width;
  const height = canvas.height;
  const pad = 36;
  const samples = state.history;

  ctx.clearRect(0, 0, width, height);
  ctx.fillStyle = "#0b0d0a";
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "#22271f";
  ctx.lineWidth = 1;
  for (let i = 0; i < 5; i += 1) {
    const y = pad + ((height - pad * 2) * i) / 4;
    ctx.beginPath();
    ctx.moveTo(pad, y);
    ctx.lineTo(width - pad, y);
    ctx.stroke();
  }

  if (samples.length < 2) return;

  const maxHash = Math.max(...samples.map((s) => s.hashRate), 0.0011);
  const xFor = (index) => pad + ((width - pad * 2) * index) / (samples.length - 1);
  const yFor = (value) => height - pad - ((height - pad * 2) * value) / maxHash;

  ctx.strokeStyle = "#ea24f7";
  ctx.lineWidth = 2;
  ctx.beginPath();
  samples.forEach((sample, index) => {
    const x = xFor(index);
    const y = yFor(sample.hashRate);
    if (index === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();

  samples.forEach((sample, index) => {
    const prev = samples[index - 1];
    if (!prev) return;
    const acceptedDelta = sample.accepted - prev.accepted;
    const rejectedDelta = sample.rejected - prev.rejected;
    const x = xFor(index);
    if (acceptedDelta > 0) {
      ctx.fillStyle = "#74c938";
      ctx.fillRect(x - 2, height - pad - Math.min(acceptedDelta * 8, 60), 4, Math.min(acceptedDelta * 8, 60));
    }
    if (rejectedDelta > 0) {
      ctx.fillStyle = "#ff4a45";
      ctx.fillRect(x + 3, height - pad - Math.min(rejectedDelta * 16, 72), 4, Math.min(rejectedDelta * 16, 72));
    }
  });
}

async function refreshDashboard(manual = false) {
  if (state.refreshPromise) return state.refreshPromise;

  state.refreshPromise = (async () => {
    try {
      text("#poll-state", "syncing");
      const data = await fetchJson("/api/dashboard");
      updateStatus(data);
      if (manual) showToast("refreshed");
    } catch (error) {
      text("#poll-state", "offline");
      showToast(error.message, "error");
    } finally {
      state.refreshPromise = null;
    }
  })();

  return state.refreshPromise;
}

async function refreshSettings(manual = false) {
  try {
    const data = await fetchJson("/api/settings");
    updateForms(data.settings);
    if (manual && data.errors?.length) showToast(`settings partial: ${data.errors.length}`, "error");
  } catch (error) {
    if (manual) showToast(error.message, "error");
  }
}

async function submitSettings(form) {
  const section = form.dataset.section;
  const payload = {};
  const formData = new FormData(form);
  formData.forEach((value, key) => {
    payload[key] = value;
  });

  const data = await fetchJson(`/api/settings/${section}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  state.dirtyForms.delete(form.id);
  showToast(`${section} saved: ${data.sent.join(", ")}`);
  await refreshSettings();
  await refreshDashboard();
}

async function runAction(action) {
  if (action === "restart" && !window.confirm("Restart miner now?")) return;
  await fetchJson(`/api/action/${action}`, { method: "POST" });
  showToast(`${action} sent`);
  window.setTimeout(() => refreshDashboard(), action === "restart" ? 8000 : 1200);
}

async function preflightScreensaver() {
  const file = $("#screensaver-form input[type=file]")?.files?.[0];
  if (!file) {
    text("#screen-output", "select a .ss file");
    return;
  }
  const data = await fetchJson(`/api/screensaver/preflight?size=${file.size}`);
  text("#screen-output", `${data.data.action} · free ${formatBytes(data.data.fsFree)} · after ${formatBytes(data.data.spaceAfter)}`);
}

async function uploadScreensaver(event) {
  event.preventDefault();
  const file = event.currentTarget.querySelector("input[type=file]")?.files?.[0];
  if (!file) {
    text("#screen-output", "select a .ss file");
    return;
  }
  const payload = new FormData();
  payload.append("file", file, file.name);
  const data = await fetchJson("/api/screensaver/upload", { method: "POST", body: payload });
  text("#screen-output", data.data.path || "uploaded");
  showToast("screensaver uploaded");
}

function bindEvents() {
  $("#refresh-now")?.addEventListener("click", () => refreshDashboard(true));
  $("#refresh-now")?.addEventListener("click", () => refreshSettings(true));
  $("#identify-top")?.addEventListener("click", () => runAction("find").catch((error) => showToast(error.message, "error")));

  $$(".tab").forEach((button) => {
    button.addEventListener("click", () => {
      $$(".tab").forEach((tab) => tab.classList.remove("active"));
      $$(".tab-page").forEach((page) => page.classList.remove("active"));
      button.classList.add("active");
      $(`#tab-${button.dataset.tab}`)?.classList.add("active");
    });
  });

  $$("form[data-section]").forEach((form) => {
    form.addEventListener("input", () => state.dirtyForms.add(form.id));
    form.addEventListener("submit", (event) => {
      event.preventDefault();
      submitSettings(form).catch((error) => showToast(error.message, "error"));
    });
  });

  $$("[data-submit-form]").forEach((button) => {
    button.addEventListener("click", () => {
      document.getElementById(button.dataset.submitForm)?.requestSubmit();
    });
  });

  $$("[data-action]").forEach((button) => {
    button.addEventListener("click", () => {
      runAction(button.dataset.action).catch((error) => showToast(error.message, "error"));
    });
  });

  $("#screen-preflight")?.addEventListener("click", () => {
    preflightScreensaver().catch((error) => showToast(error.message, "error"));
  });
  $("#screensaver-form")?.addEventListener("submit", uploadScreensaver);
}

function boot() {
  bindEvents();
  if (window.lucide) window.lucide.createIcons();
  refreshDashboard();
  refreshSettings();
  window.setInterval(refreshDashboard, 5000);
  window.setInterval(refreshSettings, 60000);
}

document.addEventListener("DOMContentLoaded", boot);
