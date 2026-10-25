/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: snake.h
 *  Descripción: "especificación" del juego Snake.
 *
 *  Snake ya no maneja menús ni pausas: de eso se encarga la
 *  consola (juego.cpp). Este módulo solo sabe jugar Snake.
 *
 *  Todos los juegos de la consola ofrecen las MISMAS cuatro
 *  funciones (comenzar, actualizar, redibujar, puntuación).
 *  Así la consola puede manejar cualquier juego de la misma
 *  forma, como una interfaz común.
 * ============================================================
 */

#ifndef SNAKE_H
#define SNAKE_H

#include "controles.h"

// Reinicia la partida y dibuja el tablero completo.
// La consola la llama al elegir SNAKE en el menú o al reintentar.
void comenzarSnake();

// Avanza el juego según el tiempo y los controles.
// Se llama en cada vuelta de loop() mientras se juega.
// Devuelve false cuando la víbora choca (fin de la partida).
bool actualizarSnake(const EstadoControles& controles);

// Vuelve a dibujar la partida tal como estaba.
// La consola la llama al salir de la pausa.
void redibujarSnake();

// Puntuación de la partida actual (para el récord y el game over).
int puntuacionSnake();

#endif  // SNAKE_H