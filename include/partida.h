#ifndef PARTIDA_H
#define PARTIDA_H

enum ResultadoPartida { PARTIDA_CONTINUA, PARTIDA_PERDIDA, PARTIDA_GANADA };
enum Dificultad { FACIL, NORMAL, DIFICIL };
enum ModoSnake { SNAKE_CLASICO, SNAKE_LIBRE, SNAKE_LABERINTO };

// La consola es la única fuente de configuración y récords.
Dificultad dificultadActual();
ModoSnake modoSnakeActual();
int recordActual();

// Limita la recuperación después de un bloqueo largo a cuatro pasos.
// El tiempo sobrante normal se conserva: el dibujo no cambia la velocidad.
inline unsigned int pasosPendientes(unsigned long ahora, unsigned long& anterior,
                                    unsigned long intervalo) {
  unsigned long pasos = (ahora - anterior) / intervalo;
  if (pasos > 4) {
    anterior = ahora - (ahora - anterior) % intervalo;
    return 4;
  }
  anterior += pasos * intervalo;
  return static_cast<unsigned int>(pasos);
}

#endif
