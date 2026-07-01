# Pico Num Emoji Keyboard

Variante del teclado web HID para Raspberry Pi Pico W.

Mantiene la misma logica del sketch original:

- Pico W en modo AP.
- Web local servida por la Pico.
- Botones web que mandan teclas por USB HID.
- `pointerdown` para baja latencia.
- `/down` y `/up` para teclas que se mantienen presionadas.

## Uso

1. Selecciona `Raspberry Pi Pico W` en Arduino IDE.
2. Selecciona `Tools > USB Stack > Pico SDK`.
3. Carga `PicoNumEmojiKeyboard.ino`.
4. Conectate al Wi-Fi `Pico-NumEmoji`.
5. Abri `http://192.168.42.1/`.

## Mapeo emoji a tecla

Los botones muestran emojis, pero mandan letras:

| Emoji | Tecla enviada |
| --- | --- |
| Cara feliz | `q` |
| Cara con lentes | `w` |
| Fuego | `e` |
| Corazon | `r` |
| Pulgar arriba | `t` |
| Diana | `y` |
| Rayo | `u` |
| Brillos | `i` |
| Check | `o` |

Para cambiar el mapeo, edita los `data-k` de los botones `.emoji` en el HTML embebido.
