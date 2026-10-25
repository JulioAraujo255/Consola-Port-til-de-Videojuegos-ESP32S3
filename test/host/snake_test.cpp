#include "soporte.h"
#include "../../src/snake.cpp"

static void llenarTablero() {
  serpiente[0] = {24,(int8_t)(FILAS-1)};
  comida = {25,(int8_t)(FILAS-1)};
  largo = 1;
  for (int c = 0; c < COLUMNAS; c++) for (int f = 0; f < FILAS; f++) {
    Celda celda = {(int8_t)c,(int8_t)f};
    if ((f == FILAS-1 && c >= 24) || esObstaculo(celda)) continue;
    serpiente[largo++] = celda;
  }
  COMPROBAR(largo == capacidadTablero()-1);
}

int main() {
  comenzarSnake();
  COMPROBAR(apareceTexto("Puntaje:") && apareceTexto("Nivel:") && apareceTexto("Record:") && apareceTexto("Combo:"));
  COMPROBAR(ORIGEN_Y>=ALTO_MARCADOR && ORIGEN_Y+FILAS*TAMANO_CELDA<=ALTO_PANTALLA);
  EstadoControles c = {};
  c.arriba = true; leerDireccion(c);
  c.arriba = false; c.izquierda = true; leerDireccion(c);
  COMPROBAR(cantidadGiros == 2);
  COMPROBAR(avanzarSerpiente()); COMPROBAR(serpiente[0].fila == 8);
  COMPROBAR(avanzarSerpiente()); COMPROBAR(serpiente[0].columna == 7);
  comenzarSnake();
  c = {}; c.izquierda = true; leerDireccion(c);
  COMPROBAR(cantidadGiros == 0); // Reversa rechazada.
  serpiente[0] = {25,9}; serpiente[1] = {24,9}; serpiente[2] = {23,9}; comida = {0,0};
  COMPROBAR(!avanzarSerpiente());
  modoPrueba = SNAKE_LIBRE;
  COMPROBAR(avanzarSerpiente()); COMPROBAR(serpiente[0].columna == 0);
  for (ModoSnake modo : {SNAKE_CLASICO, SNAKE_LABERINTO}) {
    modoPrueba = modo; comenzarSnake(); llenarTablero();
    colocarComida(); // Solo queda una celda libre; termina y la encuentra.
    COMPROBAR(comida.columna == 25 && comida.fila == FILAS-1);
    relojPrueba += intervaloActual();
    COMPROBAR(actualizarSnake({}) == PARTIDA_GANADA);
    COMPROBAR(largo == capacidadTablero());
  }
  modoPrueba = SNAKE_LABERINTO; comenzarSnake();
  serpiente[0] = {5,4}; serpiente[1] = {4,4}; serpiente[2] = {3,4};
  COMPROBAR(!avanzarSerpiente());
  modoPrueba = SNAKE_CLASICO; comenzarSnake(); comidas = 5; colocarComida();
  COMPROBAR(comidaEspecial); int plazo = vidaEspecial;
  relojPrueba += 20000; redibujarSnake(); COMPROBAR(vidaEspecial == plazo);
  comida = {9,9}; COMPROBAR(avanzarSerpiente()); COMPROBAR(puntuacion == 3);
  comenzarSnake(); vidaEspecial = 1; comidaEspecial = true; comida = {0,0};
  COMPROBAR(avanzarSerpiente()); COMPROBAR(!comidaEspecial);
  // La cabeza puede ocupar la celda que la cola abandona.
  comenzarSnake(); largo = 4; serpiente[0]={8,9}; serpiente[1]={8,10};
  serpiente[2]={9,10}; serpiente[3]={9,9}; comida={0,0};
  COMPROBAR(avanzarSerpiente());
  std::puts("Snake: OK (cola, bordes, laberinto, victoria, comida, pausa y cola movil)");
}
