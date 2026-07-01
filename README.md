# Pico Keyboard Mouse

Teclado y mouse virtual servidos por una **Raspberry Pi Pico W**. Conectás un teléfono o tablet por WiFi, abrís una página web y controlás por USB HID el equipo al que está enchufada la Pico.

```
  [teléfono / tablet]  ──WiFi──►  [Pico W + web]  ──USB HID──►  [PC / consola / etc.]
```

Proyecto pensado para **Arduino IDE** con el core [arduino-pico](https://github.com/earlephilhower/arduino-pico) (Earle Philhower).

---

## El proyecto es `PicoKeyboardMouseWiFi`

**La versión actual y recomendada del proyecto es [`PicoKeyboardMouseWiFi`](PicoKeyboardMouseWiFi/).** Es la que se usa en el día a día: se conecta a tu red WiFi local, tiene portal de configuración con **escaneo de redes**, acceso por nombre **`picokm.local`** y **reset de WiFi con el botón BOOTSEL**.

El resto de las carpetas son **material de referencia y versiones preliminares** que quedan en el repo para mostrar la evolución del proyecto y ejemplos de uso más simples:

| Carpeta | Estado | Qué hace | Conexión WiFi |
| --- | --- | --- | --- |
| [`PicoKeyboardMouseWiFi`](PicoKeyboardMouseWiFi/) | **Proyecto principal** | Teclado completo + trackpad + botones de mouse, en tu red local | Portal de config + STA (`picokm.local`) |
| [`PicoKeyboardMouse`](PicoKeyboardMouse/) | Referencia | Igual, pero con AP fijo (sin router) | AP fijo (`Pico-KeyboardMouse`) |
| [`PicoWebHIDKeyboard`](PicoWebHIDKeyboard/) | Preliminar | Teclado virtual compacto | AP fijo (`Pico-Keyboard`) |
| [`PicoNumEmojiKeyboard`](PicoNumEmojiKeyboard/) | Preliminar | Teclado numérico + emojis (mapeados a teclas) | AP fijo (`Pico-NumEmoji`) |

> Si solo querés usar el proyecto, andá directo a [`PicoKeyboardMouseWiFi`](PicoKeyboardMouseWiFi/).

---

## Requisitos

- **Hardware:** Raspberry Pi Pico **W** (WiFi obligatorio) + cable USB
- **Software:** Arduino IDE 1.8+ o 2.x (solo si vas a compilar)
- **Core:** [Raspberry Pi Pico/RP2040 by Earle F. Philhower](https://github.com/earlephilhower/arduino-pico)
- **Configuración de placa:**
  - Board: `Raspberry Pi Pico W`
  - USB Stack: `Pico SDK` (necesario para `Keyboard.h` / `Mouse.h`)
  - **Flash Size:** `2MB (Sketch: 1984KB, FS: 64KB)` u otra opción **con FS** (obligatorio para guardar WiFi)

No se usan librerías externas: todo corre con WiFi, WebServer, Keyboard, Mouse y (en la variante WiFi) LittleFS + DNSServer + SimpleMDNS del core.

> **Importante:** si compilás vos, no uses `2MB (no FS)`. Sin partición de archivos, LittleFS no puede guardar `/wifi.cfg` y el portal dirá que no se pudo guardar la config. El `.uf2` incluido en el repo ya viene compilado con FS de 64 KB.

---

## Instalación

### Opción 1 — Flashear el `.uf2` (sin compilar) ✅ recomendado

En [`PicoKeyboardMouseWiFi`](PicoKeyboardMouseWiFi/) se incluye `PicoKeyboardMouseWiFi.uf2` ya compilado. No necesitás Arduino IDE:

1. Con la Pico **desenchufada**, mantené presionado el botón **BOOTSEL**.
2. Sin soltarlo, conectá la Pico por USB a la PC. Aparece como una unidad USB llamada **`RPI-RP2`**.
3. Soltá BOOTSEL y **arrastrá `PicoKeyboardMouseWiFi.uf2`** dentro de esa unidad.
4. La Pico se reinicia sola con el firmware cargado. Listo.

> El botón BOOTSEL solo entra al modo de flasheo cuando lo mantenés **mientras enchufás** la Pico. Durante el uso normal, mantenerlo ~3 s sirve para **borrar la config WiFi** (ver abajo).

### Opción 2 — Compilar desde Arduino IDE

1. Instalá el core arduino-pico y seleccioná Board `Raspberry Pi Pico W` + USB Stack `Pico SDK`.
2. En **Tools → Flash Size**, elegí **`2MB (Sketch: 1984KB, FS: 64KB)`** (u otra con FS; **no** uses `2MB (no FS)`).
3. Abrí `PicoKeyboardMouseWiFi/PicoKeyboardMouseWiFi.ino` y subí el sketch.

---

## Inicio rápido (`PicoKeyboardMouseWiFi`)

1. Flasheá el firmware (por `.uf2` o compilando).
2. La primera vez, la Pico levanta el AP de configuración **`Pico-Keyboard-Setup`** (abierto, sin contraseña).
3. Conectate a esa red y abrí `http://192.168.42.1/` (en Android suele abrirse el portal solo).
4. Tocá **Buscar redes**, elegí tu WiFi de la lista y escribí la contraseña. Guardá.
5. La Pico se reinicia y se une a tu red local.
6. Desde otro dispositivo de la **misma red**, abrí **`http://picokm.local`** (o la IP, que aparece en la propia página y en el banner).

---

## Características (proyecto principal)

- Teclado QWERTY: F1–F12, números, símbolos, modificadores, flechas, navegación
- Combinaciones: Ctrl+Alt+Del, Alt+Tab, etc.
- Trackpad táctil con movimiento del cursor USB
- Botones de mouse: izquierdo, medio, derecho, click, scroll
- Interfaz optimizada para **uso en horizontal** y pantalla completa
- Teclas mantenidas con `/down` y `/up` (Shift, Ctrl, flechas, drag con botón del mouse)
- `/release` al cerrar la pestaña para no dejar teclas trabadas
- **Portal de configuración con escaneo de redes** (elegís el SSID de una lista)
- **Acceso por `picokm.local`** vía mDNS (no necesitás saber la IP)
- **Reset de WiFi con BOOTSEL** (mantener ~3 s) para cambiar de red sin reflashear
- LED de la placa como feedback discreto de teclas (apagado en reposo, sin destellos durante el trackpad)

---

## Cómo funciona

- La Pico levanta un servidor HTTP en el puerto 80.
- La web usa `fetch()` (HTTP GET) — **no WebSockets**.
- Cada acción llama un endpoint (`/key`, `/move`, `/mouseDown`, …) y la Pico emite el evento HID por USB.

Ejemplos:

| Endpoint | Acción |
| --- | --- |
| `/key?k=a` | Pulsa y suelta `a` |
| `/down?k=shift` | Mantiene Shift |
| `/up?k=shift` | Suelta Shift |
| `/combo?k=ctrl,alt,delete` | Combinación momentánea |
| `/move?dx=12&dy=-8` | Mueve el mouse |
| `/mouseDown?b=left` | Mantiene click izquierdo |
| `/click?b=left&n=2` | Doble click |
| `/wheel?v=-4` | Scroll |
| `/release` | Suelta teclado y mouse |
| `/wifi/scan` | Lista las redes WiFi visibles (JSON) |
| `/wifi/reset` | Borra credenciales y vuelve al portal |

---

## Notas importantes

### `picokm.local` (mDNS)

El acceso por nombre funciona en dispositivos con mDNS/Bonjour (Windows 10+, macOS, iOS, Android reciente). Si tu dispositivo no lo resuelve, usá la IP que muestra la página.

### Android y datos móviles (solo variantes con AP fijo)

Si el teléfono está conectado al AP de la Pico **y** tiene datos móviles activos, Chrome puede enviar las peticiones `fetch()` por la red del operador en lugar del WiFi local. **Solución:** desactivar datos móviles, o usar `PicoKeyboardMouseWiFi` en red local (no tiene este problema).

### Layout de teclado

La librería `Keyboard` del core usa layout **US**. Letras y dígitos funcionan bien; algunos símbolos pueden variar si el host tiene layout español u otro.

### Seguridad

Por diseño, quien tenga acceso a la web de la Pico puede tipear y mover el mouse en el host USB. Usá contraseñas de WiFi fuertes y una red de confianza.

---

## Estructura del repositorio

```
PicoKeyboardMouse/
├── README.md
├── PicoKeyboardMouseWiFi/    # ★ Proyecto principal: red local + portal + picokm.local
│   ├── PicoKeyboardMouseWiFi.ino
│   └── PicoKeyboardMouseWiFi.uf2   # binario listo para flashear sin compilar
├── PicoKeyboardMouse/        # Referencia: teclado + mouse, AP fijo
├── PicoWebHIDKeyboard/       # Preliminar: teclado básico, AP fijo
└── PicoNumEmojiKeyboard/     # Preliminar: teclado numérico + emojis
```

Cada carpeta tiene su propio `.ino` y README con detalles específicos.

---

## Licencia

Sin licencia definida por ahora. Usalo bajo tu propio criterio; si publicás un fork, conviene agregar una.
