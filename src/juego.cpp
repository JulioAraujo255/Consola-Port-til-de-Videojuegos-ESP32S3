/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: juego.cpp
 *  Descripción: "cuerpo" de la consola (interfaz final, a color).
 *
 *  MÁQUINA DE ESTADOS:
 *  - MENU    --joystick elige, A confirma-->  JUGANDO
 *  - JUGANDO --C-->  PAUSA  --C--> JUGANDO
 *  - PAUSA   --B-->  MENU
 *  - JUGANDO --choque--> FIN
 *  - FIN: joystick elige "Reintentar" o "Menu", A confirma.
 *
 *  Cada juego tiene su propio récord (mientras la consola
 *  está encendida).
 *
 *  SONIDO (FOP-01): la consola hace sonar el menú, la pausa,
 *  el choque y el game over; cada juego hace sonar lo suyo
 *  (comer, saltar, subir de nivel). El botón D silencia o
 *  activa el sonido en cualquier momento.
 * ============================================================
 */

#include "juego.h"
#include "pantalla.h"
#include "snake.h"
#include "dino.h"
#include "carreras.h"
#include "sonido.h"

// ------------------------------------------------------------
//  1. ÍCONOS DEL MENÚ (7 x 7 píxeles, 'h' = píxel pintado)
// ------------------------------------------------------------
static const char* const ICONO_SNAKE[7] = {
  "......h",
  ".....hh",
  "....h..",
  "hhhh...",
  "h......",
  "h......",
  "hhh....",
};
static const char* const ICONO_DINO[7] = {
  ".....hh",
  "..hhhhh",
  ".hhhhhh",
  "hhhhhh.",
  "hhhhh..",
  ".h.h...",
  ".h.h...",
};
static const char* const ICONO_CARRERAS[7] = {
  ".hhhhh.",
  "hh...hh",
  ".hhhhh.",
  ".hhhhh.",
  ".hhhhh.",
  "hh...hh",
  ".hhhhh.",
};

// ------------------------------------------------------------
//  2. TABLA DE JUEGOS
//  Cada fila guarda el nombre del juego, su color e ícono en el
//  menú, y SUS cuatro funciones.
//  Guardar una función en una variable se llama "puntero a
//  función": la consola llama a JUEGOS[i].comenzar() sin
//  saber de qué juego se trata.
//  Un juego todavía no programado tiene "nullptr" (vacío) y
//  en el menú aparece como PROXIMAMENTE.
// ------------------------------------------------------------
struct Juego {
  const char* nombre;
  uint16_t color;                 // Color del botón en el menú
  const char* const* icono;       // Ícono de 7 x 7
  void (*comenzar)();
  bool (*actualizar)(const EstadoControles&);
  void (*redibujar)();
  int  (*puntuacion)();
};

static const Juego JUEGOS[] = {
  { "SNAKE",    COLOR_JUEGO_SNAKE,    ICONO_SNAKE,
    comenzarSnake,    actualizarSnake,    redibujarSnake,    puntuacionSnake },
  { "DINO RUN", COLOR_JUEGO_DINO,     ICONO_DINO,
    comenzarDino,     actualizarDino,     redibujarDino,     puntuacionDino },
  { "CARRERAS", COLOR_JUEGO_CARRERAS, ICONO_CARRERAS,
    comenzarCarreras, actualizarCarreras, redibujarCarreras, puntuacionCarreras },
};

// Cantidad de juegos de la tabla (se calcula sola)
const int CANTIDAD_JUEGOS = sizeof(JUEGOS) / sizeof(JUEGOS[0]);

// Devuelve true si el juego ya está programado
static bool juegoDisponible(int indice) {
  return JUEGOS[indice].comenzar != nullptr;
}

// ------------------------------------------------------------
//  3. VARIABLES DE LA CONSOLA
//  "static": privadas de este archivo.
// ------------------------------------------------------------
static EstadoJuego estadoActual = ESTADO_MENU;

static int juegoActual   = 0;                 // Fila de JUEGOS elegida en el menú
static int records[CANTIDAD_JUEGOS] = { 0 };  // Un récord por juego (FO-12)
static int ultimaPuntuacion = 0;              // Puntos de la última partida

// Pausa al chocar, para ver y oír el choque antes del game over
const unsigned long PAUSA_CHOQUE_MS = 600;

// Pantalla de fin: 0 = Reintentar, 1 = Menu
static int opcionFin = 0;

// Estado anterior del joystick (para detectar "un toque")
static bool arribaAnterior = false;
static bool abajoAnterior  = false;

// ------------------------------------------------------------
//  4. ELEMENTOS DE DIBUJO REUTILIZABLES
// ------------------------------------------------------------

// Fondo azul noche con marco y esquinas decoradas
// (pantallas de menú y fin)
static void dibujarFondoConMarco() {
  rectanguloRelleno(0, 0, ANCHO_PANTALLA, ALTO_PANTALLA, COLOR_FONDO_UI);
  rectanguloBorde(2, 2, ANCHO_PANTALLA - 4, ALTO_PANTALLA - 4, COLOR_MARCO_UI);

  const int ultX = ANCHO_PANTALLA - 5;   // 155
  const int ultY = ALTO_PANTALLA - 5;    // 123
  for (int k = 0; k < 3; k++) {
    int d = 4 + k * 3;
    dibujarLinea(4, 4 + d, 4 + d, 4, COLOR_ADORNO);
    dibujarLinea(ultX, 4 + d, ultX - d, 4, COLOR_ADORNO);
    dibujarLinea(4, ultY - d, 4 + d, ultY, COLOR_ADORNO);
    dibujarLinea(ultX, ultY - d, ultX - d, ultY, COLOR_ADORNO);
  }
}

// Título grande y centrado con una sombra corrida 1 píxel
static void escribirTituloConSombra(int y, const char* texto, uint16_t color) {
  int x = (ANCHO_PANTALLA - anchoTexto(texto, 2)) / 2;
  escribirTexto(x + 1, y + 1, texto, COLOR_ROJO_SOMBRA, 2);
  escribirTexto(x, y, texto, color, 2);
}

// Dibuja un ícono de 7 x 7 en el color indicado
static void dibujarIcono(int x, int y, const char* const icono[], uint16_t color) {
  for (int fila = 0; fila < 7; fila++) {
    for (int col = 0; col < 7; col++) {
      if (icono[fila][col] == 'h') {
        dibujarPixel(x + col, y + fila, color);
      }
    }
  }
}

// Corona de 7 x 5 píxeles (ícono del récord)
static void dibujarCorona(int x, int y) {
  const char* forma[5] = {
    "1..1..1",
    "11.1.11",
    "1111111",
    "1111111",
    ".11111."
  };
  for (int fila = 0; fila < 5; fila++) {
    for (int col = 0; col < 7; col++) {
      if (forma[fila][col] == '1') {
        dibujarPixel(x + col, y + fila, COLOR_AMARILLO);
      }
    }
  }
}

// Escribe "RECORD: N" centrado, con la corona a la izquierda
static void dibujarLineaRecord(int y, int valor, uint16_t colorTexto) {
  char texto[20];
  snprintf(texto, sizeof(texto), "RECORD: %d", valor);
  int x = (ANCHO_PANTALLA - anchoTexto(texto, 1)) / 2 + 5;
  dibujarCorona(x - 11, y);
  escribirTexto(x, y, texto, colorTexto, 1);
}

// ------------------------------------------------------------
//  5. PANTALLAS COMUNES
// ------------------------------------------------------------

// --- Opciones del menú y récord del juego seleccionado ---
// Se redibuja solo esta parte cada vez que se mueve el joystick.
static void dibujarOpcionesMenu() {
  // Borrar la zona de botones y la línea del récord
  rectanguloRelleno(26, 40, 108, 54, COLOR_FONDO_UI);
  rectanguloRelleno(10, 99, ANCHO_PANTALLA - 20, 9, COLOR_FONDO_UI);

  char texto[20];
  for (int i = 0; i < CANTIDAD_JUEGOS; i++) {
    int y = 40 + i * 19;
    const Juego& j = JUEGOS[i];
    if (i == juegoActual) {
      // Opción seleccionada: botón relleno con el color del juego
      rectanguloRedondeado(26, y, 108, 16, 3, j.color, true);
      snprintf(texto, sizeof(texto), "> %s", j.nombre);
      escribirCentrado(y + 4, texto, COLOR_BLANCO, 1);
      dibujarIcono(31, y + 4, j.icono, COLOR_BLANCO);
    } else {
      // Opción sin elegir: solo el borde del color del juego
      rectanguloRedondeado(26, y, 108, 16, 3, j.color, false);
      escribirCentrado(y + 4, j.nombre, COLOR_GRIS_CLARO, 1);
      dibujarIcono(31, y + 4, j.icono, j.color);
    }
  }

  // Debajo: récord del juego elegido, o aviso si aún no existe
  if (juegoDisponible(juegoActual)) {
    dibujarLineaRecord(100, records[juegoActual], COLOR_BLANCO);
  } else {
    escribirCentrado(100, "PROXIMAMENTE", COLOR_GRIS, 1);
  }
}

// --- Pantalla 1: MENU ---
static void dibujarPantallaMenu() {
  dibujarFondoConMarco();

  escribirTituloConSombra(12, "RETRO GAMES", COLOR_AMARILLO);
  dibujarLinea(20, 32, 139, 32, COLOR_MARCO_UI);

  dibujarOpcionesMenu();
  escribirCentrado(113, "MUEVE Y PRESIONA A", COLOR_GRIS, 1);
}

// --- Pantalla 2: PAUSA (se dibuja encima del juego) ---
static void dibujarPantallaPausa() {
  rectanguloRelleno(30, 38, 100, 52, COLOR_FONDO_UI);
  rectanguloBorde(30, 38, 100, 52, COLOR_BLANCO);
  rectanguloBorde(32, 40, 96, 48, COLOR_MARCO_UI);
  escribirTituloConSombra(45, "PAUSA", COLOR_AMARILLO);
  escribirCentrado(66, "C: SEGUIR", COLOR_BLANCO, 1);
  escribirCentrado(77, "B: MENU", COLOR_GRIS, 1);
}

// --- Opciones de la pantalla de fin ---
static void dibujarOpcionesFin() {
  rectanguloRelleno(30, 64, 100, 38, COLOR_FONDO_UI);

  if (opcionFin == 0) {
    rectanguloRedondeado(30, 64, 100, 17, 3, COLOR_AMARILLO, true);
    escribirCentrado(69, "> REINTENTAR", COLOR_NEGRO, 1);
    rectanguloRedondeado(30, 85, 100, 17, 3, COLOR_BLANCO, false);
    escribirCentrado(90, "MENU", COLOR_BLANCO, 1);
  } else {
    rectanguloRedondeado(30, 64, 100, 17, 3, COLOR_BLANCO, false);
    escribirCentrado(69, "REINTENTAR", COLOR_BLANCO, 1);
    rectanguloRedondeado(30, 85, 100, 17, 3, COLOR_AMARILLO, true);
    escribirCentrado(90, "> MENU", COLOR_NEGRO, 1);
  }
}

// --- Pantalla 3: FIN DE PARTIDA ---
static void dibujarPantallaFin() {
  dibujarFondoConMarco();

  escribirTituloConSombra(12, "GAME OVER", COLOR_ROJO);

  char texto[24];
  snprintf(texto, sizeof(texto), "PUNTUACION: %d", ultimaPuntuacion);
  escribirCentrado(36, texto, COLOR_BLANCO, 1);
  dibujarLineaRecord(48, records[juegoActual], COLOR_AMARILLO);

  dibujarOpcionesFin();
  escribirCentrado(110, "MUEVE Y PRESIONA A", COLOR_GRIS, 1);
}

// ------------------------------------------------------------
//  6. CAMBIO DE ESTADO
// ------------------------------------------------------------
static void cambiarEstado(EstadoJuego nuevoEstado) {
  EstadoJuego estadoPrevio = estadoActual;
  estadoActual = nuevoEstado;

  // Fuera de la partida no suena el motor de Carreras
  if (nuevoEstado != ESTADO_JUGANDO) {
    fijarMotor(0);
  }

  switch (nuevoEstado) {
    case ESTADO_MENU:
      Serial.println("Estado: MENU");
      dibujarPantallaMenu();
      break;

    case ESTADO_JUGANDO:
      if (estadoPrevio != ESTADO_PAUSA) {
        // Partida nueva (desde MENU o desde FIN)
        Serial.printf("Estado: JUGANDO %s (partida nueva)\r\n", JUEGOS[juegoActual].nombre);
        JUEGOS[juegoActual].comenzar();
      } else {
        // Se continúa la misma partida
        Serial.printf("Estado: JUGANDO %s (continua)\r\n", JUEGOS[juegoActual].nombre);
        JUEGOS[juegoActual].redibujar();
      }
      break;

    case ESTADO_PAUSA:
      Serial.println("Estado: PAUSA");
      dibujarPantallaPausa();
      break;

    case ESTADO_FIN:
      ultimaPuntuacion = JUEGOS[juegoActual].puntuacion();
      // La fanfarria suena solo al SUPERAR un récord anterior.
      // En la primera partida (récord 0) siempre se "bate" el récord,
      // así que ahí suena la melodía normal de game over.
      if (records[juegoActual] > 0 && ultimaPuntuacion > records[juegoActual]) {
        reproducirSonido(SONIDO_RECORD);       // ¡Récord nuevo!
      } else {
        reproducirSonido(SONIDO_GAME_OVER);
      }
      if (ultimaPuntuacion > records[juegoActual]) {
        records[juegoActual] = ultimaPuntuacion;
      }
      opcionFin = 0;
      Serial.printf("Estado: FIN %s (puntuacion %d, record %d)\r\n",
                    JUEGOS[juegoActual].nombre, ultimaPuntuacion, records[juegoActual]);
      dibujarPantallaFin();
      break;
  }
}

// ------------------------------------------------------------
//  7. FUNCIONES PÚBLICAS
// ------------------------------------------------------------
void iniciarJuego() {
  for (int i = 0; i < CANTIDAD_JUEGOS; i++) {
    records[i] = 0;
  }
  juegoActual = 0;
  iniciarSonido();
  cambiarEstado(ESTADO_MENU);
  reproducirSonido(SONIDO_INICIO);
}

void actualizarJuego(const EstadoControles& controles) {
  // "Un toque" del joystick: ahora está movido y antes no
  bool toqueArriba = controles.arriba && !arribaAnterior;
  bool toqueAbajo  = controles.abajo  && !abajoAnterior;

  // D: silenciar / activar el sonido (en cualquier pantalla)
  if (controles.d.recienPresionado) {
    alternarSilencio();
  }

  switch (estadoActual) {

    case ESTADO_MENU:
      // Mover la selección (al llegar al final vuelve al principio)
      if (toqueArriba) {
        juegoActual = (juegoActual + CANTIDAD_JUEGOS - 1) % CANTIDAD_JUEGOS;
        dibujarOpcionesMenu();
        reproducirSonido(SONIDO_MENU_MOVER);
      } else if (toqueAbajo) {
        juegoActual = (juegoActual + 1) % CANTIDAD_JUEGOS;
        dibujarOpcionesMenu();
        reproducirSonido(SONIDO_MENU_MOVER);
      }
      // A: entrar al juego (solo si ya está programado)
      if (controles.a.recienPresionado && juegoDisponible(juegoActual)) {
        reproducirSonido(SONIDO_MENU_ELEGIR);
        cambiarEstado(ESTADO_JUGANDO);
      }
      break;

    case ESTADO_JUGANDO:
      if (controles.c.recienPresionado) {
        reproducirSonido(SONIDO_PAUSA);
        cambiarEstado(ESTADO_PAUSA);
      }
      // El juego avanza; si devuelve false, chocó: se oye el choque,
      // se espera un instante (sin cortar el sonido) y va al game over
      else if (!JUEGOS[juegoActual].actualizar(controles)) {
        fijarMotor(0);
        reproducirSonido(SONIDO_CHOQUE);
        esperarConSonido(PAUSA_CHOQUE_MS);
        cambiarEstado(ESTADO_FIN);
      }
      break;

    case ESTADO_PAUSA:
      if (controles.c.recienPresionado) {
        reproducirSonido(SONIDO_PAUSA);
        cambiarEstado(ESTADO_JUGANDO);
      } else if (controles.b.recienPresionado) {
        cambiarEstado(ESTADO_MENU);
      }
      break;

    case ESTADO_FIN:
      if (toqueArriba || toqueAbajo) {
        opcionFin = 1 - opcionFin;
        dibujarOpcionesFin();
        reproducirSonido(SONIDO_MENU_MOVER);
      }
      if (controles.a.recienPresionado) {
        reproducirSonido(SONIDO_MENU_ELEGIR);
        cambiarEstado(opcionFin == 0 ? ESTADO_JUGANDO : ESTADO_MENU);
      }
      break;
  }

  arribaAnterior = controles.arriba;
  abajoAnterior  = controles.abajo;
}