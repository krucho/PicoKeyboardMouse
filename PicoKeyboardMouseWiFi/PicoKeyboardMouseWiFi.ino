#include <DNSServer.h>
#include <Keyboard.h>
#include <LittleFS.h>
#include <Mouse.h>
#include <WebServer.h>
#include <WiFi.h>
#include <SimpleMDNS.h>

// AP de configuracion (portal cautivo). Se usa cuando no hay credenciales
// guardadas o no se pudo conectar a la red local.
static const char *SETUP_AP_SSID = "Pico-Keyboard-Setup";
// AP abierto (sin contraseña) para facilitar la configuración inicial.

// Nombre mDNS: una vez conectado a la red local, entra desde otro dispositivo
// de la misma red a http://picokm.local (sin necesidad de saber la IP).
static const char *MDNS_HOSTNAME = "picokm";

static const char *WIFI_FILE = "/wifi.cfg";
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;

// Mantener presionado el boton BOOTSEL este tiempo borra las credenciales WiFi
// guardadas y reinicia la Pico en modo configuracion.
static const uint32_t BOOTSEL_RESET_MS = 3000;

IPAddress apIp(192, 168, 42, 1);
IPAddress apGateway(192, 168, 42, 1);
IPAddress apSubnet(255, 255, 255, 0);

WebServer server(80);
DNSServer dns;
uint32_t lastBlinkMs = 0;
bool configMode = false;

static const char CONFIG_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Configurar WiFi - Pico Keyboard</title>
  <style>
    :root { color-scheme: dark; --bg:#0d1117; --panel:#161c24; --text:#f5f9fc; --muted:#96a5b2; --accent:#ffd166; --border:#293641; }
    * { box-sizing: border-box; }
    body { margin:0; min-height:100svh; background:var(--bg); color:var(--text); font-family:system-ui,sans-serif; padding:16px; }
    main { max-width:440px; margin:0 auto; }
    h1 { font-size:20px; margin:0 0 8px; }
    p { color:var(--muted); font-size:14px; line-height:1.45; }
    form { display:grid; gap:10px; margin-top:16px; padding:14px; background:var(--panel); border:1px solid var(--border); border-radius:8px; }
    label { font-size:13px; font-weight:600; }
    input { width:100%; padding:12px; border-radius:7px; border:1px solid var(--border); background:#111820; color:var(--text); font:inherit; }
    button { padding:12px; border:0; border-radius:7px; background:var(--accent); color:#1a1402; font:inherit; font-weight:700; cursor:pointer; }
    button:disabled { opacity:0.6; cursor:default; }
    button.secondary { background:#26313b; color:var(--text); border:1px solid var(--border); }
    .note { margin-top:14px; font-size:12px; color:var(--muted); }
    .nets { display:grid; gap:6px; max-height:240px; overflow:auto; }
    .net { display:flex; align-items:center; gap:8px; padding:11px 12px; background:#111820; border:1px solid var(--border); border-radius:7px; cursor:pointer; text-align:left; color:var(--text); font:inherit; font-weight:600; box-shadow:none; }
    .net.sel { border-color:var(--accent); background:#1c2530; color:var(--text); }
    .net .name { flex:1 1 auto; overflow:hidden; text-overflow:ellipsis; white-space:nowrap; }
    .net .meta { font-size:11px; color:var(--muted); font-weight:400; white-space:nowrap; }
    .muted { color:var(--muted); font-size:13px; padding:4px 2px; }
  </style>
</head>
<body>
  <main>
    <h1>Configurar WiFi</h1>
    <p>Eleg&iacute; tu red de la lista (o escrib&iacute; el nombre) y guard&aacute; la contrase&ntilde;a. La Pico se reiniciar&aacute; y se unir&aacute; a esa WiFi.</p>
    <form id="cfgForm" method="POST" action="/wifi/save">
      <button type="button" id="scanBtn" class="secondary">Buscar redes</button>
      <div id="nets" class="nets"></div>
      <div>
        <label for="ssid">Nombre de red (SSID)</label>
        <input id="ssid" name="ssid" required autocomplete="off" maxlength="32">
      </div>
      <div>
        <label for="pass">Contrase&ntilde;a</label>
        <input id="pass" name="pass" type="password" autocomplete="off" maxlength="63" placeholder="Dejar vacio si es red abierta">
      </div>
      <button type="submit">Guardar y conectar</button>
    </form>
    <p class="note">Tras conectar, entr&aacute; desde otro dispositivo de la misma red a <strong>http://picokm.local</strong> (o a la IP que muestre la p&aacute;gina). Para cambiar de red m&aacute;s adelante: mant&eacute;n presionado el bot&oacute;n <strong>BOOTSEL</strong> unos 3 segundos, o abr&iacute; <a href="/wifi/reset" style="color:var(--accent)">/wifi/reset</a>.</p>
  </main>
  <script>
    var scanBtn = document.getElementById("scanBtn");
    var nets = document.getElementById("nets");
    var ssidInput = document.getElementById("ssid");
    var passInput = document.getElementById("pass");

    function esc(s) {
      return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }
    function bars(r) {
      if (r >= -55) return "\u2022\u2022\u2022\u2022";
      if (r >= -65) return "\u2022\u2022\u2022";
      if (r >= -75) return "\u2022\u2022";
      return "\u2022";
    }
    function pick(el, ssid) {
      ssidInput.value = ssid;
      nets.querySelectorAll(".net").forEach(function (n) { n.classList.remove("sel"); });
      el.classList.add("sel");
      passInput.focus();
    }
    function scan() {
      scanBtn.disabled = true;
      scanBtn.textContent = "Buscando...";
      nets.innerHTML = '<div class="muted">Escaneando redes (puede tardar unos segundos)...</div>';
      fetch("/wifi/scan", { cache: "no-store" })
        .then(function (r) { return r.json(); })
        .then(function (list) {
          var seen = {};
          list.forEach(function (n) {
            if (!n.ssid) return;
            if (!(n.ssid in seen) || n.rssi > seen[n.ssid].rssi) seen[n.ssid] = n;
          });
          var arr = Object.keys(seen).map(function (k) { return seen[k]; });
          arr.sort(function (a, b) { return b.rssi - a.rssi; });
          if (!arr.length) {
            nets.innerHTML = '<div class="muted">No se encontraron redes. Prob&aacute; de nuevo.</div>';
            return;
          }
          nets.innerHTML = "";
          arr.forEach(function (n) {
            var b = document.createElement("button");
            b.type = "button";
            b.className = "net";
            b.innerHTML = '<span class="name">' + esc(n.ssid) + '</span>'
                        + '<span class="meta">' + (n.enc ? "\uD83D\uDD12 " : "") + bars(n.rssi) + '</span>';
            b.addEventListener("click", function () { pick(b, n.ssid); });
            nets.appendChild(b);
          });
        })
        .catch(function () {
          nets.innerHTML = '<div class="muted">No se pudo escanear. Escrib&iacute; el nombre a mano.</div>';
        })
        .then(function () {
          scanBtn.disabled = false;
          scanBtn.textContent = "Buscar redes";
        });
    }
    scanBtn.addEventListener("click", scan);
    scan();
  </script>
</body>
</html>
)HTML";

struct NamedKey {
  const char *name;
  uint8_t code;
};

const NamedKey SPECIAL_KEYS[] = {
  {"esc", KEY_ESC},
  {"tab", KEY_TAB},
  {"caps", KEY_CAPS_LOCK},
  {"shift", KEY_LEFT_SHIFT},
  {"ctrl", KEY_LEFT_CTRL},
  {"alt", KEY_LEFT_ALT},
  {"gui", KEY_LEFT_GUI},
  {"backspace", KEY_BACKSPACE},
  {"enter", KEY_RETURN},
  {"space", ' '},
  {"up", KEY_UP_ARROW},
  {"down", KEY_DOWN_ARROW},
  {"left", KEY_LEFT_ARROW},
  {"right", KEY_RIGHT_ARROW},
  {"home", KEY_HOME},
  {"end", KEY_END},
  {"pgup", KEY_PAGE_UP},
  {"pgdn", KEY_PAGE_DOWN},
  {"insert", KEY_INSERT},
  {"delete", KEY_DELETE},
  {"f1", KEY_F1},
  {"f2", KEY_F2},
  {"f3", KEY_F3},
  {"f4", KEY_F4},
  {"f5", KEY_F5},
  {"f6", KEY_F6},
  {"f7", KEY_F7},
  {"f8", KEY_F8},
  {"f9", KEY_F9},
  {"f10", KEY_F10},
  {"f11", KEY_F11},
  {"f12", KEY_F12},
};

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <title>Pico Keyboard Mouse</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #0d1117;
      --panel: #161c24;
      --panel-2: #111820;
      --key: #26313b;
      --key-strong: #344352;
      --key-wide: #3d4957;
      --mouse: #213f3a;
      --down: #ffd166;
      --text: #f5f9fc;
      --muted: #96a5b2;
      --border: #293641;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    html, body {
      margin: 0;
      height: 100%;
      background: var(--bg);
      color: var(--text);
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      touch-action: manipulation;
      user-select: none;
      overflow: hidden;
    }
    body {
      display: flex;
      flex-direction: column;
      padding: 6px;
      gap: 6px;
    }
    header {
      display: flex;
      align-items: center;
      gap: 10px;
      padding: 0 4px;
      flex: 0 0 auto;
    }
    h1 {
      margin: 0;
      font-size: 14px;
      font-weight: 750;
      letter-spacing: 0;
    }
    .build { color: var(--muted); font-weight: 400; font-size: 10px; }
    .status {
      margin-left: auto;
      color: var(--muted);
      font-size: 11px;
      text-align: right;
      max-width: 38vw;
      overflow: hidden;
      text-overflow: ellipsis;
      white-space: nowrap;
    }
    .iconbtn {
      background: var(--panel-2);
      border: 1px solid var(--border);
      color: var(--text);
      padding: 4px 10px;
      border-radius: 6px;
      font-size: 12px;
      cursor: pointer;
    }

    /* Banner de advertencia (datos moviles / orientacion) */
    .banner {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 6px 10px;
      background: #322a10;
      border: 1px solid #6a5523;
      color: #ffd76b;
      border-radius: 7px;
      font-size: 11px;
      line-height: 1.3;
      flex: 0 0 auto;
    }
    .banner.hidden { display: none; }
    .banner button {
      margin-left: auto;
      background: #58481c;
      color: #ffd76b;
      border: 1px solid #8a6f2c;
      padding: 4px 10px;
      border-radius: 5px;
      font-size: 11px;
      min-height: 0;
      box-shadow: none;
    }

    /* Bloque principal: ocupa todo el alto restante */
    .shell {
      display: grid;
      grid-template-columns: minmax(0, 1fr) 220px;
      gap: 6px;
      flex: 1 1 auto;
      min-height: 0;
    }
    .board, .mouse-panel {
      display: flex;
      flex-direction: column;
      gap: 4px;
      padding: 5px;
      border: 1px solid var(--border);
      border-radius: 7px;
      background: var(--panel);
      min-height: 0;
    }
    .board { gap: 4px; }
    .row {
      display: grid;
      grid-template-columns: repeat(15, minmax(0, 1fr));
      gap: 4px;
      flex: 1 1 0;
      min-height: 0;
    }
    .nav-row {
      display: grid;
      grid-template-columns: repeat(6, minmax(0, 1fr));
      gap: 4px;
      flex: 1 1 0;
      min-height: 0;
    }
    button {
      min-width: 0;
      min-height: 0;
      height: 100%;
      border: 0;
      border-radius: 5px;
      background: var(--key);
      color: var(--text);
      font: inherit;
      font-size: clamp(10px, 1.6vw, 14px);
      font-weight: 700;
      letter-spacing: 0;
      box-shadow: inset 0 -2px 0 rgb(0 0 0 / 0.25);
      padding: 0;
    }
    button:active,
    button.down {
      background: var(--down);
      color: #181202;
      box-shadow: none;
    }
    .special { background: var(--key-strong); color: #e1eaf1; font-size: clamp(9px, 1.3vw, 12px); }
    .wide { background: var(--key-wide); }
    .w2 { grid-column: span 2; }
    .w3 { grid-column: span 3; }
    .w4 { grid-column: span 4; }
    .w5 { grid-column: span 5; }
    .w6 { grid-column: span 6; }

    /* Panel del mouse: trackpad chico arriba, botones abajo */
    .mouse-panel { gap: 5px; }
    .mp-label {
      font-size: 9px;
      color: var(--muted);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      padding: 0 2px;
    }
    .trackpad {
      position: relative;
      flex: 0 0 auto;
      height: 32%;
      min-height: 90px;
      max-height: 160px;
      border: 2px dashed #4a6c66;
      border-radius: 7px;
      background:
        linear-gradient(90deg, rgb(255 255 255 / 0.05) 1px, transparent 1px),
        linear-gradient(0deg, rgb(255 255 255 / 0.05) 1px, transparent 1px),
        var(--mouse);
      background-size: 22px 22px;
      overflow: hidden;
      touch-action: none;
    }
    .trackpad::after {
      content: "trackpad";
      position: absolute;
      inset: 50% auto auto 50%;
      transform: translate(-50%, -50%);
      color: rgb(255 255 255 / 0.28);
      font-size: 11px;
      letter-spacing: 1px;
      text-transform: uppercase;
      pointer-events: none;
    }
    .mouse-buttons, .wheel-row {
      display: grid;
      grid-template-columns: repeat(3, minmax(0, 1fr));
      gap: 4px;
      flex: 1 1 0;
      min-height: 0;
    }
    .wheel-row.cols4 { grid-template-columns: repeat(4, minmax(0, 1fr)); }
    .mouse-buttons button,
    .wheel-row button { background: #284b45; font-size: clamp(9px, 1.3vw, 12px); }

    /* Overlay para forzar rotacion a horizontal */
    .rotate-overlay {
      position: fixed;
      inset: 0;
      background: rgb(13 17 23 / 0.96);
      display: none;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      gap: 18px;
      z-index: 9999;
      padding: 24px;
      text-align: center;
    }
    .rotate-overlay .icon {
      font-size: 64px;
      animation: spin 1.6s ease-in-out infinite;
    }
    .rotate-overlay h2 { margin: 0; font-size: 20px; }
    .rotate-overlay p { margin: 0; color: var(--muted); font-size: 13px; max-width: 320px; }
    @keyframes spin {
      0%, 100% { transform: rotate(-90deg); }
      50%      { transform: rotate(0deg); }
    }
    @media (orientation: portrait) {
      .rotate-overlay { display: flex; }
    }

    /* Pantallas muy chicas en horizontal: comprimir aun mas */
    @media (orientation: landscape) and (max-height: 380px) {
      body { padding: 4px; gap: 4px; }
      .board, .mouse-panel { padding: 4px; gap: 3px; }
      .row, .nav-row { gap: 3px; }
      header { font-size: 12px; }
      .banner { font-size: 10px; padding: 4px 8px; }
    }
  </style>
</head>
<body>
  <header>
    <h1>Pico Keyboard Mouse <span class="build">WiFi</span></h1>
    <button class="iconbtn" id="fsBtn" type="button">Fullscreen</button>
    <div id="status" class="status">(JS NO corrio)</div>
  </header>

  <div id="banner" class="banner">
    <span><strong>Red local:</strong> abr&iacute; <strong>http://picokm.local</strong> o la IP <strong id="ipHint">&mdash;</strong>. Para cambiar de red: mant&eacute;n <strong>BOOTSEL</strong> 3s o <a href="/wifi/reset" style="color:#ffd76b">reconfigurar WiFi</a>.</span>
    <button id="bannerOk" type="button">OK, entendido</button>
  </div>

  <div id="errbox" style="display:none;background:#3a1a1a;border:1px solid #8a3a3a;color:#ffb0b0;padding:8px 10px;border-radius:6px;font-size:12px;line-height:1.4;font-family:monospace;white-space:pre-wrap;word-break:break-all;"></div>

  <section class="shell">
    <div class="board" aria-label="Teclado completo">
      <div class="row fn-row">
        <button class="special" data-k="esc">Esc</button>
        <button class="special" data-k="f1">F1</button>
        <button class="special" data-k="f2">F2</button>
        <button class="special" data-k="f3">F3</button>
        <button class="special" data-k="f4">F4</button>
        <button class="special" data-k="f5">F5</button>
        <button class="special" data-k="f6">F6</button>
        <button class="special" data-k="f7">F7</button>
        <button class="special" data-k="f8">F8</button>
        <button class="special" data-k="f9">F9</button>
        <button class="special" data-k="f10">F10</button>
        <button class="special" data-k="f11">F11</button>
        <button class="special" data-k="f12">F12</button>
        <button class="special" data-k="insert">Ins</button>
        <button class="special" data-k="delete">Del</button>
      </div>
        <div class="row">
          <button data-k="`">`</button>
          <button data-k="1">1</button>
          <button data-k="2">2</button>
          <button data-k="3">3</button>
          <button data-k="4">4</button>
          <button data-k="5">5</button>
          <button data-k="6">6</button>
          <button data-k="7">7</button>
          <button data-k="8">8</button>
          <button data-k="9">9</button>
          <button data-k="0">0</button>
          <button data-k="-">-</button>
          <button data-k="=">=</button>
          <button class="special w2" data-hold="backspace">Bksp</button>
        </div>
        <div class="row">
          <button class="special w2" data-k="tab">Tab</button>
          <button data-k="q">Q</button>
          <button data-k="w">W</button>
          <button data-k="e">E</button>
          <button data-k="r">R</button>
          <button data-k="t">T</button>
          <button data-k="y">Y</button>
          <button data-k="u">U</button>
          <button data-k="i">I</button>
          <button data-k="o">O</button>
          <button data-k="p">P</button>
          <button data-k="[">[</button>
          <button data-k="]">]</button>
          <button data-k="backslash">\</button>
        </div>
        <div class="row">
          <button class="special w2" data-k="caps">Caps</button>
          <button data-k="a">A</button>
          <button data-k="s">S</button>
          <button data-k="d">D</button>
          <button data-k="f">F</button>
          <button data-k="g">G</button>
          <button data-k="h">H</button>
          <button data-k="j">J</button>
          <button data-k="k">K</button>
          <button data-k="l">L</button>
          <button data-k=";">;</button>
          <button data-k="'">'</button>
          <button class="special w2" data-k="enter">Enter</button>
        </div>
        <div class="row">
          <button class="special w2" data-hold="shift">Shift</button>
          <button data-k="z">Z</button>
          <button data-k="x">X</button>
          <button data-k="c">C</button>
          <button data-k="v">V</button>
          <button data-k="b">B</button>
          <button data-k="n">N</button>
          <button data-k="m">M</button>
          <button data-k=",">,</button>
          <button data-k=".">.</button>
          <button data-k="/">/</button>
          <button class="special w2" data-hold="shift">Shift</button>
        </div>
        <div class="row">
          <button class="special" data-hold="ctrl">Ctrl</button>
          <button class="special" data-hold="gui">Win</button>
          <button class="special" data-hold="alt">Alt</button>
          <button class="wide w6" data-k="space">Space</button>
          <button class="special" data-hold="alt">Alt</button>
          <button class="special" data-hold="ctrl">Ctrl</button>
          <button class="special" data-hold="left">&larr;</button>
          <button class="special" data-hold="down">&darr;</button>
          <button class="special" data-hold="up">&uarr;</button>
          <button class="special" data-hold="right">&rarr;</button>
        </div>
        <div class="nav-row">
          <button class="special" data-k="home">Home</button>
          <button class="special" data-k="end">End</button>
          <button class="special" data-k="pgup">PgUp</button>
          <button class="special" data-k="pgdn">PgDn</button>
          <button class="special" data-combo="ctrl,alt,delete">Ctrl Alt Del</button>
          <button class="special" data-combo="alt,tab">Alt Tab</button>
        </div>
      </div>

    <div class="mouse-panel" aria-label="Mouse virtual">
      <div class="mp-label">Mouse</div>
      <div id="trackpad" class="trackpad" role="application" aria-label="Trackpad"></div>
      <div class="mouse-buttons">
        <button data-mousehold="left">L</button>
        <button data-mousehold="middle">M</button>
        <button data-mousehold="right">R</button>
      </div>
      <div class="wheel-row">
        <button data-click="left">Click</button>
        <button data-wheel="4">Wh&uarr;</button>
        <button data-wheel="-4">Wh&darr;</button>
      </div>
    </div>
  </section>

  <div class="rotate-overlay" aria-hidden="true">
    <div class="icon">&#x1f4f1;</div>
    <h2>Rot&aacute; el dispositivo</h2>
    <p>Esta interfaz est&aacute; dise&ntilde;ada para usarse en horizontal. Gir&aacute; el tel&eacute;fono para empezar.</p>
  </div>

  <!-- Script #1: instala un handler global de errores ANTES de cargar el script
       principal. Si el principal tiene un error de syntax al parsear, esto lo
       atrapa y muestra la linea y mensaje en el status. -->
  <script>
    try { document.getElementById("status").textContent = "(JS #1 ok)"; } catch (e) {}
    function __showErr(label, payload) {
      try {
        var box = document.getElementById("errbox");
        box.style.display = "block";
        box.textContent = label + "\n" + payload;
        document.getElementById("status").textContent = label;
        document.title = label + " | Pico";
      } catch (e) {}
    }
    window.addEventListener("error", function(ev) {
      var payload = "";
      payload += "lineno: " + (ev.lineno || "?") + "\n";
      payload += "colno:  " + (ev.colno  || "?") + "\n";
      payload += "message: " + (ev.message || "?") + "\n";
      payload += "filename: " + (ev.filename || "?") + "\n";
      if (ev.error && ev.error.stack) payload += "stack: " + ev.error.stack;
      __showErr("ERR", payload);
    });
    window.addEventListener("unhandledrejection", function(ev) {
      var r = ev && ev.reason ? (ev.reason.message || ev.reason) : "?";
      __showErr("REJ", String(r));
    });
  </script>

  <script>
    var statusEl = document.getElementById("status");
    var trackpad = document.getElementById("trackpad");
    var banner   = document.getElementById("banner");
    var fsBtn    = document.getElementById("fsBtn");
    var bannerOk = document.getElementById("bannerOk");
    var queue = Promise.resolve();

    // ===== Banner de "datos moviles" - dismiss persistente =====
    try {
      if (banner && localStorage.getItem("pico-banner-ok") === "1") {
        banner.classList.add("hidden");
      }
    } catch (e) {}
    if (bannerOk) {
      bannerOk.addEventListener("click", function() {
        if (banner) banner.classList.add("hidden");
        try { localStorage.setItem("pico-banner-ok", "1"); } catch (e) {}
      });
    }

    // ===== Fullscreen + lock a landscape (sin async/await, mas compatible) =====
    function toggleFullscreen() {
      try {
        if (!document.fullscreenElement) {
          var req = document.documentElement.requestFullscreen
                 || document.documentElement.webkitRequestFullscreen;
          if (!req) { statusEl.textContent = "fs no soportado"; return; }
          var p = req.call(document.documentElement);
          if (p && p.then) {
            p.then(function() {
              try {
                if (screen.orientation && screen.orientation.lock) {
                  screen.orientation.lock("landscape").catch(function(){});
                }
              } catch (e) {}
              fsBtn.textContent = "Salir";
            }).catch(function() { statusEl.textContent = "fs error"; });
          } else {
            fsBtn.textContent = "Salir";
          }
        } else {
          if (document.exitFullscreen) document.exitFullscreen();
          fsBtn.textContent = "Fullscreen";
        }
      } catch (e) {
        statusEl.textContent = "fs err " + e.message;
      }
    }
    if (fsBtn) fsBtn.addEventListener("click", toggleFullscreen);
    document.addEventListener("fullscreenchange", function() {
      if (fsBtn) fsBtn.textContent = document.fullscreenElement ? "Salir" : "Fullscreen";
    });

    function pulse(button) {
      button.classList.add("down");
      setTimeout(() => button.classList.remove("down"), 90);
    }

    function send(path) {
      statusEl.textContent = "enviando";
      queue = queue
        .catch(() => {})
        .then(() => fetch(path, { cache: "no-store" }))
        .then(() => { statusEl.textContent = "ok"; })
        .catch(() => { statusEl.textContent = "error"; });
    }

    // ========== Botones (teclas + mouse) ==========
    // Sólo enganchamos botones que declaran una acción HID via data-*
    document.querySelectorAll("button").forEach((button) => {
      const ds = button.dataset;
      if (!(ds.k || ds.combo || ds.hold || ds.mousehold || ds.click || ds.dblclick || ds.wheel || ds.move)) {
        return; // skip botones de UI (fsBtn, bannerOk)
      }

      button.addEventListener("pointerdown", (event) => {
        event.preventDefault();
        try { button.setPointerCapture(event.pointerId); } catch (e) {}

        const key       = ds.k;
        const combo     = ds.combo;
        const hold      = ds.hold;
        const mouseHold = ds.mousehold;
        const click     = ds.click;
        const dbl       = ds.dblclick;
        const wheel     = ds.wheel;
        const move      = ds.move;

        if (hold || mouseHold) button.classList.add("down");
        else pulse(button);

        if (key)       send("/key?k="       + encodeURIComponent(key));
        if (combo)     send("/combo?k="     + encodeURIComponent(combo));
        if (hold)      send("/down?k="      + encodeURIComponent(hold));
        if (mouseHold) send("/mouseDown?b=" + encodeURIComponent(mouseHold));
        if (click)     send("/click?b="     + encodeURIComponent(click));
        if (dbl)       send("/click?b="     + encodeURIComponent(dbl) + "&n=2");
        if (wheel)     send("/wheel?v="     + encodeURIComponent(wheel));
        if (move) {
          const p = move.split(",");
          send("/move?dx=" + encodeURIComponent(p[0]) + "&dy=" + encodeURIComponent(p[1]));
        }
      }, { passive: false });

      const release = (event) => {
        if (!ds.hold && !ds.mousehold) return;
        event.preventDefault();
        button.classList.remove("down");
        if (ds.hold)      send("/up?k="      + encodeURIComponent(ds.hold));
        if (ds.mousehold) send("/mouseUp?b=" + encodeURIComponent(ds.mousehold));
      };
      button.addEventListener("pointerup", release, { passive: false });
      button.addEventListener("pointercancel", release, { passive: false });
    });

    // ========== Trackpad ==========
    let trackId = null;
    let lastX = 0, lastY = 0;
    let pendingDx = 0, pendingDy = 0;
    let moving = false;

    const clamp = (v) => Math.max(-127, Math.min(127, Math.round(v)));

    function flushMove() {
      if (moving) return;
      const dx = clamp(pendingDx);
      const dy = clamp(pendingDy);
      if (dx === 0 && dy === 0) return;
      pendingDx -= dx;
      pendingDy -= dy;
      moving = true;
      fetch("/move?dx=" + dx + "&dy=" + dy, { cache: "no-store" })
        .catch(() => {})
        .then(() => {
          moving = false;
          if (Math.abs(pendingDx) >= 1 || Math.abs(pendingDy) >= 1) flushMove();
        });
    }

    trackpad.addEventListener("pointerdown", (event) => {
      event.preventDefault();
      trackId = event.pointerId;
      try { trackpad.setPointerCapture(trackId); } catch (e) {}
      lastX = event.clientX;
      lastY = event.clientY;
    }, { passive: false });

    trackpad.addEventListener("pointermove", (event) => {
      if (event.pointerId !== trackId) return;
      event.preventDefault();
      pendingDx += (event.clientX - lastX) * 1.5;
      pendingDy += (event.clientY - lastY) * 1.5;
      lastX = event.clientX;
      lastY = event.clientY;
      flushMove();
    }, { passive: false });

    const endTrack = (event) => {
      if (event.pointerId !== trackId) return;
      try { trackpad.releasePointerCapture(trackId); } catch (e) {}
      trackId = null;
    };
    trackpad.addEventListener("pointerup", endTrack, { passive: false });
    trackpad.addEventListener("pointercancel", endTrack, { passive: false });

    addEventListener("visibilitychange", () => {
      if (document.hidden) send("/release");
    });

    fetch("/status", { cache: "no-store" })
      .then((r) => r.text())
      .then((t) => {
        if (t.indexOf("ok:") === 0) {
          var ip = t.substring(3);
          statusEl.textContent = ip;
          var ipHint = document.getElementById("ipHint");
          if (ipHint) ipHint.textContent = ip;
        } else {
          statusEl.textContent = "listo";
        }
      })
      .catch(() => { statusEl.textContent = "listo"; });
  </script>
</body>
</html>
)HTML";

static uint32_t littleFsPartitionBytes() {
  return (uint32_t)(FS_END - FS_START);
}

bool findSpecialKey(const String &name, uint8_t &code) {
  for (size_t i = 0; i < sizeof(SPECIAL_KEYS) / sizeof(SPECIAL_KEYS[0]); i++) {
    if (name == SPECIAL_KEYS[i].name) {
      code = SPECIAL_KEYS[i].code;
      return true;
    }
  }
  return false;
}

bool findAsciiAlias(const String &name, char &code) {
  if (name == "backslash") {
    code = '\\';
    return true;
  }
  return false;
}

// Feedback visual: el LED (ligado al chip WiFi en la Pico W) queda apagado en
// reposo y solo destella brevemente en acciones discretas de teclado/mouse.
// NO se usa en el movimiento del trackpad para no cargar el chip WiFi con
// destellos continuos durante el arrastre.
void noteAction(const char *action) {
  Serial.println(action);
  digitalWrite(LED_BUILTIN, HIGH);
  lastBlinkMs = millis();
}

// Registrar una accion sin tocar el LED (para eventos de alta frecuencia).
void noteActionQuiet(const char *action) {
  Serial.println(action);
}

uint8_t mouseButtonByName(String button) {
  button.toLowerCase();
  if (button == "right") return MOUSE_RIGHT;
  if (button == "middle") return MOUSE_MIDDLE;
  return MOUSE_LEFT;
}

int clampMouseDelta(int value) {
  if (value > 127) return 127;
  if (value < -127) return -127;
  return value;
}

void sendKeyByName(String key) {
  key.toLowerCase();

  uint8_t code;
  if (findSpecialKey(key, code)) {
    Keyboard.write(code);
    return;
  }

  char asciiCode;
  if (findAsciiAlias(key, asciiCode)) {
    Keyboard.write(asciiCode);
    return;
  }

  if (key.length() == 1) {
    Keyboard.write(key[0]);
  }
}

void pressKeyByName(String key) {
  key.toLowerCase();

  uint8_t code;
  if (findSpecialKey(key, code)) {
    Keyboard.press(code);
    return;
  }

  char asciiCode;
  if (findAsciiAlias(key, asciiCode)) {
    Keyboard.press(asciiCode);
    return;
  }

  if (key.length() == 1) {
    Keyboard.press(key[0]);
  }
}

void releaseKeyByName(String key) {
  key.toLowerCase();

  uint8_t code;
  if (findSpecialKey(key, code)) {
    Keyboard.release(code);
    return;
  }

  char asciiCode;
  if (findAsciiAlias(key, asciiCode)) {
    Keyboard.release(asciiCode);
    return;
  }

  if (key.length() == 1) {
    Keyboard.release(key[0]);
  }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  if (configMode) {
    server.send_P(200, "text/html; charset=utf-8", CONFIG_HTML);
  } else {
    server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
  }
}

bool ensureLittleFS() {
  static bool ready = false;
  if (ready) return true;

  const uint32_t partBytes = littleFsPartitionBytes();
  if (partBytes == 0) {
    Serial.println("LittleFS: sin particion FS en flash (Flash Size = no FS).");
    Serial.println("  Arduino IDE: Tools -> Flash Size -> 2MB (Sketch: 1984KB, FS: 64KB)");
    return false;
  }

  ready = LittleFS.begin();
  if (!ready) {
    Serial.println("LittleFS: formateando...");
    ready = LittleFS.format() && LittleFS.begin();
  }
  if (!ready) {
    Serial.println("LittleFS: no se pudo montar");
    return false;
  }

  Serial.print("LittleFS: ");
  Serial.print(partBytes);
  Serial.println(" bytes disponibles");
  return true;
}

bool loadWifiCreds(String &ssid, String &pass) {
  ssid = "";
  pass = "";
  if (!ensureLittleFS() || !LittleFS.exists(WIFI_FILE)) return false;

  File file = LittleFS.open(WIFI_FILE, "r");
  if (!file) return false;

  ssid = file.readStringUntil('\n');
  pass = file.readStringUntil('\n');
  ssid.trim();
  pass.trim();
  file.close();
  return ssid.length() > 0;
}

bool saveWifiCreds(const String &ssid, const String &pass) {
  if (!ensureLittleFS()) return false;

  File file = LittleFS.open(WIFI_FILE, "w");
  if (!file) return false;

  file.println(ssid);
  file.println(pass);
  file.close();
  return true;
}

bool clearWifiCreds() {
  if (!ensureLittleFS()) return false;
  if (LittleFS.exists(WIFI_FILE)) LittleFS.remove(WIFI_FILE);
  return true;
}

void startMdns() {
  if (MDNS.begin(MDNS_HOSTNAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("mDNS activo: http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
  } else {
    Serial.println("mDNS: no se pudo iniciar");
  }
}

bool connectToWifi(const String &ssid, const String &pass) {
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_HOSTNAME);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid.c_str(), pass.c_str());

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("No se pudo conectar a la red guardada");
  return false;
}

void startConfigPortal() {
  configMode = true;

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  WiFi.softAP(SETUP_AP_SSID);  // red abierta, sin WPA
  dns.start(53, "*", apIp);

  Serial.println();
  Serial.println("Modo configuracion (AP abierto)");
  Serial.print("SSID: ");
  Serial.println(SETUP_AP_SSID);
  Serial.println("Password: (ninguna)");
  Serial.print("Abrir: http://");
  Serial.println(apIp.toString());
}

void handleWifiFsStatus() {
  if (littleFsPartitionBytes() == 0) {
    server.send(200, "text/plain",
                "err:El firmware no tiene particion de archivos (LittleFS). "
                "Re-flashea PicoKeyboardMouseWiFi.uf2 actualizado del repo.");
    return;
  }
  if (!ensureLittleFS()) {
    server.send(200, "text/plain",
                "err:No se pudo montar LittleFS. Reinicia la Pico o vuelve a flashear el firmware.");
    return;
  }
  server.send(200, "text/plain", "ok");
}

void handleWifiSave() {
  if (!server.hasArg("ssid")) {
    server.send(400, "text/plain", "missing ssid");
    return;
  }

  String ssid = server.arg("ssid");
  String pass = server.hasArg("pass") ? server.arg("pass") : "";
  ssid.trim();
  pass.trim();

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "ssid vacio");
    return;
  }

  if (!saveWifiCreds(ssid, pass)) {
    String err = "No se pudo guardar la configuracion WiFi.";
    if (littleFsPartitionBytes() == 0) {
      err += " El firmware fue compilado sin particion de archivos (Flash Size: no FS). "
             "Re-flashea el PicoKeyboardMouseWiFi.uf2 actualizado del repositorio.";
    } else {
      err += " LittleFS no respondio; reinicia la Pico e intenta de nuevo.";
    }
    server.send(500, "text/html; charset=utf-8",
                "<!doctype html><html><body style='font-family:sans-serif;background:#111;color:#eee;padding:24px'>"
                "<h1>Error al guardar</h1><p>" + err + "</p>"
                "<p><a href='/' style='color:#ffd166'>Volver</a></p></body></html>");
    return;
  }

  server.send(200, "text/html; charset=utf-8",
              "<!doctype html><html><body style='font-family:sans-serif;background:#111;color:#eee;padding:24px'>"
              "<h1>Guardado</h1><p>Reiniciando la Pico para unirse a <strong>" +
              ssid + "</strong>...</p></body></html>");
  delay(800);
  rp2040.restart();
}

void handleWifiReset() {
  clearWifiCreds();
  server.send(200, "text/html; charset=utf-8",
              "<!doctype html><html><body style='font-family:sans-serif;background:#111;color:#eee;padding:24px'>"
              "<h1>WiFi borrada</h1><p>Reiniciando en modo configuracion...</p></body></html>");
  delay(800);
  rp2040.restart();
}

String jsonEscape(const String &in) {
  String out;
  out.reserve(in.length() + 4);
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((uint8_t)c < 0x20) {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

void handleWifiScan() {
  int count = WiFi.scanNetworks();
  if (count < 0) count = 0;

  String json = "[";
  for (int i = 0; i < count; i++) {
    const char *ssid = WiFi.SSID(i);
    if (!ssid || ssid[0] == '\0') continue;  // ocultar redes sin nombre
    if (json.length() > 1) json += ",";
    bool encrypted = WiFi.encryptionType(i) != ENC_TYPE_NONE;
    json += "{\"ssid\":\"";
    json += jsonEscape(String(ssid));
    json += "\",\"rssi\":";
    json += String((int)WiFi.RSSI(i));
    json += ",\"enc\":";
    json += encrypted ? "true" : "false";
    json += "}";
  }
  json += "]";

  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.send(200, "application/json", json);
}

void handleStatus() {
  if (configMode) {
    server.send(200, "text/plain", "setup:" + apIp.toString());
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    server.send(200, "text/plain", "ok:" + WiFi.localIP().toString());
    return;
  }
  server.send(200, "text/plain", "offline");
}

void handleCaptiveProbe() {
  handleRoot();
}

void handleKey() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  noteAction("key");
  sendKeyByName(server.arg("k"));
  server.send(204);
}

void handleDown() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  noteAction("key down");
  pressKeyByName(server.arg("k"));
  server.send(204);
}

void handleUp() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  noteAction("key up");
  releaseKeyByName(server.arg("k"));
  server.send(204);
}

void handleCombo() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  noteAction("combo");
  String combo = server.arg("k");
  int start = 0;
  while (start < combo.length()) {
    int comma = combo.indexOf(',', start);
    String part = comma == -1 ? combo.substring(start) : combo.substring(start, comma);
    part.trim();
    pressKeyByName(part);
    start = comma == -1 ? combo.length() : comma + 1;
  }

  delay(35);
  Keyboard.releaseAll();
  server.send(204);
}

void handleMove() {
  int dx = server.hasArg("dx") ? clampMouseDelta(server.arg("dx").toInt()) : 0;
  int dy = server.hasArg("dy") ? clampMouseDelta(server.arg("dy").toInt()) : 0;
  int wheel = server.hasArg("w") ? clampMouseDelta(server.arg("w").toInt()) : 0;

  noteActionQuiet("mouse move");
  Mouse.move(dx, dy, wheel);
  server.send(204);
}

void handleMouseDown() {
  noteAction("mouse down");
  Mouse.press(mouseButtonByName(server.arg("b")));
  server.send(204);
}

void handleMouseUp() {
  noteAction("mouse up");
  Mouse.release(mouseButtonByName(server.arg("b")));
  server.send(204);
}

void handleMouseClick() {
  noteAction("mouse click");
  uint8_t button = mouseButtonByName(server.arg("b"));
  int count = server.hasArg("n") ? server.arg("n").toInt() : 1;
  if (count < 1) count = 1;
  if (count > 3) count = 3;

  for (int i = 0; i < count; i++) {
    Mouse.click(button);
    delay(45);
  }
  server.send(204);
}

void handleWheel() {
  int value = server.hasArg("v") ? clampMouseDelta(server.arg("v").toInt()) : 0;
  noteAction("mouse wheel");
  Mouse.move(0, 0, value);
  server.send(204);
}

void handleRelease() {
  noteAction("release all");
  Keyboard.releaseAll();
  Mouse.release(MOUSE_LEFT);
  Mouse.release(MOUSE_RIGHT);
  Mouse.release(MOUSE_MIDDLE);
  server.send(204);
}

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void registerServerRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/wifi/save", HTTP_POST, handleWifiSave);
  server.on("/wifi/reset", HTTP_GET, handleWifiReset);
  server.on("/wifi/scan", HTTP_GET, handleWifiScan);
  server.on("/wifi/fsstatus", HTTP_GET, handleWifiFsStatus);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/generate_204", HTTP_GET, handleCaptiveProbe);
  server.on("/hotspot-detect.html", HTTP_GET, handleCaptiveProbe);
  server.on("/connecttest.txt", HTTP_GET, handleCaptiveProbe);
  server.on("/key", HTTP_GET, handleKey);
  server.on("/down", HTTP_GET, handleDown);
  server.on("/up", HTTP_GET, handleUp);
  server.on("/combo", HTTP_GET, handleCombo);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/mouseDown", HTTP_GET, handleMouseDown);
  server.on("/mouseUp", HTTP_GET, handleMouseUp);
  server.on("/click", HTTP_GET, handleMouseClick);
  server.on("/wheel", HTTP_GET, handleWheel);
  server.on("/release", HTTP_GET, handleRelease);
  server.onNotFound(handleNotFound);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Keyboard.begin();
  Mouse.begin();
  Serial.begin(115200);
  delay(200);

  String ssid;
  String pass;
  const bool hasCreds = loadWifiCreds(ssid, pass);
  bool connected = false;

  if (hasCreds) {
    connected = connectToWifi(ssid, pass);
  }

  if (!connected) {
    if (hasCreds) {
      Serial.println("Credenciales guardadas invalidas; modo configuracion");
    } else {
      Serial.println("Sin credenciales guardadas; modo configuracion");
    }
    startConfigPortal();
    ensureLittleFS();
  } else {
    configMode = false;
    startMdns();
  }

  registerServerRoutes();
  server.begin();

  digitalWrite(LED_BUILTIN, LOW);

  Serial.println();
  Serial.println("Pico Keyboard Mouse WiFi listo");
  if (configMode) {
    Serial.println("Modo: portal de configuracion (AP)");
  } else {
    Serial.println("Modo: red local (STA)");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Tambien podes abrir: http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
  }
}

// Mantener presionado BOOTSEL borra las credenciales WiFi y reinicia al portal.
void checkBootselReset() {
  static uint32_t pressedSince = 0;
  static uint32_t lastPoll = 0;
  const uint32_t now = millis();

  // Leer BOOTSEL suspende brevemente el acceso a flash, asi que no lo hacemos
  // en cada iteracion del loop.
  if (now - lastPoll < 40) return;
  lastPoll = now;

  if (BOOTSEL) {
    if (pressedSince == 0) {
      pressedSince = now;
    } else if (now - pressedSince >= BOOTSEL_RESET_MS) {
      Serial.println("BOOTSEL mantenido: borrando credenciales WiFi...");
      clearWifiCreds();
      for (int i = 0; i < 6; i++) {  // parpadeo de confirmacion
        digitalWrite(LED_BUILTIN, LOW);
        delay(80);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(80);
      }
      rp2040.restart();
    }
  } else {
    pressedSince = 0;
  }
}

void loop() {
  if (configMode) {
    dns.processNextRequest();
  }
  server.handleClient();
  checkBootselReset();
  if (lastBlinkMs != 0 && millis() - lastBlinkMs > 45) {
    digitalWrite(LED_BUILTIN, LOW);
    lastBlinkMs = 0;
  }
}
