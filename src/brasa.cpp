/*
 * JUMP-MAN: plataformas original para 160 x 128.
 * A/arriba salta, B dispara con flor, C pausa y D silencia.
 * Mapas de 96 x 14 tiles, dos mundos de cuatro etapas, jefes en 1-4 y 2-4.
 * La física usa pasos de 33 ms; el dibujo compone en RAM y envía solo bandas
 * modificadas. No usa PSRAM, asignaciones dinámicas ni esperas bloqueantes.
 */
#include "brasa.h"
#include "pantalla.h"
#include "sonido.h"
#include "interfaz.h"
#include <math.h>

namespace {
constexpr int TILE = 8, COLUMNAS = 96, FILAS = 14;
constexpr int HUD = ALTO_MARCADOR, ALTO_ESCENA = ALTO_PANTALLA-HUD, ANCHO_MUNDO = COLUMNAS * TILE;
constexpr int ANCHO_AVATAR = 7, TOTAL_NIVELES = 8;
constexpr unsigned long PASO_MS = 33;
constexpr int MAX_ENEMIGOS = 10, MAX_ITEMS = 6, MAX_FUEGOS = 3, MAX_ATAQUES = 4;
constexpr int MAX_PUNTAJES_FLOTANTES=6, DURACION_PUNTAJE=30;
enum Tile : uint8_t { VACIO, TIERRA, PIEDRA, LADRILLO, BLOQUE_HONGO, BLOQUE_FLOR, BLOQUE_MONEDA, USADO };
enum Poder { PEQUENO, GRANDE, FUEGO };
enum Fase { ENTRADA, JUGANDO, REAPARECER, SALIDA, TERMINADO };
struct Avatar {
  float x, y, vy;
  int mirando, invulnerable;
  Poder poder;
  bool suelo;
};
struct Enemigo { bool activo; float x, y; int direccion; };
struct Item { bool activo; float x, y, vy; int direccion, emergiendo; bool flor; };
struct Proyectil { bool activo; float x, y, vx, vy; int vida; };
struct Jefe { bool activo; float x, y, vy; int hp, hpMax, direccion, invulnerable, reloj; };
struct PuntajeFlotante { int x,y,valor,vida; };
struct Nivel {
  const char* nombre;
  uint16_t cielo, suelo, acento;
  uint8_t huecos[4]; // Columnas iniciales; 0 significa sin hueco.
  uint8_t anchoHueco;
  uint8_t escalones[3];
  uint8_t enemigos[6];
};
// Geometría determinista: arranque seguro, huecos de 2/3 tiles y llegada firme.
static const Nivel NIVELES[TOTAL_NIVELES] = {
  {"PRADERA", 0x65BF,0x7485,0x3668, {29,53,0,0},2, {39,65,0}, {22,43,62,78,0,0}},
  {"GRUTAS", 0x18CB,0x6B6D,0x7C1F, {25,49,73,0},2, {35,61,0}, {20,39,58,81,0,0}},
  {"RUINAS", 0x9E7C,0xA48A,0xDD24, {31,55,76,0},3, {41,66,0}, {23,44,61,82,0,0}},
  {"FORTIN", 0x2089,0x6B4D,0xF3A5, {30,50,0,0},2, {39,0,0}, {23,43,57,0,0,0}},
  {"BOSQUE", 0x4CF5,0x4B65,0xAFE5, {27,51,75,0},3, {38,63,0}, {20,40,60,82,0,0}},
  {"CRISTAL", 0x212C,0x6B7A,0x867F, {25,48,72,0},3, {35,59,0}, {21,39,57,80,0,0}},
  {"CUMBRES", 0xA6DF,0x7C54,0xFDE9, {28,53,77,0},3, {39,65,0}, {22,44,62,84,0,0}},
  {"CALDERA", 0x2807,0x72CA,0xFB40, {30,50,0,0},3, {39,0,0}, {23,43,57,0,0,0}}
};

static uint8_t mapa[FILAS][COLUMNAS];
static Avatar avatar;
static Enemigo enemigos[MAX_ENEMIGOS];
static Item items[MAX_ITEMS];
static Proyectil fuegos[MAX_FUEGOS], ataques[MAX_ATAQUES];
static Jefe jefe;
static PuntajeFlotante puntajesFlotantes[MAX_PUNTAJES_FLOTANTES];
static int nivel, puntos, vidas, camara, puntoControl;
static int relojFase, relojAnimacion, recarga, saltoBuffer, coyote;
static int bloqueGolpeX, bloqueGolpeY, golpeTicks;
static int pasoAvatar, gestoDisparo;
static bool caminando, agachado;
static bool panelVisible;
static Fase faseMostrada;
static bool arribaAnterior, arenaCerrada, disparoPedido;
static Fase fase;
static ResultadoPartida resultado;
static unsigned long ultimoPaso;
static unsigned int revisionMapa;
static int firmaMostrada[160];
static bool firmaValida, marcadorValido;
static unsigned long escenasCompuestas;
// 69.120 bytes de framebuffer y 2.560 de envío. Comparación exacta, sin hashes.
static uint16_t imagen[ALTO_ESCENA][ANCHO_PANTALLA];
static uint16_t anterior[ALTO_ESCENA][ANCHO_PANTALLA];
static uint16_t banda[TILE * ANCHO_PANTALLA];
static bool imagenValida;

constexpr int ALTO_PEQUENO=10, ALTO_GRANDE=14;
static int altoPoder(Poder poder) { return poder==PEQUENO?ALTO_PEQUENO:ALTO_GRANDE; }
static int altoAvatar() { return agachado?ALTO_PEQUENO:altoPoder(avatar.poder); }
static bool nivelJefe() { return nivel % 4 == 3; }
static bool superpuestos(float x,float y,int a,int h,float xx,float yy,int aa,int hh) {
  return x < xx+aa && xx < x+a && y < yy+hh && yy < y+h;
}
static Tile celda(int col, int fila) {
  if (col < 0 || col >= COLUMNAS || fila < 0) return PIEDRA;
  if (fila >= FILAS) return VACIO; // Caídas por huecos.
  return (Tile)mapa[fila][col];
}
static bool solido(float x,float y,int a,int h) {
  int c0=(int)floorf(x/TILE), c1=(int)floorf((x+a-0.01f)/TILE);
  int f0=(int)floorf(y/TILE), f1=(int)floorf((y+h-0.01f)/TILE);
  for (int f=f0; f<=f1; f++) for (int c=c0; c<=c1; c++) if (celda(c,f)!=VACIO) return true;
  return false;
}
static void acreditarPuntos(int valor,float x,float y) {
  puntos+=valor;
  // Si están ocupados, se reemplaza el aviso más antiguo; los puntos no se pierden.
  int elegido=0;
  for(int i=0;i<MAX_PUNTAJES_FLOTANTES;i++) {
    if(puntajesFlotantes[i].vida==0) { elegido=i; break; }
    if(puntajesFlotantes[i].vida<puntajesFlotantes[elegido].vida) elegido=i;
  }
  puntajesFlotantes[elegido]={(int)x,max(1,(int)y-7),valor,DURACION_PUNTAJE};
}
static void avanzarPuntajesFlotantes() {
  for(PuntajeFlotante& aviso:puntajesFlotantes) if(aviso.vida>0) {
    aviso.vida--;
    if(aviso.vida%2==0 && aviso.y>1) aviso.y--;
  }
}
static void actualizarPostura(const EstadoControles& c) {
  float pies=avatar.y+altoAvatar();
  bool bajar=avatar.poder!=PEQUENO && c.abajo && avatar.suelo;
  if(agachado && !bajar && solido(avatar.x,pies-altoPoder(avatar.poder),ANCHO_AVATAR,altoPoder(avatar.poder)))
    bajar=true;
  agachado=bajar;
  avatar.y=pies-altoAvatar();
}
static void limpiarDinamicos() {
  memset(items,0,sizeof(items)); memset(fuegos,0,sizeof(fuegos)); memset(ataques,0,sizeof(ataques));
  disparoPedido=false; saltoBuffer=0; recarga=0; coyote=0;
  gestoDisparo=0; pasoAvatar=0; caminando=false;
  memset(puntajesFlotantes,0,sizeof(puntajesFlotantes));
}
static void cargarNivel() {
  memset(mapa,0,sizeof(mapa));
  revisionMapa++;
  const Nivel& n=NIVELES[nivel];
  for (int c=0;c<COLUMNAS;c++) for(int f=12;f<FILAS;f++) mapa[f][c]=TIERRA;
  for (uint8_t inicio:n.huecos) if(inicio)
    for(int c=inicio;c<inicio+n.anchoHueco;c++) mapa[12][c]=mapa[13][c]=VACIO;
  // Tres zonas de recursos antes de la meta/arena. Siempre hay hongo y flor.
  for (int base : {10, 34, 68}) {
    mapa[8][base]=BLOQUE_HONGO; mapa[8][base+1]=LADRILLO;
    mapa[8][base+2]=BLOQUE_FLOR; mapa[8][base+3]=BLOQUE_MONEDA;
  }
  for (uint8_t c:n.escalones) if(c) {
    mapa[11][c]=PIEDRA; mapa[11][c+1]=PIEDRA; mapa[10][c+1]=PIEDRA;
    mapa[9][c+5]=LADRILLO; mapa[9][c+6]=BLOQUE_MONEDA;
  }
  memset(enemigos,0,sizeof(enemigos)); int i=0;
  for(uint8_t c:n.enemigos) if(c && i<MAX_ENEMIGOS)
    enemigos[i++]={true,(float)c*TILE,88,-1};
  limpiarDinamicos();
  agachado=false;
  avatar={16,86,0,1,0,avatar.poder,true};
  avatar.y=(float)(96-altoAvatar());
  puntoControl=16; camara=0; arenaCerrada=false; golpeTicks=0;
  int hp=5+(int)dificultadActual()+(nivel/4)*2;
  jefe={nivelJefe(),ANCHO_MUNDO-65.0f,74,0,hp,hp,-1,0,0};
  fase=ENTRADA; relojFase=30; imagenValida=false; panelVisible=false;
  firmaValida=false; marcadorValido=false;
}
static void entrarSalida() {
  if (fase!=JUGANDO) return;
  puntos+=200; fase=SALIDA; relojFase=45; reproducirSonido(SONIDO_NIVEL); imagenValida=false;
}
static void morir() {
  if(fase!=JUGANDO) return;
  vidas--; avatar.poder=PEQUENO; agachado=false; avatar.vy=0;
  fase=REAPARECER; relojFase=36; limpiarDinamicos();
  reproducirSonido(SONIDO_CHOQUE); imagenValida=false;
}
static void recibirDano() {
  if(avatar.invulnerable || fase!=JUGANDO) return;
  if(avatar.poder==PEQUENO) { morir(); return; }
  float pies=avatar.y+altoAvatar();
  avatar.poder=avatar.poder==FUEGO?GRANDE:PEQUENO;
  if(avatar.poder==PEQUENO) agachado=false;
  avatar.y=pies-altoAvatar(); avatar.invulnerable=60;
  reproducirSonido(SONIDO_CHOQUE);
}
static void golpearBloque(int c,int f) {
  Tile t=celda(c,f);
  if(c<0 || c>=COLUMNAS || f<0 || f>=FILAS) return;
  if(t==LADRILLO && avatar.poder!=PEQUENO) {
    mapa[f][c]=VACIO; revisionMapa++; puntos+=5; reproducirSonido(SONIDO_PISAR);
  } else if(t==BLOQUE_MONEDA) {
    mapa[f][c]=USADO; revisionMapa++; acreditarPuntos(25,c*TILE+4.0f,(float)(f*TILE));
    reproducirSonido(SONIDO_COMER);
  } else if(t==BLOQUE_HONGO || t==BLOQUE_FLOR) {
    for(Item& item:items) if(!item.activo) {
      item={true,(float)c*TILE,(float)f*TILE,0,1,8,t==BLOQUE_FLOR};
      mapa[f][c]=USADO; revisionMapa++; reproducirSonido(SONIDO_POTENCIA); break;
    }
  }
  bloqueGolpeX=c; bloqueGolpeY=f; golpeTicks=6;
}
// Desplazamiento por subpasos de <=1 px: no atraviesa tiles a gran velocidad.
static void moverAvatarX(float desplazamiento) {
  int pasos=(int)ceilf(fabsf(desplazamiento));
  if(!pasos) return;
  float paso=desplazamiento/pasos;
  for(int i=0;i<pasos;i++) {
    if(solido(avatar.x+paso,avatar.y,ANCHO_AVATAR,altoAvatar())) break;
    avatar.x+=paso;
  }
}
static void moverAvatarY(float desplazamiento) {
  avatar.suelo=false;
  int pasos=(int)ceilf(fabsf(desplazamiento));
  if(!pasos) { avatar.suelo=solido(avatar.x,avatar.y+1,ANCHO_AVATAR,altoAvatar()); return; }
  float paso=desplazamiento/pasos;
  for(int i=0;i<pasos;i++) {
    if(solido(avatar.x,avatar.y+paso,ANCHO_AVATAR,altoAvatar())) {
      if(paso<0) {
        int f=(int)floorf((avatar.y+paso)/TILE);
        int c0=(int)floorf(avatar.x/TILE),c1=(int)floorf((avatar.x+ANCHO_AVATAR-0.01f)/TILE);
        // Un bloque por golpe: prioriza el centro de la cabeza.
        int centro=(int)(avatar.x+ANCHO_AVATAR/2)/TILE;
        if(celda(centro,f)!=VACIO) golpearBloque(centro,f);
        else golpearBloque(celda(c0,f)!=VACIO?c0:c1,f);
      } else {
        // Apoyar los pies exactamente en la cara del tile. Una fracción por
        // encima del suelo hacía chocar la cabeza en pasillos de altura justa.
        int fila=(int)floorf((avatar.y+paso+altoAvatar()-0.01f)/TILE);
        avatar.y=(float)(fila*TILE-altoAvatar());
        avatar.suelo=true;
      }
      avatar.vy=0; return;
    }
    avatar.y+=paso;
  }
}
static void disparar() {
  if(avatar.poder!=FUEGO || recarga>0) return;
  for(Proyectil& p:fuegos) if(!p.activo) {
    p={true,avatar.x+(avatar.mirando>0?7:-4),avatar.y+altoAvatar()/2.0f,
       avatar.mirando*3.0f,-1.0f,100};
    recarga=10; gestoDisparo=6; reproducirSonido(SONIDO_DISPARO); return;
  }
}
static void moverAvatar(const EstadoControles& c) {
  actualizarPostura(c);
  if(gestoDisparo>0) gestoDisparo--;
  if(avatar.invulnerable>0) avatar.invulnerable--;
  if(recarga>0) recarga--;
  if(avatar.suelo) coyote=3; else if(coyote>0) coyote--;
  if(saltoBuffer>0 && coyote>0) {
    avatar.vy=-4.4f; avatar.suelo=false; saltoBuffer=0; coyote=0;
    reproducirSonido(SONIDO_SALTO);
  }
  if(saltoBuffer>0) saltoBuffer--;
  float dx=0;
  if(c.izquierda!=c.derecha) { dx=c.derecha?1.8f:-1.8f; avatar.mirando=c.derecha?1:-1; }
  if(agachado) dx*=0.55f;
  float xAnterior=avatar.x;
  moverAvatarX(dx);
  float gravedad=0.25f;
  if(avatar.vy<0 && !c.a.presionado && !c.arriba) gravedad=0.48f;
  avatar.vy=min(5.0f,avatar.vy+gravedad);
  moverAvatarY(avatar.vy);
  caminando=avatar.suelo && fabsf(avatar.x-xAnterior)>0.05f;
  pasoAvatar=caminando?(pasoAvatar+1)%16:0;
  if(disparoPedido) { disparar(); disparoPedido=false; }
  if(avatar.y>ALTO_ESCENA+8) { morir(); return; }
  // Checkpoint en terreno firme, a mitad del recorrido; no se mueve hacia atrás.
  if(avatar.x>=384 && avatar.x<480 && avatar.suelo && puntoControl==16) {
    puntoControl=(int)avatar.x; puntos+=50; reproducirSonido(SONIDO_COMER);
  }
  if(nivelJefe() && !arenaCerrada && avatar.x>=ANCHO_MUNDO-154) {
    arenaCerrada=true;
    for(int f=2;f<12;f++) mapa[f][75]=PIEDRA;
    revisionMapa++;
  }
  if(avatar.x>=ANCHO_MUNDO-23 && (!nivelJefe() || !jefe.activo)) entrarSalida();
}
static void moverItems() {
  for(Item& item:items) if(item.activo) {
    if(item.emergiendo>0) { item.y-=1; item.emergiendo--; continue; }
    if(!item.flor) {
      float dx=item.direccion*0.65f;
      if(solido(item.x+dx,item.y,8,8)) item.direccion=-item.direccion; else item.x+=dx;
      item.vy=min(3.0f,item.vy+0.22f);
      if(solido(item.x,item.y+item.vy,8,8)) item.vy=0; else item.y+=item.vy;
    }
    if(item.y>ALTO_ESCENA+8) { item.activo=false; continue; }
    if(superpuestos(avatar.x,avatar.y,ANCHO_AVATAR,altoAvatar(),item.x,item.y,8,8)) {
      Poder nuevo=item.flor?FUEGO:(avatar.poder==PEQUENO?GRANDE:avatar.poder);
      int nuevoAlto=agachado?ALTO_PEQUENO:altoPoder(nuevo);
      float nuevaY=avatar.y+altoAvatar()-nuevoAlto;
      // Nunca crecer dentro de una plataforma o techo.
      if(!solido(avatar.x,nuevaY,ANCHO_AVATAR,nuevoAlto)) {
        avatar.y=nuevaY; avatar.poder=nuevo; item.activo=false;
        acreditarPuntos(50,item.x+4,item.y);
        reproducirSonido(SONIDO_POTENCIA);
      }
    }
  }
}
static void moverEnemigos(float piesAnteriores) {
  float rapidez=dificultadActual()==FACIL?0.40f:dificultadActual()==DIFICIL?0.70f:0.55f;
  rapidez+=(nivel/4)*0.1f;
  for(Enemigo& e:enemigos) if(e.activo) {
    if(fabsf(e.x-avatar.x)>ANCHO_PANTALLA+24) continue;
    float dx=rapidez*e.direccion;
    float frente=e.x+(e.direccion>0?8:-1);
    if(solido(e.x+dx,e.y,8,8) || !solido(frente,e.y+8,1,2)) e.direccion=-e.direccion;
    else e.x+=dx;
    if(superpuestos(avatar.x,avatar.y,ANCHO_AVATAR,altoAvatar(),e.x,e.y,8,8)) {
      if(avatar.vy>=0 && piesAnteriores<=e.y+3) {
        e.activo=false; avatar.vy=-3.0f; acreditarPuntos(50,e.x+4,e.y);
        reproducirSonido(SONIDO_PISAR);
      } else recibirDano();
    }
  }
}
static void herirJefe() {
  if(!jefe.activo || !arenaCerrada || jefe.invulnerable) return;
  jefe.hp--; jefe.invulnerable=15; reproducirSonido(SONIDO_PISAR);
  if(jefe.hp<=0) {
    jefe.activo=false; acreditarPuntos(500,jefe.x+9,jefe.y); reproducirSonido(SONIDO_JEFE);
    for(Proyectil& p:ataques) p.activo=false;
  }
}
static void moverJefe(float piesAnteriores) {
  if(!jefe.activo || !arenaCerrada) return;
  if(jefe.invulnerable>0) jefe.invulnerable--;
  jefe.reloj++;
  float dx=jefe.direccion*(0.55f+0.15f*(nivel/4));
  if(jefe.x+dx<ANCHO_MUNDO-136 || jefe.x+dx>ANCHO_MUNDO-26) jefe.direccion=-jefe.direccion;
  else jefe.x+=dx;
  int saltoCada=nivel>=4?75:90;
  if(jefe.reloj%saltoCada==0 && jefe.y>=73) jefe.vy=-3.2f;
  jefe.vy=min(4.0f,jefe.vy+0.20f); jefe.y+=jefe.vy;
  if(jefe.y>=74) { jefe.y=74; jefe.vy=0; }
  int periodo=dificultadActual()==FACIL?85:dificultadActual()==DIFICIL?50:65;
  if(jefe.hp*2<=jefe.hpMax) periodo=max(35,periodo*3/4);
  if(jefe.reloj%periodo==0) for(Proyectil& p:ataques) if(!p.activo) {
    float dir=avatar.x<jefe.x?-1.0f:1.0f;
    float origenX=jefe.x+(dir<0?-5:19);
    float origenY=jefe.y+10;
    float distancia=max(16.0f,fabsf(avatar.x-origenX));
    float vy=constrain((avatar.y+altoAvatar()/2.0f-origenY)*2.0f/distancia,-1.0f,1.0f);
    p={true,origenX,origenY,dir*2.0f,vy,150}; break;
  }
  if(superpuestos(avatar.x,avatar.y,ANCHO_AVATAR,altoAvatar(),jefe.x,jefe.y,18,22)) {
    if(avatar.vy>=0 && piesAnteriores<=jefe.y+4) {
      herirJefe(); avatar.vy=-3.8f;
    } else recibirDano();
  }
}
static void moverProyectiles() {
  for(Proyectil& p:fuegos) if(p.activo) {
    if(--p.vida<=0 || solido(p.x+p.vx,p.y,4,4)) { p.activo=false; continue; }
    p.x+=p.vx; p.vy=min(3.0f,p.vy+0.18f);
    if(solido(p.x,p.y+p.vy,4,4)) p.vy=p.vy>0?-2.1f:0.5f;
    else p.y+=p.vy;
    if(p.y>ALTO_ESCENA || p.x<0 || p.x>=ANCHO_MUNDO) p.activo=false;
    for(Enemigo& e:enemigos) if(p.activo && e.activo && superpuestos(p.x,p.y,4,4,e.x,e.y,8,8)) {
      e.activo=false; p.activo=false; acreditarPuntos(50,e.x+4,e.y);
      reproducirSonido(SONIDO_PISAR);
    }
    if(p.activo && jefe.activo && superpuestos(p.x,p.y,4,4,jefe.x,jefe.y,18,22)) {
      p.activo=false; herirJefe();
    }
  }
  for(Proyectil& p:ataques) if(p.activo) {
    p.x+=p.vx; p.y+=p.vy;
    if(--p.vida<=0 || p.y<0 || p.y>ALTO_ESCENA || solido(p.x,p.y,4,4)) { p.activo=false; continue; }
    if(superpuestos(p.x,p.y,4,4,avatar.x,avatar.y,ANCHO_AVATAR,altoAvatar())) {
      p.activo=false; recibirDano();
    }
  }
}
static void pasoJuego(const EstadoControles& c) {
  relojAnimacion=(relojAnimacion+1)%360;
  if(fase!=JUGANDO) {
    if(fase==TERMINADO || --relojFase>0) return;
    imagenValida=false;
    if(fase==ENTRADA) fase=JUGANDO;
    else if(fase==SALIDA) {
      if(++nivel>=TOTAL_NIVELES) { fase=TERMINADO; resultado=PARTIDA_GANADA; return; }
      cargarNivel();
    } else if(fase==REAPARECER) {
      if(vidas<=0) { fase=TERMINADO; resultado=PARTIDA_PERDIDA; return; }
      if(arenaCerrada) { cargarNivel(); } // Reinicia la arena y al jefe.
      else {
        avatar={ (float)puntoControl,86,0,1,60,PEQUENO,true };
        camara=constrain((int)avatar.x-56,0,ANCHO_MUNDO-ANCHO_PANTALLA);
        // Aparta enemigos del punto de aparición para que no haya una muerte inmediata.
        for(Enemigo& e:enemigos) if(fabsf(e.x-avatar.x)<32) e.activo=false;
        fase=ENTRADA; relojFase=30;
      }
    }
    saltoBuffer=0; disparoPedido=false; coyote=0;
    return;
  }
  if(golpeTicks>0) golpeTicks--;
  avanzarPuntajesFlotantes();
  float piesAnteriores=avatar.y+altoAvatar();
  moverAvatar(c);
  if(fase!=JUGANDO) return;
  moverItems(); moverEnemigos(piesAnteriores);
  if(fase!=JUGANDO) return;
  moverJefe(piesAnteriores);
  if(fase!=JUGANDO) return;
  moverProyectiles();
  camara=constrain((int)avatar.x-56,0,ANCHO_MUNDO-ANCHO_PANTALLA);
  if(arenaCerrada) camara=ANCHO_MUNDO-ANCHO_PANTALLA;
}

// Todos los sprites siguientes son originales, dibujados en el framebuffer.
static void pixel(int x,int y,uint16_t color) {
  if(x>=0 && x<ANCHO_PANTALLA && y>=0 && y<ALTO_ESCENA) imagen[y][x]=color;
}
static void caja(int x,int y,int a,int h,uint16_t color) {
  for(int f=max(0,y);f<min(ALTO_ESCENA,y+h);f++)
    for(int c=max(0,x);c<min(ANCHO_PANTALLA,x+a);c++) imagen[f][c]=color;
}
static void dibujarTile(int x,int y,Tile t) {
  const Nivel& n=NIVELES[min(nivel,TOTAL_NIVELES-1)];
  if(t==TIERRA) {
    caja(x,y,8,8,n.suelo); caja(x,y,8,2,n.acento);
    pixel(x+2,y+5,0x4208); pixel(x+6,y+3,0xBDF7);
  } else if(t==PIEDRA || t==LADRILLO) {
    caja(x,y,8,8,t==PIEDRA?0x7BEF:0xB386);
    caja(x,y,8,1,0xCE59); caja(x,y+7,8,1,0x4208);
    caja(x+3,y+1,1,6,0x4208);
  } else if(t!=VACIO) {
    caja(x,y,8,8,t==USADO?0x6B4D:0xFCE4);
    caja(x+1,y+1,6,1,0xFFE0); caja(x,y+7,8,1,0x6242);
    if(t!=USADO) {
      caja(x+3,y+2,3,1,COLOR_BLANCO); pixel(x+5,y+3,COLOR_BLANCO);
      caja(x+3,y+4,2,1,COLOR_BLANCO); pixel(x+3,y+6,COLOR_BLANCO);
    }
  }
}
static void dibujarAvatar() {
  if(fase==REAPARECER && relojAnimacion%6<3) return;
  int x=(int)avatar.x-camara,y=(int)avatar.y,h=altoAvatar();
  uint16_t ropa=avatar.invulnerable?COLOR_BLANCO:avatar.poder==FUEGO?0xFFF4:0xFD24;
  caja(x+1,y,5,3,ropa); caja(x,y+2,7,2,ropa);
  caja(x+1,y+3,5,3,0xFE75); pixel(x+(avatar.mirando>0?5:1),y+3,COLOR_NEGRO);
  caja(x+1,y+6,5,h-8,ropa); caja(x+2,y+6,3,h-8,0x3597);
  int paso=caminando?(pasoAvatar/4):0;
  if(!avatar.suelo) {
    caja(x,y+h-4,2,2,0x216C); caja(x+4,y+h-2,3,2,0x216C);
  } else if(paso==1) {
    caja(x,y+h-2,3,2,0x216C); caja(x+4,y+h-3,2,2,0x216C);
  } else if(paso==3) {
    caja(x+1,y+h-3,2,2,0x216C); caja(x+4,y+h-2,3,2,0x216C);
  } else {
    caja(x+1,y+h-2,2,2,0x216C); caja(x+4,y+h-2,2,2,0x216C);
  }
  if(gestoDisparo>0) {
    // Brazo hacia el disparo, con mano fuera de la silueta del cuerpo.
    int brazo=avatar.mirando>0?x+5:x-3;
    caja(brazo,y+6,5,2,ropa);
    caja(avatar.mirando>0?x+8:x-3,y+6,2,2,0xFE75);
    caja(avatar.mirando>0?x:x+6,y+7,1,2,ropa);
  } else {
    int balanceo=paso==1?-1:paso==3?1:0;
    caja(x,y+6+balanceo,1,2,ropa); caja(x+6,y+6-balanceo,1,2,ropa);
    pixel(x,y+8+balanceo,0xFE75); pixel(x+6,y+8-balanceo,0xFE75);
  }
  if(avatar.poder==FUEGO) pixel(x+3,y+1,0xF800);
}
static void dibujarPuntajesFlotantes() {
  static const uint8_t DIGITOS[11][5]={
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7},{0,2,7,2,0}
  };
  for(const PuntajeFlotante& aviso:puntajesFlotantes) if(aviso.vida>0) {
    char texto[12];snprintf(texto,sizeof(texto),"+%d",aviso.valor);
    int x=aviso.x-camara-((int)strlen(texto)*4-1)/2;
    // Dos pasadas: contorno y relleno. No se dibuja sobre la TFT por separado.
    for(int pasada=0;pasada<2;pasada++) for(int i=0;texto[i];i++) {
      int digito=texto[i]=='+'?10:texto[i]-'0';
      if(digito<0 || digito>10) continue;
      for(int f=0;f<5;f++) for(int c=0;c<3;c++) if(DIGITOS[digito][f]&(1<<(2-c))) {
        if(pasada==0) caja(x+i*4+c-1,aviso.y+f-1,3,3,COLOR_NEGRO);
        else pixel(x+i*4+c,aviso.y+f,COLOR_AMARILLO);
      }
    }
  }
}
static void dibujarItem(const Item& item) {
  int x=(int)item.x-camara,y=(int)item.y;
  if(item.flor) {
    caja(x+3,y+4,2,4,0x3668); caja(x+1,y+5,5,1,0x3668);
    caja(x+1,y+1,6,3,0xF81F); caja(x+2,y,4,5,0xF81F);
    caja(x+3,y+1,2,2,0xFFE0);
  } else {
    caja(x+2,y+4,4,4,0xFFF4); caja(x,y+2,8,3,0x07FF);
    caja(x+1,y,6,4,0x07FF); pixel(x+2,y+1,COLOR_BLANCO); pixel(x+5,y+2,COLOR_BLANCO);
  }
  // Durante la emergencia solo se ve la parte que sobresale del bloque.
  if(item.emergiendo>0) {
    int fila=((int)item.y+8-item.emergiendo)/TILE;
    int columna=(int)item.x/TILE;
    int yBloque=fila*TILE;
    if(golpeTicks>0 && columna==bloqueGolpeX && fila==bloqueGolpeY)
      yBloque-=golpeTicks>3?2:1;
    dibujarTile(columna*TILE-camara,yBloque,celda(columna,fila));
  }
}
static void componerEscena() {
  escenasCompuestas++;
  const Nivel& n=NIVELES[min(nivel,TOTAL_NIVELES-1)];
  for(int y=0;y<ALTO_ESCENA;y++) for(int x=0;x<ANCHO_PANTALLA;x++) {
    int paisaje=(x+camara/3)%100;
    imagen[y][x]=y>72+abs(paisaje-50)/4?n.acento:n.cielo;
  }
  for(int f=0;f<FILAS;f++) for(int c=camara/TILE;c<=min(COLUMNAS-1,(camara+159)/TILE);c++) {
    int y=f*TILE;
    if(golpeTicks>0 && c==bloqueGolpeX && f==bloqueGolpeY) y-=golpeTicks>3?2:1;
    dibujarTile(c*TILE-camara,y,celda(c,f));
  }
  // Poste de checkpoint y puerta de salida.
  int cp=384-camara;
  caja(cp,76,2,20,COLOR_BLANCO); caja(cp+2,76,8,5,puntoControl>16?COLOR_AMARILLO:COLOR_GRIS);
  int meta=ANCHO_MUNDO-23-camara;
  caja(meta,72,16,24,0xC618); caja(meta+3,78,10,18,0x2965);
  if(nivelJefe() && jefe.activo) caja(meta+3,79,10,1,COLOR_ROJO);
  for(const Item& item:items) if(item.activo) dibujarItem(item);
  for(const Enemigo& e:enemigos) if(e.activo) {
    int x=(int)e.x-camara,y=(int)e.y;
    caja(x+1,y,6,6,0x86A4); caja(x,y+4,8,3,0x5444);
    pixel(x+2,y+1,COLOR_NEGRO); pixel(x+5,y+1,COLOR_NEGRO);
    caja(x+(relojAnimacion/5%2),y+7,3,1,COLOR_NEGRO); caja(x+4,y+7,3,1,COLOR_NEGRO);
  }
  if(jefe.activo) {
    int x=(int)jefe.x-camara,y=(int)jefe.y;
    caja(x+2,y,14,6,jefe.invulnerable?COLOR_BLANCO:0xB3FC); caja(x,y+5,18,14,0x79B4);
    caja(x+2,y+18,5,4,0x390B); caja(x+11,y+18,5,4,0x390B);
    caja(x+1,y-3,3,5,0xFFE0); caja(x+14,y-3,3,5,0xFFE0);
    caja(x+4,y+6,3,2,COLOR_AMARILLO); caja(x+11,y+6,3,2,COLOR_AMARILLO);
    caja(x+6,y+12,6,2,COLOR_NEGRO);
  }
  dibujarAvatar();
  for(const Proyectil& p:fuegos) if(p.activo) {
    caja((int)p.x-camara,(int)p.y,4,4,0xFBA0); caja((int)p.x-camara+1,(int)p.y+1,2,2,0xFFE0);
  }
  for(const Proyectil& p:ataques) if(p.activo) caja((int)p.x-camara,(int)p.y,4,4,COLOR_ROJO);
  if(nivelJefe() && arenaCerrada && jefe.activo) {
    caja(44,3,72,5,COLOR_NEGRO); caja(45,4,70*jefe.hp/jefe.hpMax,3,COLOR_ROJO);
  }
  dibujarPuntajesFlotantes();
}
static bool tileCambio(int c,int f) {
  if(!imagenValida) return true;
  for(int y=f*TILE;y<min(ALTO_ESCENA,(f+1)*TILE);y++)
    if(memcmp(&imagen[y][c*TILE],&anterior[y][c*TILE],TILE*sizeof(uint16_t))) return true;
  return false;
}
static void enviarEscena() {
  for(int f=0;f<FILAS;f++) {
    int c=0;
    while(c<ANCHO_PANTALLA/TILE) {
      if(!tileCambio(c,f)) { c++; continue; }
      int inicio=c++;
      while(c<ANCHO_PANTALLA/TILE && tileCambio(c,f)) c++;
      int ancho=(c-inicio)*TILE;
      int alto=min(TILE,ALTO_ESCENA-f*TILE);
      for(int y=0;y<alto;y++) memcpy(banda+y*ancho,&imagen[f*TILE+y][inicio*TILE],ancho*sizeof(uint16_t));
      dibujarImagenRGB565(inicio*TILE,HUD+f*TILE,ancho,alto,banda);
    }
  }
  memcpy(anterior,imagen,sizeof(imagen)); imagenValida=true;
}
static bool cambioVisible() {
  int firma[160]={},indice=0;
  auto guardar=[&](int valor) { firma[indice++]=valor; };
  guardar(nivel);guardar(puntos);guardar(vidas);guardar((int)avatar.poder);
  guardar((int)avatar.x);guardar((int)avatar.y);guardar(avatar.mirando);
  guardar(avatar.invulnerable>0);guardar(avatar.suelo);
  guardar(caminando?pasoAvatar/4:0);guardar(gestoDisparo>0);guardar(agachado);
  guardar(camara);guardar(puntoControl>16);guardar((int)fase);
  guardar((int)revisionMapa);guardar(sonidoHabilitado());
  bool golpeVisible=golpeTicks>0 && bloqueGolpeX*TILE+TILE>camara && bloqueGolpeX*TILE<camara+160;
  guardar(golpeVisible?bloqueGolpeX:0);guardar(golpeVisible?bloqueGolpeY:0);
  guardar(golpeVisible?(golpeTicks>3?2:1):0);
  auto actor=[&](bool activo,float x,float y,int ancho,int alto,int rasgo) {
    bool visible=activo && x+ancho>camara && x<camara+160 && y+alto>0 && y<ALTO_ESCENA;
    guardar(visible);guardar(visible?(int)x:0);guardar(visible?(int)y:0);guardar(visible?rasgo:0);
  };
  for(const Enemigo& e:enemigos) actor(e.activo,e.x,e.y,8,8,(relojAnimacion/5)%2);
  for(const Item& item:items) actor(item.activo,item.x,item.y,8,8,(item.flor?1:0)|(item.emergiendo>0?2:0));
  for(const Proyectil& p:fuegos) actor(p.activo,p.x,p.y,4,4,0);
  for(const Proyectil& p:ataques) actor(p.activo,p.x,p.y,4,4,0);
  actor(jefe.activo,jefe.x,jefe.y-3,18,25,jefe.invulnerable>0);
  guardar(jefe.activo?jefe.hp:0);guardar(jefe.activo?jefe.hpMax:0);guardar(arenaCerrada);
  for(const PuntajeFlotante& aviso:puntajesFlotantes)
    actor(aviso.vida>0,aviso.x-10.0f,aviso.y-1.0f,24,7,aviso.valor);
  // 143 enteros, sin padding ni hashes: la comparación no tiene colisiones.
  static_assert(20+MAX_ENEMIGOS*4+MAX_ITEMS*4+(MAX_FUEGOS+MAX_ATAQUES)*4+7+
                MAX_PUNTAJES_FLOTANTES*4<=160,"Firma visual insuficiente");
  bool cambio=!firmaValida || !imagenValida || memcmp(firma,firmaMostrada,sizeof(firma))!=0;
  if(cambio) { memcpy(firmaMostrada,firma,sizeof(firma));firmaValida=true; }
  return cambio;
}
static void dibujarMarcador() {
  static int puntosMostrados,vidasMostradas,nivelMostrado,poderMostrado;
  static bool sonidoMostrado;
  if(marcadorValido && puntosMostrados==puntos && vidasMostradas==vidas &&
     nivelMostrado==nivel && poderMostrado==(int)avatar.poder && sonidoMostrado==sonidoHabilitado()) return;
  char izquierda[32],derecha[32];
  rectanguloRelleno(0,0,ANCHO_PANTALLA,HUD,COLOR_BARRA);
  snprintf(izquierda,sizeof(izquierda),"Puntaje:%d",puntos);
  snprintf(derecha,sizeof(derecha),"Vidas:%d",vidas);
  filaMarcador(2,izquierda,derecha);
  snprintf(izquierda,sizeof(izquierda),"Nivel:%d-%d",min(nivel/4+1,2),nivel%4+1);
  const char* poder=avatar.poder==FUEGO?"Fuego":avatar.poder==GRANDE?"Grande":"Pequeno";
  snprintf(derecha,sizeof(derecha),"Poder:%s",poder);
  filaMarcador(11,izquierda,derecha);
  dibujarAltavoz();
  puntosMostrados=puntos;vidasMostradas=vidas;nivelMostrado=nivel;
  poderMostrado=(int)avatar.poder;sonidoMostrado=sonidoHabilitado();marcadorValido=true;
}
static void dibujarJuego() {
  bool panel=fase==ENTRADA || fase==SALIDA || fase==REAPARECER;
  if(panel && panelVisible && faseMostrada==fase) return;
  if(!cambioVisible()) return;
  componerEscena(); enviarEscena(); dibujarMarcador();
  panelVisible=panel; faseMostrada=fase;
  if(panel) {
    rectanguloRelleno(15,42,130,39,COLOR_FONDO_UI);
    char texto[24];
    if(fase==REAPARECER) snprintf(texto,sizeof(texto),"VIDAS: %d",vidas);
    else if(fase==SALIDA) snprintf(texto,sizeof(texto),"ETAPA SUPERADA");
    else snprintf(texto,sizeof(texto),"MUNDO %d-%d",nivel/4+1,nivel%4+1);
    escribirCentrado(49,texto,COLOR_AMARILLO,1);
    escribirCentrado(65,NIVELES[min(nivel,TOTAL_NIVELES-1)].nombre,COLOR_BLANCO,1);
    // El panel se dibuja fuera del framebuffer: forzar su restauración al salir.
    imagenValida=false;
  }
}
} // namespace

void comenzarBrasa() {
  nivel=0; puntos=0; vidas=dificultadActual()==FACIL?4:dificultadActual()==DIFICIL?2:3;
  avatar={}; avatar.poder=PEQUENO; relojAnimacion=0; arribaAnterior=false;
  escenasCompuestas=0;
  resultado=PARTIDA_CONTINUA; cargarNivel(); ultimoPaso=millis(); dibujarJuego();
}
ResultadoPartida actualizarBrasa(const EstadoControles& c) {
  if(fase==JUGANDO) {
    if(c.a.recienPresionado || (c.arriba && !arribaAnterior)) saltoBuffer=4;
    if(c.b.recienPresionado) disparoPedido=true;
  }
  arribaAnterior=c.arriba;
  unsigned int pasos=pasosPendientes(millis(),ultimoPaso,PASO_MS);
  if(!pasos) return resultado;
  for(unsigned int i=0;i<pasos && resultado==PARTIDA_CONTINUA;i++) pasoJuego(c);
  if(resultado==PARTIDA_CONTINUA) dibujarJuego();
  return resultado;
}
void redibujarBrasa() {
  saltoBuffer=0; disparoPedido=false; arribaAnterior=false; ultimoPaso=millis();
  imagenValida=false; firmaValida=false; marcadorValido=false; panelVisible=false; dibujarJuego();
}
int puntuacionBrasa() { return puntos; }
