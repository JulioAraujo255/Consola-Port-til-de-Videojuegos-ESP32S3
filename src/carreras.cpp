/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: carreras.cpp
 *  Descripción: "cuerpo" del juego Carreras (interfaz final, a color).
 *
 *  REGLAS:
 *  - Ruta de 3 carriles vista desde arriba. El auto del
 *    jugador está abajo y solo cambia de carril.
 *  - Los autos rivales bajan por carriles al azar.
 *  - Los puntos suben con la distancia recorrida.
 *  - Cada 100 puntos sube el nivel y la ruta va más rápido.
 *  - Se pierde al chocar con un auto rival.
 *
 *  CONTROLES:
 *  - Joystick izquierda / derecha: cambiar de carril
 *    (un carril por cada toque).
 * ============================================================
 */

#include "carreras.h"
#include "pantalla.h"
#include "sonido.h"

// ------------------------------------------------------------
//  1. CONSTANTES DEL JUEGO
// ------------------------------------------------------------

// Colores de Carreras (RGB565)
const uint16_t COLOR_PASTO        = 0x44A6;
const uint16_t COLOR_ASFALTO      = 0x528B;
const uint16_t COLOR_LINEA_BLANCA = 0xF79E;
const uint16_t COLOR_CORDON_ROJO  = 0xE0E3;
const uint16_t COLOR_AUTO_JUGADOR = 0xE0E3;  // Rojo
const uint16_t COLOR_VIDRIO       = 0x969F;  // Celeste
const uint16_t COLOR_RUEDA        = 0x10A2;
const uint16_t COLOR_ARBOL        = 0x1B25;
const uint16_t COLOR_TRONCO       = 0x6A23;
// Los rivales salen con uno de estos colores, al azar
const uint16_t COLORES_RIVALES[3] = { 0x2B7C, 0xFE45, 0xF79E };   // Azul, amarillo, blanco

// Zona de juego (debajo de la barra de puntos)
const int ALTO_BARRA = 11;   // Barra de puntos (filas 0 a 10)
const int Y_AREA     = 11;   // Primera fila de la zona de juego

// Ruta: cordones rojo y blanco de 2 píxeles y dos líneas de carril
const int X_BORDE_IZQUIERDO = 34;
const int X_BORDE_DERECHO   = 125;
const int X_ASFALTO         = 36;    // El asfalto va entre los cordones
const int ANCHO_ASFALTO     = 89;
const int X_LINEA_CARRIL[2] = { 65, 95 };
const int LARGO_RAYA        = 6;    // Raya de 6 px y espacio de 6 px
const int LARGO_CORDON      = 4;    // Tramos rojo / blanco de 4 px
const int CICLO_RAYAS       = 24;   // Múltiplo de 12 (rayas) y de 8 (cordones)
const int CANTIDAD_CARRILES = 3;

// Autos (dibujo de 10 x 14 píxeles)
const int ANCHO_AUTO = 10;
const int ALTO_AUTO  = 14;
const int X_CARRIL[CANTIDAD_CARRILES] = { 45, 75, 105 };   // Columna izquierda del auto
const int Y_JUGADOR  = 108;
const int MAX_RIVALES = 4;

// Separación entre autos rivales (en píxeles recorridos)
const int SEPARACION_MINIMA = 40;
const int SEPARACION_MAXIMA = 80;

// Árboles (dibujo de 7 x 9) a los costados de la ruta
const int ALTO_ARBOL      = 9;
const int CANTIDAD_ARBOLES = 10;
// Recorrido de un árbol antes de volver a aparecer arriba:
// el alto de la zona de juego (116) + el alto del árbol (9)
const int CICLO_ARBOLES   = (ALTO_PANTALLA - Y_AREA) + ALTO_ARBOL;

// Tiempo y velocidad.
// La velocidad está en DÉCIMAS de píxel por cuadro (20 = 2,0 px)
// para poder aumentarla de a poco sin usar decimales.
const unsigned long INTERVALO_CUADRO_MS = 25;   // 40 cuadros por segundo
const int VELOCIDAD_INICIAL  = 20;    // 2,0 píxeles por cuadro
const int VELOCIDAD_MAXIMA   = 50;    // 5,0 píxeles por cuadro
const int AUMENTO_VELOCIDAD  = 5;     // +0,5 por nivel
const int PUNTOS_POR_NIVEL   = 100;
const int PIXELES_POR_PUNTO  = 10;    // Distancia necesaria para sumar 1 punto
// Motor: frecuencia base del ronroneo (Hz), sube con cada nivel
const int MOTOR_BASE_HZ      = 70;
const int MOTOR_POR_NIVEL_HZ = 10;

// ------------------------------------------------------------
//  2. DIBUJOS (SPRITES)
//   X = carrocería   w = vidrios   R = ruedas
//   A = copa del árbol   t = tronco
//   . = fondo: asfalto debajo de los autos, pasto debajo de los árboles
// ------------------------------------------------------------

// Auto visto desde arriba (el frente hacia arriba)
static const char* const AUTO[ALTO_AUTO] = {
  "..XXXXXX..",
  ".XXXXXXXX.",
  "RX.wwww.XR",
  "RX.wwww.XR",
  ".XXXXXXXX.",
  ".XXXXXXXX.",
  ".XXXXXXXX.",
  ".XXXXXXXX.",
  ".XXXXXXXX.",
  ".XXXXXXXX.",
  "RX.wwww.XR",
  "RX.wwww.XR",
  ".XXXXXXXX.",
  "..XXXXXX..",
};

// Pino de dos pisos
static const char* const ARBOL[ALTO_ARBOL] = {
  "...A...",
  "..AAA..",
  ".AAAAA.",
  "AAAAAAA",
  "..AAA..",
  ".AAAAA.",
  "AAAAAAA",
  "...t...",
  "...t...",
};

// ------------------------------------------------------------
//  3. TIPOS DE DATOS
// ------------------------------------------------------------
struct Rival {
  bool activo;      // false = lugar libre en la lista
  int carril;       // 0, 1 o 2
  uint16_t color;   // Color de la carrocería
  int y;            // Fila superior actual
  int yDibujado;    // Dónde se dibujó por última vez
};

struct Arbol {
  int x;
  int y;
  int yDibujado;
};

// ------------------------------------------------------------
//  4. VARIABLES DEL JUEGO
//  "static": privadas de este archivo.
// ------------------------------------------------------------
static int  puntuacion = 0;
static int  nivel      = 1;
static long distancia  = 0;
static int  velocidad  = VELOCIDAD_INICIAL;   // En décimas de píxel por cuadro
static int  restoDecimas = 0;                 // Décimas que sobraron del cuadro anterior

static int carrilJugador = 1;          // Empieza en el carril del medio

static Rival rivales[MAX_RIVALES];
static int distanciaDesdeUltimo = 0;   // Píxeles desde el último rival creado
static int separacionSiguiente  = SEPARACION_MAXIMA;

// Árboles: posición inicial (x, y). Tres columnas a los costados.
static Arbol arboles[CANTIDAD_ARBOLES];
static const int ARBOLES_INICIALES[CANTIDAD_ARBOLES][2] = {
  { 10,  3 }, { 10, 34 }, { 10, 65 }, { 10, 96 },
  { 21, 30 }, { 21, 92 },
  { 140, 18 }, { 140, 49 }, { 140, 80 }, { 140, 111 },
};

static int desplazamientoRayas = 0;    // Para que las líneas de carril "avancen"

// Estado anterior del joystick (para detectar "un toque")
static bool izquierdaAnterior = false;
static bool derechaAnterior   = false;

static unsigned long ultimoCuadro = 0;

// ------------------------------------------------------------
//  5. DIBUJO DE SPRITES
// ------------------------------------------------------------

// Color de cada letra. La carrocería cambia según el auto y
// el fondo según dónde está el dibujo, por eso se reciben.
static uint16_t colorDeLetra(char letra, uint16_t colorCarroceria, uint16_t colorFondo) {
  switch (letra) {
    case 'X': return colorCarroceria;
    case 'w': return COLOR_VIDRIO;
    case 'R': return COLOR_RUEDA;
    case 'A': return COLOR_ARBOL;
    case 't': return COLOR_TRONCO;
    default:  return colorFondo;   // '.'
  }
}

// Dibuja un sprite, INCLUIDO el fondo (así tapa lo anterior).
// - Junta las letras seguidas del mismo color en un solo rectángulo.
// - Recorta las filas que quedan fuera de la zona de juego
//   (los autos y árboles entran por arriba y salen por abajo).
static void dibujarSprite(int x, int y, const char* const filas[], int alto, int ancho,
                          uint16_t colorCarroceria, uint16_t colorFondo) {
  for (int f = 0; f < alto; f++) {
    int yFila = y + f;
    if (yFila < Y_AREA || yFila >= ALTO_PANTALLA) continue;   // Fuera de la zona

    const char* fila = filas[f];
    int inicio = 0;
    while (inicio < ancho) {
      uint16_t color = colorDeLetra(fila[inicio], colorCarroceria, colorFondo);
      int fin = inicio + 1;
      while (fin < ancho && colorDeLetra(fila[fin], colorCarroceria, colorFondo) == color) fin++;
      rectanguloRelleno(x + inicio, yFila, fin - inicio, 1, color);
      inicio = fin;
    }
  }
}

// Pinta un rectángulo del color de fondo, recortado a la zona de juego
static void borrarZona(int x, int y, int ancho, int alto, uint16_t colorFondo) {
  int y0 = max(y, Y_AREA);
  int y1 = min(y + alto, ALTO_PANTALLA);
  if (y1 > y0) {
    rectanguloRelleno(x, y0, ancho, y1 - y0, colorFondo);
  }
}

// Mueve un dibujo hacia abajo: dibuja en la posición nueva y
// borra la franja de arriba que quedó libre
static void moverSpriteHaciaAbajo(int x, int yViejo, int yNuevo,
                                  const char* const filas[], int alto, int ancho,
                                  uint16_t colorCarroceria, uint16_t colorFondo) {
  if (yNuevo > yViejo) {
    borrarZona(x, yViejo, ancho, min(yNuevo - yViejo, alto), colorFondo);
  }
  dibujarSprite(x, yNuevo, filas, alto, ancho, colorCarroceria, colorFondo);
}

// ------------------------------------------------------------
//  6. DIBUJO DEL ESCENARIO
// ------------------------------------------------------------

// --- Barra superior: puntos y nivel ---
static void dibujarMarcador() {
  char texto[16];

  snprintf(texto, sizeof(texto), "PUNTOS:%05d", puntuacion);
  escribirTextoConFondo(2, 2, texto, COLOR_BLANCO, COLOR_BARRA, 1);

  snprintf(texto, sizeof(texto), "NIVEL:%d", nivel);
  int x = ANCHO_PANTALLA - 2 - anchoTexto(texto, 1);
  escribirTextoConFondo(x, 2, texto, COLOR_AMARILLO, COLOR_BARRA, 1);
}

// --- Columna a rayas que "avanza" hacia abajo ---
// Sirve para las líneas de carril (blanco / asfalto) y para los
// cordones (rojo / blanco). Se dibuja por tramos, sin borrar
// antes, así no parpadea.
static void dibujarColumnaRayada(int x, int ancho, int largo,
                                 uint16_t colorRaya, uint16_t colorEspacio) {
  int periodo = 2 * largo;
  // Primera fila del patrón visible (puede empezar antes de la zona)
  int inicio = Y_AREA - ((Y_AREA - desplazamientoRayas) % periodo + periodo) % periodo;
  for (int y = inicio; y < ALTO_PANTALLA; y += periodo) {
    int r0 = max(y, Y_AREA);
    int r1 = min(y + largo, ALTO_PANTALLA);
    if (r1 > r0) rectanguloRelleno(x, r0, ancho, r1 - r0, colorRaya);
    int e0 = max(y + largo, Y_AREA);
    int e1 = min(y + periodo, ALTO_PANTALLA);
    if (e1 > e0) rectanguloRelleno(x, e0, ancho, e1 - e0, colorEspacio);
  }
}

// --- Líneas de carril y cordones ---
static void dibujarRayas() {
  for (int k = 0; k < 2; k++) {
    dibujarColumnaRayada(X_LINEA_CARRIL[k], 1, LARGO_RAYA, COLOR_LINEA_BLANCA, COLOR_ASFALTO);
  }
  dibujarColumnaRayada(X_BORDE_IZQUIERDO, 2, LARGO_CORDON, COLOR_CORDON_ROJO, COLOR_LINEA_BLANCA);
  dibujarColumnaRayada(X_BORDE_DERECHO,   2, LARGO_CORDON, COLOR_CORDON_ROJO, COLOR_LINEA_BLANCA);
}

// --- Auto del jugador ---
static void dibujarJugador() {
  dibujarSprite(X_CARRIL[carrilJugador], Y_JUGADOR, AUTO, ALTO_AUTO, ANCHO_AUTO,
                COLOR_AUTO_JUGADOR, COLOR_ASFALTO);
}

// --- Ruta completa (al empezar o al salir de la pausa) ---
static void dibujarEscenario() {
  rectanguloRelleno(0, 0, ANCHO_PANTALLA, ALTO_BARRA, COLOR_BARRA);
  dibujarMarcador();

  // Pasto de fondo y asfalto en el medio
  rectanguloRelleno(0, Y_AREA, ANCHO_PANTALLA, ALTO_PANTALLA - Y_AREA, COLOR_PASTO);
  rectanguloRelleno(X_ASFALTO, Y_AREA, ANCHO_ASFALTO, ALTO_PANTALLA - Y_AREA, COLOR_ASFALTO);
  dibujarRayas();

  for (int i = 0; i < CANTIDAD_ARBOLES; i++) {
    dibujarSprite(arboles[i].x, arboles[i].y, ARBOL, ALTO_ARBOL, 7, 0, COLOR_PASTO);
    arboles[i].yDibujado = arboles[i].y;
  }
  for (int i = 0; i < MAX_RIVALES; i++) {
    if (rivales[i].activo) {
      dibujarSprite(X_CARRIL[rivales[i].carril], rivales[i].y, AUTO, ALTO_AUTO, ANCHO_AUTO,
                    rivales[i].color, COLOR_ASFALTO);
      rivales[i].yDibujado = rivales[i].y;
    }
  }
  dibujarJugador();
}

// ------------------------------------------------------------
//  7. LÓGICA DEL JUGADOR
// ------------------------------------------------------------

// Cambia de carril con un toque del joystick (izquierda / derecha)
static void moverJugador(const EstadoControles& controles) {
  int carrilNuevo = carrilJugador;
  if (controles.izquierda && !izquierdaAnterior && carrilJugador > 0) {
    carrilNuevo--;
  } else if (controles.derecha && !derechaAnterior && carrilJugador < CANTIDAD_CARRILES - 1) {
    carrilNuevo++;
  }
  izquierdaAnterior = controles.izquierda;
  derechaAnterior   = controles.derecha;

  if (carrilNuevo != carrilJugador) {
    reproducirSonido(SONIDO_CARRIL);
    borrarZona(X_CARRIL[carrilJugador], Y_JUGADOR, ANCHO_AUTO, ALTO_AUTO, COLOR_ASFALTO);
    carrilJugador = carrilNuevo;
    dibujarJugador();
  }
}

// ------------------------------------------------------------
//  8. LÓGICA DE LOS RIVALES Y CHOQUES
// ------------------------------------------------------------

// Crea un rival nuevo arriba de la pantalla, en un carril al azar
static void crearRival() {
  for (int i = 0; i < MAX_RIVALES; i++) {
    if (!rivales[i].activo) {
      rivales[i].carril    = random(CANTIDAD_CARRILES);
      rivales[i].color     = COLORES_RIVALES[random(3)];
      rivales[i].y         = Y_AREA - ALTO_AUTO;   // Todavía no se ve
      rivales[i].yDibujado = rivales[i].y;
      rivales[i].activo    = true;
      return;
    }
  }
}

// Baja los rivales "paso" píxeles y crea uno nuevo cuando se
// recorrió la separación elegida al azar
static void moverRivales(int paso) {
  for (int i = 0; i < MAX_RIVALES; i++) {
    Rival& r = rivales[i];
    if (!r.activo) continue;
    r.y += paso;
    moverSpriteHaciaAbajo(X_CARRIL[r.carril], r.yDibujado, r.y,
                          AUTO, ALTO_AUTO, ANCHO_AUTO, r.color, COLOR_ASFALTO);
    r.yDibujado = r.y;
    if (r.y >= ALTO_PANTALLA) {
      r.activo = false;   // Salió por abajo
    }
  }

  distanciaDesdeUltimo += paso;
  if (distanciaDesdeUltimo >= separacionSiguiente) {
    crearRival();
    distanciaDesdeUltimo = 0;
    separacionSiguiente = random(SEPARACION_MINIMA, SEPARACION_MAXIMA + 1);
  }
}

// Baja los árboles; al salir por abajo vuelven a aparecer arriba
static void moverArboles(int paso) {
  for (int i = 0; i < CANTIDAD_ARBOLES; i++) {
    Arbol& a = arboles[i];
    a.y += paso;
    if (a.y >= ALTO_PANTALLA) {
      borrarZona(a.x, a.yDibujado, 7, ALTO_ARBOL, COLOR_PASTO);   // Borrar lo que quedaba abajo
      a.y -= CICLO_ARBOLES;                          // Vuelve a entrar por arriba
      a.yDibujado = a.y;
    }
    moverSpriteHaciaAbajo(a.x, a.yDibujado, a.y, ARBOL, ALTO_ARBOL, 7, 0, COLOR_PASTO);
    a.yDibujado = a.y;
  }
}

// ¿Algún rival está en el carril del jugador y a su altura?
static bool hayChoque() {
  for (int i = 0; i < MAX_RIVALES; i++) {
    const Rival& r = rivales[i];
    if (r.activo && r.carril == carrilJugador &&
        r.y + ALTO_AUTO > Y_JUGADOR + 1 && r.y < Y_JUGADOR + ALTO_AUTO - 1) {
      return true;
    }
  }
  return false;
}

// Ronroneo del motor según el nivel (más nivel = más agudo)
static void encenderMotor() {
  fijarMotor(MOTOR_BASE_HZ + (nivel - 1) * MOTOR_POR_NIVEL_HZ);
}

// ------------------------------------------------------------
//  9. AVANCE DEL JUEGO
// ------------------------------------------------------------

static void avanzarEscenario() {
  // Cuántos píxeles enteros se avanza en este cuadro
  // (las décimas que sobran se guardan para el siguiente)
  restoDecimas += velocidad;
  int paso = restoDecimas / 10;
  restoDecimas %= 10;
  if (paso == 0) return;

  desplazamientoRayas = (desplazamientoRayas + paso) % CICLO_RAYAS;
  dibujarRayas();
  moverArboles(paso);
  moverRivales(paso);

  // Un rival en el mismo carril puede pisar una fila del jugador
  // justo antes de que se detecte el choque: se redibuja siempre
  // (es un solo auto, no cuesta casi nada)
  dibujarJugador();

  // Puntos por distancia y nivel
  distancia += paso;
  int nuevosPuntos = distancia / PIXELES_POR_PUNTO;
  if (nuevosPuntos != puntuacion) {
    puntuacion = nuevosPuntos;
    int nuevoNivel = 1 + puntuacion / PUNTOS_POR_NIVEL;
    if (nuevoNivel != nivel) {
      nivel = nuevoNivel;
      velocidad = min(VELOCIDAD_INICIAL + (nivel - 1) * AUMENTO_VELOCIDAD, VELOCIDAD_MAXIMA);
      reproducirSonido(SONIDO_NIVEL);
      encenderMotor();
      Serial.printf("Carreras: nivel %d\r\n", nivel);
    }
    dibujarMarcador();
  }
}

// Deja todo listo para una partida nueva
static void reiniciarPartida() {
  puntuacion = 0;
  nivel      = 1;
  distancia  = 0;
  velocidad  = VELOCIDAD_INICIAL;
  restoDecimas = 0;
  carrilJugador = 1;
  desplazamientoRayas = 0;
  izquierdaAnterior = false;
  derechaAnterior   = false;

  for (int i = 0; i < MAX_RIVALES; i++) {
    rivales[i].activo = false;
  }
  distanciaDesdeUltimo = 0;
  separacionSiguiente  = SEPARACION_MAXIMA;   // Un respiro al empezar

  for (int i = 0; i < CANTIDAD_ARBOLES; i++) {
    arboles[i].x = ARBOLES_INICIALES[i][0];
    arboles[i].y = ARBOLES_INICIALES[i][1];
    arboles[i].yDibujado = arboles[i].y;
  }
}

// ------------------------------------------------------------
//  10. FUNCIONES PÚBLICAS (las que usa la consola)
// ------------------------------------------------------------

void comenzarCarreras() {
  reiniciarPartida();
  dibujarEscenario();
  encenderMotor();
  ultimoCuadro = millis();
}

bool actualizarCarreras(const EstadoControles& controles) {
  // El cambio de carril se lee en cada llamada, para no perder toques
  moverJugador(controles);

  if (millis() - ultimoCuadro >= INTERVALO_CUADRO_MS) {
    ultimoCuadro = millis();
    avanzarEscenario();
  }

  if (hayChoque()) {
    Serial.printf("Carreras: choque! Puntuacion: %d\r\n", puntuacion);
    return false;   // La consola hace sonar el choque y espera un instante
  }
  return true;
}

void redibujarCarreras() {
  dibujarEscenario();
  encenderMotor();
  ultimoCuadro = millis();
}

int puntuacionCarreras() {
  return puntuacion;
}