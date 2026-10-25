#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
using std::min;
using std::max;
using std::abs;
template<class T> T constrain(T value, T low, T high) { return min(high, max(low, value)); }
inline unsigned long relojPrueba = 0;
inline unsigned long millis() { return relojPrueba; }
inline void delay(unsigned long ms) { relojPrueba += ms; }
inline std::mt19937 aleatorioPrueba(123456);
inline long random(long maximo) { return (long)(aleatorioPrueba() % maximo); }
inline long random(long minimo, long maximo) { return minimo + random(maximo - minimo); }
struct SerialPrueba {
  template<class... Args> void printf(const char*, Args...) {}
  void println(const char*) {}
};
inline SerialPrueba Serial;
