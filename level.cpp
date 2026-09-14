// level.cpp - nivel: parser + colores + dibujado del room
#include "level.h"
#include "text.h"
#include <cstring>

bool levelFromBytes(const std::vector<uint8_t>& d, Level& lv) {
    if (d.size() < sizeof(Level)) return false;
    memcpy(&lv, d.data(), sizeof(Level));
    return true;
}

uint32_t tileColor(SDL_Surface* s, uint8_t t) {
    switch (t) {
        case 0x01: return SDL_MapRGBA(s->format, 90, 90, 110, 255);   // floor
        case 0x02: return SDL_MapRGBA(s->format, 220, 220, 220, 255); // spike
        case 0x03: return SDL_MapRGBA(s->format, 130, 90, 60, 255);   // pillar
        case 0x04: return SDL_MapRGBA(s->format, 150, 150, 60, 255);  // gate
        case 0x07: return SDL_MapRGBA(s->format, 60, 110, 160, 255);  // tapestry
        case 0x09: return SDL_MapRGBA(s->format, 120, 70, 40, 255);   // bigpillar
        case 0x0B: return SDL_MapRGBA(s->format, 150, 130, 100, 255); // loose
        case 0x0C: return SDL_MapRGBA(s->format, 50, 90, 140, 255);   // doortop
        case 0x0D: return SDL_MapRGBA(s->format, 90, 160, 170, 255);  // mirror
        case 0x10: return SDL_MapRGBA(s->format, 120, 40, 120, 255);  // potion
        case 0x12: return SDL_MapRGBA(s->format, 170, 40, 40, 255);   // doortop chomper
        case 0x13: return SDL_MapRGBA(s->format, 60, 40, 40, 255);    // torch/skel
        case 0x14: return SDL_MapRGBA(s->format, 190, 60, 40, 255);   // wall
        case 0x17: return SDL_MapRGBA(s->format, 210, 200, 60, 255);  // balcony
        case 0x18: return SDL_MapRGBA(s->format, 150, 150, 150, 255); // debris
        case 0x19: return SDL_MapRGBA(s->format, 120, 150, 90, 255);  // chomper
        case 0x1C: return SDL_MapRGBA(s->format, 160, 120, 200, 255); // lattice
        default:   return SDL_MapRGBA(s->format, 80, 80, 90, 255);
    }
}

uint32_t tintBg(SDL_Surface* s, uint32_t c) {
    Uint8 r, g, b, a;
    SDL_GetRGBA(c, s->format, &r, &g, &b, &a);
    return SDL_MapRGBA(s->format, r / 2 + 40, g / 2 + 40, b / 2 + 40, 255);
}

void drawRoom(SDL_Surface* gbuf, const Level& lv, int room) {
    // fondo (bg) en tono apagado
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 10; col++) {
            uint8_t t = (lv.bg[(room - 1) * 30 + row * 10 + col] & 0x1F);
            if (t == 0x00) continue;
            SDL_Rect r = { col * 32, (int)tileTopOfRow(row), 32, 63 };
            SDL_FillRect(gbuf, &r, tintBg(gbuf, tileColor(gbuf, t)));
        }
    }
    // frente (fg): tiles de colision
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 10; col++) {
            uint8_t t = tileTypeAt(lv, room, col, row);
            if (t == 0x00) continue;
            SDL_Rect r = { col * 32, (int)tileTopOfRow(row), 32, 63 };
            SDL_FillRect(gbuf, &r, tileColor(gbuf, t));
        }
    }
}

void drawColliders(SDL_Surface* gbuf, const Level& lv, int room, int px, int topY, int w, int h) {
    uint32_t yellow = SDL_MapRGBA(gbuf->format, 255, 255, 80, 255);
    uint32_t green  = SDL_MapRGBA(gbuf->format, 60, 255, 120, 255);

    // AABB del player
    SDL_Rect body = { px, topY, w, h };
    SDL_FillRect(gbuf, &body, SDL_MapRGBA(gbuf->format, 255, 255, 80, 60));  // relleno suave
    SDL_Rect b = { body.x - 1, body.y - 1, body.w + 2, body.h + 2 };
    for (int i = 0; i < 2; i++) { SDL_FillRect(gbuf, &b, yellow); b.x++; b.y++; b.w -= 2; b.h -= 2; }
    SDL_FillRect(gbuf, &b, SDL_MapRGBA(gbuf->format, 0, 0, 0, 255));

    // columnas de tiles que tocan el AABB
    int c0 = px / 32;
    int c1 = (px + w - 1) / 32;
    if (c0 < 0) c0 = 0;
    if (c1 > 9) c1 = 9;

    // linea de suelo / superficie de cada fila donde aterriza (y_land)
    for (int r = 0; r < 3; r++) {
        int y = (int)tileSurface(r);
        for (int x = 0; x < 320; x += 2) {
            int xx = x, yy = y;
            if (yy < 0 || yy >= 200) continue;
            setPixel(gbuf, xx, yy, green);
        }
    }
}