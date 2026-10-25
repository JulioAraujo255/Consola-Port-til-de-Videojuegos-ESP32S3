#pragma once
#include "snake.h"
#include "dino.h"
#include "carreras.h"
#include "brasa.h"
static int iniciosPrueba = 0, redibujosPrueba = 0, puntosPrueba = 0;
static ResultadoPartida resultadoPrueba = PARTIDA_CONTINUA;
void comenzarSnake() { iniciosPrueba++; puntosPrueba = 0; }
void comenzarDino() { comenzarSnake(); }
void comenzarCarreras() { comenzarSnake(); }
void comenzarBrasa() { comenzarSnake(); }
void redibujarSnake() { redibujosPrueba++; }
void redibujarDino() { redibujarSnake(); }
void redibujarCarreras() { redibujarSnake(); }
void redibujarBrasa() { redibujarSnake(); }
ResultadoPartida actualizarSnake(const EstadoControles&) { return resultadoPrueba; }
ResultadoPartida actualizarDino(const EstadoControles&) { return resultadoPrueba; }
ResultadoPartida actualizarCarreras(const EstadoControles&) { return resultadoPrueba; }
ResultadoPartida actualizarBrasa(const EstadoControles&) { return resultadoPrueba; }
int puntuacionSnake() { return puntosPrueba; }
int puntuacionDino() { return puntosPrueba; }
int puntuacionCarreras() { return puntosPrueba; }
int puntuacionBrasa() { return puntosPrueba; }
