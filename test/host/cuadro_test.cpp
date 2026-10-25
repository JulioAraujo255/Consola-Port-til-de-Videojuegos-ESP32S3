#include "cuadro.h"
#include <cstdio>
#include <cstdlib>

#define VERIFICAR(x) do {if(!(x)){std::fprintf(stderr,"Linea %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
  CuadroRGB565<160,128> cuadro;
  uint16_t pantalla[128][160]={};int envios=0;
  int pixelesEnviados=0;
  auto enviar=[&](int x,int y,int a,int h,uint16_t* datos,int salto) {
    envios++;
    pixelesEnviados+=a*h;
    for(int f=0;f<h;f++) memcpy(&pantalla[y+f][x],datos+f*salto,a*sizeof(uint16_t));
  };
  cuadro.presentar(enviar);VERIFICAR(envios==0);
  // El borrado temporal y el sprite nuevo no llegan por separado a la TFT.
  cuadro.pixeles[30][20]=0xFFFF;cuadro.ensuciar(20,30,1,1);cuadro.presentar(enviar);
  VERIFICAR(pantalla[30][20]==0xFFFF && envios==1);
  VERIFICAR(pixelesEnviados==1); // Recorta el tile al píxel realmente cambiado.
  cuadro.pixeles[30][20]=0;cuadro.ensuciar(20,30,1,1);
  VERIFICAR(pantalla[30][20]==0xFFFF);
  cuadro.pixeles[30][20]=0x07E0;cuadro.ensuciar(20,30,1,1);
  cuadro.presentar(enviar);VERIFICAR(pantalla[30][20]==0x07E0 && envios==2);
  cuadro.presentar(enviar);VERIFICAR(envios==2);
  // Repetir el marcador o borrar/restaurar un sprite idéntico no envía nada.
  cuadro.pixeles[30][20]=0;cuadro.ensuciar(20,30,1,1);
  cuadro.pixeles[30][20]=0x07E0;cuadro.presentar(enviar);VERIFICAR(envios==2);
  // Recorte en los cuatro bordes, y tiles contiguos en una sola banda.
  cuadro.ensuciar(-10,-10,4,4);cuadro.ensuciar(170,130,8,8);VERIFICAR(!cuadro.pendiente());
  cuadro.pixeles[0][0]=0xFFFF;cuadro.pixeles[0][9]=0xFFFF;
  cuadro.ensuciar(-2,-2,12,4);envios=0;cuadro.presentar(enviar);VERIFICAR(envios==1);
  cuadro.pixeles[127][159]=0xF800;cuadro.ensuciar(159,127,5,5);cuadro.presentar(enviar);
  VERIFICAR(pantalla[127][159]==0xF800);
  for(int y=0;y<128;y++)for(int x=0;x<160;x++)cuadro.pixeles[y][x]=(uint16_t)(x+y*160+1);
  envios=0;cuadro.ensuciar(0,0,160,128);cuadro.presentar(enviar);VERIFICAR(envios==1);
  VERIFICAR(memcmp(pantalla,cuadro.pixeles,sizeof(pantalla))==0 && !cuadro.pendiente());
  // Patrones irregulares: no se pierden tiles de filas adyacentes más anchas.
  for(int i=0;i<2000;i++) {
    int x=(i*37)%160,y=(i*23)%128;
    cuadro.pixeles[y][x]^=0xFFFF;cuadro.ensuciar(x,y,1,1);
    if(i%13==0) {cuadro.presentar(enviar);VERIFICAR(memcmp(pantalla,cuadro.pixeles,sizeof(pantalla))==0);}
  }
  cuadro.presentar(enviar);VERIFICAR(memcmp(pantalla,cuadro.pixeles,sizeof(pantalla))==0);
  std::puts("Pantalla: OK (comparacion exacta, sin reenvios identicos, recorte, fusion vertical y patrones mixtos)");
}
