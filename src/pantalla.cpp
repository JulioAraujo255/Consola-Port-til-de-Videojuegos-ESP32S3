/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: pantalla.cpp
 *  Descripción: "cuerpo" del módulo de pantalla.
 *
 *  Todas las funciones reciben coordenadas LÓGICAS (160 x 128)
 *  y les suman un desplazamiento:
 *  - Placa física (ST7735 de 160 x 128): desplazamiento 0.
 *  - Wokwi (ILI9341 de 320 x 240): área centrada.
 * ============================================================
 */

#include "pantalla.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include "pines.h"

// ------------------------------------------------------------
//  1. OBJETO DE PANTALLA (según el entorno)
//  "static": solo este archivo usa el objeto directamente.
//  El resto del programa dibuja a través de las funciones.
// ------------------------------------------------------------
#ifdef MODO_SIMULACION
  #include <Adafruit_ILI9341.h>
  // ILI9341: orden de pines (SPI, DC, CS, RST)
  static Adafruit_ILI9341 tft(&SPI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
#else
  #include <Adafruit_ST7735.h>
  // ST7735: orden de pines (SPI, CS, DC, RST)
  static Adafruit_ST7735 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);
#endif

// ------------------------------------------------------------
//  AJUSTE DE COLORES (solo placa física)
//  Algunas pantallas ST7735 traen el rojo y el azul cambiados
//  de lugar. Si en la placa la manzana se ve azul o la víbora
//  roja, poner esto en true.
// ------------------------------------------------------------
#define INTERCAMBIAR_ROJO_AZUL false

// Devuelve el color listo para enviar a la pantalla.
// RGB565: 5 bits de rojo | 6 de verde | 5 de azul.
// Si hace falta, intercambia los 5 bits del rojo con los del azul.
static uint16_t ajustarColor(uint16_t color) {
#if !defined(MODO_SIMULACION) && INTERCAMBIAR_ROJO_AZUL
  uint16_t rojo  = (color >> 11) & 0x1F;
  uint16_t verde = (color >> 5)  & 0x3F;
  uint16_t azul  =  color        & 0x1F;
  return (azul << 11) | (verde << 5) | rojo;
#else
  return color;
#endif
}

// ------------------------------------------------------------
//  2. DESPLAZAMIENTO DEL ÁREA LÓGICA
// ------------------------------------------------------------
static int desplazamientoX = 0;
static int desplazamientoY = 0;

// ------------------------------------------------------------
//  3. INICIO Y LIMPIEZA
// ------------------------------------------------------------
void iniciarPantalla() {
  // Pines del bus SPI: (SCK, MISO, MOSI, CS). MISO no se usa.
  SPI.begin(PIN_TFT_SCK, -1, PIN_TFT_MOSI, PIN_TFT_CS);

#ifdef MODO_SIMULACION
  tft.begin();
#else
  tft.initR(INITR_BLACKTAB);
#endif

  tft.setRotation(1);       // Horizontal
  tft.setTextWrap(false);   // El texto no salta de línea solo
  tft.fillScreen(COLOR_NEGRO);

  // Se centra el área de 160 x 128 en la pantalla física
  desplazamientoX = (tft.width()  - ANCHO_PANTALLA) / 2;
  desplazamientoY = (tft.height() - ALTO_PANTALLA)  / 2;

#ifdef MODO_SIMULACION
  // Marco gris alrededor del área para ver el tamaño real
  tft.drawRect(desplazamientoX - 2, desplazamientoY - 2,
               ANCHO_PANTALLA + 4, ALTO_PANTALLA + 4, 0x4208);
#endif
}

void limpiarPantalla() {
  tft.fillRect(desplazamientoX, desplazamientoY,
               ANCHO_PANTALLA, ALTO_PANTALLA, COLOR_NEGRO);
}

// ------------------------------------------------------------
//  4. FIGURAS BÁSICAS
// ------------------------------------------------------------
void dibujarPixel(int x, int y, uint16_t color) {
  tft.drawPixel(x + desplazamientoX, y + desplazamientoY, ajustarColor(color));
}

void dibujarLinea(int x0, int y0, int x1, int y1, uint16_t color) {
  tft.drawLine(x0 + desplazamientoX, y0 + desplazamientoY,
               x1 + desplazamientoX, y1 + desplazamientoY, ajustarColor(color));
}

void rectanguloRelleno(int x, int y, int ancho, int alto, uint16_t color) {
  tft.fillRect(x + desplazamientoX, y + desplazamientoY, ancho, alto, ajustarColor(color));
}

void rectanguloBorde(int x, int y, int ancho, int alto, uint16_t color) {
  tft.drawRect(x + desplazamientoX, y + desplazamientoY, ancho, alto, ajustarColor(color));
}

void rectanguloRedondeado(int x, int y, int ancho, int alto, int radio,
                          uint16_t color, bool relleno) {
  if (relleno) {
    tft.fillRoundRect(x + desplazamientoX, y + desplazamientoY,
                      ancho, alto, radio, ajustarColor(color));
  } else {
    tft.drawRoundRect(x + desplazamientoX, y + desplazamientoY,
                      ancho, alto, radio, ajustarColor(color));
  }
}

// ------------------------------------------------------------
//  5. TEXTO
// ------------------------------------------------------------

// Ancho en píxeles de un texto. Cada letra ocupa 6 columnas
// (5 de dibujo + 1 de espacio); se descuenta el espacio final.
int anchoTexto(const char* texto, uint8_t tamano) {
  int largo = strlen(texto);
  if (largo == 0) return 0;
  return largo * 6 * tamano - tamano;
}

// Texto con fondo transparente (solo se pintan las letras).
void escribirTexto(int x, int y, const char* texto, uint16_t color, uint8_t tamano) {
  tft.setTextSize(tamano);
  tft.setTextColor(ajustarColor(color));
  tft.setCursor(x + desplazamientoX, y + desplazamientoY);
  tft.print(texto);
}

// Texto con fondo: borra lo que había debajo de cada letra.
// Útil para números que cambian (puntuación, nivel).
void escribirTextoConFondo(int x, int y, const char* texto,
                           uint16_t color, uint16_t fondo, uint8_t tamano) {
  tft.setTextSize(tamano);
  tft.setTextColor(ajustarColor(color), ajustarColor(fondo));
  tft.setCursor(x + desplazamientoX, y + desplazamientoY);
  tft.print(texto);
}

// Texto centrado horizontalmente en el área lógica.
void escribirCentrado(int y, const char* texto, uint16_t color, uint8_t tamano) {
  int x = (ANCHO_PANTALLA - anchoTexto(texto, tamano)) / 2;
  escribirTexto(x, y, texto, color, tamano);
}