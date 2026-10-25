#pragma once
#include <Arduino.h>

// Bus I2C simulado para ejecutar la lectura física real de controles.cpp.
struct WirePrueba {
  uint8_t registros[256]={};
  uint8_t seleccionado=0;
  bool conectado=true;
  void reiniciar() {
    memset(registros,8,sizeof(registros));
    registros[0x10]=registros[0x11]=128;
    conectado=true;
  }
  void begin(int,int) {}
  void setClock(unsigned long) {}
  void beginTransmission(uint8_t) {}
  void write(uint8_t registro) { seleccionado=registro; }
  uint8_t endTransmission() { return conectado?0:4; }
  uint8_t requestFrom(uint8_t,uint8_t cantidad) { return conectado?cantidad:0; }
  int read() { return registros[seleccionado]; }
};
inline WirePrueba Wire;
