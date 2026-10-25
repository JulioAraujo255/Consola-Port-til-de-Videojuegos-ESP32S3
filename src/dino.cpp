/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: dino.cpp
 *  Descripción: "cuerpo" del juego Dino Run (interfaz final, a color).
 *
 *  REGLAS:
 *  - El triceratops corre solo; hay que saltar las piedras y
 *    agacharse (o saltar) ante las aves.
 *  - Los puntos suben con la distancia recorrida.
 *  - Cada 100 puntos aumenta la velocidad del juego, y el
 *    salto se vuelve más rápido en la misma proporción
 *    (misma altura, menos tiempo en el aire).
 *  - Las aves aparecen a partir de los 100 puntos.
 *  - Se pierde al chocar con un obstáculo.
 *
 *  CONTROLES:
 *  - A o joystick arriba: saltar.
 *  - Joystick abajo: agacharse (en el aire, baja más rápido).
 * ============================================================
 */

#include "dino.h"
#include "pantalla.h"
#include "sonido.h"
#include "interfaz.h"
#include <math.h>

// ------------------------------------------------------------
//  1. CONSTANTES DEL JUEGO
// ------------------------------------------------------------

// Colores de Dino Run (RGB565)
const uint16_t COLOR_CIELO        = 0x8E5E;
const uint16_t COLOR_SOL          = 0xFEE7;
const uint16_t COLOR_NUBE         = 0xF7DF;
const uint16_t COLOR_CERRO_LEJANO = 0xCCAC;
const uint16_t COLOR_CERRO_CERCA  = 0xB40A;
const uint16_t COLOR_ARENA        = 0xEE50;
const uint16_t COLOR_ARENA_OSC    = 0xCD0B;
const uint16_t COLOR_DINO_CUERPO  = 0x4508;  // Verde
const uint16_t COLOR_DINO_PATAS   = 0x33C6;  // Verde oscuro
const uint16_t COLOR_DINO_GORGUERA = 0xCB67; // Naranja
const uint16_t COLOR_DINO_CUERNO  = 0xFF9A;  // Marfil
const uint16_t COLOR_PIEDRA       = 0x8410;
const uint16_t COLOR_PIEDRA_OSC   = 0x5ACB;
const uint16_t COLOR_AVE_ALA      = 0x79E5;  // Marrón
const uint16_t COLOR_AVE_CUERPO   = 0x9286;
const uint16_t COLOR_AVE_PICO     = 0xFDA0;  // Naranja

// Escenario
const int ALTO_BARRA = ALTO_MARCADOR;
const int Y_SUELO    = 104;  // Línea del suelo (las patas se apoyan aquí)

// Todos los sprites se dibujan al doble de tamaño
const int ESCALA = 2;

// Triceratops
const int ANCHO_DINO    = 22;                 // Columnas del dibujo
const int X_DINO        = 14;                 // No se mueve en horizontal
const int ALTO_PARADO   = 11 * ESCALA;        // 22 píxeles
const int ALTO_AGACHADO = 7  * ESCALA;        // 14 píxeles

// Obstáculos
const int ANCHO_PIEDRA = 10;   // Columnas del dibujo (20 píxeles)
const int ALTO_PIEDRA  = 5;    // Filas del dibujo   (10 píxeles)
const int ANCHO_AVE    = 9;    // Columnas (18 píxeles)
const int ALTO_AVE     = 4;    // Filas    (8 píxeles)
const int Y_AVE        = 82;   // A esta altura choca si está parado, pero no agachado
const int MAX_OBSTACULOS = 3;  // Como mucho 3 en pantalla a la vez

// Separación entre obstáculos (en píxeles recorridos)
const int SEPARACION_MINIMA = 130;
const int SEPARACION_MAXIMA = 220;
const int PUNTOS_PARA_AVES  = 100;   // Desde aquí pueden aparecer aves
const int PROBABILIDAD_AVE  = 35;    // % de que el obstáculo sea un ave

// Física del salto (en píxeles por cuadro, a la velocidad inicial)
const float IMPULSO_SALTO  = 4.8f;   // Velocidad hacia arriba al saltar
const float GRAVEDAD       = 0.35f;  // Cuánto frena la subida en cada cuadro
const float GRAVEDAD_EXTRA = 3.0;   // Multiplicador si se mantiene abajo en el aire

// Tiempo y velocidad.
// La velocidad está en DÉCIMAS de píxel por cuadro (30 = 3,0 px)
// para poder aumentarla de a poco sin usar decimales.
const unsigned long INTERVALO_CUADRO_MS = 25;   // 40 cuadros por segundo
const int VELOCIDAD_INICIAL  = 30;    // 3,0 píxeles por cuadro
const int VELOCIDAD_MAXIMA   = 45;    // 4,5 píxeles por cuadro
const int AUMENTO_VELOCIDAD  = 5;     // +0,5 píxeles por cuadro...
const int PUNTOS_POR_AUMENTO = 100;   // ...cada 100 puntos
const int CUADROS_POR_PASO   = 4;     // Cada cuántos cuadros cambian las patas
const int PIXELES_POR_PUNTO  = 12;    // Distancia necesaria para sumar 1 punto

// ------------------------------------------------------------
//  2. DIBUJOS (SPRITES)
//  Cada letra es un "píxel" del dibujo:
//   B / H = cuerpo y cabeza (verde)   F = gorguera (verde medio)
//   L = patas   T = cola              h = cuernos / pico (amarillo)
//   e = ojo (negro)                   . = fondo (negro)
//   R / r = piedra (claro / oscuro)   W = alas   C = cuerpo del ave
//   p = pico del ave
//  El fondo '.' NO es negro: se pinta el cielo, cerro o arena
//  que corresponde a ese lugar (ver pintarFondo).
// ------------------------------------------------------------

// Triceratops parado: 8 filas de cuerpo + 3 filas de patas
static const char* const CUERPO_PARADO[8] = {
  "...............FF.....",
  "..............FFFF....",
  ".....BBBBBBB.FFFFFhhh.",
  "...BBBBBBBBBBFFFFHHH..",
  "..BBBBBBBBBBBBFFHHeHH.",
  ".BBBBBBBBBBBBBBBHHHHHh",
  "TBBBBBBBBBBBBBBB.HHH..",
  "TT.BBBBBBBBBBBB.......",
};

// Triceratops agachado: 5 filas de cuerpo + 2 filas de patas
static const char* const CUERPO_AGACHADO[5] = {
  ".............FFFFhhh..",
  "...BBBBBBBBBBFFFHHeHH.",
  "..BBBBBBBBBBBBBBHHHHHh",
  "TBBBBBBBBBBBBBBB.HHH..",
  "TT.BBBBBBBBBBBB.......",
};

// Patas: en el aire (las cuatro quietas) y los dos pasos al correr
static const char* const PATAS_AIRE[3] = {
  "...LL...LL..LL.LL.....",
  "...LL...LL..LL.LL.....",
  "..LLL..LLL.LLL.LLL....",
};
static const char* const PATAS_PASO_1[3] = {
  "...LL...LL..LL.LL.....",
  "...LL.......LL........",
  "..LLL......LLL........",
};
static const char* const PATAS_PASO_2[3] = {
  "...LL...LL..LL.LL.....",
  "........LL......LL....",
  ".......LLL.....LLL....",
};

// Piedra
static const char* const PIEDRA[ALTO_PIEDRA] = {
  "...RRRR...",
  ".RRRRRRR..",
  "RRRrRRRRR.",
  "RRRRRRrRRR",
  "RRRRRRRRRR",
};

static const char* const ROCA_ALTA[7] = {
  "....RR....", "...RRRR...", "..RRRRRR..", ".RRRrRRRR.",
  "RRRRRRRRR.", "RRRRRRrRRR", "RRRRRRRRRR"
};

// Ave (mira a la izquierda): alas arriba y alas extendidas
static const char* const AVE_ALAS_ARRIBA[ALTO_AVE] = {
  "W.......W",
  ".WW...WW.",
  "..WWCWW..",
  ".pCCC....",
};
static const char* const AVE_ALAS_EXTENDIDAS[ALTO_AVE] = {
  ".........",
  ".........",
  "WWWWCWWWW",
  ".pCCC....",
};

// ------------------------------------------------------------
//  3. TIPOS DE DATOS
// ------------------------------------------------------------
enum TipoObstaculo {
  OBSTACULO_PIEDRA,
  OBSTACULO_AVE,
  OBSTACULO_ROCA_ALTA,
  OBSTACULO_AVE_ALTA
};

struct Obstaculo {
  bool activo;          // false = lugar libre en la lista
  TipoObstaculo tipo;
  int x;                // Posición actual (columna izquierda, en píxeles)
  int xDibujado;        // Dónde se dibujó por última vez
  bool superado;
};

// ------------------------------------------------------------
//  4. VARIABLES DEL JUEGO
//  "static": privadas de este archivo.
// ------------------------------------------------------------
static int  puntuacion = 0;
static int bonus = 0;
static bool arribaAnterior = false;
static unsigned long momentoSalto = 0;
static int polvoRestante = 0;
static bool nocheDibujada = false;
static long distancia  = 0;          // Píxeles recorridos en la partida
static int  velocidad  = VELOCIDAD_INICIAL;   // En décimas de píxel por cuadro
static int  restoDecimas = 0;        // Décimas que sobraron del cuadro anterior

// Estado del triceratops
static float altura      = 0;        // Píxeles por encima del suelo (0 = apoyado)
static float velocidadY  = 0;        // Positiva = sube, negativa = baja
static bool  agachado    = false;
static bool  saltoPedido = false;    // Se pidió saltar con A (ver actualizarDino)
static int   contadorCuadros = 0;
static int   pasoActual      = 0;    // 0 o 1: qué par de patas está apoyado

// Lo último que se dibujó del triceratops (para redibujar solo si cambió)
static int yDibujado     = -1;
static int altoDibujado  = 0;
static int formaDibujada = -1;

// Obstáculos
static Obstaculo obstaculos[MAX_OBSTACULOS];
static int distanciaDesdeUltimo = 0;   // Píxeles desde el último obstáculo creado
static int separacionSiguiente  = SEPARACION_MAXIMA;

// Desplazamiento del suelo (para que parezca que avanza)
static int desplazamientoSuelo = 0;

static unsigned long ultimoCuadro = 0;

// Altura de los cerros en cada columna (se calcula una sola vez).
// Por debajo de estas filas hay cerro; por encima, cielo.
static uint8_t cimaCerroLejano[ANCHO_PANTALLA];
static uint8_t cimaCerroCerca[ANCHO_PANTALLA];
static bool cerrosCalculados = false;

static bool esAve(const Obstaculo& o) {
  return o.tipo == OBSTACULO_AVE || o.tipo == OBSTACULO_AVE_ALTA;
}
static bool esNoche() { return (distancia / 3600) % 2 == 1; }

// ------------------------------------------------------------
//  5. FONDO Y DIBUJO DE SPRITES
// ------------------------------------------------------------

// Calcula la cima de los cerros en cada columna (con senos,
// para que queden ondulados). Se hace una sola vez.
static void calcularCerros() {
  if (cerrosCalculados) return;
  for (int x = 0; x < ANCHO_PANTALLA; x++) {
    cimaCerroLejano[x] = 56 + (int)(6 * sin(x / 17.0) + 3 * sin(x / 7.0 + 1));
    cimaCerroCerca[x]  = 72 + (int)(4 * sin(x / 11.0 + 2));
  }
  cerrosCalculados = true;
}

// Color del escenario en un punto (sin personajes encima).
// El sol y las nubes no se tienen en cuenta: están más arriba
// de donde llega cualquier personaje.
static uint16_t colorFondo(int x, int y) {
  if (y < ALTO_BARRA)          return COLOR_BARRA;
  if (y == Y_SUELO)            return COLOR_ARENA_OSC;
  if (y > Y_SUELO)             return COLOR_ARENA;
  if (y >= cimaCerroCerca[x])  return COLOR_CERRO_CERCA;
  if (y >= cimaCerroLejano[x]) return COLOR_CERRO_LEJANO;
  return esNoche() ? COLOR_FONDO_UI : COLOR_CIELO;
}

// Vuelve a pintar el escenario en un rectángulo (sirve para
// "borrar" a un personaje). En cada fila junta los píxeles
// seguidos del mismo color en un solo rectángulo.
static void pintarFondo(int x, int y, int ancho, int alto) {
  int x0 = max(x, 0);
  int x1 = min(x + ancho, ANCHO_PANTALLA);
  int y0 = max(y, ALTO_BARRA);
  int y1 = min(y + alto, ALTO_PANTALLA);

  for (int fila = y0; fila < y1; fila++) {
    int inicio = x0;
    while (inicio < x1) {
      uint16_t color = colorFondo(inicio, fila);
      int fin = inicio + 1;
      while (fin < x1 && colorFondo(fin, fila) == color) fin++;
      rectanguloRelleno(inicio, fila, fin - inicio, 1, color);
      inicio = fin;
    }
  }
}

// Color de cada letra de los sprites ('.' = fondo, se trata aparte)
static uint16_t colorDeLetra(char letra) {
  switch (letra) {
    case 'B': case 'H': return COLOR_DINO_CUERPO;
    case 'L': case 'T': return COLOR_DINO_PATAS;
    case 'F': return COLOR_DINO_GORGUERA;
    case 'h': return COLOR_DINO_CUERNO;
    case 'R': return COLOR_PIEDRA;
    case 'r': return COLOR_PIEDRA_OSC;
    case 'W': return COLOR_AVE_ALA;
    case 'C': return COLOR_AVE_CUERPO;
    case 'p': return COLOR_AVE_PICO;
    default:  return COLOR_NEGRO;   // 'e' (ojo)
  }
}

// Dibuja varias filas de un sprite, INCLUIDO el fondo, así lo
// que había antes en ese lugar queda tapado.
// - Junta las letras seguidas iguales en un solo rectángulo
//   (es mucho más rápido que píxel por píxel).
// - Donde hay '.', vuelve a pintar el escenario.
// - Recorta lo que quede fuera de la pantalla (los obstáculos
//   entran por la derecha y salen por la izquierda).
static void dibujarFilas(int x, int y, const char* const filas[], int cantidad, int ancho) {
  for (int f = 0; f < cantidad; f++) {
    const char* fila = filas[f];
    int yFila = y + f * ESCALA;
    int inicio = 0;
    while (inicio < ancho) {
      char letra = fila[inicio];
      bool esFondo = (letra == '.');
      uint16_t color = esFondo ? 0 : colorDeLetra(letra);
      int fin = inicio + 1;
      while (fin < ancho &&
             (fila[fin] == '.') == esFondo &&
             (esFondo || colorDeLetra(fila[fin]) == color)) {
        fin++;
      }

      int xa = x + inicio * ESCALA;
      int an = (fin - inicio) * ESCALA;
      if (esFondo) {
        pintarFondo(xa, yFila, an, ESCALA);
      } else {
        // Recorte horizontal a los límites de la pantalla
        int x0 = max(xa, 0);
        int x1 = min(xa + an, ANCHO_PANTALLA);
        if (x1 > x0) rectanguloRelleno(x0, yFila, x1 - x0, ESCALA, color);
      }
      inicio = fin;
    }
  }
}

// "Borrar" = volver a pintar el escenario en ese rectángulo
static void borrarZona(int x, int y, int ancho, int alto) {
  if (ancho > 0 && alto > 0) pintarFondo(x, y, ancho, alto);
}

// ------------------------------------------------------------
//  6. DIBUJO DEL TRICERATOPS
// ------------------------------------------------------------

// Patas que corresponden en este momento
static const char* const* patasActuales() {
  if (altura > 0)      return PATAS_AIRE;
  if (pasoActual == 0) return PATAS_PASO_1;
  return PATAS_PASO_2;
}

// Código que identifica la forma actual (agachado y patas)
static int formaActual() {
  int patas = (altura > 0) ? 0 : 1 + pasoActual;
  return (agachado ? 10 : 0) + patas;
}

// Fila superior del triceratops en pantalla, según su estado
static int altoDino() { return agachado ? ALTO_AGACHADO : ALTO_PARADO; }
static int yDino()    { return Y_SUELO - altoDino() - (int)altura; }

// Dibuja el triceratops donde está ahora.
// Solo borra la parte de la posición anterior que quedó libre.
static void dibujarDino(bool forzar) {
  int alto  = altoDino();
  int y     = yDino();
  int forma = formaActual();

  if (!forzar && y == yDibujado && forma == formaDibujada) return;  // Nada cambió

  // 1. Borrar lo que quedó fuera del dibujo nuevo
  if (yDibujado >= 0) {
    int ancho      = ANCHO_DINO * ESCALA;
    int viejoAbajo = yDibujado + altoDibujado;
    int nuevoAbajo = y + alto;
    if (yDibujado < y) {   // Franja de arriba
      borrarZona(X_DINO, yDibujado, ancho, min(viejoAbajo, y) - yDibujado);
    }
    if (viejoAbajo > nuevoAbajo) {   // Franja de abajo
      int desde = max(nuevoAbajo, yDibujado);
      borrarZona(X_DINO, desde, ancho, viejoAbajo - desde);
    }
  }

  // 2. Dibujar cuerpo y patas
  if (agachado) {
    dibujarFilas(X_DINO, y, CUERPO_AGACHADO, 5, ANCHO_DINO);
    dibujarFilas(X_DINO, y + 5 * ESCALA, patasActuales() + 1, 2, ANCHO_DINO);
  } else {
    dibujarFilas(X_DINO, y, CUERPO_PARADO, 8, ANCHO_DINO);
    dibujarFilas(X_DINO, y + 8 * ESCALA, patasActuales(), 3, ANCHO_DINO);
  }

  yDibujado     = y;
  altoDibujado  = alto;
  formaDibujada = forma;
}

// ------------------------------------------------------------
//  7. DIBUJO DE LOS OBSTÁCULOS
// ------------------------------------------------------------

// Medidas en pantalla de cada tipo de obstáculo
static int anchoObstaculo(const Obstaculo& o) {
  return (esAve(o) ? ANCHO_AVE : ANCHO_PIEDRA) * ESCALA;
}
static int altoObstaculo(const Obstaculo& o) {
  return (esAve(o) ? ALTO_AVE : o.tipo == OBSTACULO_ROCA_ALTA ? 7 : ALTO_PIEDRA) * ESCALA;
}
static int yObstaculo(const Obstaculo& o) {
  return esAve(o) ? (o.tipo == OBSTACULO_AVE_ALTA ? 70 : Y_AVE) : Y_SUELO - altoObstaculo(o);
}

// Dibuja un obstáculo en su posición actual y borra la franja
// que dejó libre a la derecha al moverse hacia la izquierda.
static void dibujarObstaculo(Obstaculo& o) {
  int ancho = anchoObstaculo(o);
  int alto  = altoObstaculo(o);
  int y     = yObstaculo(o);

  // Franja libre: entre el borde derecho nuevo y el viejo
  if (o.xDibujado > o.x) {
    borrarZona(o.x + ancho, y, o.xDibujado - o.x, alto);
  }

  if (!esAve(o)) {
    dibujarFilas(o.x, y, o.tipo == OBSTACULO_ROCA_ALTA ? ROCA_ALTA : PIEDRA,
                  alto / ESCALA, ANCHO_PIEDRA);
  } else {
    // El ave aletea: cambia de dibujo cada 8 cuadros
    bool alasArriba = ((distancia / 24) % 2) == 0;
    dibujarFilas(o.x, y, alasArriba ? AVE_ALAS_ARRIBA : AVE_ALAS_EXTENDIDAS,
                 ALTO_AVE, ANCHO_AVE);
  }
  o.xDibujado = o.x;
}

// ------------------------------------------------------------
//  8. DIBUJO DEL ESCENARIO
// ------------------------------------------------------------

// --- Barra superior: puntos y récord ---
static void dibujarMarcador() {
  char izquierda[32],derecha[24];
  rectanguloRelleno(0,0,ANCHO_PANTALLA,ALTO_BARRA,COLOR_BARRA);
  snprintf(izquierda,sizeof(izquierda),"Puntaje:%d",puntuacion);
  filaMarcador(2,izquierda);
  snprintf(izquierda,sizeof(izquierda),"Record:%d",max(recordActual(),puntuacion));
  int base=dificultadActual()==FACIL?25:dificultadActual()==DIFICIL?35:VELOCIDAD_INICIAL;
  snprintf(derecha,sizeof(derecha),"Nivel:%d",1+(velocidad-base)/AUMENTO_VELOCIDAD);
  filaMarcador(11,izquierda,derecha);
  dibujarAltavoz();
}

// --- Cielo con sol y nubes, y los dos cerros ---
static void dibujarCieloYCerros() {
  rectanguloRelleno(0, ALTO_BARRA, ANCHO_PANTALLA, Y_SUELO - ALTO_BARRA,
                    esNoche() ? COLOR_FONDO_UI : COLOR_CIELO);

  // Sol: círculo de radio 4
  for (int dy = -4; dy <= 4; dy++) {
    for (int dx = -4; dx <= 4; dx++) {
      if (dx * dx + dy * dy <= 17) dibujarPixel(134 + dx, 27 + dy, esNoche() ? COLOR_BLANCO : COLOR_SOL);
    }
  }

  // Dos nubes (cada una son dos rectángulos)
  rectanguloRelleno(20, 27, 18, 4, COLOR_NUBE);
  rectanguloRelleno(24, 25, 9, 3, COLOR_NUBE);
  rectanguloRelleno(78, 23, 18, 4, COLOR_NUBE);
  rectanguloRelleno(82, 21, 9, 3, COLOR_NUBE);

  // Cerros: una columna por cada x, desde la cima hasta el suelo
  for (int x = 0; x < ANCHO_PANTALLA; x++) {
    rectanguloRelleno(x, cimaCerroLejano[x], 1, Y_SUELO - cimaCerroLejano[x], COLOR_CERRO_LEJANO);
    rectanguloRelleno(x, cimaCerroCerca[x],  1, Y_SUELO - cimaCerroCerca[x],  COLOR_CERRO_CERCA);
  }
}

// --- Marcas del suelo (piedritas y rayitas en la arena) ---
// Se dibujan según el desplazamiento: al cambiarlo, parece
// que el suelo avanza hacia la izquierda.
static void dibujarMarcasSuelo(int desplazamiento, bool borrar) {
  uint16_t color = borrar ? COLOR_ARENA : COLOR_ARENA_OSC;
  for (int x = 0; x < ANCHO_PANTALLA; x++) {
    int p = x + desplazamiento;
    if (p % 11 == 3)  dibujarPixel(x, Y_SUELO + 4, color);
    if (p % 11 == 8)  dibujarPixel(x, Y_SUELO + 8, color);
    if (p % 23 == 8)  dibujarLinea(x, Y_SUELO + 2, min(x + 2, ANCHO_PANTALLA - 1), Y_SUELO + 2, color);
  }
}

// --- Escenario completo (al empezar o al salir de la pausa) ---
static void dibujarEscenario() {
  calcularCerros();
  nocheDibujada = esNoche();

  rectanguloRelleno(0, 0, ANCHO_PANTALLA, ALTO_BARRA, COLOR_BARRA);
  dibujarMarcador();
  dibujarCieloYCerros();

  // Arena: línea del suelo un poco más oscura y marcas
  rectanguloRelleno(0, Y_SUELO, ANCHO_PANTALLA, ALTO_PANTALLA - Y_SUELO, COLOR_ARENA);
  dibujarLinea(0, Y_SUELO, ANCHO_PANTALLA - 1, Y_SUELO, COLOR_ARENA_OSC);
  dibujarMarcasSuelo(desplazamientoSuelo, false);

  for (int i = 0; i < MAX_OBSTACULOS; i++) {
    if (obstaculos[i].activo) {
      obstaculos[i].xDibujado = obstaculos[i].x;   // Pantalla limpia: nada que borrar
      dibujarObstaculo(obstaculos[i]);
    }
  }

  yDibujado = -1;          // Pantalla limpia: nada que borrar
  dibujarDino(true);
}

// ------------------------------------------------------------
//  9. LÓGICA DEL TRICERATOPS
// ------------------------------------------------------------

// Factor de rapidez del salto: 1,0 a la velocidad inicial.
// Si el juego va 1,5 veces más rápido, el salto también.
static float factorSalto() {
  return (float)velocidad / VELOCIDAD_INICIAL;
}

// Lee los controles y aplica la física del salto (un cuadro)
static void moverDino(const EstadoControles& controles) {
  float factor = factorSalto();
  bool enElSuelo = (altura <= 0);

  if (enElSuelo) {
    agachado = controles.abajo;
    // Saltar: con A (pedido guardado) o con el joystick hacia arriba
    if (!agachado && saltoPedido) {
      velocidadY = IMPULSO_SALTO * factor;
      reproducirSonido(SONIDO_SALTO);
    }
  } else {
    agachado = false;   // En el aire no se agacha
  }
  saltoPedido = false;

  // Física: la altura cambia según la velocidad, y la gravedad
  // va frenando la subida hasta que el triceratops cae.
  // Impulso x factor y gravedad x factor² = MISMA altura máxima,
  // pero el salto dura menos (es más rápido).
  if (altura > 0 || velocidadY > 0) {
    float gravedad = GRAVEDAD * factor * factor;
    if (controles.abajo) gravedad *= GRAVEDAD_EXTRA;
    else if (!controles.a.presionado && !controles.arriba && altura >= 20) gravedad *= 1.5f;
    altura += velocidadY;
    velocidadY -= gravedad;
    if (altura <= 0) {   // Tocó el suelo
      altura = 0;
      velocidadY = 0;
      polvoRestante = 5;
    }
  }
  if (polvoRestante > 0) polvoRestante--;

  // Animación de las patas (solo se ve en el suelo)
  contadorCuadros++;
  if (contadorCuadros >= CUADROS_POR_PASO) {
    contadorCuadros = 0;
    pasoActual = 1 - pasoActual;
  }
}

// ------------------------------------------------------------
//  10. LÓGICA DE LOS OBSTÁCULOS Y CHOQUES
// ------------------------------------------------------------

// Crea un obstáculo nuevo fuera de la pantalla, a la derecha
static void crearObstaculo() {
  for (int i = 0; i < MAX_OBSTACULOS; i++) {
    if (!obstaculos[i].activo) {
      bool puedeSerAve = (puntuacion >= PUNTOS_PARA_AVES);
      obstaculos[i].tipo = (puedeSerAve && random(100) < PROBABILIDAD_AVE)
                           ? OBSTACULO_AVE : OBSTACULO_PIEDRA;
      if (esAve(obstaculos[i]) && random(100) < 35) obstaculos[i].tipo = OBSTACULO_AVE_ALTA;
      else if (!esAve(obstaculos[i]) && puntuacion >= 150 && random(100) < 35)
        obstaculos[i].tipo = OBSTACULO_ROCA_ALTA;
      obstaculos[i].x = ANCHO_PANTALLA;
      obstaculos[i].xDibujado = ANCHO_PANTALLA;
      obstaculos[i].activo = true;
      obstaculos[i].superado = false;
      return;
    }
  }
}

// Mueve los obstáculos "paso" píxeles a la izquierda y crea
// uno nuevo cuando se recorrió la separación elegida al azar
static void moverObstaculos(int paso) {
  for (int i = 0; i < MAX_OBSTACULOS; i++) {
    Obstaculo& o = obstaculos[i];
    if (!o.activo) continue;
    o.x -= paso;
    if (!o.superado && o.x + anchoObstaculo(o) <= X_DINO + 4) {
      o.superado = true;
      bonus += 5;
    }
    if (o.x + anchoObstaculo(o) <= 0) {
      o.activo = false;   // Salió por la izquierda
    }
  }

  distanciaDesdeUltimo += paso;
  if (distanciaDesdeUltimo >= separacionSiguiente) {
    crearObstaculo();
    distanciaDesdeUltimo = 0;
    // Al menos 130 px: permite terminar un salto completo antes del siguiente.
    int minimo = dificultadActual() == FACIL ? 170 : dificultadActual() == DIFICIL ? SEPARACION_MINIMA : 150;
    separacionSiguiente = random(minimo, SEPARACION_MAXIMA + 1);
  }
}

// ¿Se superponen dos rectángulos? (x, y, ancho, alto)
static bool seSuperponen(int x1, int y1, int a1, int h1,
                         int x2, int y2, int a2, int h2) {
  return x1 < x2 + a2 && x2 < x1 + a1 &&
         y1 < y2 + h2 && y2 < y1 + h1;
}

// Revisa si el triceratops toca algún obstáculo.
// Las "cajas de choque" son un poco más chicas que los dibujos,
// para que no se pierda por rozar una esquina vacía.
static bool hayChoque() {
  int dx = X_DINO + 4;
  int dy = yDino() + (agachado ? 2 : 4);
  int da = ANCHO_DINO * ESCALA - 8;
  int dh = Y_SUELO - (int)altura - dy;

  for (int i = 0; i < MAX_OBSTACULOS; i++) {
    const Obstaculo& o = obstaculos[i];
    if (!o.activo) continue;
    int ox = o.x + 2;
    int oa = anchoObstaculo(o) - 4;
    int oy = yObstaculo(o) + 2;
    int oh = altoObstaculo(o) - 2;
    if (seSuperponen(dx, dy, da, dh, ox, oy, oa, oh)) {
      return true;
    }
  }
  return false;
}

// ------------------------------------------------------------
//  11. AVANCE DEL JUEGO
// ------------------------------------------------------------

// Avanza el escenario: suelo, obstáculos, puntos y velocidad
static void avanzarEscenario() {
  // Cuántos píxeles enteros se avanza en este cuadro
  // (las décimas que sobran se guardan para el siguiente)
  restoDecimas += velocidad;
  int paso = restoDecimas / 10;
  restoDecimas %= 10;

  desplazamientoSuelo = (desplazamientoSuelo + paso) % (11 * 23);

  moverObstaculos(paso);

  // Puntos por distancia
  distancia += paso;
  int nuevosPuntos = distancia / PIXELES_POR_PUNTO + bonus;
  if (nuevosPuntos != puntuacion) {
    puntuacion = nuevosPuntos;

    // Cada 100 puntos, más velocidad (hasta el máximo)
    int base = dificultadActual() == FACIL ? 25 : dificultadActual() == DIFICIL ? 35 : VELOCIDAD_INICIAL;
    int nuevaVelocidad = base + (distancia / PIXELES_POR_PUNTO / PUNTOS_POR_AUMENTO) * AUMENTO_VELOCIDAD;
    nuevaVelocidad = min(nuevaVelocidad, VELOCIDAD_MAXIMA);
    if (nuevaVelocidad != velocidad) {
      velocidad = nuevaVelocidad;
      reproducirSonido(SONIDO_NIVEL);
      Serial.printf("Dino: velocidad %d.%d px por cuadro\r\n", velocidad / 10, velocidad % 10);
    }
  }
}

// Deja todo listo para una partida nueva
static void reiniciarPartida() {
  puntuacion = 0;
  distancia  = 0;
  velocidad = dificultadActual() == FACIL ? 25 : dificultadActual() == DIFICIL ? 35 : VELOCIDAD_INICIAL;
  bonus = 0;
  arribaAnterior = false;
  polvoRestante = 0;
  restoDecimas = 0;
  altura     = 0;
  velocidadY = 0;
  agachado   = false;
  saltoPedido = false;
  contadorCuadros = 0;
  pasoActual = 0;
  desplazamientoSuelo = 0;

  for (int i = 0; i < MAX_OBSTACULOS; i++) {
    obstaculos[i].activo = false;
  }
  distanciaDesdeUltimo = 0;
  separacionSiguiente  = SEPARACION_MAXIMA;   // Un respiro al empezar
}

// ------------------------------------------------------------
//  12. FUNCIONES PÚBLICAS (las que usa la consola)
// ------------------------------------------------------------

void comenzarDino() {
  reiniciarPartida();
  dibujarEscenario();
  ultimoCuadro = millis();
}

ResultadoPartida actualizarDino(const EstadoControles& controles) {
  // loop() llama a esta función cada ~5 ms, pero el juego avanza
  // cada 25 ms. "recienPresionado" dura una sola llamada, así que
  // se guarda el pedido de salto para no perderlo.
  if ((controles.a.recienPresionado || (controles.arriba && !arribaAnterior)) && altura <= 0) {
    saltoPedido = true;
    momentoSalto = millis();
  }
  arribaAnterior = controles.arriba;
  if (saltoPedido && millis() - momentoSalto > 120) saltoPedido = false;

  unsigned int pasos = pasosPendientes(millis(), ultimoCuadro, INTERVALO_CUADRO_MS);
  if (!pasos) return PARTIDA_CONTINUA;
  Obstaculo anteriores[MAX_OBSTACULOS];
  memcpy(anteriores, obstaculos, sizeof(anteriores));
  int sueloAnterior = desplazamientoSuelo;
  int puntosAnteriores = puntuacion;
  ResultadoPartida resultado = PARTIDA_CONTINUA;
  for (unsigned int i = 0; i < pasos; i++) {
    moverDino(controles);
    avanzarEscenario();
    if (hayChoque()) { resultado = PARTIDA_PERDIDA; break; }
  }
  if (esNoche() != nocheDibujada) dibujarEscenario();
  else {
    for (const Obstaculo& o : anteriores) {
      if (o.activo) borrarZona(o.xDibujado, yObstaculo(o), anchoObstaculo(o), altoObstaculo(o));
    }
    borrarZona(3, Y_SUELO - 2, 11, 11);
    dibujarMarcasSuelo(sueloAnterior, true);
    dibujarMarcasSuelo(desplazamientoSuelo, false);
    for (Obstaculo& o : obstaculos) if (o.activo) {
      o.xDibujado = o.x;
      dibujarObstaculo(o);
    }
    bool forzarDino = false;
    for (int i = 0; i < MAX_OBSTACULOS; i++) {
      const Obstaculo& viejo = anteriores[i];
      const Obstaculo& nuevo = obstaculos[i];
      if ((viejo.activo && viejo.x < X_DINO + ANCHO_DINO * ESCALA &&
           viejo.x + anchoObstaculo(viejo) > X_DINO) ||
          (nuevo.activo && nuevo.x < X_DINO + ANCHO_DINO * ESCALA &&
           nuevo.x + anchoObstaculo(nuevo) > X_DINO)) forzarDino = true;
    }
    dibujarDino(forzarDino);
  }
  if (polvoRestante > 0) {
    dibujarPixel(5 + polvoRestante, Y_SUELO + 1, COLOR_ARENA_OSC);
    dibujarPixel(10 - polvoRestante, Y_SUELO - 1, COLOR_ARENA_OSC);
  }
  if (puntosAnteriores != puntuacion) dibujarMarcador();
  if (resultado == PARTIDA_PERDIDA)
    for (Obstaculo& o : obstaculos) if (o.activo) dibujarObstaculo(o);
  return resultado;
}

void redibujarDino() {
  saltoPedido = false;
  arribaAnterior = false;
  dibujarEscenario();
  ultimoCuadro = millis();
}

int puntuacionDino() {
  return puntuacion;
}
