#include <Keyboard.h>
#include <WebServer.h>
#include <WiFi.h>

const char *AP_SSID = "Pico-NumEmoji";
const char *AP_PASS = "pico12345";  // 8 caracteres minimo para WPA/WPA2.

IPAddress apIp(192, 168, 42, 1);
IPAddress apGateway(192, 168, 42, 1);
IPAddress apSubnet(255, 255, 255, 0);

WebServer server(80);

struct NamedKey {
  const char *name;
  uint8_t code;
};

const NamedKey SPECIAL_KEYS[] = {
  {"backspace", KEY_BACKSPACE},
  {"enter", KEY_RETURN},
  {"space", ' '},
  {"tab", KEY_TAB},
  {"esc", KEY_ESC},
  {"up", KEY_UP_ARROW},
  {"down", KEY_DOWN_ARROW},
  {"left", KEY_LEFT_ARROW},
  {"right", KEY_RIGHT_ARROW},
};

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="es">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
  <title>Pico Num Emoji</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #0e1116;
      --panel: #171c23;
      --key: #28313a;
      --key-strong: #344250;
      --number: #263f5e;
      --emoji: #3d3657;
      --down: #ffd166;
      --text: #f7fbff;
      --muted: #9caab7;
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
      width: min(720px, 100%);
      margin: 0 auto;
      padding: 12px;
    }
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 8px 2px 12px;
    }
    h1 {
      margin: 0;
      font-size: 18px;
      font-weight: 750;
      letter-spacing: 0;
    }
    .status {
      color: var(--muted);
      font-size: 13px;
      text-align: right;
    }
    .panel {
      display: grid;
      gap: 12px;
      padding: 10px;
      border: 1px solid #29333c;
      border-radius: 8px;
      background: var(--panel);
    }
    .grid {
      display: grid;
      gap: 7px;
    }
    .digits {
      grid-template-columns: repeat(5, minmax(0, 1fr));
    }
    .numpad {
      grid-template-columns: repeat(4, minmax(0, 1fr));
    }
    .emoji-grid {
      grid-template-columns: repeat(3, minmax(0, 1fr));
    }
    button {
      position: relative;
      min-width: 0;
      min-height: 58px;
      border: 0;
      border-radius: 7px;
      background: var(--key);
      color: var(--text);
      font: inherit;
      font-size: 22px;
      font-weight: 750;
      box-shadow: inset 0 -2px 0 rgb(0 0 0 / 0.28);
    }
    button:active,
    button.down {
      background: var(--down);
      color: #171103;
      transform: translateY(1px);
      box-shadow: none;
    }
    .num { background: var(--number); }
    .special { background: var(--key-strong); font-size: 15px; }
    .emoji {
      background: var(--emoji);
      min-height: 74px;
      font-size: 34px;
    }
    .emoji small {
      position: absolute;
      right: 9px;
      bottom: 7px;
      color: var(--muted);
      font-size: 11px;
      font-weight: 800;
      letter-spacing: 0;
    }
    .emoji:active small,
    .emoji.down small {
      color: #4d3900;
    }
    .wide { grid-column: span 2; }
    .tall { grid-row: span 2; min-height: 123px; }
    @media (max-width: 460px) {
      main { padding: 8px; }
      .panel { gap: 10px; padding: 8px; }
      .grid { gap: 6px; }
      button { min-height: 52px; font-size: 19px; }
      .emoji { min-height: 68px; font-size: 31px; }
      .special { font-size: 13px; }
    }
  </style>
</head>
<body>
  <main>
    <header>
      <h1>Pico Num Emoji</h1>
      <div id="status" class="status">listo</div>
    </header>

    <section class="panel" aria-label="Teclado numerico y emojis">
      <div class="grid digits" aria-label="Numeros">
        <button class="num" data-k="1">1</button>
        <button class="num" data-k="2">2</button>
        <button class="num" data-k="3">3</button>
        <button class="num" data-k="4">4</button>
        <button class="num" data-k="5">5</button>
        <button class="num" data-k="6">6</button>
        <button class="num" data-k="7">7</button>
        <button class="num" data-k="8">8</button>
        <button class="num" data-k="9">9</button>
        <button class="num" data-k="0">0</button>
      </div>

      <div class="grid numpad" aria-label="Numpad">
        <button class="num" data-k="7">7</button>
        <button class="num" data-k="8">8</button>
        <button class="num" data-k="9">9</button>
        <button class="special" data-hold="backspace">Bksp</button>
        <button class="num" data-k="4">4</button>
        <button class="num" data-k="5">5</button>
        <button class="num" data-k="6">6</button>
        <button class="special tall" data-k="enter">Enter</button>
        <button class="num" data-k="1">1</button>
        <button class="num" data-k="2">2</button>
        <button class="num" data-k="3">3</button>
        <button class="num wide" data-k="0">0</button>
        <button class="num" data-k=".">.</button>
      </div>

      <div class="grid emoji-grid" aria-label="Emojis">
        <button class="emoji" data-k="q">&#128512;<small>Q</small></button>
        <button class="emoji" data-k="w">&#128526;<small>W</small></button>
        <button class="emoji" data-k="e">&#128293;<small>E</small></button>
        <button class="emoji" data-k="r">&#10084;&#65039;<small>R</small></button>
        <button class="emoji" data-k="t">&#128077;<small>T</small></button>
        <button class="emoji" data-k="y">&#127919;<small>Y</small></button>
        <button class="emoji" data-k="u">&#9889;<small>U</small></button>
        <button class="emoji" data-k="i">&#10024;<small>I</small></button>
        <button class="emoji" data-k="o">&#9989;<small>O</small></button>
      </div>

      <div class="grid digits" aria-label="Controles">
        <button class="special" data-hold="left">&larr;</button>
        <button class="special" data-hold="down">&darr;</button>
        <button class="special" data-hold="up">&uarr;</button>
        <button class="special" data-hold="right">&rarr;</button>
        <button class="special" data-k="space">Space</button>
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
        const hold = button.dataset.hold;
        if (hold) button.classList.add("down");
        else pulse(button);
        if (key) send("/key?k=" + encodeURIComponent(key));
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
  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  WiFi.softAP(AP_SSID, AP_PASS);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/key", HTTP_GET, handleKey);
  server.on("/down", HTTP_GET, handleDown);
  server.on("/up", HTTP_GET, handleUp);
  server.on("/release", HTTP_GET, handleRelease);
  server.onNotFound(handleNotFound);
  server.begin();

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println();
  Serial.println("Pico Num Emoji Keyboard listo");
  Serial.print("AP: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}
