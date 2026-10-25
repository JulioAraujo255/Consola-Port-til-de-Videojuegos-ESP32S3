#ifndef BRASA_H
#define BRASA_H

#include "controles.h"
#include "partida.h"

// Plataformas original: dos mundos de cuatro niveles; cada cuarto tiene jefe.
void comenzarBrasa();
ResultadoPartida actualizarBrasa(const EstadoControles& controles);
void redibujarBrasa();
int puntuacionBrasa();

#endif
