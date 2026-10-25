/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: controles.cpp
 *  Descripción: "cuerpo" del módulo de controles.
 *
 *  - MODO_SIMULACION: joystick analógico + 4 pulsadores (Wokwi).
 *  - Placa física: gamepad I2C Emakefun "Joystick Handle"
 *    (dirección 0x5A). Se usa el mapa de registros de la
 *    biblioteca del fabricante (JoystickHandle.h), con lectura
 *    propia por I2C en los pines definidos en pines.h.
 * ============================================================
 */

#include "controles.h"
#include "pines.h"

// ------------------------------------------------------------
//  1. CONSTANTES COMUNES
// ------------------------------------------------------------

// Después de aceptar un cambio de un botón, se ignoran los
// rebotes durante este tiempo (antirrebote, criterio FO-04).
const unsigned long TIEMPO_ANTIRREBOTE_MS = 30;

// ------------------------------------------------------------
//  2. ANTIRREBOTE (común a ambos modos)
//  Antirrebote "de flanco inicial": el cambio se acepta APENAS
//  ocurre y luego se bloquean los rebotes por 30 ms. Así no se
//  pierden pulsaciones cortas aunque el juego tarde en cada
//  vuelta de loop() (Dino Run y Carreras dibujan mucho).
// ------------------------------------------------------------

struct Antirrebote {
  bool estadoEstable;           // Último estado aceptado
  unsigned long momentoCambio;  // Momento en que se aceptó el último cambio
};

// Un registro por botón: 0 = A, 1 = B, 2 = C, 3 = D
static Antirrebote antirrebote[4];

// Recibe la lectura cruda de un botón y devuelve su estado filtrado.
static EstadoBoton procesarBoton(int indice, bool lecturaCruda) {
  Antirrebote& ar = antirrebote[indice];
  EstadoBoton resultado;
  resultado.recienPresionado = false;

  unsigned long ahora = millis();

  // Se acepta el cambio solo si pasó el tiempo de bloqueo
  // desde el cambio anterior (si no, es un rebote)
  if (lecturaCruda != ar.estadoEstable &&
      (ahora - ar.momentoCambio) >= TIEMPO_ANTIRREBOTE_MS) {
    ar.estadoEstable = lecturaCruda;
    ar.momentoCambio = ahora;
    resultado.recienPresionado = lecturaCruda;   // true solo al apretar
  }

  resultado.presionado = ar.estadoEstable;
  return resultado;
}

// ============================================================
//  3. VERSIÓN SIMULACIÓN (Wokwi)
// ============================================================
#ifdef MODO_SIMULACION

// El conversor analógico mide de 0 a 4095 (reposo: ~2048).
const int UMBRAL_BAJO = 1000;
const int UMBRAL_ALTO = 3000;

void iniciarControles() {
  pinMode(PIN_SIM_BTN_A, INPUT_PULLUP);
  pinMode(PIN_SIM_BTN_B, INPUT_PULLUP);
  pinMode(PIN_SIM_BTN_C, INPUT_PULLUP);
  pinMode(PIN_SIM_BTN_D, INPUT_PULLUP);
  analogReadResolution(12);
}

EstadoControles leerControles() {
  EstadoControles estado;

  // En Wokwi: arriba e izquierda = valores altos
  int vertical   = analogRead(PIN_SIM_JOY_VERT);
  int horizontal = analogRead(PIN_SIM_JOY_HORZ);

  estado.arriba    = vertical   > UMBRAL_ALTO;
  estado.abajo     = vertical   < UMBRAL_BAJO;
  estado.izquierda = horizontal > UMBRAL_ALTO;
  estado.derecha   = horizontal < UMBRAL_BAJO;

  // Botones con INPUT_PULLUP: LOW = apretado
  estado.a = procesarBoton(0, digitalRead(PIN_SIM_BTN_A) == LOW);
  estado.b = procesarBoton(1, digitalRead(PIN_SIM_BTN_B) == LOW);
  estado.c = procesarBoton(2, digitalRead(PIN_SIM_BTN_C) == LOW);
  estado.d = procesarBoton(3, digitalRead(PIN_SIM_BTN_D) == LOW);

  return estado;
}

// ============================================================
//  4. VERSIÓN PLACA FÍSICA (gamepad I2C Emakefun, 0x5A)
// ============================================================
#else

#include <Wire.h>

// --- 4.1 Ajustes para calibrar en la clase 3 ---------------

// Si una dirección del joystick sale al revés, cambiar a true.
#define INVERTIR_EJE_X  false
#define INVERTIR_EJE_Y  true

// true: muestra en el monitor serie los valores crudos del
// gamepad cada 300 ms (para verificar ejes y botones).
// Cambiar a false cuando todo funcione bien.
#define DEPURAR_GAMEPAD false

// --- 4.2 Registros del gamepad (JoystickHandle.h) ----------
const uint8_t REG_EJE_X   = 0x10;
const uint8_t REG_EJE_Y   = 0x11;
const uint8_t REG_BOTON_Z = 0x20;   // Presionar el joystick
const uint8_t REG_BOTON_C = 0x21;   // "DOWN"  (C: botón de abajo)
const uint8_t REG_BOTON_A = 0x22;   // "UP"    (botón de arriba)
const uint8_t REG_BOTON_B = 0x23;   // "RIGHT" (botón derecho)
const uint8_t REG_BOTON_D = 0x24;   // "LEFT"  (D: botón izquierdo)

// --- 4.3 Códigos de evento de los botones ------------------
const uint8_t EVENTO_PRESIONADO       = 0;
const uint8_t EVENTO_REPETICION       = 2;
const uint8_t EVENTO_MANTENIDO_INICIO = 5;
const uint8_t EVENTO_MANTENIDO        = 6;

// Los ejes miden de 0 a 255 (reposo: ~128).
const int EJE_BAJO = 64;
const int EJE_ALTO = 192;

// --- 4.4 Estado interno -------------------------------------
static bool gamepadConectado = false;
static unsigned long ultimoIntentoConexion = 0;
static unsigned long ultimoInformeDepuracion = 0;

// Lee un registro del gamepad. Devuelve false si no responde.
static bool leerRegistro(uint8_t registro, uint8_t& valor) {
  Wire.beginTransmission(GAMEPAD_DIRECCION_I2C);
  Wire.write(registro);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  if (Wire.requestFrom((uint8_t)GAMEPAD_DIRECCION_I2C, (uint8_t)1) != 1) {
    return false;
  }
  valor = Wire.read();
  return true;
}

// Comprueba si el gamepad responde en el bus I2C.
static bool detectarGamepad() {
  Wire.beginTransmission(GAMEPAD_DIRECCION_I2C);
  return Wire.endTransmission() == 0;
}

// Un botón cuenta como apretado si su evento indica que
// está presionado o mantenido.
static bool codigoEsPresionado(uint8_t codigo) {
  return codigo == EVENTO_PRESIONADO ||
         codigo == EVENTO_REPETICION ||
         codigo == EVENTO_MANTENIDO_INICIO ||
         codigo == EVENTO_MANTENIDO;
}

void iniciarControles() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(100000);   // 100 kHz: velocidad estándar de I2C

  gamepadConectado = detectarGamepad();
  if (gamepadConectado) {
    Serial.println("Gamepad detectado en 0x5A.");
  } else {
    Serial.println("ATENCION: no se detecta el gamepad en 0x5A. Revisar cables (SDA=8, SCL=9, VCC=3V3, GND).");
  }
}

EstadoControles leerControles() {
  EstadoControles estado = {};   // Todo en false por defecto

  // Si el gamepad no respondía, se reintenta cada 2 segundos
  // (por si se conectó tarde o había un cable flojo).
  if (!gamepadConectado) {
    if (millis() - ultimoIntentoConexion >= 2000) {
      ultimoIntentoConexion = millis();
      gamepadConectado = detectarGamepad();
      if (gamepadConectado) {
        Serial.println("Gamepad conectado.");
      }
    }
    return estado;
  }

  // --- Joystick ---
  uint8_t ejeX = 128, ejeY = 128;
  bool lecturaOk = leerRegistro(REG_EJE_X, ejeX) && leerRegistro(REG_EJE_Y, ejeY);

  if (!lecturaOk) {
    gamepadConectado = false;
    Serial.println("ATENCION: se perdio la comunicacion con el gamepad.");
    return estado;
  }

  if (INVERTIR_EJE_X) ejeX = 255 - ejeX;
  if (INVERTIR_EJE_Y) ejeY = 255 - ejeY;

  estado.izquierda = ejeX < EJE_BAJO;
  estado.derecha   = ejeX > EJE_ALTO;
  estado.arriba    = ejeY < EJE_BAJO;
  estado.abajo     = ejeY > EJE_ALTO;

  // --- Botones ---
  uint8_t codA = 8, codB = 8, codC = 8, codD = 8, codZ = 8;
  leerRegistro(REG_BOTON_A, codA);
  leerRegistro(REG_BOTON_B, codB);
  leerRegistro(REG_BOTON_C, codC);
  leerRegistro(REG_BOTON_D, codD);

  estado.a = procesarBoton(0, codigoEsPresionado(codA));
  estado.b = procesarBoton(1, codigoEsPresionado(codB));
  estado.c = procesarBoton(2, codigoEsPresionado(codC));
  estado.d = procesarBoton(3, codigoEsPresionado(codD));

  // --- Diagnóstico (solo si DEPURAR_GAMEPAD está en true) ---
  if (DEPURAR_GAMEPAD && millis() - ultimoInformeDepuracion >= 300) {
    ultimoInformeDepuracion = millis();
    leerRegistro(REG_BOTON_Z, codZ);
    Serial.printf("Gamepad X=%3u Y=%3u | A=%u B=%u C=%u D=%u Z=%u\r\n",
                  ejeX, ejeY, codA, codB, codC, codD, codZ);
  }

  return estado;
}

#endif
