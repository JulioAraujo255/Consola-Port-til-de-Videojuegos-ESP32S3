/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: sonido.h
 *  Descripción: "especificación" del módulo de sonido (FOP-01).
 *
 *  Sonido retro estilo consola de 8 bits con el zumbador
 *  pasivo (GPIO 4). Cada efecto es una lista de "pasos"
 *  (notas, barridos de frecuencia o ruido).
 *
 *  El sonido corre en su PROPIA TAREA (FreeRTOS), en paralelo
 *  con el juego: aunque el juego esté dibujando una pantalla
 *  completa, el sonido sigue avanzando sin cortarse.
 * ============================================================
 */

#ifndef SONIDO_H
#define SONIDO_H

#include <Arduino.h>

// ------------------------------------------------------------
//  EFECTOS DISPONIBLES
// ------------------------------------------------------------
enum EfectoSonido {
  SONIDO_INICIO,        // Melodía al encender la consola
  SONIDO_MENU_MOVER,    // "tic" al mover la selección
  SONIDO_MENU_ELEGIR,   // Dos notas que suben (botón A)
  SONIDO_PAUSA,         // Al pausar y al seguir
  SONIDO_COMER,         // Snake come una manzana
  SONIDO_SALTO,         // Dino Run: el triceratops salta
  SONIDO_CARRIL,        // Carreras: cambio de carril
  SONIDO_NIVEL,         // Sube el nivel o la velocidad
  SONIDO_CHOQUE,        // Choque (ruido + barrido hacia abajo)
  SONIDO_GAME_OVER,     // Melodía triste
  SONIDO_RECORD         // Fanfarria de récord nuevo
};

// ------------------------------------------------------------
//  FUNCIONES DEL MÓDULO
// ------------------------------------------------------------

// Prepara el zumbador y arranca la tarea de sonido.
// Se llama una vez al iniciar.
void iniciarSonido();

// Empieza a sonar un efecto (reemplaza al que estuviera sonando).
void reproducirSonido(EfectoSonido efecto);

// Sonido de motor de fondo (Carreras): frecuencia base en Hz.
// 0 = sin motor. Los efectos suenan por encima del motor.
void fijarMotor(int frecuenciaBase);

// Espera unos milisegundos (el sonido sigue sonando mientras tanto).
void esperarConSonido(unsigned long milisegundos);

// Activa o silencia todo el sonido (botón D).
void alternarSilencio();

#endif  // SONIDO_H