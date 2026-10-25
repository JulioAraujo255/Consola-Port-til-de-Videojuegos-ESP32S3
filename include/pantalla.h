/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: pantalla.h
 *  Descripción: "especificación" del módulo de pantalla.
 *
 *  El juego dibuja siempre en un área LÓGICA de 160 x 128
 *  píxeles (el tamaño de la pantalla real ST7735).
 *  - En la placa física, esa área ocupa toda la pantalla.
 *  - En Wokwi (pantalla de 320 x 240), se dibuja centrada.
 *  El juego no necesita saber en cuál de las dos está.
 * ============================================================
 */

#ifndef PANTALLA_H
#define PANTALLA_H

#include <Arduino.h>

// ------------------------------------------------------------
//  TAMAÑO LÓGICO DE LA PANTALLA (ST7735 en horizontal)
// ------------------------------------------------------------
const int ANCHO_PANTALLA = 160;
const int ALTO_PANTALLA  = 128;

// ------------------------------------------------------------
//  PALETA DE COLORES (RGB565) - según la interfaz v2
// ------------------------------------------------------------
const uint16_t COLOR_NEGRO        = 0x0000;
const uint16_t COLOR_VERDE        = 0x07E0;  // Textos, marcos, cabeza
const uint16_t COLOR_VERDE_MEDIO  = 0x05E0;  // Cuerpo de la serpiente
const uint16_t COLOR_VERDE_OSCURO = 0x0320;  // Marcos secundarios
const uint16_t COLOR_VERDE_GRILLA = 0x0140;  // Puntos de la grilla
const uint16_t COLOR_COMIDA       = 0xAFE5;  // Verde amarillento

// ------------------------------------------------------------
//  PALETA DE LA INTERFAZ FINAL (a color)
//  Menú, pausa, game over y barra de puntos de los juegos.
//  Los colores propios de cada juego están en su archivo
//  (snake.cpp, dino.cpp, carreras.cpp).
// ------------------------------------------------------------
const uint16_t COLOR_BLANCO       = 0xFFFF;
const uint16_t COLOR_GRIS_CLARO   = 0xCE59;  // Opciones no elegidas
const uint16_t COLOR_GRIS         = 0x8475;  // Textos de ayuda
const uint16_t COLOR_FONDO_UI     = 0x10A5;  // Azul noche (fondo de menús)
const uint16_t COLOR_MARCO_UI     = 0x3A97;  // Azul (marcos y líneas)
const uint16_t COLOR_ADORNO       = 0x065F;  // Celeste (esquinas del marco)
const uint16_t COLOR_AMARILLO     = 0xFE87;  // Títulos, récord, selección
const uint16_t COLOR_ROJO         = 0xE186;  // "GAME OVER"
const uint16_t COLOR_ROJO_SOMBRA  = 0x68A2;  // Sombra de los títulos
const uint16_t COLOR_BARRA        = 0x18E5;  // Barra de puntos de los juegos

// Color de cada juego en el menú
const uint16_t COLOR_JUEGO_SNAKE    = 0x2CA7;  // Verde
const uint16_t COLOR_JUEGO_DINO     = 0xDB62;  // Naranja
const uint16_t COLOR_JUEGO_CARRERAS = 0xC8E3;  // Rojo

// ------------------------------------------------------------
//  FUNCIONES DEL MÓDULO
//  Todas las coordenadas son LÓGICAS (0..159, 0..127).
// ------------------------------------------------------------

// Inicia el bus SPI y la pantalla. Se llama una vez en setup().
void iniciarPantalla();

// Presenta únicamente el resultado final del cuadro pendiente.
void presentarPantalla();

// Pinta toda el área lógica de negro.
void limpiarPantalla();

// Figuras básicas
void dibujarPixel(int x, int y, uint16_t color);
void dibujarLinea(int x0, int y0, int x1, int y1, uint16_t color);
void rectanguloRelleno(int x, int y, int ancho, int alto, uint16_t color);
void rectanguloBorde(int x, int y, int ancho, int alto, uint16_t color);
void rectanguloRedondeado(int x, int y, int ancho, int alto, int radio,
                          uint16_t color, bool relleno);

// Compone una imagen RGB565 en RAM, recortada al área lógica.
// presentarPantalla() enviará el resultado final por bloques SPI.
// El búfer es mutable por compatibilidad con Adafruit_SPITFT; no se altera.
void dibujarImagenRGB565(int x, int y, int ancho, int alto, uint16_t* pixeles);

// Texto (fuente estándar: cada letra mide 6 x 8 píxeles por tamaño)
int  anchoTexto(const char* texto, uint8_t tamano);
void escribirTexto(int x, int y, const char* texto, uint16_t color, uint8_t tamano);
void escribirTextoConFondo(int x, int y, const char* texto,
                           uint16_t color, uint16_t fondo, uint8_t tamano);
void escribirCentrado(int y, const char* texto, uint16_t color, uint8_t tamano);

#endif  // PANTALLA_H
