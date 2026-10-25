#pragma once
#include "pantalla.h"
#include "sonido.h"
#include "partida.h"
#include <cstdlib>
#include <vector>
#include <string>

// Cuando PlatformIO ya descargó GFX, las capturas usan la misma fuente.
#if __has_include("../../.pio/libdeps/simulacion/Adafruit GFX Library/glcdfont.c")
#include "../../.pio/libdeps/simulacion/Adafruit GFX Library/glcdfont.c"
#define PRUEBA_FUENTE_GFX
#endif

#define COMPROBAR(condicion) do { if (!(condicion)) { \
  std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condicion); std::exit(1); \
} } while (0)

inline int saltosPrueba = 0;
inline int motorPrueba = 0;
inline bool sonidoPrueba = true;
inline uint16_t pixelesPrueba[ALTO_PANTALLA][ANCHO_PANTALLA] = {};
inline int enviosImagenPrueba = 0;
inline int avisosControlesPrueba = 0;
inline std::vector<std::string> textosPrueba;
inline bool apareceTexto(const char* prefijo) {
  for(const std::string& texto:textosPrueba) if(texto.rfind(prefijo,0)==0) return true;
  return false;
}
void dibujarImagenRGB565(int x,int y,int a,int h,uint16_t* datos) {
  enviosImagenPrueba++;
  for(int f=0;f<h;f++) for(int c=0;c<a;c++) {
    if(x+c>=0 && x+c<ANCHO_PANTALLA && y+f>=0 && y+f<ALTO_PANTALLA)
      pixelesPrueba[y+f][x+c]=datos[f*a+c];
  }
}
void dibujarPixel(int x, int y, uint16_t color) {
  if (x >= 0 && x < ANCHO_PANTALLA && y >= 0 && y < ALTO_PANTALLA) pixelesPrueba[y][x] = color;
}
void rectanguloRelleno(int x, int y, int a, int h, uint16_t color) {
  for (int f = max(y, 0); f < min(y + h, ALTO_PANTALLA); f++)
    for (int c = max(x, 0); c < min(x + a, ANCHO_PANTALLA); c++) dibujarPixel(c, f, color);
}
void dibujarLinea(int x0, int y0, int x1, int y1, uint16_t color) {
  int dx = abs(x1-x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1-y0), sy = y0 < y1 ? 1 : -1, error = dx + dy;
  while (true) {
    dibujarPixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * error;
    if (e2 >= dy) { error += dy; x0 += sx; }
    if (e2 <= dx) { error += dx; y0 += sy; }
  }
}
void rectanguloBorde(int x, int y, int a, int h, uint16_t color) {
  dibujarLinea(x,y,x+a-1,y,color); dibujarLinea(x,y+h-1,x+a-1,y+h-1,color);
  dibujarLinea(x,y,x,y+h-1,color); dibujarLinea(x+a-1,y,x+a-1,y+h-1,color);
}
void rectanguloRedondeado(int x,int y,int a,int h,int,uint16_t c,bool relleno) {
  if (relleno) rectanguloRelleno(x,y,a,h,c); else rectanguloBorde(x,y,a,h,c);
}
int anchoTexto(const char* t, uint8_t s) { return *t?(int)strlen(t)*6*s-s:0; }
void escribirTexto(int x,int y,const char* texto,uint16_t color,uint8_t escala) {
  textosPrueba.emplace_back(texto);
#ifdef PRUEBA_FUENTE_GFX
  for(int i=0;texto[i];i++) for(int c=0;c<5;c++) for(int f=0;f<8;f++)
    if(font[(uint8_t)texto[i]*5+c]&(1<<f))
      rectanguloRelleno(x+(i*6+c)*escala,y+f*escala,escala,escala,color);
#endif
}
void escribirTextoConFondo(int x,int y,const char* texto,uint16_t color,uint16_t fondo,uint8_t escala) {
  rectanguloRelleno(x,y,(int)strlen(texto)*6*escala,8*escala,fondo);
  escribirTexto(x,y,texto,color,escala);
}
void escribirCentrado(int y,const char* texto,uint16_t color,uint8_t escala) {
  if(strcmp(texto,"SUELTA CONTROLES")==0) avisosControlesPrueba++;
  escribirTexto((ANCHO_PANTALLA-anchoTexto(texto,escala))/2,y,texto,color,escala);
}
void iniciarSonido() {}
void reproducirSonido(EfectoSonido e) { if (e == SONIDO_SALTO) saltosPrueba++; }
void fijarMotor(int f) { motorPrueba = f; }
void esperarConSonido(unsigned long ms) { delay(ms); }
void alternarSilencio() { sonidoPrueba = !sonidoPrueba; }
bool sonidoHabilitado() { return sonidoPrueba; }

#ifndef PRUEBA_CONSOLA
inline Dificultad dificultadPrueba = NORMAL;
inline ModoSnake modoPrueba = SNAKE_CLASICO;
Dificultad dificultadActual() { return dificultadPrueba; }
ModoSnake modoSnakeActual() { return modoPrueba; }
int recordActual() { return 0; }
#endif
