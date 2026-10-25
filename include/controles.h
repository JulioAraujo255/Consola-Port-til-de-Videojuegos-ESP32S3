/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: controles.h
 *  Descripción: "especificación" del módulo de controles.
 *
 *  Define QUÉ información de los controles recibe el juego.
 *  El CÓMO se obtiene (Wokwi o gamepad real) está en
 *  controles.cpp, y el juego no necesita saberlo.
 * ============================================================
 */

#ifndef CONTROLES_H
#define CONTROLES_H

#include <Arduino.h>

// ------------------------------------------------------------
//  ESTADO DE UN BOTÓN
//  Similar a un registro (%ROWTYPE) en PL/SQL: agrupa datos
//  relacionados en una sola variable.
// ------------------------------------------------------------
struct EstadoBoton {
  bool presionado;        // true mientras el botón está apretado
  bool recienPresionado;  // true SOLO en la lectura en que se apretó
                          // (sirve para menús: un toque = una acción)
};

// ------------------------------------------------------------
//  ESTADO COMPLETO DE LOS CONTROLES
//  Es lo que el juego recibe en cada lectura.
// ------------------------------------------------------------
struct EstadoControles {
  // Direcciones del joystick
  bool arriba;
  bool abajo;
  bool izquierda;
  bool derecha;

  // Botones de acción
  EstadoBoton a;   // Jugar / confirmar
  EstadoBoton b;   // Volver al inicio
  EstadoBoton c;   // Pausa
  EstadoBoton d;   // Reservado
};

// ------------------------------------------------------------
//  FUNCIONES DEL MÓDULO
// ------------------------------------------------------------

// Prepara los pines o la comunicación de los controles.
// Se llama una sola vez, en setup().
void iniciarControles();

// Lee los controles y devuelve su estado actual.
// Se llama en cada vuelta de loop().
EstadoControles leerControles();

#endif  // CONTROLES_H