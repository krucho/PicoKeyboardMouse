# Pico Web HID Keyboard

Sketch para Raspberry Pi Pico W con Arduino IDE y el community core de Earle Philhower.

La Pico W crea un access point, sirve una web con teclas virtuales y envia teclas al equipo conectado por USB usando HID Keyboard.

## Uso

1. Instala el core `Raspberry Pi Pico/RP2040` de Earle Philhower en Arduino IDE.
2. Selecciona `Tools > Board > Raspberry Pi Pico W`.
3. Selecciona `Tools > USB Stack > Pico SDK`.
4. Abre `PicoWebHIDKeyboard.ino` y carga el sketch.
5. Conecta el telefono o notebook al Wi-Fi `Pico-Keyboard`.
6. Abre `http://192.168.4.1/`.

## Datos del AP

```cpp
const char *AP_SSID = "Pico-Keyboard";
const char *AP_PASS = "pico12345";
```

La clave debe tener al menos 8 caracteres. Cambiala antes de cargar el sketch si el teclado va a quedar cerca de otras personas.

## Latencia

La web usa `pointerdown`, no `click`, para mandar la tecla en el instante del toque. Flechas, modificadores y Backspace usan endpoints de presionar/soltar (`/down` y `/up`), por lo que el sistema operativo del equipo USB puede repetir la tecla mientras la mantenes tocada.

## Endpoints

- `/key?k=a`: presiona y suelta una tecla.
- `/down?k=left`: mantiene una tecla presionada.
- `/up?k=left`: suelta una tecla.
- `/combo?k=ctrl,alt,delete`: presiona una combinacion y la suelta.
- `/release`: suelta todas las teclas.
