# Pico Keyboard Mouse

Variante extrema: teclado completo + trackpad + botones de mouse para Raspberry Pi Pico W.

## Uso

1. En Arduino IDE selecciona `Raspberry Pi Pico W`.
2. En `Tools > USB Stack`, selecciona `Pico SDK`.
3. Carga `PicoKeyboardMouse.ino`.
4. Conectate al Wi-Fi `Pico-KeyboardMouse`.
5. Abri `http://192.168.42.1/`.
6. **Importante en Android**: cuando te conectes al Wi-Fi del Pico (que no
   tiene salida a internet), Android puede dejar **los datos moviles**
   activos en paralelo. Si estan activos, el navegador rutea las requests
   por la red movil y nunca llegan al Pico (solo los `<a href>` directos
   funcionan, las acciones via JS no). **Desactiva los datos moviles**
   mientras uses la pagina y todo va a andar.
7. La interfaz esta pensada para **uso en horizontal**. Hay un boton
   `Fullscreen` que entra a pantalla completa y bloquea la orientacion.

## Que incluye

- Teclado QWERTY completo con F1-F12, numeros, simbolos, modificadores, flechas y navegacion.
- Trackpad web que mueve el cursor USB HID.
- Botones Left, Middle y Right con presionar/soltar.
- Boton Click, Double Click, Scroll Up y Scroll Down.
- Endpoint `/release` para soltar teclado y mouse si se cierra la pagina.

## Endpoints principales

- `/key?k=a`: presiona y suelta una tecla.
- `/down?k=shift`: mantiene una tecla presionada.
- `/up?k=shift`: suelta una tecla.
- `/combo?k=ctrl,alt,delete`: presiona una combinacion y la suelta.
- `/move?dx=12&dy=-8`: mueve el mouse.
- `/mouseDown?b=left`: mantiene presionado un boton del mouse.
- `/mouseUp?b=left`: suelta un boton del mouse.
- `/click?b=left&n=2`: hace click o doble click.
- `/wheel?v=-4`: mueve la rueda.
- `/release`: suelta todo.
