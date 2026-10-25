/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: pines.h
 *  Descripción: asignación de pines (GPIO) de todos los
 *               componentes de la consola.
 *
 *  Fuente: diagrama de conexiones del equipo (Integrante 2).
 *  Si cambia alguna conexión física, se corrige SOLO acá.
 *
 *  Placa: ESP32-S3 DevKitC-1 N16R8
 *  IMPORTANTE: no usar GPIO 35, 36 ni 37 (los usa la PSRAM).
 * ============================================================
 */

#ifndef PINES_H   // Evita que este archivo se incluya dos veces
#define PINES_H

// ------------------------------------------------------------
//  PANTALLA TFT 1,8" ST7735 (interfaz SPI)
//  En Wokwi se usa la ILI9341 con estos MISMOS pines.
//  VCC y LED/BLK van a 3V3; GND a GND.
// ------------------------------------------------------------
#define PIN_TFT_SCK    12   // Reloj del bus SPI
#define PIN_TFT_MOSI   11   // Datos SPI (en el módulo dice "SDA")
#define PIN_TFT_CS     10   // Selección del chip de la pantalla
#define PIN_TFT_DC      6   // Dato o comando (en el módulo dice "A0")
#define PIN_TFT_RST     7   // Reinicio de la pantalla

// ------------------------------------------------------------
//  GAMEPAD I2C (joystick + botones A, B, C, D)
//  Se conecta con el cable PH2.0. VCC a 3V3 (¡no a 5 V!).
// ------------------------------------------------------------
#define PIN_I2C_SDA     8   // Línea de datos I2C
#define PIN_I2C_SCL     9   // Línea de reloj I2C
#define GAMEPAD_DIRECCION_I2C  0x5A   // Dirección del módulo

// ------------------------------------------------------------
//  BUZZER PASIVO (opcional)
//  GPIO 4 -> resistencia 1 kΩ -> base del transistor NPN.
// ------------------------------------------------------------
#define PIN_BUZZER      4

// ------------------------------------------------------------
//  SOLO PARA SIMULACIÓN EN WOKWI
//  Wokwi no tiene el gamepad I2C, así que se reemplaza por un
//  joystick analógico y cuatro pulsadores comunes.
//  Estos pines NO se usan en la placa física.
// ------------------------------------------------------------
#ifdef MODO_SIMULACION
  #define PIN_SIM_JOY_VERT  1   // Eje vertical del joystick (analógico)
  #define PIN_SIM_JOY_HORZ  2   // Eje horizontal del joystick (analógico)
  #define PIN_SIM_BTN_A    15   // Botón A: jugar / confirmar
  #define PIN_SIM_BTN_B    16   // Botón B: volver al inicio
  #define PIN_SIM_BTN_C    17   // Botón C: pausa
  #define PIN_SIM_BTN_D    18   // Botón D: silenciar / activar sonido
#endif

#endif  // PINES_H
