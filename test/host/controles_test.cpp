#define PRUEBA_CONSOLA
#include "soporte.h"
#include "juegos_falsos.h"
#include "../../src/controles.cpp"
#include "../../src/juego.cpp"

static EstadoControles botonFisico(uint8_t registro,uint8_t codigo) {
  Wire.registros[registro]=codigo;
  relojPrueba+=31;
  EstadoControles c=leerControles();
  actualizarJuego(c);
  return c;
}
int main() {
  Wire.reiniciar();relojPrueba=100;
  iniciarControles();iniciarJuego();
  cambiarEstado(ESTADO_CUENTA);relojPrueba+=3000;actualizarJuego(leerControles());
  COMPROBAR(estadoActual==ESTADO_JUGANDO && iniciosPrueba==1 && sonidoPrueba);
  // Registros literales de la placa: C=DOWN/0x21, D=LEFT/0x24.
  // No se usan REG_BOTON_C/D como entrada: detectaría el intercambio original.
  EstadoControles c=botonFisico(0x21,0);
  COMPROBAR(c.c.recienPresionado && !c.d.presionado && !c.d.recienPresionado);
  COMPROBAR(estadoActual==ESTADO_PAUSA && sonidoPrueba);
  botonFisico(0x21,8);puntosPrueba=123;
  c=botonFisico(0x21,0);
  COMPROBAR(c.c.recienPresionado && estadoActual==ESTADO_CUENTA && sonidoPrueba);
  botonFisico(0x21,8);relojPrueba+=3000;actualizarJuego(leerControles());
  COMPROBAR(estadoActual==ESTADO_JUGANDO && iniciosPrueba==1 && puntosPrueba==123);
  c=botonFisico(0x24,0);
  COMPROBAR(c.d.recienPresionado && !c.c.recienPresionado);
  COMPROBAR(estadoActual==ESTADO_JUGANDO && !sonidoPrueba);
  botonFisico(0x24,8);
  botonFisico(0x21,0);COMPROBAR(estadoActual==ESTADO_PAUSA && !sonidoPrueba);
  botonFisico(0x21,8);
  botonFisico(0x24,0);COMPROBAR(estadoActual==ESTADO_PAUSA && sonidoPrueba);
  botonFisico(0x24,8);
  botonFisico(0x21,0);COMPROBAR(estadoActual==ESTADO_CUENTA && sonidoPrueba);
  botonFisico(0x21,8);relojPrueba+=3000;actualizarJuego(leerControles());
  COMPROBAR(estadoActual==ESTADO_JUGANDO && puntosPrueba==123 && iniciosPrueba==1);
  std::puts("Controles: OK (I2C simulado -> C pausa/reanuda, D sonido, partida conservada)");
}
