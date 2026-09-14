// tiles.h - entorno grafico: sprites del DAT de entorno + render fiel de la sala
#pragma once
#include <SDL.h>
#include <cstdint>
#include <vector>
#include "img.h"
#include "level.h"

// Chtabs cargados desde el DAT de entorno (VDUNGEON/VPALACE) y PRINCE.DAT (chtab 150)
struct TileGfx {
    std::vector<Image> env;    // env: index = id de tile 1-based -> recurso 200+id
    std::vector<Image> wall;   // paredes: id 1..17 -> recurso 360+id
    std::vector<Image> fire;   // chtab 150 (antorcha/botella/espada): id 1..23 -> recurso 150+id
    uint8_t envVga[16][3] = {};
    uint8_t wallVga[16][3] = {};
    bool ok = false;
    bool hasFire = false;
};

// Carga el chtab de tiles desde un DAT de entorno (p.ej. "prince/VDUNGEON.DAT")
TileGfx loadEnvGfx(const char* datPath, const char* princePath);

// levelClass por numero de nivel (tabla tbl_level_type de SDLPoP; niveles 1..16)
inline int levelClassForLevel(int n) {
    static const int tbl[16] = {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0};
    if (n < 1) n = 1;
    if (n > 16) n = 16;
    return tbl[n - 1];
}

// Convierte bg[720] en modificadores de tile (port de load_alter_mod / alter_mods_allrm)
void applyAlterMods(Level& lv);

// Dibuja la sala 'room' de nivel con los sprites reales (port de draw_room/draw_tile).
// levelClass: 0=dungeon, 1=palace. Devuelve false si g no esta cargado.
bool drawRoomTiles(SDL_Surface* buf, const TileGfx& g, const Level& lv, int room, int levelClass);