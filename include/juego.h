/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: juego.h
 *  Descripción: "especificación" de la consola (interfaz v3).
 *
 *  La consola funciona como una MÁQUINA DE ESTADOS: en cada
 *  momento está en uno solo de los cuatro estados, y los
 *  controles provocan el paso de un estado a otro
 *  (ver el diagrama de navegación de la interfaz v3).
 *
 *  El menú, la pausa y el game over son COMUNES a todos los
 *  juegos. Cada juego (snake.cpp, etc.) solo sabe jugar.
 * ============================================================
 */

#ifndef JUEGO_H
#define JUEGO_H

#include "controles.h"

// ------------------------------------------------------------
//  ESTADOS DE LA CONSOLA
//  Similar a una restricción CHECK en una base de datos:
//  la variable de estado solo puede tomar estos valores.
// ------------------------------------------------------------
enum EstadoJuego {
  ESTADO_MENU,      // Menú "RETRO GAMES" (elegir juego con joystick + A)
  ESTADO_JUGANDO,   // Partida en curso del juego elegido
  ESTADO_PAUSA,     // Partida detenida (botón C)
  ESTADO_FIN        // Game over (Reintentar / Menu)
};

// ------------------------------------------------------------
//  FUNCIONES DEL MÓDULO
// ------------------------------------------------------------

// Prepara la consola y muestra el menú.
// Se llama una vez en setup(), después de iniciar la pantalla.
void iniciarJuego();

// Avanza la consola un paso según el estado actual y los controles.
// Se llama en cada vuelta de loop().
void actualizarJuego(const EstadoControles& controles);

#endif  // JUEGO_H