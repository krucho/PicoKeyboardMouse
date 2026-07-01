# Pico Keyboard Mouse WiFi

**Esta es la versión principal del proyecto.** Misma interfaz que `PicoKeyboardMouse`, pero pensada para usarse en **tu red WiFi local** (modo Station). Las credenciales se guardan en flash (LittleFS) mediante un **portal de configuración** con **escaneo de redes**; no hace falta hardcodear SSID ni contraseña en el sketch.

Una vez conectada, entrás desde cualquier dispositivo de la misma red a **`http://picokm.local`** (vía mDNS), sin necesidad de saber la IP.

## Instalación

### Opción 1 — Flashear el `.uf2` (sin compilar) ✅ recomendado

Este directorio incluye `PicoKeyboardMouseWiFi.uf2` ya compilado. No necesitás Arduino IDE:

1. Con la Pico **desenchufada**, mantené presionado el botón **BOOTSEL**.
2. Sin soltarlo, conectá la Pico por USB. Aparece como una unidad USB llamada **`RPI-RP2`**.
3. Soltá BOOTSEL y **arrastrá `PicoKeyboardMouseWiFi.uf2`** dentro de esa unidad.
4. La Pico se reinicia sola con el firmware cargado.

### Opción 2 — Compilar desde Arduino IDE

Abrí `PicoKeyboardMouseWiFi.ino` (Board: **Raspberry Pi Pico W**, USB Stack: **Pico SDK**) y subí el sketch.

**Flash Size obligatorio:** en **Tools → Flash Size** elegí **`2MB (Sketch: 1984KB, FS: 64KB)`** u otra opción que incluya FS. Con `2MB (no FS)` el portal no puede guardar la WiFi.

## Flujo de uso

### Primera vez (sin credenciales guardadas)

1. Flasheá el firmware (por `.uf2` o compilando).
2. La Pico levanta un AP de configuración:
   - **SSID:** `Pico-Keyboard-Setup`
   - **Password:** ninguna (red abierta)
   - **IP:** `http://192.168.42.1/`
3. Conectate a esa red desde el teléfono.
4. Abrí `http://192.168.42.1/` (Android suele abrir el portal solo).
5. Tocá **Buscar redes** y elegí tu WiFi de la lista (o escribí el SSID a mano). Ingresá la contraseña.
6. Guardá: la Pico almacena los datos, se reinicia y se une a esa red.
7. Desde otro dispositivo de la **misma red**, abrí **`http://picokm.local`**.

### Uso normal

- Teléfono y Pico en la **misma red local**.
- Abrís **`http://picokm.local`** (o la IP) en el navegador.
- El indicador arriba a la derecha y el banner muestran la **IP actual** (`/status`), por si preferís usarla directamente.

### Cambiar de red WiFi

Dos formas de borrar las credenciales y volver al portal `Pico-Keyboard-Setup`:

- **Botón BOOTSEL:** mantenelo presionado **~3 segundos** durante el uso normal. El LED parpadea como confirmación y la Pico reinicia en modo configuración. Ideal cuando no tenés acceso a la web (p. ej. cambiaste de red).
- **Desde la web:** enlace **reconfigurar WiFi** en el banner, o visitá `http://picokm.local/wifi/reset`.

> El BOOTSEL solo entra al modo de flasheo si lo mantenés **mientras enchufás** la Pico. Mantenerlo durante el uso normal borra la config WiFi (no toca el firmware).

## Feedback del LED

El LED de la placa (ligado al chip WiFi en la Pico W) queda **apagado en reposo** y solo **destella brevemente** en acciones discretas de teclado/mouse. **No** parpadea durante el movimiento del trackpad, para no cargar el chip WiFi con destellos continuos.

## Diferencias con `PicoKeyboardMouse` (AP fijo)

| | AP fijo | Esta versión WiFi |
|---|---|---|
| Conexión del teléfono | Directo al AP de la Pico | Misma WiFi que la Pico |
| Credenciales | No aplica | Portal web (con escaneo) + LittleFS |
| Internet en el teléfono | No (salvo datos móviles) | Sí |
| Dirección | Siempre `192.168.42.1` | `picokm.local` o IP por DHCP |

## Transporte

Sigue siendo **HTTP + `fetch()`**, sin WebSockets. Mismos endpoints (`/key`, `/move`, `/mouseDown`, etc.), más `/wifi/scan` (lista de redes en JSON) y `/wifi/reset`.

## Archivos en flash

- `/wifi.cfg` — dos líneas: SSID y contraseña (creado al guardar desde el portal).

## Troubleshooting

- **"No se pudo guardar" al configurar WiFi:** el firmware no tiene partición de archivos (LittleFS). Re-flasheá el `PicoKeyboardMouseWiFi.uf2` del repo, o si compilás: **Tools → Flash Size → 2MB (Sketch: 1984KB, FS: 64KB)** — no uses `2MB (no FS)`.
- **No conecta a mi WiFi después de configurar:** revisá SSID/contraseña; si falla, vuelve sola al modo setup tras 20 s de timeout.
- **`picokm.local` no abre:** algunos dispositivos (Android viejos) no resuelven mDNS. Usá la IP que muestra la página/banner, o buscá `picokm` en la lista de clientes DHCP del router.
- **No sé la IP y no tengo la web:** mantené **BOOTSEL ~3 s** para volver al portal y reconfigurar, o abrí el monitor serie USB (115200 baud) tras el reinicio.
- **El escaneo no muestra redes:** volvé a tocar **Buscar redes** (el escaneo tarda unos segundos), o escribí el SSID a mano.
