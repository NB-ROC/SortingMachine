const Dashboard = (() => {
  const state = { colors: {}, stats: { totalSorted: 0, sortRate: 0, uptime: 0 }, status: 'offline', speed: 50, mode: 'auto', thresholds: { confidence: 85, rejectLimit: 10, sensitivity: 7 } };
  const PALETTE = { red: '#ff3a5c', green: '#3df5c4', blue: '#0be0ff', yellow: '#ffd230', brown: '#c8854a', orange: '#ff8c00', purple: '#b44fff', white: '#e8ecf4', black: '#4a566e', pink: '#ff6eb4', cyan: '#00e5ff', gray: '#6b7a95', grey: '#6b7a95' };
  const FALLBACKS = ['#ff6eb4', '#7fff00', '#ff8c00', '#b44fff', '#00cfff', '#ffaa33'];
  let fbIdx = 0;
  const getColor = n => { const k = n.toLowerCase(); if (!PALETTE[k]) PALETTE[k] = FALLBACKS[fbIdx++ % FALLBACKS.length]; return PALETTE[k]; };
  const $ = id => document.getElementById(id);
  setInterval(() => $('clockDisplay').textContent = new Date().toLocaleTimeString('en-GB'), 1000);
  $('clockDisplay').textContent = new Date().toLocaleTimeString('en-GB');
  const ctx = $('donutChart').getContext('2d');
  function drawDonut(data) {
    const W = 160, cx = 80, cy = 80, ro = 66, ri = 40;
    const total = Object.values(data).reduce((a, b) => a + b, 0) || 1;
    ctx.clearRect(0, 0, W, W);
    ctx.beginPath(); ctx.arc(cx, cy, ro, 0, Math.PI * 2); ctx.arc(cx, cy, ri, 0, Math.PI * 2, true); ctx.fillStyle = '#111520'; ctx.fill();
    let start = -Math.PI / 2;
    for (const [name, count] of Object.entries(data)) {
      const slice = (count / total) * Math.PI * 2;
      ctx.beginPath(); ctx.moveTo(cx + ri * Math.cos(start), cy + ri * Math.sin(start));
      ctx.arc(cx, cy, ro, start, start + slice); ctx.arc(cx, cy, ri, start + slice, start, true);
      ctx.closePath(); ctx.fillStyle = getColor(name); ctx.fill(); start += slice;
    }
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillStyle = '#fff'; ctx.font = 'bold 20px Syne'; ctx.fillText(total, cx, cy - 7);
    ctx.fillStyle = '#4a566e'; ctx.font = '9px DM Mono'; ctx.fillText('TOTAL', cx, cy + 11);
  }
  function updateLegend(data) {
    const total = Object.values(data).reduce((a, b) => a + b, 0) || 1;
    $('donutLegend').innerHTML = Object.entries(data).map(([n, c]) => `<div class="leg"><div class="leg-dot" style="background:${getColor(n)}"></div>${n} <span style="color:#fff;margin-left:3px">${((c / total) * 100).toFixed(0)}%</span></div>`).join('');
  }
  function renderColorBars(data) {
    if (!Object.keys(data).length) return;
    const total = Object.values(data).reduce((a, b) => a + b, 0) || 1;
    $('colorBars').innerHTML = Object.entries(data).sort((a, b) => b[1] - a[1]).map(([name, count]) => {
      const pct = ((count / total) * 100).toFixed(1), col = getColor(name);
      return `<div class="color-row"><div class="color-row-head"><div class="color-id"><div class="swatch" style="background:${col}"></div>${name}</div><span class="color-num">${count.toLocaleString()}</span></div><div class="track"><div class="fill" style="width:${pct}%;background:${col}"></div></div><div class="pct">${pct}%</div></div>`;
    }).join('');
  }
  function renderStats(s) {
    $('kpiTotal').innerHTML = `${Number(s.totalSorted).toLocaleString()} <em>units</em>`;
    $('kpiRate').innerHTML = `${s.sortRate} <em>/ min</em>`;
    let u = s.uptime; if (typeof u === 'number') { const h = Math.floor(u / 3600).toString().padStart(2, '0'), m = Math.floor((u % 3600) / 60).toString().padStart(2, '0'), sc = (u % 60).toString().padStart(2, '0'); u = `${h}:${m}:${sc}`; }
    $('kpiUptime').textContent = u || '00:00:00';
  }
  const SEV = { info: { bg: '#0d1e30', color: '#0be0ff', label: 'INFO' }, warn: { bg: '#221c00', color: '#ffd230', label: 'WARN' }, error: { bg: '#220010', color: '#ff3a5c', label: 'ERROR' }, ok: { bg: '#001a14', color: '#3df5c4', label: 'OK' } };
  function logEvent(msg, sev = 'info') {
    const feed = $('eventFeed'), s = SEV[sev] || SEV.info, time = new Date().toLocaleTimeString('en-GB');
    const row = document.createElement('div'); row.className = 'feed-row'; row.style.borderLeftColor = s.color;
    row.innerHTML = `<span class="feed-time">${time}</span><span class="feed-msg">${msg}</span><span class="feed-badge" style="background:${s.bg};color:${s.color}">${s.label}</span>`;
    feed.prepend(row); while (feed.children.length > 80) feed.removeChild(feed.lastChild);
  }
  const STATUS_CFG = { online: { dot: '#3df5c4', label: 'ONLINE' }, offline: { dot: '#4a566e', label: 'OFFLINE' }, warning: { dot: '#ffd230', label: 'WARNING' }, error: { dot: '#ff3a5c', label: 'ERROR' } };
  function setStatus(s) { state.status = s; const c = STATUS_CFG[s] || STATUS_CFG.online; const dot = $('statusDot'); dot.style.background = c.dot; dot.style.boxShadow = `0 0 10px ${c.dot}`; $('statusText').textContent = c.label; }
  function syncStrip() { $('notifMode').textContent = state.mode.toUpperCase(); $('notifConf').textContent = state.thresholds.confidence + '%'; }
  const pub = {
    updateColors(map) { state.colors = { ...state.colors, ...map }; renderColorBars(state.colors); drawDonut(state.colors); updateLegend(state.colors); },
    updateStats(s) { Object.assign(state.stats, s); renderStats(state.stats); },
    setStatus, logEvent,
    updateMachineState(name) { $('kpiState').textContent = name.replace(/_/g, ' ').toUpperCase(); $('notifState').textContent = name.toUpperCase(); $('notifState').style.color = name === 'cycle_done' ? 'var(--dim)' : 'var(--accent)'; const LOG = ['cycle_done', 'index_to_scanner', 'set_container']; if (LOG.includes(name)) { logEvent(`State → ${name}`, name === 'cycle_done' ? 'ok' : 'info'); } },
    updateDetection(d) { const col = getColor(d.color); const set = (id, fn) => { const el = $(id); if (el) fn(el); }; set('detColor', el => { el.innerHTML = `<span class="swatch-inline" style="background:${col}"></span>${d.color.toUpperCase()}`; el.style.color = col; }); set('detStable', el => { el.textContent = d.stablehit ? '✓  YES' : '⚠  NO'; el.className = 'det-val ' + (d.stablehit ? 'ok' : 'warn'); }); set('detSep', el => { el.textContent = d.separation; el.className = 'det-val ' + (d.separation > 6000 ? 'ok' : 'warn'); }); set('detBest', el => { el.textContent = d.bestdist; }); if (d.rgbc) { const sw = `rgb(${d.rgbc.r},${d.rgbc.g},${d.rgbc.b})`; set('detRgb', el => { el.innerHTML = `<span class="swatch-inline" style="background:${sw}"></span>${d.rgbc.r}, ${d.rgbc.g}, ${d.rgbc.b}`; }); set('notifRgb', el => { el.textContent = `${d.rgbc.r},${d.rgbc.g},${d.rgbc.b}`; }); } set('notifLastColor', el => { el.textContent = d.color.toUpperCase(); el.style.color = col; }); set('notifSep', el => { el.textContent = d.separation; el.style.color = d.separation > 6000 ? 'var(--ok)' : 'var(--warn)'; }); logEvent(`Sorted ${d.color.toUpperCase()} — sep:${d.separation}`, 'info'); },
    onSend: null, onSpeedChange: null, onModeChange: null, onThresholdChange: null,
  };
  drawDonut({}); syncStrip(); logEvent('Dashboard initialized', 'ok');
  return pub;
})();

/* =========================================================================
   ESP32 API client  (see api-docs.md)
   -------------------------------------------------------------------------
   This dashboard talks to the sensor over the documented HTTP + SSE API.
   It does NOT need to be served from the ESP32 — host it anywhere (localhost,
   a static host, etc.) and point it at the device. CORS is open, so a
   cross-origin frontend works fine.

   Override the host with either:
     • a query param:   index.html?host=192.168.1.42
     • localStorage:    localStorage.setItem('esp32_host', '192.168.1.42')
   Defaults to the mDNS name 'esp32.local'.
   ========================================================================= */
const API = (() => {
  const params = new URLSearchParams(location.search);
  const override = params.get('host');
  if (override) localStorage.setItem('esp32_host', override);
  const host = override || localStorage.getItem('esp32_host') || 'esp32.local';
  return `http://${host}`;
})();

/* ---- Outbound: POST /api/action ----------------------------------------
   The HTTP layer requires every body to carry a `type` field (otherwise 400)
   and forwards the JSON straight through to the MCU over Serial2. */
async function postAction(body) {
  try {
    // The firmware now answers the CORS preflight (OPTIONS /api/action), so we
    // can send proper application/json and the body parses reliably.
    const res = await fetch(`${API}/api/action`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body),
    });
    if (!res.ok) {
      let detail = '';
      try { detail = (await res.text()).trim().slice(0, 120); } catch { /* no body */ }
      Dashboard.logEvent(`Action rejected (HTTP ${res.status})${detail ? ' — ' + detail : ''}`, 'error');
      return false;
    }
    const el = document.getElementById('notifLastSend');
    if (el) el.textContent = new Date().toLocaleTimeString('en-GB');
    return true;
  } catch {
    Dashboard.logEvent('Send failed — device unreachable', 'error');
    return false;
  }
}

// Named machine commands → { type:'command', command:<name> } per api-docs.md.
function sendAction(command) {
  Dashboard.logEvent('Action sent: ' + command, 'info');
  return postAction({ type: 'command', command }); // resolves true/false
}

/* ---- Inbound: shared dispatch ------------------------------------------
   Used for both live SSE events and replayed cache messages. */

// Client-side aggregation: the dashboard derives its own color breakdown and
// throughput from the detection stream instead of depending on the MCU to
// push pre-rolled colors/stats. Keeps it loose from the device.
const tally = {};            // color -> running count
let totalSorted = 0;
const recentHits = [];       // timestamps (ms) for a rolling 60s sort-rate
function recordDetection(color) {
  if (!color) return;
  tally[color] = (tally[color] || 0) + 1;
  totalSorted++;
  const now = Date.now();
  recentHits.push(now);
  while (recentHits.length && now - recentHits[0] > 60000) recentHits.shift();
  Dashboard.updateColors({ ...tally });
  Dashboard.updateStats({ totalSorted, sortRate: recentHits.length });
}

// Map the API's detection_final shape onto what Dashboard.updateDetection
// expects (that renderer predates this API — see api-docs.md "Message shapes").
function adaptDetection(m) {
  const dists = Array.isArray(m.distances) ? [...m.distances].sort((a, b) => a - b) : [];
  return {
    color: m.color,
    stablehit: m.stable_hit,
    bestdist: dists[0],
    // separation = gap between the best and next-best color match
    separation: dists.length > 1 ? +(dists[1] - dists[0]).toFixed(2) : 0,
    rgbc: Array.isArray(m.rgbc) ? { r: m.rgbc[0], g: m.rgbc[1], b: m.rgbc[2] } : null,
  };
}

function handleMessage(m) {
  if (!m || typeof m !== 'object') return;
  // Legacy aggregate fields — honored if the MCU still emits them.
  if (m.colors) Dashboard.updateColors(m.colors);
  if (m.stats) Dashboard.updateStats(m.stats);
  if (m.status) Dashboard.setStatus(m.status);
  if (m.event) Dashboard.logEvent(m.event.message, m.event.severity);

  switch (m.type) {
    case 'state':
      Dashboard.updateMachineState(m.state);
      break;
    case 'detection_final':
      // 'unknown' still logs (via updateDetection) but isn't added to the total
      if (m.color && m.color.toLowerCase() !== 'unknown') recordDetection(m.color);
      Dashboard.updateDetection(adaptDetection(m));
      break;
    case 'detection_sample':
      // intermediate guesses — ignored; only the final result is shown/counted
      break;
    case 'calibration_point':
      Dashboard.logEvent(`Calibration point: ${m.label} (dist ${m.distance})`, 'info');
      break;
    case 'calibration_saved':
      Dashboard.logEvent('Calibration saved', 'ok');
      break;
  }
}

/* ---- Connection --------------------------------------------------------- */

// GET /api/cache — replay the last 20 messages so a fresh page isn't blank.
async function loadCache() {
  try {
    const res = await fetch(`${API}/api/cache`);
    if (!res.ok) return;
    (await res.json()).forEach(handleMessage);
  } catch { /* offline; SSE + status poll will report it */ }
}

// GET /api/status — health + uptime (the SSE stream doesn't push these).
async function pollStatus() {
  try {
    const res = await fetch(`${API}/api/status`);
    if (!res.ok) throw new Error(res.status);
    const s = await res.json();
    Dashboard.updateStats({ uptime: Math.floor(s.uptime_ms / 1000) });
    Dashboard.setStatus('online');
  } catch {
    Dashboard.setStatus('offline');
  }
}

// GET /api/events — live stream. EventSource auto-reconnects and replays via
// Last-Event-ID on its own, so no manual retry/reload is needed.
function connectSSE() {
  const es = new EventSource(`${API}/api/events`);
  es.addEventListener('meta', () => {
    Dashboard.setStatus('online');
    Dashboard.logEvent('Connected to ESP32', 'ok');
  });
  ['detection_final', 'detection_sample', 'calibration_point', 'calibration_saved', 'state']
    .forEach(type => es.addEventListener(type, e => {
      let m; try { m = JSON.parse(e.data); } catch { return; }
      if (!m.type) m.type = type;
      handleMessage(m);
    }));
  es.onmessage = e => { try { handleMessage(JSON.parse(e.data)); } catch { /* ignore */ } };
  es.onerror = () => Dashboard.setStatus('warning'); // EventSource retries automatically
  return es;
}

/* ---- Boot --------------------------------------------------------------- */
Dashboard.setStatus('offline'); // start offline; proven online by the first status poll / SSE connect
loadCache();
connectSSE();
pollStatus();
setInterval(pollStatus, 5000);

let paused = false;
async function togglePause() {
  const ok = await sendAction('toggle_pause');
  if (!ok) return; // command was rejected/unreachable — leave the UI unchanged
  paused = !paused;
  const btn = document.getElementById('pauseToggle');
  document.getElementById('pauseIcon').textContent = paused ? '⏸️' : '▶️';
  document.getElementById('pauseLabel').textContent = paused ? 'PAUSED' : 'RUNNING';
  btn.style.borderColor = paused ? 'var(--warn)' : 'var(--border2)';
  btn.style.color = paused ? 'var(--warn)' : 'var(--dim2)';
  document.getElementById('notifMode').textContent = paused ? 'PAUSED' : 'RUNNING';
}

