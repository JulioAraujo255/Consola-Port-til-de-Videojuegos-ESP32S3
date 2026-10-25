/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: carreras.h
 *  Descripción: "especificación" del juego Carreras.
 *
 *  Una ruta de 3 carriles vista desde arriba: el jugador
 *  cambia de carril (joystick izquierda / derecha) para
 *  esquivar los autos rivales. Los puntos suben con la
 *  distancia y cada 100 puntos sube el nivel (más velocidad).
 *
 *  Ofrece las MISMAS cuatro funciones que los otros juegos,
 *  para que la consola (juego.cpp) lo maneje de la misma forma.
 * ============================================================
 */

#ifndef CARRERAS_H
#define CARRERAS_H

#include "controles.h"
#include "partida.h"

// Reinicia la partida y dibuja la ruta completa.
void comenzarCarreras();

// Avanza el juego según el tiempo y los controles.
// Se llama en cada vuelta de loop() mientras se juega.
// Devuelve PARTIDA_PERDIDA al chocar, o PARTIDA_CONTINUA.
ResultadoPartida actualizarCarreras(const EstadoControles& controles);

// Vuelve a dibujar la partida tal como estaba (al salir de la pausa).
void redibujarCarreras();

// Puntuación de la partida actual.
int puntuacionCarreras();

#endif  // CARRERAS_H
