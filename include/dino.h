/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: dino.h
 *  Descripción: "especificación" del juego Dino Run.
 *
 *  Un triceratops corre por el desierto: salta las piedras
 *  (A o joystick arriba) y se agacha ante las aves (joystick
 *  abajo). Los puntos suben con la distancia recorrida.
 *
 *  Ofrece las MISMAS cuatro funciones que Snake, para que la
 *  consola (juego.cpp) lo maneje de la misma forma.
 * ============================================================
 */

#ifndef DINO_H
#define DINO_H

#include "controles.h"
#include "partida.h"

// Reinicia la partida y dibuja el escenario completo.
void comenzarDino();

// Avanza el juego según el tiempo y los controles.
// Se llama en cada vuelta de loop() mientras se juega.
// Devuelve PARTIDA_PERDIDA al chocar, o PARTIDA_CONTINUA.
ResultadoPartida actualizarDino(const EstadoControles& controles);

// Vuelve a dibujar la partida tal como estaba (al salir de la pausa).
void redibujarDino();

// Puntuación de la partida actual.
int puntuacionDino();

#endif  // DINO_H
