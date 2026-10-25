# Mini consola ESP32-S3

Cuatro juegos sobre una TFT ST7735 de 160 × 128, gamepad I2C y buzzer: Snake, Dino Run, Carreras y JUMP-MAN. El entorno `simulacion` usa una ILI9341 en Wokwi con el mismo tamaño lógico. La rama `mejoras/jugabilidad-consola` amplía el firmware base sin cambiar los pines.

## Uso

| Pantalla | Controles |
| --- | --- |
| Menú | Arriba/abajo seleccionan juego; A abre opciones; B muestra ayuda |
| Opciones | Arriba/abajo cambian dificultad; en Snake, izquierda/derecha cambian modo; A inicia; B vuelve |
| Partida | C pausa; D alterna sonido (altavoz con ondas o tachado) |
| Pausa | C reanuda con cuenta regresiva; B sale al menú |
| Fin | Arriba/abajo seleccionan reintentar o menú; A confirma; tras 15 s sin uso vuelve al menú |

La cuenta de tres segundos se ejecuta al comenzar y reanudar. Si quedan controles mantenidos, muestra `SUELTA CONTROLES` hasta soltarlos. No avanza la partida durante esa cuenta. El choque dura 600 ms y tiene un marco intermitente; D sigue respondiendo durante la animación.

La salida de la cuenta comprueba primero si los controles están sueltos: el aviso solo aparece si siguen mantenidos durante 250 ms adicionales. Así se evita mostrarlo fugazmente al empezar una partida normal.

En el gamepad físico Emakefun, A es el botón superior (`0x22`), B el derecho (`0x23`), C el inferior (`0x21`) y D el izquierdo (`0x24`). C pausa y reanuda; D solo alterna el sonido. La asignación sigue el [ejemplo del fabricante](https://emakefun-docs.readthedocs.io/zh-cn/latest/sensors/base_modules/joystick_handle/). En Wokwi se conservan los pulsadores C y D en GPIO 17 y 18, respectivamente.

## Mejoras implementadas

### Consola

- Dificultad fácil, normal y difícil, aplicada a velocidad y/o separación de obstáculos.
- Récords en NVS mediante `Preferences`, separados por juego, dificultad y modo de Snake. Solo se escribe cuando se supera un récord. Si NVS no está disponible, la consola continúa con récords en RAM y lo informa por serie.
- Una única fuente de récords: Dino ya no guarda una copia propia. Los marcadores muestran el mayor entre el récord guardado y los puntos actuales.
- Ayuda por juego, indicador de sonido y cuenta regresiva.
- La pantalla de fin vuelve al menú después de 15 segundos de inactividad. Cualquier botón o dirección del joystick reinicia el plazo; un control mantenido también cuenta como actividad. El temporizador solo corre en la pantalla de fin.
- Marcadores de dos filas con etiquetas completas: `Puntaje`, `Nivel`, `Record`, `Combo`, `Vidas` y `Poder`, según el juego. El altavoz gráfico indica sonido activo con ondas y silencio con una cruz.
- Resultados explícitos: partida en curso, derrota y victoria.
- Choque y transiciones sin `delay` en la máquina de estados.
- Todos los juegos y menús componen el cuadro en un lienzo RGB565 estático. Al final de `setup()` y de cada `loop()`, `presentarPantalla()` compara exactamente los tiles tocados con la imagen previamente enviada, omite los que terminaron iguales, fusiona zonas verticales contiguas y recorta cada envío a los píxeles modificados. Los borrados intermedios no se muestran en la TFT. El compositor usa 82.240 bytes de RAM interna para dos imágenes y las marcas de suciedad, sin asignación dinámica. Esto reduce tráfico y ventanas de escritura; no proporciona sincronización con el barrido físico de la TFT, por lo que la ausencia total de artefactos debe comprobarse en la placa.
- Dino y Carreras separan la simulación del dibujo: conservan el resto de tiempo y recuperan hasta cuatro pasos de 25 ms por actualización. Comprueban choques en cada paso y dibujan una sola vez. Si hay un bloqueo mayor, descartan el exceso para evitar una recuperación incontrolable.

### Snake

El tablero utiliza 26 × 17 celdas de 6 píxeles para dejar espacio a las dos filas de marcador.

- Cola de dos giros; cada entrada se valida contra la última dirección aceptada. No permite invertir 180°.
- Comida elegida uniformemente entre las celdas libres, sin bucles de reintentos al llenar el tablero.
- Victoria al ocupar todas las celdas jugables; el laberinto descuenta sus paredes.
- Modos clásico, sin bordes y laberinto. Las paredes del laberinto están presentes desde el inicio y dejan pasos en el centro.
- Cada cinco comidas aparece una manzana dorada: vale tres puntos base y dura 55 movimientos. Al agotarse el plazo se convierte en una manzana normal en el mismo lugar.
- Comer en un máximo de 18 movimientos desde la comida anterior aumenta el multiplicador hasta ×3. Fuera del plazo vuelve a ×1. La primera comida no recibe combo.
- El nivel sube cada cinco comidas, independientemente de los puntos extra. Intervalos iniciales de 300/250/190 ms según dificultad, con mínimo de 80 ms.
- Cabeza iluminada al comer, récord y multiplicador en el marcador. La pausa conserva los plazos de comida y combo porque se cuentan en movimientos.

### Dino Run

- A y joystick arriba usan pulsación inicial: sostener arriba no repite saltos al aterrizar.
- Mantener el control prolonga el salto; soltarlo aumenta la gravedad una vez alcanzada una altura mínima segura. Abajo conserva la caída rápida y el agachado.
- Solicitud de salto conservada hasta el próximo paso, con caducidad de 120 ms; las pulsaciones en el aire se ignoran.
- Piedra normal, roca alta y aves a dos alturas. Las variantes se introducen gradualmente.
- Separación mínima de 170/150/130 px según dificultad, preservando el espacio necesario para aterrizar antes del siguiente obstáculo.
- Cinco puntos por obstáculo superado, una sola vez. La velocidad progresa por distancia, evitando saltos bruscos de dificultad por los bonus.
- Cambio de cielo entre día y noche cada 300 puntos de distancia y polvo al aterrizar. El marcador muestra el nivel efectivo de velocidad.

### Carreras

- Cambio animado de carril: hasta ocho píxeles por paso, con choques calculados según la X real del auto.
- Formaciones de uno o dos rivales. Cada fila deja un carril seguro como máximo a un carril del seguro anterior. Los rivales comparten velocidad para que las filas no se alcancen y cierren el recorrido.
- Separación mínima acotada a 68 px incluso en niveles avanzados; permite pasar entre filas dobles a la velocidad máxima de turbo.
- Arriba acelera, abajo frena y A activa turbo. Carga de 100 unidades, consumo de dos por paso y recuperación de una cada dos pasos cuando se suelta A. La velocidad efectiva se mantiene entre 1,5 y 6 px por paso.
- Cinco puntos por adelantamiento; ocho si se pasó cerca durante la maniobra. La proximidad requiere cercanía horizontal mientras los autos coinciden verticalmente.
- Más probabilidad de filas dobles desde el nivel 3. Tras alcanzar la velocidad base máxima, la progresión se aplica a densidad y separación, con límites.
- Frecuencia de motor acotada por velocidad base, con variación durante turbo. El número de nivel ya no hace crecer indefinidamente su frecuencia.
- El renderizado restaura las líneas de carril al cruzarlas, evitando rastros de asfalto y sprites.

### JUMP-MAN: aventura de plataformas

Juego original de desplazamiento lateral, inspirado en las mecánicas de los plataformas clásicos. Tiene ocho etapas agrupadas en dos mundos de cuatro. Cada mapa mide 768 × 112 píxeles; la cámara muestra una ventana de 160 × 108, debajo del marcador. Los cuatro píxeles inferiores no visibles corresponden al subsuelo.

| Mundo | Etapas |
| --- | --- |
| 1 | Pradera, Grutas, Ruinas, Fortín (jefe) |
| 2 | Bosque, Cristal, Cumbres, Caldera (jefe final) |

Controles: izquierda/derecha para caminar; A o arriba para saltar; mantener el salto permite llegar más alto; B lanza fuego cuando se tiene la flor. C y D mantienen sus funciones comunes. El marcador muestra `Puntaje`, `Vidas`, `Nivel` y `Poder` con su nombre completo (Pequeno, Grande o Fuego).

El avatar anima piernas y brazos al caminar, recoge las piernas al saltar y extiende el brazo durante seis pasos al disparar. El tamaño crecido mide 14 píxeles frente a los 10 del pequeño, dejando margen en pasos de 16 píxeles; los aterrizajes apoyan exactamente los pies en el suelo. Abajo permite agacharse y avanzar más despacio: al soltarlo, solo vuelve a ponerse de pie cuando cabe bajo el techo. Los paneles de etapa se dibujan una vez y se restauran al salir, en lugar de repintarse en cada paso.

El dibujo de JUMP-MAN se omite por completo cuando su estado visible no cambia: compara coordenadas enteras, poses, objetos que aparecen en pantalla, cambios de bloques y datos del marcador. Los enemigos fuera de cámara siguen simulándose, pero no obligan a recomponer una imagen idéntica. El marcador tiene su propia caché. Esto evita recalcular el escenario continuamente al quedarse quieto. La invulnerabilidad mantiene al personaje y al jefe visibles con un cambio estable de color, en lugar de alternar su desaparición; la comprobación del parpadeo residual en la TFT física sigue siendo necesaria.

- Golpear los bloques desde abajo libera hongos, flores o monedas. Cada bloque entrega su recompensa una sola vez y queda usado. Si no hay espacio para otro item, conserva su contenido.
- Los hongos emergen, caminan y pueden caer; las flores permanecen sobre su bloque. El hongo hace crecer y la flor permite disparar. La recogida comprueba que el nuevo tamaño quepa bajo el techo.
- Pisar un enemigo o recoger un hongo/flor muestra `+50` sobre el evento. Los poderes repetidos también suman 50 puntos una sola vez y conservan el poder actual. Hay avisos equivalentes para enemigos eliminados con fuego, monedas (`+25`) y jefes (`+500`). Los números ascienden durante 30 pasos de juego (aproximadamente un segundo), se desplazan con sus coordenadas del mundo y se congelan en pausa. Se dibujan dentro del framebuffer, con contorno para contrastar; nunca sobre la TFT en una pasada separada. Se permiten hasta seis avisos simultáneos, reemplazando el más antiguo si se llena la lista, sin perder puntos.
- El avatar grande rompe ladrillos. El daño quita primero el fuego, luego el tamaño y finalmente una vida; después de perder un poder hay dos segundos de invulnerabilidad.
- Los enemigos patrullan, evitan bordes y pueden eliminarse con pisotones o fuego. Los disparos rebotan en el suelo y están limitados a tres simultáneos, con un tiempo mínimo entre tiros.
- Los huecos de dos o tres tiles y los escalones se pueden superar con el avatar pequeño. Existe un checkpoint sobre suelo firme a mitad de cada etapa. Morir conserva el estado de los bloques al reanudar desde ese checkpoint.
- Los niveles 1-4 y 2-4 cierran una arena. El jefe patrulla, salta y dispara hacia el avatar; su barra de vida aparece arriba de la arena. Se puede vencer con fuego o pisotones. Al llegar a media vida dispara más rápido. El segundo jefe tiene más vida y salta con mayor frecuencia. Morir en la arena reinicia esa etapa y restaura al jefe.
- El jefe bloquea la salida hasta ser derrotado. Tras completar 1-4 se pasa a 2-1; después de 2-4 se registra victoria de campaña. La puntuación y los poderes se conservan entre etapas.
- Vidas iniciales: cuatro en fácil, tres en normal y dos en difícil. La dificultad también ajusta la velocidad de enemigos, vida del jefe y frecuencia de sus ataques.
- Puntuación: moneda 25, item 50, enemigo 50, ladrillo roto 5, checkpoint 50, etapa 200 y jefe 500.
- Las introducciones, muertes y pasos de nivel usan temporizadores de simulación: la pausa los congela. La partida no se guarda al apagar; sí se conserva su récord, separado por dificultad.

El motor corre a pasos de 33 ms, sin asignaciones dinámicas ni PSRAM. Cuando cambia el estado visible, compone una imagen RGB565 de 34.560 bytes y la compara con otra del mismo tamaño para transferir solamente las bandas modificadas al compositor común. La última banda tiene cuatro filas. `dibujarImagenRGB565()` conserva el recorte lógico y el centrado de Wokwi; el envío físico aplica el ajuste opcional de color. El código de niveles y mecánicas está en `src/brasa.cpp`.

## Alcance de esta versión

Esta rama implementa el paquete funcional anterior. Del catálogo inicial de posibilidades quedan para una ampliación: carretera con curvas, rivales con velocidades individuales, aceite/baches, combustible, metas por etapas, rocas dobles/anchas, colisiones de Dino con varias cajas y animaciones específicas de desintegración/derrape. El efecto de choque de esta versión es común a los cuatro juegos. Las velocidades distintas de rivales o una carretera móvil necesitan replantear la garantía de ruta libre antes de incorporarlas.

## Verificación

Compilar ambos entornos con PlatformIO:

```powershell
pio run -e fisico -e simulacion
```

Pruebas de lógica en Windows con Visual Studio Build Tools (C++):

```powershell
./test/host/run.ps1
```

Las siete suites compilan el código real de los juegos, la consola, los controles y el compositor de cuadros con reloj, sonido, pantalla, bus I2C y NVS simulados. Cubren giros, reversas, entrada en la cola móvil, bordes, paredes, tablero completo, caducidad de comida, pausa, saltos variables, aves, ventanas de salto por velocidad, recuperación temporal, desbordamiento del reloj, bonus únicos, turbo, colisiones durante cambios de carril, 10.000 formaciones, rutas entre filas dobles y restauración del framebuffer. La suite de consola comprueba navegación de los cuatro juegos, cuenta regresiva sin aviso fugaz, reanudación sin reinicio, victoria, choque no bloqueante, separación de récords y respaldo en RAM cuando falla NVS. También verifica el dibujo del altavoz y el retorno a los 15 segundos exactos, el reinicio del plazo con actividad y el desbordamiento de `millis()`. La del compositor verifica que los borrados intermedios no lleguen al destino, que los cuadros idénticos no envíen píxeles, el recorte, la fusión vertical y 2.000 cambios distribuidos en patrones mixtos. La de controles ejecuta `controles.cpp` y la consola juntos: introduce pulsaciones en los registros físicos `0x21` y `0x24`, comprueba C para pausar/reanudar sin alterar el sonido, D para silenciar sin reanudar, y la conservación de la partida al continuar.

La suite de JUMP-MAN verifica bloques de un solo uso, crecimiento bajo techos, hongos y flores, límite/recarga de proyectiles, rebotes, daño, invulnerabilidad, checkpoints, jefes, paso entre mundos y victoria/derrota. Una búsqueda de estados usando la física real encuentra rutas de izquierda a derecha en las ocho geometrías con el avatar pequeño y crecido, sin enemigos: valida la accesibilidad del terreno, no constituye una prueba automatizada de combate completo. Para el crecido trata los ladrillos como indestructibles, comprobando que exista una ruta sin necesitar romperlos. Las pruebas separadas verifican pisotones y ataques del jefe, posturas, animaciones y paneles que no se repintan continuamente. También comprueba que los envíos parciales coincidan con el framebuffer completo y genera capturas BMP en `.pio/host-tests/`.

Las regresiones de reposo comprueban 90 pasos consecutivos sin recomponer la escena, transferir imágenes ni volver a dibujar el marcador cuando no hay cambios visibles. Los avisos de puntaje se prueban con poderes repetidos, acreditación única, desplazamiento de cámara, ascenso, pausa, expiración sin rastros y saturación de la lista. También se comprueba que la invulnerabilidad no haga desaparecer al avatar.

Las pruebas de host no verifican latencia I2C, tiempo SPI, audio físico ni persistencia tras un corte real de alimentación. Antes de usar la rama como firmware principal, probar en Wokwi y en la placa:

1. Completar una partida por juego y dificultad; superar un récord, apagar/encender y verificar que se conserva.
2. Verificar dirección del joystick y saltos cortos/largos con el gamepad real.
3. Pausar y reanudar en pleno salto, con comida dorada y con turbo; confirmar que no hay avances durante la cuenta.
4. Comprobar fluidez de Dino y Carreras a máxima dificultad y ausencia de parpadeos en la TFT.
5. Silenciar durante partida, pausa y choque; comprobar que el motor se detiene al salir al menú.
6. En JUMP-MAN, obtener ambos poderes, disparar con B, perder una vida, reanudar desde checkpoint y completar ambos jefes. Validar la fluidez con desplazamiento continuo en la TFT: la compilación y las pruebas de host no miden los FPS del hardware.

La compilación genera los binarios; no carga el firmware a la placa.
