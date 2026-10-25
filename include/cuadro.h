#ifndef CUADRO_H
#define CUADRO_H
#include <stdint.h>
#include <string.h>

// Memoria estática: las operaciones de borrado y dibujo se componen antes
// de enviar el resultado. La TFT nunca recibe los borrados intermedios.
template<int Ancho, int Alto, int Tile=8> struct CuadroRGB565 {
  static_assert(Ancho%Tile==0 && Alto%Tile==0, "Dimensiones por tiles completos");
  uint16_t pixeles[Alto][Ancho] = {};
  uint16_t anterior[Alto][Ancho] = {};
  bool sucios[Alto/Tile][Ancho/Tile] = {};

  void ensuciar(int x,int y,int ancho,int alto) {
    if(ancho<=0 || alto<=0) return;
    int x0=x<0?0:x, y0=y<0?0:y;
    int x1=x+ancho>Ancho?Ancho:x+ancho, y1=y+alto>Alto?Alto:y+alto;
    if(x1<=x0 || y1<=y0) return;
    for(int f=y0/Tile;f<=(y1-1)/Tile;f++)
      for(int c=x0/Tile;c<=(x1-1)/Tile;c++) sucios[f][c]=true;
  }
  bool pendiente() const {
    for(int f=0;f<Alto/Tile;f++) for(int c=0;c<Ancho/Tile;c++) if(sucios[f][c]) return true;
    return false;
  }
  template<class Enviar> void presentar(Enviar enviar) {
    // Una zona borrada y restaurada al mismo valor no necesita viajar por SPI.
    for(int f=0;f<Alto/Tile;f++) for(int c=0;c<Ancho/Tile;c++) if(sucios[f][c]) {
      bool cambio=false;
      for(int y=f*Tile;y<(f+1)*Tile;y++)
        if(memcmp(&pixeles[y][c*Tile],&anterior[y][c*Tile],Tile*sizeof(uint16_t))) { cambio=true; break; }
      sucios[f][c]=cambio;
    }
    for(int f=0;f<Alto/Tile;f++) {
      int c=0;
      while(c<Ancho/Tile) {
        if(!sucios[f][c]) { c++; continue; }
        int inicio=c++;
        while(c<Ancho/Tile && sucios[f][c]) c++;
        // Fusiona verticalmente los tramos contiguos: menos ventanas de escritura.
        int filas=1;
        while(f+filas<Alto/Tile) {
          bool completa=true;
          for(int col=inicio;col<c;col++) if(!sucios[f+filas][col]) completa=false;
          if(!completa) break;
          filas++;
        }
        int x0=c*Tile,y0=(f+filas)*Tile,x1=inicio*Tile-1,y1=f*Tile-1;
        for(int y=f*Tile;y<(f+filas)*Tile;y++) for(int x=inicio*Tile;x<c*Tile;x++)
          if(pixeles[y][x]!=anterior[y][x]) {
            if(x<x0) x0=x;
            if(x>x1) x1=x;
            if(y<y0) y0=y;
            if(y>y1) y1=y;
          }
        if(x1>=x0 && y1>=y0) {
          // El receptor conoce el paso entre filas; no se copia un cuadro extra.
          enviar(x0,y0,x1-x0+1,y1-y0+1,&pixeles[y0][x0],Ancho);
          for(int y=y0;y<=y1;y++) memcpy(&anterior[y][x0],&pixeles[y][x0],(x1-x0+1)*sizeof(uint16_t));
        }
        for(int fila=f;fila<f+filas;fila++)
          for(int col=inicio;col<c;col++) sucios[fila][col]=false;
      }
    }
  }
};
#endif
