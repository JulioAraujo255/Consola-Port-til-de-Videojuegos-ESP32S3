/*
 * ============================================================
 *  MINI CONSOLA DE VIDEOJUEGOS - ESP32-S3
 *  Archivo: sonido.cpp
 *  Descripción: "cuerpo" del módulo de sonido (FOP-01).
 *
 *  CÓMO FUNCIONA:
 *  - El zumbador pasivo suena a la frecuencia de una señal
 *    PWM generada por el periférico LEDC del ESP32.
 *  - Cada efecto es una lista de PASOS. Un paso puede ser:
 *      TONO     : va de una frecuencia a otra en un tiempo
 *                 (si son iguales, es una nota fija; si no,
 *                 un "barrido" en escalones, estilo 8 bits).
 *      RUIDO    : frecuencias al azar entre dos valores
 *                 (para choques y explosiones).
 *      SILENCIO : una pausa entre notas.
 *  - Una TAREA de FreeRTOS revisa el sonido cada 5 ms, en
 *    paralelo con el juego. Así el sonido no se corta aunque
 *    el juego tarde en dibujar una pantalla.
 * ============================================================
 */

#include "sonido.h"
#include "pines.h"

// ------------------------------------------------------------
//  1. CONFIGURACIÓN
// ------------------------------------------------------------
const int CANAL_LEDC          = 0;    // Canal PWM del zumbador (core 2.x)
const int RESOLUCION_LEDC     = 10;   // Bits de resolución del PWM
const int PERIODO_TAREA_MS    = 5;    // Cada cuánto se revisa el sonido
const int ESCALON_BARRIDO_MS  = 15;   // Los barridos cambian de nota cada 15 ms
const int CAMBIO_RUIDO_MS     = 12;   // El ruido cambia de frecuencia cada 12 ms
const int CAMBIO_MOTOR_MS     = 30;   // El motor alterna entre dos tonos cada 30 ms

// ------------------------------------------------------------
//  2. NOTAS MUSICALES (Hz)
// ------------------------------------------------------------
const uint16_t NOTA_E4  = 330;
const uint16_t NOTA_F4  = 349;
const uint16_t NOTA_FS4 = 370;
const uint16_t NOTA_G4  = 392;
const uint16_t NOTA_C5  = 523;
const uint16_t NOTA_E5  = 659;
const uint16_t NOTA_G5  = 784;
const uint16_t NOTA_B5  = 988;
const uint16_t NOTA_C6  = 1047;
const uint16_t NOTA_E6  = 1319;

// ------------------------------------------------------------
//  3. TIPOS DE DATOS
// ------------------------------------------------------------
enum TipoPaso : uint8_t {
  TONO,
  RUIDO,
  SILENCIO,
  FIN        // Marca el final de la lista de pasos
};

struct Paso {
  TipoPaso tipo;
  uint16_t frecuenciaInicio;   // Hz
  uint16_t frecuenciaFin;      // Hz (igual a la de inicio = nota fija)
  uint16_t duracionMs;
};

// ------------------------------------------------------------
//  4. EFECTOS (cada uno es una lista de pasos que termina en FIN)
// ------------------------------------------------------------
static const Paso EF_INICIO[] = {
  { TONO, NOTA_C5, NOTA_C5, 90 }, { TONO, NOTA_G5, NOTA_G5, 90 },
  { TONO, NOTA_C6, NOTA_C6, 90 }, { TONO, NOTA_E6, NOTA_E6, 220 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_MENU_MOVER[] = {
  { TONO, 1200, 1200, 20 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_MENU_ELEGIR[] = {
  { TONO, NOTA_E5, NOTA_E5, 70 }, { TONO, NOTA_B5, NOTA_B5, 110 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_PAUSA[] = {
  { TONO, 880, 440, 90 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_COMER[] = {
  { TONO, 900, 1700, 45 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_SALTO[] = {
  { TONO, 350, 1100, 135 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_CARRIL[] = {
  { RUIDO, 900, 1800, 36 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_NIVEL[] = {
  { TONO, NOTA_C5, NOTA_C5, 60 }, { TONO, NOTA_E5, NOTA_E5, 60 },
  { TONO, NOTA_G5, NOTA_G5, 60 }, { TONO, NOTA_C6, NOTA_C6, 130 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_CHOQUE[] = {
  { RUIDO, 80, 900, 180 },
  { TONO, 420, 70, 315 },
  { FIN, 0, 0, 0 }
};
static const Paso EF_GAME_OVER[] = {
  { TONO, NOTA_G4,  NOTA_G4,  160 }, { SILENCIO, 0, 0, 40 },
  { TONO, NOTA_FS4, NOTA_FS4, 160 }, { SILENCIO, 0, 0, 40 },
  { TONO, NOTA_F4,  NOTA_F4,  160 }, { SILENCIO, 0, 0, 40 },
  { TONO, NOTA_E4,  260,      495 },
  { FIN, 0, 0, 0 }
};
// Fanfarria "ta-ta-ta-taaan" (distinta del arpegio de subir de nivel)
static const Paso EF_RECORD[] = {
  { TONO, NOTA_G5, NOTA_G5, 90 },  { SILENCIO, 0, 0, 30 },
  { TONO, NOTA_G5, NOTA_G5, 90 },  { SILENCIO, 0, 0, 30 },
  { TONO, NOTA_G5, NOTA_G5, 90 },  { SILENCIO, 0, 0, 30 },
  { TONO, NOTA_E6, NOTA_E6, 120 }, { TONO, NOTA_C6, NOTA_C6, 420 },
  { FIN, 0, 0, 0 }
};

// Tabla: efecto -> su lista de pasos (mismo orden que EfectoSonido)
static const Paso* const EFECTOS[] = {
  EF_INICIO, EF_MENU_MOVER, EF_MENU_ELEGIR, EF_PAUSA, EF_COMER,
  EF_SALTO, EF_CARRIL, EF_NIVEL, EF_CHOQUE, EF_GAME_OVER, EF_RECORD
};

// ------------------------------------------------------------
//  5. VARIABLES DEL MÓDULO
//  Las usan DOS tareas a la vez (el juego y el sonido), así que
//  se leen y escriben dentro de una "sección crítica": mientras
//  una tarea las está tocando, la otra espera.
// ------------------------------------------------------------
static portMUX_TYPE candado = portMUX_INITIALIZER_UNLOCKED;

static volatile bool sonidoActivo = true;
static const Paso* efectoActual = nullptr;    // nullptr = no suena ningún efecto
static int indicePaso = 0;
static unsigned long inicioPaso = 0;          // Momento en que empezó el paso actual
static int frecuenciaMotor = 0;               // 0 = sin motor

// Solo los usa la tarea de sonido
static uint16_t frecuenciaSonando = 0;        // Lo que suena ahora (0 = nada)
static uint16_t ultimoRuido = 0;              // Última frecuencia de ruido elegida
static unsigned long ultimoCambioRuido = 0;

// ------------------------------------------------------------
//  6. MANEJO DEL ZUMBADOR (periférico LEDC)
//  La forma de usar LEDC cambió entre las versiones 2 y 3 del
//  núcleo de Arduino para ESP32; se usa la que corresponda.
// ------------------------------------------------------------
static void prepararZumbador() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(PIN_BUZZER, 1000, RESOLUCION_LEDC);
  ledcWriteTone(PIN_BUZZER, 0);
#else
  ledcSetup(CANAL_LEDC, 1000, RESOLUCION_LEDC);
  ledcAttachPin(PIN_BUZZER, CANAL_LEDC);
  ledcWriteTone(CANAL_LEDC, 0);
#endif
}

// Hace sonar una frecuencia (0 = silencio).
// Solo le habla al zumbador si la frecuencia cambió.
static void sonar(uint16_t frecuencia) {
  if (frecuencia == frecuenciaSonando) return;
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(PIN_BUZZER, frecuencia);
#else
  ledcWriteTone(CANAL_LEDC, frecuencia);
#endif
  frecuenciaSonando = frecuencia;
}

// ------------------------------------------------------------
//  7. CÁLCULO DE LA FRECUENCIA
// ------------------------------------------------------------

// Frecuencia que corresponde al paso actual, según el tiempo
static uint16_t frecuenciaDelPaso(const Paso& paso, unsigned long transcurrido,
                                  unsigned long ahora) {
  switch (paso.tipo) {
    case TONO: {
      // Barrido en escalones: el tiempo se redondea a múltiplos
      // de 15 ms, así la nota cambia de a saltos (estilo 8 bits)
      unsigned long t = (transcurrido / ESCALON_BARRIDO_MS) * ESCALON_BARRIDO_MS;
      long diferencia = (long)paso.frecuenciaFin - (long)paso.frecuenciaInicio;
      return paso.frecuenciaInicio + diferencia * (long)t / paso.duracionMs;
    }
    case RUIDO:
      // Una frecuencia al azar nueva cada 12 ms
      if (ultimoRuido == 0 || ahora - ultimoCambioRuido >= CAMBIO_RUIDO_MS) {
        ultimoRuido = random(paso.frecuenciaInicio, paso.frecuenciaFin + 1);
        ultimoCambioRuido = ahora;
      }
      return ultimoRuido;
    default:
      return 0;   // SILENCIO
  }
}

// Ronroneo del motor: alterna entre la frecuencia base y una
// un poco más aguda ("brrr")
static uint16_t frecuenciaDelMotor(int base, unsigned long ahora) {
  bool agudo = (ahora / CAMBIO_MOTOR_MS) % 2;
  return agudo ? base + base / 6 : base;
}

// Calcula qué tiene que sonar en este momento
static uint16_t calcularFrecuencia() {
  unsigned long ahora = millis();
  uint16_t frecuencia = 0;

  taskENTER_CRITICAL(&candado);
  if (efectoActual != nullptr) {
    // Saltar los pasos que ya terminaron
    unsigned long transcurrido = ahora - inicioPaso;
    while (efectoActual[indicePaso].tipo != FIN &&
           transcurrido >= efectoActual[indicePaso].duracionMs) {
      inicioPaso   += efectoActual[indicePaso].duracionMs;
      transcurrido -= efectoActual[indicePaso].duracionMs;
      indicePaso++;
      ultimoRuido = 0;   // Paso nuevo: si es ruido, elegir otra frecuencia
    }

    if (efectoActual[indicePaso].tipo == FIN) {
      efectoActual = nullptr;   // Terminó el efecto
    } else {
      frecuencia = frecuenciaDelPaso(efectoActual[indicePaso], transcurrido, ahora);
    }
  }
  int motor = frecuenciaMotor;
  bool hayEfecto = (efectoActual != nullptr);
  taskEXIT_CRITICAL(&candado);

  // Si no hay efecto, suena el motor (si está encendido)
  if (!hayEfecto && motor > 0) {
    frecuencia = frecuenciaDelMotor(motor, ahora);
  }
  return frecuencia;
}

// ------------------------------------------------------------
//  8. TAREA DE SONIDO
//  Es como un segundo loop() que corre en paralelo: cada 5 ms
//  calcula la frecuencia y la envía al zumbador.
// ------------------------------------------------------------
static void tareaSonido(void* parametro) {
  (void)parametro;
  while (true) {
    sonar(sonidoActivo ? calcularFrecuencia() : 0);
    vTaskDelay(pdMS_TO_TICKS(PERIODO_TAREA_MS));
  }
}

// ------------------------------------------------------------
//  9. FUNCIONES PÚBLICAS
// ------------------------------------------------------------
void iniciarSonido() {
  prepararZumbador();
  // Tarea "sonido": 2 KB de memoria, prioridad 2 (más que loop(),
  // que tiene 1), así puede interrumpir al juego cuando le toca.
  xTaskCreate(tareaSonido, "sonido", 2048, nullptr, 2, nullptr);
}

void reproducirSonido(EfectoSonido efecto) {
  taskENTER_CRITICAL(&candado);
  efectoActual = EFECTOS[efecto];
  indicePaso   = 0;
  inicioPaso   = millis();
  ultimoRuido  = 0;
  taskEXIT_CRITICAL(&candado);
}

void fijarMotor(int frecuenciaBase) {
  taskENTER_CRITICAL(&candado);
  frecuenciaMotor = frecuenciaBase;
  taskEXIT_CRITICAL(&candado);
}

void esperarConSonido(unsigned long milisegundos) {
  // La tarea de sonido sigue funcionando durante la espera
  delay(milisegundos);
}

void alternarSilencio() {
  sonidoActivo = !sonidoActivo;
  Serial.printf("Sonido: %s\r\n", sonidoActivo ? "ACTIVADO" : "SILENCIADO");
  if (sonidoActivo) {
    reproducirSonido(SONIDO_MENU_ELEGIR);   // Aviso de que volvió el sonido
  }
}