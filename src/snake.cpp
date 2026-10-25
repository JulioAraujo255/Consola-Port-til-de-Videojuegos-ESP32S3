/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: snake.cpp
 *  Descripción: "cuerpo" del juego Snake (interfaz final, a color).
 *
 *  REGLAS:
 *  - Área de juego: grilla de 26 x 17 celdas de 6 px.
 *  - Cada comida suma 1 punto y agrega un segmento.
 *  - Cada 5 puntos sube un nivel y la víbora se acelera.
 *  - Se pierde al chocar con el borde o con el propio cuerpo.
 *
 *  El menú, la pausa y el game over están en juego.cpp
 *  (son comunes a todos los juegos de la consola).
 * ============================================================
 */

#include "snake.h"
#include "pantalla.h"
#include "sonido.h"
#include "interfaz.h"

// ------------------------------------------------------------
//  1. CONSTANTES DEL JUEGO
// ------------------------------------------------------------

// Grilla del área de juego
const int TAMANO_CELDA = 6;    // Cada celda mide 6 x 6 píxeles
const int COLUMNAS     = 26;   // 26 x 6 = 156 píxeles de ancho
const int FILAS        = 17;
const int ORIGEN_X     = 2;    // Esquina superior izquierda de la grilla
const int ORIGEN_Y     = ALTO_MARCADOR+1;
const int LARGO_MAXIMO = COLUMNAS * FILAS;   // Toda la grilla ocupada

// Reglas
const int LARGO_INICIAL    = 3;
const int PUNTOS_POR_NIVEL = 5;

// Velocidad: tiempo entre pasos de la víbora (menor = más rápido)
const unsigned long INTERVALO_INICIAL_MS   = 250;
const unsigned long REDUCCION_POR_NIVEL_MS = 20;
const unsigned long INTERVALO_MINIMO_MS    = 80;

// Colores de Snake (RGB565)
const uint16_t COLOR_PASTO_1      = 0x7DE7;  // Cuadros claros del tablero
const uint16_t COLOR_PASTO_2      = 0x6546;  // Cuadros oscuros del tablero
const uint16_t COLOR_BORDE_SNAKE  = 0x3B63;  // Borde del tablero
const uint16_t COLOR_VIBORA       = 0x2B7B;  // Azul
const uint16_t COLOR_VIBORA_OSC   = 0x1A34;  // Contorno de cada segmento
const uint16_t COLOR_VIBORA_PANZA = 0x5D1F;  // Puntito claro del cuerpo
const uint16_t COLOR_MANZANA      = 0xD8E3;
const uint16_t COLOR_HOJA         = 0x2C63;
const uint16_t COLOR_TALLO        = 0x7A22;
const uint16_t COLOR_BRILLO       = 0xFCB2;  // Reflejo de la manzana

// ------------------------------------------------------------
//  2. TIPOS DE DATOS
// ------------------------------------------------------------

// Direcciones posibles de la víbora
enum Direccion {
  DIR_ARRIBA,
  DIR_ABAJO,
  DIR_IZQUIERDA,
  DIR_DERECHA
};

// Una posición en la grilla (columna y fila, no píxeles)
struct Celda {
  int8_t columna;
  int8_t fila;
};

// ------------------------------------------------------------
//  3. VARIABLES DEL JUEGO
//  "static": privadas de este archivo.
// ------------------------------------------------------------
static int puntuacion = 0;
static int nivel      = 1;

// La víbora: serpiente[0] es la cabeza; serpiente[largo-1], la cola
static Celda serpiente[LARGO_MAXIMO];
static int largo = LARGO_INICIAL;

static Direccion direccionActual    = DIR_DERECHA;  // Hacia donde se mueve
static Direccion giros[2];
static int cantidadGiros = 0;
static int ultimaEntrada = -1;
static int comidas = 0;
static int pasosSinComer = 1000;
static int combo = 1;
static bool comidaEspecial = false;
static int vidaEspecial = 0; // En pasos de juego: la pausa no consume el plazo.
static bool ocupadas[COLUMNAS][FILAS];

static Celda comida;                 // Posición de la comida
static unsigned long ultimoPaso = 0; // Momento del último movimiento

// ------------------------------------------------------------
//  4. DIBUJO DE LA VÍBORA Y LA COMIDA
// ------------------------------------------------------------

// Convierte una celda de la grilla a píxeles (esquina superior izquierda)
static int pixelX(const Celda& c) { return ORIGEN_X + c.columna * TAMANO_CELDA; }
static int pixelY(const Celda& c) { return ORIGEN_Y + c.fila    * TAMANO_CELDA; }

// Color del pasto de una celda (tablero a cuadros, como un tablero de ajedrez)
static uint16_t colorPasto(const Celda& c) {
  return ((c.columna + c.fila) % 2 == 0) ? COLOR_PASTO_1 : COLOR_PASTO_2;
}

static bool esObstaculo(const Celda& c) {
  return modoSnakeActual() == SNAKE_LABERINTO &&
         (c.columna == 6 || c.columna == 19) &&
         c.fila >= 4 && c.fila <= 14 && (c.fila < 8 || c.fila > 10);
}

static int capacidadTablero() { return LARGO_MAXIMO - (modoSnakeActual() == SNAKE_LABERINTO ? 16 : 0); }

// Dibuja un segmento de 6 x 6.
// - Cuerpo: contorno oscuro, relleno azul y un puntito claro
//   en cuadros alternados (parece la piel con escamas).
// - Cabeza: toda azul, con dos ojos que miran hacia donde avanza.
static void dibujarSegmento(const Celda& c, bool esCabeza) {
  int x = pixelX(c);
  int y = pixelY(c);

  if (!esCabeza) {
    rectanguloRelleno(x, y, TAMANO_CELDA, TAMANO_CELDA, COLOR_VIBORA_OSC);
    rectanguloRelleno(x + 1, y + 1, 4, 4, COLOR_VIBORA);
    if ((c.columna + c.fila) % 2 == 1) {
      dibujarPixel(x + 2, y + 2, COLOR_VIBORA_PANZA);
    }
    return;
  }

  rectanguloRelleno(x, y, TAMANO_CELDA, TAMANO_CELDA,
                    pasosSinComer == 0 ? COLOR_ADORNO : COLOR_VIBORA);

  // Ojos: blanco + pupila negra, según la dirección
  // (ojoX1, ojoY1) y (ojoX2, ojoY2) = blancos; (dx, dy) = hacia dónde mira la pupila
  int ojoX1, ojoY1, ojoX2, ojoY2, dx = 0, dy = 0;
  switch (direccionActual) {
    case DIR_DERECHA:   ojoX1 = 4; ojoY1 = 1; ojoX2 = 4; ojoY2 = 4; dx =  1; break;
    case DIR_IZQUIERDA: ojoX1 = 1; ojoY1 = 1; ojoX2 = 1; ojoY2 = 4; dx = -1; break;
    case DIR_ARRIBA:    ojoX1 = 1; ojoY1 = 1; ojoX2 = 4; ojoY2 = 1; dy = -1; break;
    default:            ojoX1 = 1; ojoY1 = 4; ojoX2 = 4; ojoY2 = 4; dy =  1; break;  // Abajo
  }
  dibujarPixel(x + ojoX1, y + ojoY1, COLOR_BLANCO);
  dibujarPixel(x + ojoX2, y + ojoY2, COLOR_BLANCO);
  dibujarPixel(x + ojoX1 + dx, y + ojoY1 + dy, COLOR_NEGRO);
  dibujarPixel(x + ojoX2 + dx, y + ojoY2 + dy, COLOR_NEGRO);
}

// Borra una celda: vuelve a pintar el pasto que le corresponde
static void borrarCelda(const Celda& c) {
  rectanguloRelleno(pixelX(c), pixelY(c), TAMANO_CELDA, TAMANO_CELDA, colorPasto(c));
}

// Dibuja toda la víbora (al empezar o al salir de la pausa)
static void dibujarSerpienteCompleta() {
  for (int i = largo - 1; i >= 0; i--) {
    dibujarSegmento(serpiente[i], i == 0);
  }
}

// Dibuja la comida: una manzana roja con tallo, hoja y brillo
static void dibujarComida() {
  if (largo >= capacidadTablero()) return;
  int x = pixelX(comida);
  int y = pixelY(comida);
  uint16_t color = comidaEspecial ? COLOR_AMARILLO : COLOR_MANZANA;
  rectanguloRelleno(x + 1, y + 2, 4, 4, color);
  rectanguloRelleno(x, y + 3, 6, 2, color);
  dibujarPixel(x + 3, y + 1, COLOR_TALLO);
  dibujarPixel(x + 4, y,     COLOR_HOJA);
  dibujarPixel(x + 2, y + 3, COLOR_BRILLO);
}

// ------------------------------------------------------------
//  5. PANTALLA DEL JUEGO
// ------------------------------------------------------------

// --- Barra superior: puntuación y nivel ---
static void dibujarMarcador() {
  char izquierda[32],derecha[24];
  rectanguloRelleno(0,0,ANCHO_PANTALLA,ALTO_MARCADOR,COLOR_BARRA);
  snprintf(izquierda,sizeof(izquierda),"Puntaje:%d",puntuacion);
  snprintf(derecha,sizeof(derecha),"Nivel:%d",nivel);
  filaMarcador(2,izquierda,derecha);
  snprintf(izquierda,sizeof(izquierda),"Record:%d",max(recordActual(),puntuacion));
  snprintf(derecha,sizeof(derecha),"Combo:x%d",combo);
  filaMarcador(11,izquierda,derecha);
  dibujarAltavoz();
}

// --- Tablero vacío: barra oscura arriba y pasto a cuadros ---
static void dibujarPantallaJuego() {
  rectanguloRelleno(0, 0, ANCHO_PANTALLA, ALTO_MARCADOR, COLOR_BARRA);
  dibujarMarcador();

  // Borde del tablero (lo que queda alrededor de la grilla)
  rectanguloRelleno(0, ALTO_MARCADOR, ANCHO_PANTALLA, ALTO_PANTALLA - ALTO_MARCADOR, COLOR_BORDE_SNAKE);

  // Cuadros de pasto
  for (int columna = 0; columna < COLUMNAS; columna++) {
    for (int fila = 0; fila < FILAS; fila++) {
      Celda c = { (int8_t)columna, (int8_t)fila };
      borrarCelda(c);
      if (esObstaculo(c)) {
        rectanguloRelleno(pixelX(c), pixelY(c), TAMANO_CELDA, TAMANO_CELDA, COLOR_GRIS);
        rectanguloBorde(pixelX(c), pixelY(c), TAMANO_CELDA, TAMANO_CELDA, COLOR_GRIS_CLARO);
      }
    }
  }
}

// ------------------------------------------------------------
//  6. LÓGICA DE LA VÍBORA
// ------------------------------------------------------------

// Selección uniforme entre las celdas libres, sin reintentos aleatorios.
static void colocarComida() {
  int libres = capacidadTablero() - largo;
  if (libres <= 0) return;
  memset(ocupadas, 0, sizeof(ocupadas));
  for (int i = 0; i < largo; i++) ocupadas[serpiente[i].columna][serpiente[i].fila] = true;
  int elegida = random(libres);
  for (int c = 0; c < COLUMNAS; c++) {
    for (int f = 0; f < FILAS; f++) {
      Celda candidata = { (int8_t)c, (int8_t)f };
      if (ocupadas[c][f] || esObstaculo(candidata)) continue;
      if (elegida-- == 0) {
        comida = candidata;
        comidaEspecial = comidas > 0 && comidas % 5 == 0;
        vidaEspecial = comidaEspecial ? 55 : 0;
        dibujarComida();
        return;
      }
    }
  }
}

// Tiempo entre pasos según el nivel (FO-08)
static unsigned long intervaloActual() {
  unsigned long inicial = dificultadActual() == FACIL ? 300 :
                          dificultadActual() == DIFICIL ? 190 : INTERVALO_INICIAL_MS;
  unsigned long reduccion = (unsigned long)(nivel - 1) * REDUCCION_POR_NIVEL_MS;
  if (reduccion >= inicial - INTERVALO_MINIMO_MS) {
    return INTERVALO_MINIMO_MS;
  }
  return inicial - reduccion;
}

// Deja todo listo para una partida nueva
static void reiniciarPartida() {
  puntuacion = 0;
  nivel = 1;
  largo = LARGO_INICIAL;

  // Víbora horizontal en el centro, mirando a la derecha
  serpiente[0] = {8, 9};   // Cabeza
  serpiente[1] = {7, 9};
  serpiente[2] = {6, 9};   // Cola

  direccionActual    = DIR_DERECHA;
  cantidadGiros = 0;
  ultimaEntrada = -1;
  comidas = 0;
  pasosSinComer = 1000;
  combo = 1;
  comidaEspecial = false;
  vidaEspecial = 0;
}

// Guarda la dirección que pide el jugador.
// No se permite girar 180 grados (la víbora chocaría consigo misma).
static void leerDireccion(const EstadoControles& controles) {
  int entrada = -1;
  if (controles.arriba) entrada = DIR_ARRIBA;
  else if (controles.abajo) entrada = DIR_ABAJO;
  else if (controles.izquierda) entrada = DIR_IZQUIERDA;
  else if (controles.derecha) entrada = DIR_DERECHA;
  if (entrada == ultimaEntrada) return;
  ultimaEntrada = entrada;
  if (entrada < 0 || cantidadGiros == 2) return;
  Direccion anterior = cantidadGiros ? giros[cantidadGiros - 1] : direccionActual;
  // Los pares opuestos son 0/1 y 2/3.
  if (entrada != anterior && entrada != ((int)anterior ^ 1)) giros[cantidadGiros++] = (Direccion)entrada;
}

// Mueve la víbora un paso.
// Devuelve false si chocó (fin de la partida).
static bool avanzarSerpiente() {
  // 1. Calcular la nueva posición de la cabeza
  if (cantidadGiros > 0) {
    direccionActual = giros[0];
    giros[0] = giros[1];
    cantidadGiros--;
  }
  Celda nuevaCabeza = serpiente[0];
  switch (direccionActual) {
    case DIR_ARRIBA:    nuevaCabeza.fila--;    break;
    case DIR_ABAJO:     nuevaCabeza.fila++;    break;
    case DIR_IZQUIERDA: nuevaCabeza.columna--; break;
    case DIR_DERECHA:   nuevaCabeza.columna++; break;
  }

  if (modoSnakeActual() == SNAKE_LIBRE) {
    nuevaCabeza.columna = (nuevaCabeza.columna + COLUMNAS) % COLUMNAS;
    nuevaCabeza.fila = (nuevaCabeza.fila + FILAS) % FILAS;
  }

  // 2. Choque con el borde (FO-06)
  if (nuevaCabeza.columna < 0 || nuevaCabeza.columna >= COLUMNAS ||
      nuevaCabeza.fila    < 0 || nuevaCabeza.fila    >= FILAS) {
    Serial.println("Snake: choque con el borde");
    return false;
  }
  if (esObstaculo(nuevaCabeza)) return false;
  pasosSinComer = min(pasosSinComer + 1, 1000);
  if (comidaEspecial && --vidaEspecial <= 0) {
    comidaEspecial = false;
    borrarCelda(comida);
    dibujarComida();
  }

  // 3. ¿Comió?
  bool comio = (nuevaCabeza.columna == comida.columna &&
                nuevaCabeza.fila    == comida.fila);

  // 4. Choque con el propio cuerpo (FO-06).
  //    Si no comió, la cola se va a mover, así que no se cuenta.
  int segmentosAComparar = comio ? largo : largo - 1;
  for (int i = 0; i < segmentosAComparar; i++) {
    if (serpiente[i].columna == nuevaCabeza.columna &&
        serpiente[i].fila    == nuevaCabeza.fila) {
      Serial.println("Snake: choque con el propio cuerpo");
      return false;
    }
  }

  // 5. Si no comió, se borra la cola de la pantalla
  if (!comio) {
    borrarCelda(serpiente[largo - 1]);
  }

  // 6. Correr todos los segmentos un lugar hacia atrás.
  //    Si comió, el largo aumenta en 1 (la cola no se pierde).
  int nuevoLargo = (comio && largo < LARGO_MAXIMO) ? largo + 1 : largo;
  for (int i = nuevoLargo - 1; i > 0; i--) {
    serpiente[i] = serpiente[i - 1];
  }
  serpiente[0] = nuevaCabeza;
  largo = nuevoLargo;

  // 7. Redibujar solo lo que cambió: la cabeza vieja pasa a
  //    ser cuerpo, y se dibuja la cabeza nueva
  dibujarSegmento(serpiente[1], false);
  dibujarSegmento(serpiente[0], true);

  // 8. Si comió: sumar puntos, actualizar nivel y nueva comida
  if (comio) {
    combo = pasosSinComer <= 18 ? min(combo + 1, 3) : 1;
    puntuacion += (comidaEspecial ? 3 : 1) * combo;
    pasosSinComer = 0;
    comidas++;
    dibujarSegmento(serpiente[0], true);
    int nivelAnterior = nivel;
    nivel = 1 + comidas / PUNTOS_POR_NIVEL;
    reproducirSonido(nivel > nivelAnterior ? SONIDO_NIVEL : SONIDO_COMER);
    dibujarMarcador();
    colocarComida();
    Serial.printf("Snake: comida! Puntuacion: %d, Nivel: %d\r\n", puntuacion, nivel);
  }

  return true;
}

// ------------------------------------------------------------
//  7. FUNCIONES PÚBLICAS (las que usa la consola)
// ------------------------------------------------------------

void comenzarSnake() {
  reiniciarPartida();
  dibujarPantallaJuego();
  dibujarSerpienteCompleta();
  colocarComida();
  ultimoPaso = millis();
}

ResultadoPartida actualizarSnake(const EstadoControles& controles) {
  // La dirección se lee siempre, para no perder ningún giro
  leerDireccion(controles);

  // La víbora avanza un paso cada cierto tiempo (según el nivel)
  if (millis() - ultimoPaso >= intervaloActual()) {
    ultimoPaso = millis();
    if (!avanzarSerpiente()) return PARTIDA_PERDIDA;
    if (largo >= capacidadTablero()) return PARTIDA_GANADA;
  }
  return PARTIDA_CONTINUA;
}

void redibujarSnake() {
  cantidadGiros = 0;
  ultimaEntrada = -1;
  dibujarPantallaJuego();
  dibujarSerpienteCompleta();
  dibujarComida();
  ultimoPaso = millis();   // Que no dé un paso apenas sale de la pausa
}

int puntuacionSnake() {
  return puntuacion;
}
