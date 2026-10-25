#include "soporte.h"
#include <vector>
#include <unordered_set>
#include <queue>
#include "../../src/brasa.cpp"

static void jugar() { comenzarBrasa(); fase=JUGANDO; }
static void ticks(int cantidad, const EstadoControles& c={}) {
  for(int i=0;i<cantidad;i++) { relojPrueba+=PASO_MS; actualizarBrasa(c); }
}
static void sinEnemigos() { for(Enemigo& e:enemigos) e.activo=false; }

// Busca una ruta con la física real, usando solo caminar a la derecha y
// pulsar/mantener/soltar salto. Los bloques usados conservan su solidez.
static bool rutaHastaMeta(int etapa,Poder poder=PEQUENO) {
  nivel=etapa; avatar.poder=poder; cargarNivel(); fase=JUGANDO; sinEnemigos();
  // La búsqueda debe ser válida sin depender de ladrillos que otra rama rompa.
  if(poder!=PEQUENO) for(auto& fila:mapa) for(uint8_t& tile:fila) if(tile==LADRILLO) tile=PIEDRA;
  struct Estado { Avatar a; int coy; bool pulsado; };
  auto prioridad=[](const Estado& a,const Estado& b) { return a.a.x<b.a.x; };
  std::priority_queue<Estado,std::vector<Estado>,decltype(prioridad)> cola(prioridad);
  cola.push({avatar,0,false});
  std::unordered_set<uint64_t> visitados;
  float maximoX=0;
  for(size_t indice=0;!cola.empty() && indice<250000;indice++) {
    Estado actual=cola.top();cola.pop();
    for(bool saltar : {false,true}) {
      avatar=actual.a; coyote=actual.coy; saltoBuffer=(!actual.pulsado && saltar)?4:0;
      fase=JUGANDO; disparoPedido=false;
      if(nivelJefe()) {
        arenaCerrada=avatar.x>=ANCHO_MUNDO-154;
        for(int f=2;f<12;f++) mapa[f][75]=arenaCerrada?PIEDRA:VACIO;
      }
      EstadoControles c={}; c.derecha=true; c.a.presionado=saltar;
      moverAvatar(c);
      maximoX=max(maximoX,avatar.x);
      if(avatar.x>ANCHO_MUNDO-27) return true;
      if(fase==REAPARECER || avatar.y>ALTO_ESCENA || avatar.y<0) continue;
      int xx=(int)avatar.x, yy=(int)avatar.y, vy=(int)((avatar.vy+5)*4);
      uint64_t clave=(uint64_t)xx | ((uint64_t)yy<<10) | ((uint64_t)vy<<18) |
                     ((uint64_t)coyote<<24) | ((uint64_t)saltar<<27);
      if(visitados.insert(clave).second) cola.push({avatar,coyote,saltar});
    }
  }
  std::fprintf(stderr,"Ruta no encontrada en nivel %d, %zu estados, X=%.1f\n",etapa,visitados.size(),maximoX);
  return false;
}

static void guardarBMP(const char* ruta) {
  FILE* archivo=std::fopen(ruta,"wb"); COMPROBAR(archivo);
  uint8_t cabecera[54]={};
  auto u32=[&](int offset,uint32_t valor) { for(int i=0;i<4;i++) cabecera[offset+i]=(uint8_t)(valor>>(8*i)); };
  auto u16=[&](int offset,uint16_t valor) { cabecera[offset]=(uint8_t)valor; cabecera[offset+1]=(uint8_t)(valor>>8); };
  cabecera[0]='B';cabecera[1]='M';u32(2,54+160*128*3);u32(10,54);u32(14,40);
  u32(18,160);u32(22,128);u16(26,1);u16(28,24);
  std::fwrite(cabecera,1,54,archivo);
  for(int y=127;y>=0;y--) for(int x=0;x<160;x++) {
    uint16_t c=pixelesPrueba[y][x];
    uint8_t bgr[3]={(uint8_t)((c&31)*255/31),(uint8_t)(((c>>5)&63)*255/63),(uint8_t)((c>>11)*255/31)};
    std::fwrite(bgr,1,3,archivo);
  }
  std::fclose(archivo);
}

int main() {
  jugar(); COMPROBAR(vidas==3 && nivel==0 && avatar.poder==PEQUENO);
  COMPROBAR(apareceTexto("Puntaje:") && apareceTexto("Vidas:") && apareceTexto("Nivel:") && apareceTexto("Poder:Pequeno"));
  // Un bloque entrega exactamente un item y queda usado.
  golpearBloque(10,8); COMPROBAR(mapa[8][10]==USADO && items[0].activo && !items[0].flor);
  golpearBloque(10,8); COMPROBAR(!items[1].activo);
  for(int i=0;i<8;i++) moverItems(); COMPROBAR(items[0].emergiendo==0 && items[0].y==56);
  avatar.x=items[0].x;avatar.y=items[0].y-2; moverItems();
  COMPROBAR(avatar.poder==GRANDE && !items[0].activo);
  golpearBloque(12,8); COMPROBAR(items[0].flor);
  for(int i=0;i<8;i++) moverItems();
  avatar.x=items[0].x;avatar.y=items[0].y-8; moverItems(); COMPROBAR(avatar.poder==FUEGO);
  int puntosAntes=puntos; golpearBloque(11,8); COMPROBAR(mapa[8][11]==VACIO && puntos==puntosAntes+5);
  // Crecimiento bajo techo: no consume el item ni mete al avatar en un bloque.
  jugar(); avatar.x=80;avatar.y=73;
  items[0]={true,80,76,0,1,0,true}; moverItems();
  COMPROBAR(avatar.poder==PEQUENO && items[0].activo);
  // Los tiros requieren flor, respetan cooldown y hay como máximo tres.
  jugar(); disparar(); COMPROBAR(!fuegos[0].activo);
  avatar.poder=FUEGO;avatar.y=80; disparar(); COMPROBAR(fuegos[0].activo);
  disparar(); COMPROBAR(!fuegos[1].activo);
  recarga=0;disparar();recarga=0;disparar();recarga=0;disparar();
  int activos=0;for(const Proyectil& p:fuegos) activos+=p.activo;
  COMPROBAR(activos==3);
  // Fuego rebota en suelo y elimina enemigos.
  fuegos[0]={true,40,92,3,1,100}; moverProyectiles(); COMPROBAR(fuegos[0].vy<0);
  jugar(); sinEnemigos(); enemigos[0]={true,80,88,1};
  fuegos[0]={true,77,88,3,0,100}; moverProyectiles();
  COMPROBAR(!enemigos[0].activo && !fuegos[0].activo && puntos==50);
  COMPROBAR(puntajesFlotantes[0].valor==50 && puntajesFlotantes[0].vida==DURACION_PUNTAJE);
  // Hongos y flores repetidos acreditan una sola vez, conservando el poder.
  for(int caso=0;caso<3;caso++) {
    jugar();avatar.x=40;avatar.poder=caso==0?GRANDE:FUEGO;avatar.y=82;
    Poder esperado=avatar.poder;
    items[0]={true,40,88,0,1,0,caso==2};moverItems();
    COMPROBAR(avatar.poder==esperado && puntos==50 && !items[0].activo);
    COMPROBAR(puntajesFlotantes[0].valor==50 && puntajesFlotantes[0].vida==30);
    moverItems();COMPROBAR(puntos==50);
  }
  // El aviso sube, sigue sus coordenadas de mundo y desaparece sin dejar rastros.
  jugar();sinEnemigos();avatar.x=40;camara=0;componerEscena();
  uint16_t sinAviso[ALTO_ESCENA][ANCHO_PANTALLA];memcpy(sinAviso,imagen,sizeof(sinAviso));
  acreditarPuntos(50,60,80);componerEscena();COMPROBAR(imagen[74][56]==COLOR_AMARILLO);
  camara=10;componerEscena();COMPROBAR(imagen[74][46]==COLOR_AMARILLO);
  camara=0;int yAviso=puntajesFlotantes[0].y;
  avanzarPuntajesFlotantes();avanzarPuntajesFlotantes();COMPROBAR(puntajesFlotantes[0].y<yAviso);
  int vidaAviso=puntajesFlotantes[0].vida;
  relojPrueba+=10000;redibujarBrasa();COMPROBAR(puntajesFlotantes[0].vida==vidaAviso);
  for(int i=0;i<28;i++) avanzarPuntajesFlotantes();
  componerEscena();COMPROBAR(memcmp(sinAviso,imagen,sizeof(sinAviso))==0);
  jugar();for(int i=0;i<8;i++)acreditarPuntos(50,40,80);
  activos=0;for(const PuntajeFlotante& p:puntajesFlotantes)activos+=p.vida>0;
  COMPROBAR(puntos==400 && activos==MAX_PUNTAJES_FLOTANTES);
  // Daño escalonado y ventana de invulnerabilidad.
  jugar(); avatar.poder=FUEGO;avatar.y=80; recibirDano(); COMPROBAR(avatar.poder==GRANDE);
  recibirDano();COMPROBAR(avatar.poder==GRANDE);avatar.invulnerable=0;recibirDano();
  COMPROBAR(avatar.poder==PEQUENO && vidas==3);avatar.invulnerable=0;recibirDano();
  COMPROBAR(vidas==2 && fase==REAPARECER); ticks(36);
  COMPROBAR(fase==ENTRADA && avatar.x==16);
  // Reaparición en checkpoint restaura también la cámara y conserva bloques usados.
  jugar();puntoControl=410;camara=450;mapa[8][10]=USADO;morir();relojFase=1;pasoJuego({});
  COMPROBAR(avatar.x==410 && camara==354 && mapa[8][10]==USADO);
  // Si la tabla de items está llena, el bloque conserva su recompensa.
  jugar();for(Item& item:items) item={true,0,0,0,1,0,false};
  golpearBloque(10,8);COMPROBAR(mapa[8][10]==BLOQUE_HONGO);
  // Choque de cabeza real: el bloque se activa durante la subida, nunca de lado.
  jugar(); avatar.x=80;avatar.y=86;avatar.vy=-4.4f;
  for(int i=0;i<10;i++) { avatar.vy+=0.25f;moverAvatarY(avatar.vy); }
  COMPROBAR(mapa[8][10]==USADO);
  jugar(); avatar.x=8;avatar.y=86; mapa[11][2]=PIEDRA;
  moverAvatarX(20); COMPROBAR(avatar.x+ANCHO_AVATAR<=16.01f); // No atraviesa paredes.
  // Pisotón válido frente a roce lateral.
  jugar();sinEnemigos();enemigos[0]={true,80,88,1};avatar.x=80;avatar.y=79;avatar.vy=1;
  moverEnemigos(87); COMPROBAR(!enemigos[0].activo && avatar.vy<0);
  COMPROBAR(puntos==50 && puntajesFlotantes[0].valor==50);
  moverEnemigos(87);COMPROBAR(puntos==50);
  jugar();avatar.y=130; moverAvatar({}); COMPROBAR(vidas==2 && fase==REAPARECER);
  // La pausa/redibujo conserva todos los temporizadores de simulación.
  jugar();avatar.invulnerable=25;recarga=7;saltoBuffer=3;disparoPedido=true;
  relojPrueba+=20000;redibujarBrasa();
  COMPROBAR(avatar.invulnerable==25 && recarga==7 && saltoBuffer==0 && !disparoPedido);
  COMPROBAR(actualizarBrasa({})==PARTIDA_CONTINUA && avatar.invulnerable==25);
  // Altura crecida con margen en corredores de 16 px, incluso al aterrizar
  // desde una posición fraccionaria. La colisión apoya los pies en Y=96 exacto.
  jugar();avatar.poder=GRANDE;avatar.y=81.7f;avatar.x=160;avatar.vy=1;
  moverAvatarY(1);COMPROBAR(avatar.y+altoAvatar()==96);
  for(int c=20;c<26;c++) mapa[9][c]=PIEDRA;
  moverAvatarX(20);COMPROBAR(avatar.x>179 && !solido(avatar.x,avatar.y,ANCHO_AVATAR,altoAvatar()));
  EstadoControles abajo={};abajo.abajo=true;
  actualizarPostura(abajo);COMPROBAR(agachado && altoAvatar()==10 && avatar.y+altoAvatar()==96);
  actualizarPostura({});COMPROBAR(!agachado && altoAvatar()==14 && avatar.y+altoAvatar()==96);
  // Al soltar abajo bajo un techo no se expande dentro de un tile.
  avatar.y=66;agachado=true;mapa[7][(int)avatar.x/TILE]=PIEDRA;
  actualizarPostura({});COMPROBAR(agachado);
  // Piernas/brazos cambian al caminar; el disparo tiene pose propia.
  jugar();sinEnemigos();EstadoControles andar={};andar.derecha=true;
  moverAvatar(andar);COMPROBAR(caminando);
  avatar.x=40;camara=0;pasoAvatar=0;componerEscena();
  uint16_t quieto[ALTO_ESCENA][ANCHO_PANTALLA];memcpy(quieto,imagen,sizeof(quieto));
  pasoAvatar=4;componerEscena();COMPROBAR(memcmp(quieto,imagen,sizeof(quieto))!=0);
  avatar.poder=FUEGO;avatar.y=(float)(96-altoAvatar());recarga=0;disparar();COMPROBAR(gestoDisparo==6);
  componerEscena();COMPROBAR(imagen[(int)avatar.y+6][(int)avatar.x+8]==0xFE75);
  for(int i=0;i<6;i++) moverAvatar({});COMPROBAR(gestoDisparo==0);
  // Invulnerabilidad visible de forma estable, sin hacer desaparecer al avatar.
  avatar.invulnerable=20;relojAnimacion=0;componerEscena();
  COMPROBAR(imagen[(int)avatar.y][(int)avatar.x+1]==COLOR_BLANCO);
  relojAnimacion=3;componerEscena();
  COMPROBAR(imagen[(int)avatar.y][(int)avatar.x+1]==COLOR_BLANCO);
  // No hay tiros ni saltos fantasmas tras la transición de etapa.
  jugar();fase=SALIDA;relojFase=1;saltoBuffer=3;disparoPedido=true;pasoJuego({});
  COMPROBAR(nivel==1 && fase==ENTRADA && saltoBuffer==0 && !disparoPedido);
  // Ocho mapas completos: una ruta existe con el avatar pequeño, sin usar poderes.
  for(int etapa=0;etapa<TOTAL_NIVELES;etapa++) COMPROBAR(rutaHastaMeta(etapa));
  for(int etapa=0;etapa<TOTAL_NIVELES;etapa++) COMPROBAR(rutaHastaMeta(etapa,GRANDE));
  // Jefes exclusivamente en el cuarto nivel de cada mundo.
  for(int etapa=0;etapa<TOTAL_NIVELES;etapa++) {
    nivel=etapa;avatar.poder=PEQUENO;cargarNivel();
    COMPROBAR(jefe.activo==(etapa%4==3));
  }
  nivel=3;cargarNivel();fase=JUGANDO;avatar.x=ANCHO_MUNDO-153;avatar.y=86;
  moverAvatar({});COMPROBAR(arenaCerrada && mapa[8][75]==PIEDRA);
  jefe.reloj=64;moverJefe(96);activos=0;for(const Proyectil& p:ataques) activos+=p.activo;
  COMPROBAR(activos==1);
  // Un pisotón real hace daño y rebota aun sin flor.
  avatar.x=jefe.x+5;avatar.y=jefe.y-9;avatar.vy=1;
  int hp=jefe.hp;moverJefe(jefe.y);COMPROBAR(jefe.hp==hp-1 && avatar.vy<0);
  herirJefe();COMPROBAR(jefe.hp==hp-1);
  while(jefe.activo) { jefe.invulnerable=0;herirJefe(); }
  COMPROBAR(jefe.hp==0 && puntos>=500);
  avatar.x=ANCHO_MUNDO-23;avatar.y=86;moverAvatar({});COMPROBAR(fase==SALIDA);
  // El primer jefe conduce a 2-1; el último termina la campaña.
  relojFase=1;pasoJuego({});COMPROBAR(nivel==4 && fase==ENTRADA);
  nivel=3;cargarNivel();fase=JUGANDO;arenaCerrada=true;jefe.hp=1;morir();relojFase=1;pasoJuego({});
  COMPROBAR(jefe.hp==jefe.hpMax && !arenaCerrada && fase==ENTRADA);
  nivel=7;cargarNivel();fase=JUGANDO;jefe.activo=false;avatar.x=ANCHO_MUNDO-23;
  moverAvatar({});relojFase=1;pasoJuego({});COMPROBAR(resultado==PARTIDA_GANADA);
  jugar();vidas=1;morir();relojFase=1;pasoJuego({});COMPROBAR(resultado==PARTIDA_PERDIDA);
  // Envíos parciales exactos y restauración tras el panel de etapa.
  jugar();imagenValida=false;dibujarJuego();enviosImagenPrueba=0;dibujarJuego();
  COMPROBAR(enviosImagenPrueba==0);
  jugar();sinEnemigos();dibujarJuego();
  unsigned long compuestas=escenasCompuestas;enviosImagenPrueba=0;size_t textos=textosPrueba.size();
  ticks(90);
  COMPROBAR(escenasCompuestas==compuestas && enviosImagenPrueba==0 && textosPrueba.size()==textos);
  enemigos[0]={true,100,88,-1};ticks(1);COMPROBAR(escenasCompuestas>compuestas && enviosImagenPrueba>0);
  COMPROBAR(textosPrueba.size()==textos); // Un enemigo no obliga a repintar el marcador.
  compuestas=escenasCompuestas;redibujarBrasa();COMPROBAR(escenasCompuestas==compuestas+1);
  avatar.x=180;camara=124;dibujarJuego();
  for(int y=0;y<ALTO_ESCENA;y++)for(int x=0;x<160;x++)
    COMPROBAR(pixelesPrueba[y+HUD][x]==imagen[y][x]);
  jugar();avatar.x=40;golpearBloque(10,8);golpearBloque(12,8);
  for(int i=0;i<8;i++) moverItems();dibujarJuego();
  guardarBMP(".pio/host-tests/brasa-pradera.bmp");
  acreditarPuntos(50,60,80);dibujarJuego();guardarBMP(".pio/host-tests/brasa-puntaje.bmp");
  nivel=3;avatar.poder=FUEGO;cargarNivel();fase=JUGANDO;arenaCerrada=true;
  camara=ANCHO_MUNDO-160;avatar.x=(float)camara+30;avatar.y=(float)(96-altoAvatar());dibujarJuego();
  guardarBMP(".pio/host-tests/brasa-jefe.bmp");
  gestoDisparo=6;dibujarJuego();guardarBMP(".pio/host-tests/brasa-disparo.bmp");
  comenzarBrasa();enviosImagenPrueba=0;ticks(10);COMPROBAR(enviosImagenPrueba==0);
  ticks(20);COMPROBAR(fase==JUGANDO && enviosImagenPrueba>0);
  for(int y=0;y<ALTO_ESCENA;y++)for(int x=0;x<160;x++)
    COMPROBAR(pixelesPrueba[y+HUD][x]==imagen[y][x]);
  std::puts("JUMP-MAN: OK (16 rutas, poderes extra, puntajes flotantes, reposo sin recomposicion, animaciones y framebuffer)");
}
