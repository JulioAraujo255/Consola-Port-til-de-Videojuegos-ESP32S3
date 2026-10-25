#pragma once
#include <map>
#include <string>
#include <cstdint>
inline std::map<std::string, int32_t> nvsPrueba;
inline int escriturasPrueba = 0;
inline bool nvsDisponiblePrueba = true;
struct Preferences {
  bool begin(const char*, bool) { return nvsDisponiblePrueba; }
  int32_t getInt(const char* clave, int32_t defecto) {
    auto it = nvsPrueba.find(clave);
    return it == nvsPrueba.end() ? defecto : it->second;
  }
  size_t putInt(const char* clave, int32_t valor) {
    nvsPrueba[clave] = valor;
    escriturasPrueba++;
    return sizeof(int32_t);
  }
};
