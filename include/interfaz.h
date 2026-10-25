#ifndef INTERFAZ_H
#define INTERFAZ_H
#include "pantalla.h"
#include "sonido.h"

constexpr int ALTO_MARCADOR=20;

inline void dibujarAltavoz(uint16_t fondo=COLOR_BARRA) {
  rectanguloRelleno(145,1,12,11,fondo);
  bool activo=sonidoHabilitado();
  uint16_t color=activo?COLOR_BLANCO:COLOR_GRIS_CLARO;
  rectanguloRelleno(145,4,3,4,color);
  for(int i=0;i<3;i++) rectanguloRelleno(148+i,4-i,1,4+2*i,color);
  if(activo) {
    dibujarLinea(152,3,153,4,color);dibujarLinea(153,4,153,7,color);dibujarLinea(153,7,152,8,color);
    dibujarLinea(154,1,156,3,color);dibujarLinea(156,3,156,8,color);dibujarLinea(156,8,154,10,color);
  } else {
    dibujarLinea(152,4,156,8,COLOR_ROJO);dibujarLinea(156,4,152,8,COLOR_ROJO);
  }
}

inline void filaMarcador(int y,const char* izquierda,const char* derecha="") {
  escribirTexto(2,y,izquierda,COLOR_BLANCO,1);
  escribirTexto(142-anchoTexto(derecha,1),y,derecha,COLOR_AMARILLO,1);
}
#endif
