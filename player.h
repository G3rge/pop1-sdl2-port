// player.h - player con el sistema de secuencias original (seg005/seg006 de SDLPoP)
// coords: x=px izquierda (tile 32px), fy=px pies; la seqtbl usa unidades internas
// originales (14/tile) convertidas con KDS=32/14 px por unidad.
#pragma once
#include "level.h"

// px por unidad interna original (tile interno = 14 unidades, tile pantalla = 32px)
constexpr float PLAYER_KDS_PX = 32.0f / 14.0f;

struct Player {
    float x = 0, fy = 0;   // borde izq y pies (pixels)
    float vy = 0;          // velocidad vertical (px/tick 60fps)
    float vx = 0;          // signo de avance (leido por resolveHoriz)
    int   dir = 1;         // 1 derecha, -1 izq
    int   w = 18, h = 56;
    int   frame = 15;      // frame de animacion actual (15 = stand)
    int   action = 0;      // accion actual (0 stand, 1 run, 7 turn)
    int   seqId = 2;       // secuencia logica actual (1 startrun, 2 stand, 5 turn...)
    int   currSeq = 0;     // offset actual dentro de la seqtbl
};

inline int player_col(const Player& p) {
    int c = (int)(p.x + p.w / 2) / 32;
    if (c < 0) c = 0;
    if (c > 9) c = 9;
    return c;
}

// el cuerpo ocupa filas de (fy-h) a fy -> comprobar si una celda horizontal esta bloqueada
bool blockedHoriz(const Level& lv, int room, int cellCol, int physRowTop, int physRowBot);

// resuelve x tras mover (caras de pared tipo seg004): devuelve x sin solapamiento
float resolveHoriz(const Level& lv, int room, const Player& p, float x);

// posicion inicial del player: baja hasta un piso si el start cae en el aire
// y reinicia el estado de animacion (frame 15 stand, seqtbl en stand)
void placeAtStart(const Level& lv, Player& p);

// secuencia de un tick de juego (12Hz): control() + play_seq() + colision horizontal.
// El control usa el estado de tecla MANTENIDO (control_x == CONTROL_PRESSED),
// como el original: pulsar y mantener derecha corre/avanza, izquierda gira atras.
void tickPlayer(const Level& lv, int room, Player& p, bool left, bool right);

// fisica por frame (60fps): gravedad + aterrizaje + reinicio al caer del nivel
void physicsPlayer(const Level& lv, int room, Player& p);

// frame de animacion (1..220) -> indice en frames[] (num. de imagen, res 401+idx)
int playerFrameImage(int frame);

// dx propio del frame (frame_table_kid) en unidades internas -> p.ej. para el
// offset de dibujo por frame de la animacion
int playerFrameDx(int frame);

// nombres legibles para debug (HUD / log de terminal)
const char* playerSeqName(int currSeq);
const char* playerActionName(int action);