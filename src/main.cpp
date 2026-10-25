/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: main.cpp
 *  Descripción: programa principal de la mini consola.
 *
 *  Organización del código por módulos:
 *  - pines.h          -> asignación de pines
 *  - pantalla.h/.cpp  -> dibujo en la pantalla
 *  - controles.h/.cpp -> lectura del joystick y los botones
 *  - juego.h/.cpp     -> lógica del juego y sus pantallas
 *
 *  Este archivo solo inicia los módulos y los conecta.
 * ============================================================
 */

#include <Arduino.h>
#include "pantalla.h"
#include "controles.h"
#include "juego.h"

// ------------------------------------------------------------
//  SETUP: se ejecuta una sola vez al encender
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== MINI CONSOLA ESP32-S3 - RETRO GAMES ===");

  iniciarPantalla();    // 1. Pantalla
  iniciarControles();   // 2. Joystick y botones
  iniciarJuego();       // 3. Juego (muestra la pantalla de inicio)
  presentarPantalla();
}

// ------------------------------------------------------------
//  LOOP: se repite continuamente
// ------------------------------------------------------------
void loop() {
  // Se leen los controles y el juego decide qué hacer con ellos
  EstadoControles controles = leerControles();
  actualizarJuego(controles);
  presentarPantalla();

  delay(5);  // Pequeña pausa entre lecturas
}
