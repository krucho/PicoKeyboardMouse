#include <Keyboard.h>
#include <WebServer.h>
#include <WiFi.h>

// Cambia estos datos antes de compilar si queres.
const char *AP_SSID = "Pico-Keyboard";
const char *AP_PASS = "pico12345";  // 8 caracteres minimo para WPA/WPA2.

WebServer server(80);

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
  <title>Pico Keyboard</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #101417;
      --panel: #171d21;
      --key: #273036;
      --key-strong: #31404a;
      --key-down: #4f9cff;
      --text: #f3f7fa;
      --muted: #98a6af;
      --accent: #ffd166;
    }
    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }
    body {
      margin: 0;
      min-height: 100svh;
      background: var(--bg);
      color: var(--text);
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      touch-action: manipulation;
      user-select: none;
    }
    main {
      width: min(980px, 100%);
      margin: 0 auto;
      padding: 12px;
    }
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 8px 2px 14px;
    }
    h1 {
      margin: 0;
      font-size: 18px;
      font-weight: 750;
      letter-spacing: 0;
    }
    .status {
      min-width: 72px;
      color: var(--muted);
      font-size: 13px;
      text-align: right;
    }
    .pad {
      display: grid;
      gap: 7px;
      padding: 10px;
      border: 1px solid #29333a;
      border-radius: 8px;
      background: var(--panel);
    }
    .row {
      display: grid;
      grid-template-columns: repeat(12, minmax(0, 1fr));
      gap: 7px;
    }
    button {
      min-width: 0;
      min-height: 46px;
      border: 0;
      border-radius: 7px;
      background: var(--key);
      color: var(--text);
      font: inherit;
      font-size: clamp(13px, 2.9vw, 18px);
      font-weight: 650;
      box-shadow: inset 0 -2px 0 rgb(0 0 0 / 0.24);
    }
    button:active,
    button.down {
      background: var(--key-down);
      color: #07111c;
      transform: translateY(1px);
      box-shadow: none;
    }
    .wide { grid-column: span 2; }
    .w3 { grid-column: span 3; }
    .w4 { grid-column: span 4; }
    .w5 { grid-column: span 5; }
    .special { background: var(--key-strong); color: #dbe7ee; }
    .accent { background: var(--accent); color: #1a1402; }
    .spacer { visibility: hidden; }
    @media (max-width: 520px) {
      main { padding: 8px; }
      .pad { gap: 6px; padding: 8px; }
      .row { gap: 6px; }
      button { min-height: 42px; font-size: 13px; }
    }
  </style>
</head>
<body>
  <main>
    <header>
      <h1>Pico Keyboard</h1>
      <div id="status" class="status">listo</div>
    </header>

    <section class="pad" aria-label="Teclado virtual">
      <div class="row">
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
      </div>
      <div class="row">
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
      </div>
      <div class="row">
        <button class="special wide" data-k="tab">Tab</button>
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
      </div>
      <div class="row">
        <button class="special wide" data-k="caps">Caps</button>
        <button data-k="a">A</button>
        <button data-k="s">S</button>
        <button data-k="d">D</button>
        <button data-k="f">F</button>
        <button data-k="g">G</button>
        <button data-k="h">H</button>
        <button data-k="j">J</button>
        <button data-k="k">K</button>
        <button data-k="l">L</button>
        <button class="accent" data-k="enter">Enter</button>
      </div>
      <div class="row">
        <button class="special wide" data-hold="shift">Shift</button>
        <button data-k="z">Z</button>
        <button data-k="x">X</button>
        <button data-k="c">C</button>
        <button data-k="v">V</button>
        <button data-k="b">B</button>
        <button data-k="n">N</button>
        <button data-k="m">M</button>
        <button data-k=",">,</button>
        <button data-k=".">.</button>
        <button class="special" data-hold="backspace">Bksp</button>
      </div>
      <div class="row">
        <button class="special" data-hold="ctrl">Ctrl</button>
        <button class="special" data-hold="alt">Alt</button>
        <button class="special w5" data-k="space">Space</button>
        <button class="special" data-hold="gui">Win</button>
        <button class="special" data-hold="left">&larr;</button>
        <button class="special" data-hold="down">&darr;</button>
        <button class="special" data-hold="up">&uarr;</button>
        <button class="special" data-hold="right">&rarr;</button>
      </div>
      <div class="row">
        <button class="special w3" data-combo="ctrl,alt,delete">Ctrl Alt Del</button>
        <button class="special wide" data-combo="alt,tab">Alt Tab</button>
        <button class="special wide" data-k="home">Home</button>
        <button class="special wide" data-k="end">End</button>
        <button class="special wide" data-k="pgup">PgUp</button>
        <button class="special wide" data-k="pgdn">PgDn</button>
        <button class="special" data-k="delete">Del</button>
      </div>
    </section>
  </main>

  <script>
    const statusEl = document.getElementById("status");
    let queue = Promise.resolve();

    function pulse(button) {
      button.classList.add("down");
      setTimeout(() => button.classList.remove("down"), 90);
    }

    function send(path) {
      statusEl.textContent = "enviando";
      queue = queue
        .catch(() => {})
        .then(() => fetch(path, { cache: "no-store" }))
        .then(() => { statusEl.textContent = "listo"; })
        .catch(() => { statusEl.textContent = "error"; });
    }

    document.querySelectorAll("button").forEach((button) => {
      button.addEventListener("pointerdown", (event) => {
        event.preventDefault();
        button.setPointerCapture(event.pointerId);
        const key = button.dataset.k;
        const combo = button.dataset.combo;
        const hold = button.dataset.hold;
        if (hold) button.classList.add("down");
        else pulse(button);
        if (key) send("/key?k=" + encodeURIComponent(key));
        if (combo) send("/combo?k=" + encodeURIComponent(combo));
        if (hold) send("/down?k=" + encodeURIComponent(hold));
      }, { passive: false });

      const release = (event) => {
        const hold = button.dataset.hold;
        if (!hold) return;
        event.preventDefault();
        button.classList.remove("down");
        send("/up?k=" + encodeURIComponent(hold));
      };

      button.addEventListener("pointerup", release, { passive: false });
      button.addEventListener("pointercancel", release, { passive: false });
    });

    addEventListener("visibilitychange", () => {
      if (document.hidden) send("/release");
    });
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

void sendKeyByName(String key) {
  key.toLowerCase();

  uint8_t code;
  if (findSpecialKey(key, code)) {
    Keyboard.write(code);
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

  if (key.length() == 1) {
    Keyboard.release(key[0]);
  }
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleKey() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  sendKeyByName(server.arg("k"));
  server.send(204);
}

void handleDown() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  pressKeyByName(server.arg("k"));
  server.send(204);
}

void handleUp() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

  releaseKeyByName(server.arg("k"));
  server.send(204);
}

void handleRelease() {
  Keyboard.releaseAll();
  server.send(204);
}

void handleCombo() {
  if (!server.hasArg("k")) {
    server.send(400, "text/plain", "missing k");
    return;
  }

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

void handleNotFound() {
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  Keyboard.begin();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/key", HTTP_GET, handleKey);
  server.on("/down", HTTP_GET, handleDown);
  server.on("/up", HTTP_GET, handleUp);
  server.on("/release", HTTP_GET, handleRelease);
  server.on("/combo", HTTP_GET, handleCombo);
  server.onNotFound(handleNotFound);
  server.begin();

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println("Pico Web HID Keyboard listo");
  Serial.print("AP: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
