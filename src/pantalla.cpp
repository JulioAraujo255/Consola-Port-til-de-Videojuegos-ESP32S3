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
#include "cuadro.h"

static CuadroRGB565<ANCHO_PANTALLA,ALTO_PANTALLA> cuadro;
class LienzoEstatico : public GFXcanvas16 {
 public:
  LienzoEstatico() : GFXcanvas16(ANCHO_PANTALLA,ALTO_PANTALLA,false) {
    buffer=&cuadro.pixeles[0][0];
  }
};
static LienzoEstatico lienzo;

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
static inline uint16_t ajustarColor(uint16_t color) {
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
  lienzo.setTextWrap(false);
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
  rectanguloRelleno(0,0,ANCHO_PANTALLA,ALTO_PANTALLA,COLOR_NEGRO);
}

void presentarPantalla() {
  if(!cuadro.pendiente()) return;
  bool escribiendo=false;
  cuadro.presentar([&](int x,int y,int ancho,int alto,uint16_t* datos,int salto) {
    if(!escribiendo) { tft.startWrite(); escribiendo=true; }
    tft.setAddrWindow(x+desplazamientoX,y+desplazamientoY,ancho,alto);
#if !defined(MODO_SIMULACION) && INTERCAMBIAR_ROJO_AZUL
    uint16_t fila[ANCHO_PANTALLA];
    for(int f=0;f<alto;f++) {
      for(int c=0;c<ancho;c++) fila[c]=ajustarColor(datos[f*salto+c]);
      tft.writePixels(fila,ancho);
    }
#else
    if(ancho==salto) tft.writePixels(datos,ancho*alto);
    else for(int f=0;f<alto;f++) tft.writePixels(datos+f*salto,ancho);
#endif
  });
  if(escribiendo) tft.endWrite();
}

// ------------------------------------------------------------
//  4. FIGURAS BÁSICAS
// ------------------------------------------------------------
void dibujarPixel(int x, int y, uint16_t color) {
  lienzo.drawPixel(x,y,color);
  cuadro.ensuciar(x,y,1,1);
}

void dibujarLinea(int x0, int y0, int x1, int y1, uint16_t color) {
  lienzo.drawLine(x0,y0,x1,y1,color);
  cuadro.ensuciar(min(x0,x1),min(y0,y1),abs(x1-x0)+1,abs(y1-y0)+1);
}

void rectanguloRelleno(int x, int y, int ancho, int alto, uint16_t color) {
  lienzo.fillRect(x,y,ancho,alto,color);
  cuadro.ensuciar(x,y,ancho,alto);
}

void rectanguloBorde(int x, int y, int ancho, int alto, uint16_t color) {
  lienzo.drawRect(x,y,ancho,alto,color);
  cuadro.ensuciar(x,y,ancho,alto);
}

void rectanguloRedondeado(int x, int y, int ancho, int alto, int radio,
                          uint16_t color, bool relleno) {
  if (relleno) {
    lienzo.fillRoundRect(x,y,ancho,alto,radio,color);
  } else {
    lienzo.drawRoundRect(x,y,ancho,alto,radio,color);
  }
  cuadro.ensuciar(x,y,ancho,alto);
}

// ------------------------------------------------------------
//  5. TEXTO
// ------------------------------------------------------------

void dibujarImagenRGB565(int x, int y, int ancho, int alto, uint16_t* pixeles) {
  if (!pixeles || ancho <= 0 || alto <= 0) return;
  int x0 = max(x, 0), y0 = max(y, 0);
  int x1 = min(x + ancho, ANCHO_PANTALLA), y1 = min(y + alto, ALTO_PANTALLA);
  if (x1 <= x0 || y1 <= y0) return;
  for (int fila = y0; fila < y1; fila++) {
    uint16_t* origen = pixeles + (fila - y) * ancho + (x0 - x);
    memcpy(&cuadro.pixeles[fila][x0],origen,(x1-x0)*sizeof(uint16_t));
  }
  cuadro.ensuciar(x0,y0,x1-x0,y1-y0);
}

// Ancho en píxeles de un texto. Cada letra ocupa 6 columnas
// (5 de dibujo + 1 de espacio); se descuenta el espacio final.
int anchoTexto(const char* texto, uint8_t tamano) {
  int largo = strlen(texto);
  if (largo == 0) return 0;
  return largo * 6 * tamano - tamano;
}

// Texto con fondo transparente (solo se pintan las letras).
void escribirTexto(int x, int y, const char* texto, uint16_t color, uint8_t tamano) {
  lienzo.setTextSize(tamano);
  lienzo.setTextColor(color);
  lienzo.setCursor(x,y);
  lienzo.print(texto);
  cuadro.ensuciar(x,y,(int)strlen(texto)*6*tamano,8*tamano);
}

// Texto con fondo: borra lo que había debajo de cada letra.
// Útil para números que cambian (puntuación, nivel).
void escribirTextoConFondo(int x, int y, const char* texto,
                           uint16_t color, uint16_t fondo, uint8_t tamano) {
  lienzo.setTextSize(tamano);
  lienzo.setTextColor(color,fondo);
  lienzo.setCursor(x,y);
  lienzo.print(texto);
  cuadro.ensuciar(x,y,(int)strlen(texto)*6*tamano,8*tamano);
}

// Texto centrado horizontalmente en el área lógica.
void escribirCentrado(int y, const char* texto, uint16_t color, uint8_t tamano) {
  int x = (ANCHO_PANTALLA - anchoTexto(texto, tamano)) / 2;
  escribirTexto(x, y, texto, color, tamano);
}
