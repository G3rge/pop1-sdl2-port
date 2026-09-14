// level.h - nivel (LEVELS.DAT): struct + geometria + tiles + dibujo
#pragma once
#include <SDL.h>
#include <cstdint>
#include <vector>

struct Level {
    uint8_t fg[720];        // tiles de frente: (room-1)*30 + row*10 + col
    uint8_t bg[720];        // tiles de fondo
    uint8_t doorlinks1[256];
    uint8_t doorlinks2[256];
    uint8_t roomlinks[96];  // 24 rooms x (left,right,up,down)
    uint8_t used_rooms;
    uint8_t roomxs[24];
    uint8_t roomys[24];
    uint8_t fill1[15];
    uint8_t start_room;
    uint8_t start_pos;
    int8_t  start_dir;
    uint8_t fill2[4];
    uint8_t guards_tile[24];
    uint8_t guards_dir[24];
    uint8_t guards_x[24];
    uint8_t guards_seq_lo[24];
    uint8_t guards_skill[24];
    uint8_t guards_seq_hi[24];
    uint8_t guards_color[24];
    uint8_t fill3[18];
};

bool levelFromBytes(const std::vector<uint8_t>& d, Level& lv);

// geometria (misma del DOS): la superficie de la fila r es y_land[r+1]
inline const float y_land[5] = { -8, 55, 118, 181, 244 };
inline int  tileColFromPx(int px)     { return px / 32; }
inline int  tileRowFromFootY(float y) { int r = (int)((y + 8) / 63.0f) - 1; if (r<0) r=0; if(r>2) r=2; return r; }
inline float tileTopOfRow(int row)    { return 63.0f * row - 8; }   // y_land[row]
inline float tileSurface(int row)     { return 63.0f * (row + 1) - 8; } // y_land[row+1]

inline uint8_t tileTypeAt(const Level& lv, int room, int col, int row) {
    if (room < 1 || room > 24) return 0x0;
    if (col < 0) col = 0;
    if (col > 9) col = 9;
    if (row < 0) row = 0;
    if (row > 2) row = 2;
    return lv.fg[(room - 1) * 30 + row * 10 + col] & 0x1F;
}

inline bool tileIsSolid(uint8_t t) {
    // solo bloquean: wall(0x14), pillar(0x03), gate(0x04), bigpillar(0x09), etc.
    return t == 0x03 || t == 0x04 || t == 0x09 || t == 0x14 || t == 0x1D ||
           t == 0x1E || t == 0x1F || t == 0x20;
}

inline bool tileIsFloor(uint8_t t) {
    // pisable: floor(1), floor_decor(7), loose(0x0B), ... (fase 1: floor y spare)
    return t == 0x01 || t == 0x07 || t == 0x0B;
}

uint32_t tileColor(SDL_Surface* s, uint8_t t);
uint32_t tintBg(SDL_Surface* s, uint32_t c);
void drawRoom(SDL_Surface* gbuf, const Level& lv, int room);

// debug colisiones (tipo 'colliders_box' del original): AABB del player + tiles solidos
void drawColliders(SDL_Surface* gbuf, const Level& lv, int room, int px, int topY, int w, int h);