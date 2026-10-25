#include "soporte.h"
#include "../../src/dino.cpp"

static float alturaMaxima(bool mantener) {
  comenzarDino(); separacionSiguiente = 100000;
  EstadoControles c = {}; c.a.recienPresionado = true; c.a.presionado = true;
  float maxima = 0;
  for (int f = 0; f < 60; f++) {
    relojPrueba += 25; COMPROBAR(actualizarDino(c) == PARTIDA_CONTINUA);
    maxima = max(maxima, altura);
    c.a.recienPresionado = false; c.a.presionado = mantener;
  }
  return maxima;
}
int main() {
  comenzarDino();COMPROBAR(apareceTexto("Puntaje:") && apareceTexto("Record:") && apareceTexto("Nivel:"));
  COMPROBAR(alturaMaxima(true) > alturaMaxima(false));
  comenzarDino(); separacionSiguiente = 100000; saltosPrueba = 0;
  EstadoControles c = {}; c.arriba = true;
  for (int f = 0; f < 100; f++) { relojPrueba += 25; actualizarDino(c); }
  COMPROBAR(saltosPrueba == 1); COMPROBAR(altura == 0);
  c.arriba = false; actualizarDino(c); c.arriba = true;
  relojPrueba += 25; actualizarDino(c); COMPROBAR(saltosPrueba == 2);
  comenzarDino(); separacionSiguiente = 100000;
  unsigned long antes = ultimoCuadro;
  relojPrueba += 83; actualizarDino({});
  COMPROBAR(ultimoCuadro == antes + 75); COMPROBAR(distancia == 9);
  // Colisión intermedia: no se salta una roca al recuperar cuadros.
  comenzarDino(); obstaculos[0] = {true, OBSTACULO_PIEDRA, 49, 49, false};
  relojPrueba += 100; COMPROBAR(actualizarDino({}) == PARTIDA_PERDIDA);
  comenzarDino(); obstaculos[0] = {true, OBSTACULO_AVE, 25, 25, false};
  agachado = true; COMPROBAR(!hayChoque());
  agachado = false; COMPROBAR(hayChoque());
  comenzarDino(); obstaculos[0] = {true, OBSTACULO_PIEDRA, -4, -4, false};
  moverObstaculos(3); COMPROBAR(bonus == 5);
  moverObstaculos(3); COMPROBAR(bonus == 5);
  altura = 10; saltoPedido = true; redibujarDino(); COMPROBAR(!saltoPedido && altura == 10);
  // Cada roca conserva al menos una ventana de salto, en todas las velocidades.
  for (int v = 25; v <= 45; v += 5) for (TipoObstaculo tipo : {OBSTACULO_PIEDRA, OBSTACULO_ROCA_ALTA}) {
    bool existeVentana = false;
    for (int x = 58; x < 150 && !existeVentana; x++) {
      comenzarDino(); velocidad = v; separacionSiguiente = 100000;
      obstaculos[0] = {true, tipo, x, x, false};
      EstadoControles salto = {}; salto.a.presionado = true; saltoPedido = true;
      bool choque = false;
      for (int f = 0; f < 80; f++) {
        moverDino(salto); avanzarEscenario();
        if (hayChoque()) { choque = true; break; }
      }
      existeVentana = !choque;
    }
    COMPROBAR(existeVentana);
  }
  comenzarDino(); separacionSiguiente = 100000;
  obstaculos[0] = {true, OBSTACULO_PIEDRA, 140, 140, false};
  for (int f = 0; f < 25; f++) {
    EstadoControles salto = {}; salto.a.presionado = true; salto.a.recienPresionado = f == 10;
    relojPrueba += 25; actualizarDino(salto);
    uint16_t parcial[ALTO_PANTALLA][ANCHO_PANTALLA];
    memcpy(parcial, pixelesPrueba, sizeof(parcial)); dibujarEscenario();
    for (int y = 11; y < ALTO_PANTALLA; y++) for (int x = 14; x < ANCHO_PANTALLA; x++)
      COMPROBAR(parcial[y][x] == pixelesPrueba[y][x]);
  }
  std::puts("Dino: OK (salto variable, flancos, tiempos, choques, bonus y pausa)");
}
