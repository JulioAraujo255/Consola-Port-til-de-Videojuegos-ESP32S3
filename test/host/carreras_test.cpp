#include "soporte.h"
#include "../../src/carreras.cpp"

int main() {
  comenzarCarreras();COMPROBAR(apareceTexto("Puntaje:") && apareceTexto("Record:") && apareceTexto("Nivel:"));
  comenzarCarreras(); nivel = 20;
  // Miles de formaciones: nunca tres carriles cerrados ni un salto de dos carriles seguros.
  for (int iteracion = 0; iteracion < 10000; iteracion++) {
    for (Rival& r : rivales) r.activo = false;
    int seguroAnterior = carrilSeguroAnterior;
    crearRival();
    int mascara = 0, cantidad = 0;
    for (const Rival& r : rivales) if (r.activo) {
      COMPROBAR(!(mascara & (1 << r.carril)));
      mascara |= 1 << r.carril; cantidad++;
    }
    COMPROBAR(cantidad >= 1 && cantidad <= 2);
    COMPROBAR(!(mascara & (1 << carrilSeguroAnterior)));
    COMPROBAR(abs(carrilSeguroAnterior - seguroAnterior) <= 1);
  }
  comenzarCarreras(); separacionSiguiente = 100000;
  EstadoControles c = {}; c.derecha = true;
  actualizarCarreras(c); COMPROBAR(carrilJugador == 2 && xJugador == 75);
  for (int f = 0; f < 4; f++) { relojPrueba += 25; actualizarCarreras(c); }
  COMPROBAR(xJugador == 105);
  // El botón mantenido no salta carriles adicionales.
  COMPROBAR(carrilJugador == 2);
  comenzarCarreras(); separacionSiguiente = 100000;
  c = {}; c.a.presionado = true;
  for (int f = 0; f < 100; f++) { relojPrueba += 25; actualizarCarreras(c); }
  COMPROBAR(turbo == 0 && !turboActivo);
  c.a.presionado = false;
  for (int f = 0; f < 200; f++) { relojPrueba += 25; actualizarCarreras(c); }
  COMPROBAR(turbo == 100);
  comenzarCarreras(); nivel = 100000; velocidad = VELOCIDAD_MAXIMA; encenderMotor();
  COMPROBAR(motorPrueba <= 130);
  // Se comprueba la X real durante el cambio, no solo el carril objetivo.
  comenzarCarreras(); xJugador = 82;
  rivales[0] = {true, 1, COLOR_BLANCO, 108, 108, false, false};
  COMPROBAR(hayChoque()); xJugador = 84; COMPROBAR(!hayChoque());
  comenzarCarreras(); rivales[0] = {true, 0, COLOR_BLANCO, 121, 121, false, false};
  moverRivales(2); COMPROBAR(bonus == 5);
  moverRivales(2); COMPROBAR(bonus == 5);
  // Las rayas cruzadas por el auto vuelven a aparecer al moverlo.
  comenzarCarreras(); xJugador = 92; dibujarJugador();
  c = {}; c.derecha = true; relojPrueba += 100; actualizarCarreras(c);
  COMPROBAR(xJugador == 105);
  for (int y = Y_JUGADOR; y < Y_JUGADOR + ALTO_AUTO; y++) {
    int fase = ((y - desplazamientoRayas) % 12 + 12) % 12;
    COMPROBAR(pixelesPrueba[y][95] == (fase < 6 ? COLOR_LINEA_BLANCA : COLOR_ASFALTO));
  }
  // Ruta física entre dos filas dobles a velocidad de turbo máxima.
  for (int anterior = 0; anterior < 3; anterior++) for (int siguiente = 0; siguiente < 3; siguiente++) {
    if (abs(anterior - siguiente) > 1) continue;
    comenzarCarreras(); separacionSiguiente = 100000;
    velocidad = 50; turbo = 100; xJugador = X_CARRIL[anterior]; carrilJugador = siguiente;
    int i = 0;
    for (int carril = 0; carril < 3; carril++) {
      if (carril != anterior) rivales[i++] = {true,carril,COLOR_BLANCO,122,122,false,false};
      if (carril != siguiente) rivales[i++] = {true,carril,COLOR_BLANCO,54,54,false,false};
    }
    c = {}; c.a.presionado = true;
    for (int f = 0; f < 20; f++) { avanzarEscenario(c); COMPROBAR(!hayChoque()); }
  }
  comenzarCarreras();
  for (int f = 0; f < 80; f++) {
    c = {}; c.izquierda = f % 12 == 0; c.derecha = f % 12 == 6;
    relojPrueba += 25;
    if (actualizarCarreras(c) == PARTIDA_PERDIDA) break;
    uint16_t parcial[ALTO_PANTALLA][ANCHO_PANTALLA];
    memcpy(parcial, pixelesPrueba, sizeof(parcial)); dibujarEscenario();
    COMPROBAR(memcmp(parcial, pixelesPrueba, sizeof(parcial)) == 0);
  }
  std::puts("Carreras: OK (10000 formaciones, carriles, turbo, motor, choques, bonus y dibujo)");
}
