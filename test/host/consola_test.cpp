#define PRUEBA_CONSOLA
#include "soporte.h"
#include "juegos_falsos.h"
#include "../../src/juego.cpp"

static void pulsar(char tecla) {
  EstadoControles c = {};
  if (tecla == 'a') c.a = {true,true};
  if (tecla == 'b') c.b = {true,true};
  if (tecla == 'c') c.c = {true,true};
  if (tecla == 'd') c.d = {true,true};
  actualizarJuego(c); actualizarJuego({});
}

int main() {
  COMPROBAR(CANTIDAD_JUEGOS == 4);
  // Altavoz gráfico: ondas activas, cruz al silenciar, sin invadir el marco.
  rectanguloRelleno(0,0,160,128,COLOR_FONDO_UI);sonidoPrueba=true;dibujarAltavoz();
  COMPROBAR(pixelesPrueba[4][145]==COLOR_BLANCO && pixelesPrueba[3][156]==COLOR_BLANCO);
  sonidoPrueba=false;dibujarAltavoz();COMPROBAR(pixelesPrueba[4][152]==COLOR_ROJO);
  COMPROBAR(pixelesPrueba[4][157]==COLOR_FONDO_UI);sonidoPrueba=true;
  iniciarJuego(); COMPROBAR(estadoActual == ESTADO_MENU);
  pulsar('b'); COMPROBAR(estadoActual == ESTADO_AYUDA);
  pulsar('a'); COMPROBAR(estadoActual == ESTADO_MENU);
  pulsar('a'); COMPROBAR(estadoActual == ESTADO_CONFIGURACION);
  pulsar('a'); COMPROBAR(estadoActual == ESTADO_CUENTA && iniciosPrueba == 1);
  EstadoControles c = {}; c.arriba = true; relojPrueba += 3000;
  actualizarJuego(c); COMPROBAR(estadoActual == ESTADO_CUENTA);
  COMPROBAR(avisosControlesPrueba==0);
  actualizarJuego({}); COMPROBAR(estadoActual == ESTADO_JUGANDO);
  COMPROBAR(avisosControlesPrueba==0);
  puntosPrueba = 12; pulsar('c'); COMPROBAR(estadoActual == ESTADO_PAUSA);
  relojPrueba += 10000; pulsar('c'); COMPROBAR(estadoActual == ESTADO_CUENTA);
  relojPrueba += 3000; actualizarJuego({});
  COMPROBAR(estadoActual == ESTADO_JUGANDO && puntosPrueba == 12 && iniciosPrueba == 1);
  resultadoPrueba = PARTIDA_PERDIDA;
  unsigned long antes = relojPrueba; actualizarJuego({});
  COMPROBAR(estadoActual == ESTADO_CHOQUE && relojPrueba == antes);
  pulsar('d'); COMPROBAR(!sonidoPrueba);
  relojPrueba += 600; actualizarJuego({});
  COMPROBAR(estadoActual == ESTADO_FIN && recordActual() == 12 && escriturasPrueba == 1);
  resultadoPrueba = PARTIDA_CONTINUA; iniciarJuego(); COMPROBAR(recordActual() == 12);
  puntosPrueba = 3; cambiarEstado(ESTADO_FIN); COMPROBAR(escriturasPrueba == 1);
  dificultad = FACIL; COMPROBAR(recordActual() == 0);
  dificultad = NORMAL; modoSnake = SNAKE_LIBRE; COMPROBAR(recordActual() == 0);
  puntosPrueba = 30; victoria = true; cambiarEstado(ESTADO_FIN);
  COMPROBAR(victoria && recordActual() == 30 && escriturasPrueba == 2);
  modoSnake = SNAKE_CLASICO; COMPROBAR(recordActual() == 12);
  nvsDisponiblePrueba = false; iniciarJuego(); COMPROBAR(!almacenamientoDisponible);
  puntosPrueba = 7; cambiarEstado(ESTADO_FIN);
  COMPROBAR(recordActual() == 7 && escriturasPrueba == 2);
  // El cuarto juego tiene récord propio; B en partida no vuelve al menú.
  juegoActual=3;dificultad=NORMAL;resultadoPrueba=PARTIDA_CONTINUA;
  cambiarEstado(ESTADO_CUENTA);relojPrueba+=3000;actualizarJuego({});
  pulsar('b');COMPROBAR(estadoActual==ESTADO_JUGANDO);
  puntosPrueba=300;resultadoPrueba=PARTIDA_GANADA;actualizarJuego({});
  COMPROBAR(estadoActual==ESTADO_FIN && victoria && recordActual()==300);
  juegoActual=0;COMPROBAR(recordActual()==7);
  resultadoPrueba=PARTIDA_CONTINUA;cambiarEstado(ESTADO_CUENTA);
  relojPrueba+=3250;c={};c.a.presionado=true;actualizarJuego(c);
  COMPROBAR(estadoActual==ESTADO_CUENTA && avisosControlesPrueba==1);
  actualizarJuego(c);COMPROBAR(avisosControlesPrueba==1);
  actualizarJuego({});COMPROBAR(estadoActual==ESTADO_JUGANDO);
  // Timeout exclusivo de FIN: cuenta desde entrada y se reinicia con cualquier control.
  resultadoPrueba=PARTIDA_CONTINUA;cambiarEstado(ESTADO_FIN);
  relojPrueba+=14999;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_FIN);
  relojPrueba++;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_MENU);
  cambiarEstado(ESTADO_FIN);relojPrueba+=14999;pulsar('d');
  relojPrueba+=14999;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_FIN);
  relojPrueba++;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_MENU);
  cambiarEstado(ESTADO_FIN);c={};c.izquierda=true;
  relojPrueba+=16000;actualizarJuego(c);COMPROBAR(estadoActual==ESTADO_FIN);
  relojPrueba+=14999;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_FIN);
  relojPrueba++;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_MENU);
  cambiarEstado(ESTADO_PAUSA);relojPrueba+=20000;actualizarJuego({});COMPROBAR(estadoActual==ESTADO_PAUSA);
  relojPrueba=0xfffffff0UL;cambiarEstado(ESTADO_FIN);relojPrueba+=15000;
  actualizarJuego({});COMPROBAR(estadoActual==ESTADO_MENU);
  unsigned long t = 0xfffffff0UL;
  COMPROBAR(pasosPendientes(0x22UL, t, 25) == 2 && t == 0x22UL);
  t = 0; COMPROBAR(pasosPendientes(1000, t, 25) == 4 && t == 1000);
  std::puts("Consola: OK (altavoz, cuenta, pausa, victoria, NVS, timeout de 15 s, actividad y rollover)");
}
