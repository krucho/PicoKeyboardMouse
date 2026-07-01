#include <Keyboard.h>
#include <Mouse.h>
#include <WebServer.h>
#include <WiFi.h>

const char *AP_SSID = "Pico-KeyboardMouse";
const char *AP_PASS = "pico12345";  // 8 caracteres minimo para WPA/WPA2.

IPAddress apIp(192, 168, 42, 1);
IPAddress apGateway(192, 168, 42, 1);
IPAddress apSubnet(255, 255, 255, 0);

WebServer server(80);
uint32_t lastBlinkMs = 0;

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
    <h1>Pico Keyboard Mouse <span class="build">v10</span></h1>
    <button class="iconbtn" id="fsBtn" type="button">Fullscreen</button>
    <div id="status" class="status">(JS NO corrio)</div>
  </header>

  <div id="banner" class="banner">
    <span><strong>Importante:</strong> en Android, desactiv&aacute; los <strong>datos m&oacute;viles</strong> mientras us&aacute;s esta p&aacute;gina. Si est&aacute;n activos, el navegador rutea el tr&aacute;fico por la red de tu operador y no llega al Pico (s&oacute;lo los <code>&lt;a href&gt;</code> directos funcionan).</span>
    <button id="bannerOk" type="button">OK, ya lo desactiv&eacute;</button>
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

    statusEl.textContent = "listo";
  </script>
</body>
</html>
)HTML";

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

void noteAction(const char *action) {
  Serial.println(action);
  digitalWrite(LED_BUILTIN, LOW);
  lastBlinkMs = millis();
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
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
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

  noteAction("mouse move");
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

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Keyboard.begin();
  Mouse.begin();
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", HTTP_GET, handleRoot);
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
  server.begin();

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println("Pico Keyboard Mouse listo");
  Serial.print("AP: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  if (lastBlinkMs != 0 && millis() - lastBlinkMs > 45) {
    digitalWrite(LED_BUILTIN, HIGH);
    lastBlinkMs = 0;
  }
}
