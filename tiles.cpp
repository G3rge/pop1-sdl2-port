// tiles.cpp - render fiel de las salas usando los sprites reales del DAT de entorno.
// Port de draw_room/draw_tile/load_alter_mod/wall_pattern de SDLPoP (seg008.c) adaptado a SDL2 directo.
#include "tiles.h"
#include "dat.h"
#include "text.h"
#include <cstring>
#include <algorithm>

using std::vector;

// ---------------- constantes de tiles (seg008.c) ----------------
enum {
    TT_EMPTY = 0x00, TT_FLOOR = 0x01, TT_SPIKE = 0x02, TT_PILLAR = 0x03, TT_GATE = 0x04,
    TT_STUCK = 0x05, TT_CLOSER = 0x06, TT_DOORTOP_FLOOR = 0x07, TT_BIGPILLAR_B = 0x08,
    TT_BIGPILLAR_T = 0x09, TT_POTION = 0x0A, TT_LOOSE = 0x0B, TT_DOORTOP = 0x0C,
    TT_MIRROR = 0x0D, TT_DEBRIS = 0x0E, TT_OPENER = 0x0F, TT_LEVELDOOR_L = 0x10,
    TT_LEVELDOOR_R = 0x11, TT_CHOMPER = 0x12, TT_TORCH = 0x13, TT_WALL = 0x14,
    TT_SKELETON = 0x15, TT_SWORD = 0x16, TT_BALCONY_L = 0x17, TT_BALCONY_R = 0x18,
    TT_LATT_PILLAR = 0x19, TT_LATT_DOWN = 0x1A, TT_LATT_SMALL = 0x1B, TT_LATT_LEFT = 0x1C,
    TT_LATT_RIGHT = 0x1D, TT_TORCH_DEBRIS = 0x1E
};

// tile_table[31][12]: base_id, floor_left, base_y, right_id, floor_right, right_y,
//                     stripe_id, topright_id, bottom_id, fore_id, fore_x, fore_y
static const int TBL[31][12] = {
    { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}, // 0x00 empty
    {41,  1,  0, 42,  1,  2,145,  0, 43,  0,  0,  0}, // 0x01 floor
    {127, 1,  0,133,  1,  2,145,  0, 43,  0,  0,  0}, // 0x02 spike
    {92,  1,  0, 93,  1,  2,  0, 94, 43, 95,  1,  0}, // 0x03 pillar
    {46,  1,  0, 47,  1,  2,  0, 48, 43, 49,  3,  0}, // 0x04 gate
    {41,  1,  1, 35,  1,  3,145,  0, 36,  0,  0,  0}, // 0x05 stuck floor
    {41,  1,  0, 42,  1,  2,145,  0, 96,  0,  0,  0}, // 0x06 close button
    {46,  1,  0,  0,  0,  2,  0,  0, 43, 49,  3,  0}, // 0x07 door top with floor
    {86,  1,  0, 87,  1,  2,  0,  0, 43, 88,  1,  0}, // 0x08 big pillar bottom
    { 0,  0,  0, 89,  0,  3,  0, 90,  0, 91,  1,  3}, // 0x09 big pillar top
    {41,  1,  0, 42,  1,  2,145,  0, 43, 12,  2, -3}, // 0x0A potion
    { 0,  1,  0,  0,  0,  0,145,  0,  0,  0,  0,  0}, // 0x0B loose floor
    { 0,  0,  0,  0,  0,  2,  0,  0, 85, 49,  3,  0}, // 0x0C door top
    {75,  1,  0, 42,  1,  2,  0,  0, 43, 77,  0,  0}, // 0x0D mirror
    {97,  1,  0, 98,  1,  2,145,  0, 43,100,  0,  0}, // 0x0E debris
    {147, 1,  0, 42,  1,  1,145,  0,149,  0,  0,  0}, // 0x0F open button
    {41,  1,  0, 37,  0,  0,  0, 38, 43,  0,  0,  0}, // 0x10 leveldoor left
    { 0,  0,  0, 39,  1,  2,  0, 40, 43,  0,  0,  0}, // 0x11 leveldoor right
    { 0,  0,  0, 42,  1,  2,145,  0, 43,  0,  0,  0}, // 0x12 chomper
    {41,  1,  0, 42,  1,  2,  0,  0, 43,  0,  0,  0}, // 0x13 torch
    { 0,  0,  0,  1,  1,  2,  0,  2,  0,  0,  0,  0}, // 0x14 wall
    {30,  1,  0, 31,  1,  2,  0,  0, 43,  0,  0,  0}, // 0x15 skeleton
    {41,  1,  0, 42,  1,  2,145,  0, 43,  0,  0,  0}, // 0x16 sword
    {41,  1,  0, 10,  0,  0,  0, 11, 43,  0,  0,  0}, // 0x17 balcony left
    { 0,  0,  0, 12,  1,  2,  0, 13, 43,  0,  0,  0}, // 0x18 balcony right
    {92,  1,  0, 42,  1,  2,145,  0, 43, 95,  1,  0}, // 0x19 lattice pillar
    { 1,  0,  0,  0,  0,  0,  0,  0,  2,  9,  0,-53}, // 0x1A lattice down
    { 3,  0,-10,  0,  0,  0,  0,  0,  0,  9,  0,-53}, // 0x1B lattice small
    { 4,  0,-10,  0,  0,  0,  0,  0,  0,  9,  0,-53}, // 0x1C lattice left
    { 5,  0,-10,  0,  0,  0,  0,  0,  0,  9,  0,-53}, // 0x1D lattice right
    {97,  1,  0, 98,  1,  2,  0,  0, 43,100,  0,  0}, // 0x1E debris with torch
};

static const int col_xh[10] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36};
static const int tbl_line[3] = {0, 10, 20};

// tablas auxiliares de sprites (seg008.c)
static const int wall_fram_bottom[4] = {7, 9, 5, 3};
static const int wall_fram_main[4] = {8, 10, 6, 4};
static const int loose_fram_bottom[12] = {43, 73, 43, 74, 74, 43, 43, 43, 74, 74, 74, 0};
static const int loose_fram_left[12] = {41, 69, 41, 70, 70, 41, 41, 41, 70, 70, 70, 0};
static const int loose_fram_right[12] = {42, 71, 42, 72, 72, 42, 42, 42, 72, 72, 72, 0};
static const int spikes_fram_left[10] = {0, 128, 129, 130, 131, 132, 131, 129, 128, 0};
static const int spikes_fram_right[10] = {0, 134, 135, 136, 137, 138, 137, 135, 134, 0};
static const int spikes_fram_fore[10] = {0, 139, 140, 141, 142, 143, 142, 140, 139, 0};
static const int potion_fram_bubb[8] = {0, 16, 17, 18, 19, 20, 21, 22};
static const int chomper_fram1[7] = {3, 2, 0, 1, 4, 3, 3};
static const int chomper_fram_bot[5] = {101, 102, 103, 104, 105};
static const int chomper_fram_top[5] = {0, 0, 111, 112, 113};
static const int chomper_fram_y[5] = {0, 0, 0x25, 0x2F, 0x32};
static const int chomper_fram_for[5] = {106, 107, 108, 109, 110};
static const int doortop_fram_top[4] = {0, 81, 83, 0};
static const int doortop_fram_bot[4] = {78, 80, 82, 0};
static const int door_fram_top[8] = {60, 61, 62, 63, 64, 65, 66, 67};
static const int door_fram_slice[9] = {67, 59, 58, 57, 56, 55, 54, 53, 52};
static const int blueline_fram1[4] = {0, 124, 125, 126};
static const int blueline_fram_y[4] = {0, -20, -20, 0};
static const int blueline_fram3[4] = {44, 44, 45, 45};

// modos de blit
enum { BL_NONE = 0, BL_OR = 2, BL_BLACK = 9, BL_TRANS = 0x10, BL_MONO = 0x40 };

static inline bool isFloorTile(int t) {
    return t == TT_FLOOR || t == TT_DOORTOP_FLOOR;
}

// ---------------- estado del renderer ----------------
struct RoomCtx {
    const TileGfx* g;
    const Level* lv;
    bool palace;

    int drawn_room, room_L, room_R, room_A, room_B, room_AL, room_AR, room_BL, room_BR;
    int drawn_row, drawn_col, draw_bottom_y, draw_main_y, draw_xh;
    int curr_tile, curr_modifier, tile_left, modifier_left;

    struct LR { int tiletype; int modifier; };
    LR leftroom[3];
    LR row_below[10];

    struct Op { const Image* im; int x, y, mode, color; };
    vector<Op> back, mid, fore;
    struct Wip { int x, y, w, h; int color; int layer; };
    vector<Wip> wipes;
    unsigned monoPal[16];
    uint32_t rng;
};

static uint32_t prandom(RoomCtx& c, uint32_t max) {
    c.rng = c.rng * 214013u + 2531011u;
    return (c.rng >> 16) % (max + 1);
}

static uint8_t tRaw(const Level& lv, int room, int col, int row) {
    return lv.fg[(room - 1) * 30 + row * 10 + col];
}
static uint8_t mRaw(const Level& lv, int room, int col, int row) {
    return lv.bg[(room - 1) * 30 + row * 10 + col];
}
static const uint8_t* roomLinks(const Level& lv, int room) {
    return &lv.roomlinks[(room - 1) * 4];
}

// sg get_tile_to_draw
static void getTile(RoomCtx& c, int room, int column, int row, int& outTile, int& outMod, int tileRoom0) {
    if (column == -1) {
        outTile = c.leftroom[row].tiletype;
        outMod = c.leftroom[row].modifier;
    } else if (room) {
        outTile = tRaw(*c.lv, room, column, row) & 0x1F;
        outMod = mRaw(*c.lv, room, column, row);
    } else {
        outMod = 0;
        outTile = tileRoom0;
    }
}

static void loadRoomLinks(RoomCtx& c, int room) {
    c.room_BR = c.room_BL = c.room_AR = c.room_AL = 0;
    if (room) {
        const uint8_t* rl = roomLinks(*c.lv, room);
        c.room_L = rl[0]; c.room_R = rl[1]; c.room_A = rl[2]; c.room_B = rl[3];
        if (c.room_A) {
            const uint8_t* ra = roomLinks(*c.lv, c.room_A);
            c.room_AL = ra[0]; c.room_AR = ra[1];
        } else {
            if (c.room_L) c.room_AL = roomLinks(*c.lv, c.room_L)[2];
            if (c.room_R) c.room_AR = roomLinks(*c.lv, c.room_R)[2];
        }
        if (c.room_B) {
            const uint8_t* rb = roomLinks(*c.lv, c.room_B);
            c.room_BL = rb[0]; c.room_BR = rb[1];
        } else {
            if (c.room_L) c.room_BL = roomLinks(*c.lv, c.room_L)[3];
            if (c.room_R) c.room_BR = roomLinks(*c.lv, c.room_R)[3];
        }
    } else {
        c.room_B = c.room_A = c.room_R = c.room_L = 0;
    }
}

static void loadLeftroom(RoomCtx& c) {
    for (int row = 0; row < 3; ++row)
        getTile(c, c.room_L, 9, row, c.leftroom[row].tiletype, c.leftroom[row].modifier, TT_WALL);
}

static void loadRowbelow(RoomCtx& c) {
    int room, room_left, row_below;
    if (c.drawn_row == 2) { room = c.room_B; room_left = c.room_BL; row_below = 0; }
    else { room = c.drawn_room; room_left = c.room_L; row_below = c.drawn_row + 1; }
    for (int column = 1; column < 10; ++column)
        getTile(c, room, column - 1, row_below, c.row_below[column].tiletype, c.row_below[column].modifier, TT_EMPTY);
    getTile(c, room_left, 9, row_below, c.row_below[0].tiletype, c.row_below[0].modifier, TT_WALL);
}

static void loadCurrAndLeft(RoomCtx& c) {
    int tile0 = c.drawn_row == 2 ? TT_FLOOR : TT_WALL;
    getTile(c, c.drawn_room, c.drawn_col, c.drawn_row, c.curr_tile, c.curr_modifier, tile0);
    getTile(c, c.drawn_room, c.drawn_col - 1, c.drawn_row, c.tile_left, c.modifier_left, tile0);
    c.draw_xh = col_xh[c.drawn_col];
}

// ---------------- blits a las colas ----------------
static const Image* pickImg(const RoomCtx& c, int chtab, int id) {
    const vector<Image>* v = nullptr;
    if (chtab == 6) v = &c.g->env;
    else if (chtab == 7) v = &c.g->wall;
    else if (chtab == 1) v = &c.g->fire;
    if (!v) return nullptr;
    if (id < 1 || id >= (int)v->size()) return nullptr;
    const Image& im = (*v)[id];
    return im.ok ? &im : nullptr;
}

// devuelve true si se anadio el sprite (que == 0 back, 1 mid, 2 fore)
static bool addOp(RoomCtx& c, int which, int chtab, int id, int xh, int xl, int ybottom, int mode, int color) {
    const Image* im = pickImg(c, chtab, id);
    if (!im) return false;
    RoomCtx::Op op;
    op.im = im;
    op.x = xh * 8 + xl;
    op.y = ybottom - im->h + 1;
    op.mode = mode;
    op.color = color;
    if (which == 0) c.back.push_back(op);
    else if (which == 1) c.mid.push_back(op);
    else c.fore.push_back(op);
    return true;
}

static void addWipe(RoomCtx& c, int layer, int left, int bottom, int height, int width, int color) {
    if ((int)c.wipes.size() > 300) return;
    RoomCtx::Wip w;
    w.x = left;
    w.y = (bottom + 1) - height;
    w.h = height;
    w.w = width;
    w.color = color;
    w.layer = layer;
    c.wipes.push_back(w);
}

static int getSpikeFrame(int modifier) {
    return (modifier & 0x80) ? 5 : modifier;
}
static int getLooseFrame(int modifier) {
    if (modifier & 0x80) { // loose_floor_delay por defecto = 11 (no > 11)
        modifier &= 0x7F;
        if (modifier > 10) return 1;
    }
    return modifier;
}

// ---------------- wall_pattern ----------------
static void drawLeftMark(RoomCtx& c, int q, int variant, int arg2, int arg1) {
    static const int LPOS[5] = {58, 41, 37, 20, 16};
    int imageId = (variant % 2) ? 15 : 14;
    int lv2 = 0;
    if (variant > 3) lv2 = arg1 + 6;
    else if (variant > 1) lv2 = arg2 + 6;
    addOp(c, q, 7, imageId, c.draw_xh + (variant == 2 || variant == 3), lv2,
          c.draw_bottom_y - LPOS[variant & 0x0F], BL_TRANS, 0);
}

static void drawRightMark(RoomCtx& c, int q, int variant, int arg1) {
    static const int RPOS[4] = {52, 42, 31, 21};
    int imageId = (variant % 2) ? 17 : 16;
    if (variant < 2) arg1 = 24;
    else arg1 -= 3;
    addOp(c, q, 7, imageId, c.draw_xh + (variant > 1), arg1,
          c.draw_bottom_y - RPOS[variant & 0x03], BL_TRANS, 0);
}

static void wallPattern(RoomCtx& c, int whichPart, int whichTable) {
    int row = c.drawn_row;
    int col = c.drawn_col;
    int q = (whichTable == 0) ? 0 : 2;
    bool isDungeon = !c.palace;
    c.rng = (uint32_t)(c.drawn_room + tbl_line[row] + col);
    prandom(c, 1); // descartar

    if (!isDungeon) {
        // palacio: rects de ladrillo + decals mono 6
        static int walls[3][44];
        {
            uint32_t saved = c.rng;
            c.rng = (uint32_t)c.drawn_room;
            prandom(c, 1);
            for (int r = 0; r < 3; r++)
                for (int sr = 0; sr < 4; sr++) {
                    int base = (sr % 2) ? 0x61 : 0x66;
                    int prev = -1;
                    for (int col2 = 0; col2 <= 10; col2++) {
                        int color;
                        do { color = base + (int)prandom(c, 3); } while (color == prev);
                        walls[r][11 * sr + col2] = color;
                        prev = color;
                    }
                }
            c.rng = saved;
        }
        const int* pw = walls[row];
        auto brick = [&](int i) { return (pw[i] & 0x0F); };
        if (whichPart) {
            addWipe(c, whichTable, 8 * c.draw_xh, c.draw_main_y - 40, 20, 32, brick(col));
            addWipe(c, whichTable, 8 * c.draw_xh, c.draw_main_y - 19, 21, 16, brick(11 + col));
            addWipe(c, whichTable, 8 * (c.draw_xh + 2), c.draw_main_y - 19, 21, 16, brick(12 + col));
            addWipe(c, whichTable, 8 * c.draw_xh, c.draw_main_y, 19, 8, brick(22 + col));
            addWipe(c, whichTable, 8 * (c.draw_xh + 1), c.draw_main_y, 19, 24, brick(23 + col));
            addOp(c, q, 7, (int)prandom(c, 2) + 3, c.draw_xh + 3, 0, c.draw_main_y - 53, BL_MONO, 6);
            addOp(c, q, 7, (int)prandom(c, 2) + 6, c.draw_xh, 0, c.draw_main_y - 34, BL_MONO, 6);
            addOp(c, q, 7, (int)prandom(c, 2) + 9, c.draw_xh, 0, c.draw_main_y - 13, BL_MONO, 6);
            addOp(c, q, 7, (int)prandom(c, 2) + 12, c.draw_xh, 0, c.draw_main_y, BL_MONO, 6);
        }
        addWipe(c, whichTable, 8 * c.draw_xh, c.draw_bottom_y, 3, 32, brick(33 + col));
        addOp(c, q, 7, (int)prandom(c, 2) + 15, c.draw_xh, 0, c.draw_bottom_y, BL_MONO, 6);
    } else {
        // dungeon: divisores/marcas con mono 6
        int middle_divider = (int)prandom(c, 1);
        int middle_divider_offset = (int)prandom(c, 4);
        int bottom_divider = (int)prandom(c, 1);
        int bottom_divider_offset = (int)prandom(c, 4);
        int bg = c.curr_modifier & 0x7F;
        if (bg == 3) { // WWW
            if (whichPart) {
                if ((int)prandom(c, 4) == 0)
                    addOp(c, q, 7, 13, c.draw_xh, 0, c.draw_bottom_y - 42, BL_NONE, 0);
                addOp(c, q, 7, 11 + middle_divider, c.draw_xh + 1, middle_divider_offset,
                      c.draw_bottom_y - 21, BL_TRANS, 0);
                if (isDungeon) {
                    if ((int)prandom(c, 4) == 0)
                        drawRightMark(c, q, (int)prandom(c, 3), middle_divider_offset);
                    if ((int)prandom(c, 4) == 0)
                        drawLeftMark(c, q, (int)prandom(c, 4), middle_divider_offset - middle_divider,
                                     bottom_divider_offset - bottom_divider);
                }
            }
            addOp(c, q, 7, 11 + bottom_divider, c.draw_xh, bottom_divider_offset,
                  c.draw_bottom_y, BL_TRANS, 0);
        } else if (bg == 0) { // SWS
            if (whichPart) {
                if (isDungeon && (int)prandom(c, 6) == 0)
                    drawLeftMark(c, q, (int)prandom(c, 1), middle_divider_offset - middle_divider,
                                 bottom_divider_offset - bottom_divider);
            }
        } else if (bg == 1) { // SWW
            if (whichPart) {
                if ((int)prandom(c, 4) == 0)
                    addOp(c, q, 7, 13, c.draw_xh, 0, c.draw_bottom_y - 42, BL_NONE, 0);
                addOp(c, q, 7, 11 + middle_divider, c.draw_xh + 1, middle_divider_offset,
                      c.draw_bottom_y - 21, BL_TRANS, 0);
                if (isDungeon) {
                    if ((int)prandom(c, 4) == 0)
                        drawRightMark(c, q, (int)prandom(c, 3), middle_divider_offset);
                    if ((int)prandom(c, 4) == 0)
                        drawLeftMark(c, q, (int)prandom(c, 3), middle_divider_offset - middle_divider,
                                     bottom_divider_offset - bottom_divider);
                }
            }
        } else { // WWS
            if (whichPart)
                addOp(c, q, 7, 11 + middle_divider, c.draw_xh + 1, middle_divider_offset,
                      c.draw_bottom_y - 21, BL_TRANS, 0);
            addOp(c, q, 7, 11 + bottom_divider, c.draw_xh, bottom_divider_offset,
                  c.draw_bottom_y, BL_TRANS, 0);
            if (whichPart) {
                if (isDungeon) {
                    if ((int)prandom(c, 4) == 0)
                        drawRightMark(c, q, (int)prandom(c, 1) + 2, middle_divider_offset);
                    if ((int)prandom(c, 4) == 0)
                        drawLeftMark(c, q, (int)prandom(c, 4), middle_divider_offset - middle_divider,
                                     bottom_divider_offset - bottom_divider);
                }
            }
        }
    }
}

// ---------------- piezas de tiles ----------------
static void drawTileTopright(RoomCtx& c);
static void drawTileBottomT(RoomCtx& c, int extraF);
static void drawLooseT(RoomCtx& c);
static void drawTileBaseT(RoomCtx& c);

static void drawTileTopright(RoomCtx& c) {
    int tt = c.row_below[c.drawn_col].tiletype & 0x1F;
    if (tt == TT_DOORTOP_FLOOR || tt == TT_DOORTOP) {
        if (!c.palace) return;
        addOp(c, 0, 6, doortop_fram_top[c.row_below[c.drawn_col].modifier & 3], c.draw_xh, 0,
              c.draw_bottom_y, BL_OR, 0);
    } else if (tt == TT_WALL) {
        addOp(c, 0, 7, 2, c.draw_xh, 0, c.draw_bottom_y, BL_OR, 0);
    } else {
        int id = TBL[tt][7]; // topright_id
        if (id) addOp(c, 0, 6, id, c.draw_xh, 0, c.draw_bottom_y, BL_OR, 0);
    }
}

static void drawTileFloorright(RoomCtx& c) {
    int ct = c.curr_tile & 0x1F;
    if (!(ct == TT_EMPTY || ct == TT_BIGPILLAR_T || ct == TT_DOORTOP || ct == TT_LATT_DOWN)) return;
    drawTileTopright(c);
    int tl = c.tile_left & 0x1F;
    if (!TBL[tl][4]) return; // floor_right
    addOp(c, 0, 6, 42, c.draw_xh, 0, TBL[TT_FLOOR][5] + c.draw_main_y, BL_BLACK, 0);
}

static void drawTileAnimTopright(RoomCtx& c) {
    int ct = c.curr_tile & 0x1F;
    if ((ct == TT_EMPTY || ct == TT_BIGPILLAR_T || ct == TT_DOORTOP)
        && (c.row_below[c.drawn_col].tiletype & 0x1F) == TT_GATE) {
        addOp(c, 0, 6, 68, c.draw_xh, 0, c.draw_bottom_y, BL_MONO, 0);
        int m = c.row_below[c.drawn_col].modifier;
        if (m > 188) m = 188;
        addOp(c, 0, 6, door_fram_top[(m >> 2) % 8], c.draw_xh, 0, c.draw_bottom_y, BL_OR, 0);
    }
}

static void drawTileRight(RoomCtx& c) {
    if ((c.curr_tile & 0x1F) == TT_WALL) return;
    int tl = c.tile_left & 0x1F;
    switch (tl) {
        default:
            if (TBL[tl][3]) { // right_id
                int id = TBL[tl][3];
                int blit = BL_OR;
                if (tl == TT_STUCK) {
                    blit = BL_TRANS;
                    int ct = c.curr_tile & 0x1F;
                    if (ct == TT_EMPTY || ct == TT_STUCK || !isFloorTile(ct)) id = 42;
                }
                addOp(c, 0, 6, id, c.draw_xh, 0, TBL[tl][5] + c.draw_main_y, blit, 0);
            }
            if (c.palace)
                addOp(c, 0, 6, TBL[tl][6], c.draw_xh, 0, c.draw_main_y - 27, BL_OR, 0);
            if (tl == TT_TORCH || tl == TT_TORCH_DEBRIS)
                addOp(c, 0, 6, 146, c.draw_xh, 0, c.draw_bottom_y - 28, BL_NONE, 0);
            break;
        case TT_EMPTY:
            if (c.modifier_left > 3) return;
            addOp(c, 0, 6, blueline_fram1[c.modifier_left], c.draw_xh, 0,
                  blueline_fram_y[c.modifier_left] + c.draw_main_y, BL_OR, 0);
            break;
        case TT_FLOOR:
            addOp(c, 0, 6, 42, c.draw_xh, 0, TBL[TT_FLOOR][5] + c.draw_main_y, BL_TRANS, 0);
            {
                int num = c.modifier_left;
                if (num > 3) num = 0;
                if (num == (c.palace ? 1 : 0)) return;
                addOp(c, 0, 6, blueline_fram3[num], c.draw_xh, 0, c.draw_main_y - 20, BL_NONE, 0);
            }
            break;
        case TT_DOORTOP_FLOOR:
        case TT_DOORTOP:
            if (!c.palace) return;
            addOp(c, 0, 6, doortop_fram_bot[c.modifier_left & 3], c.draw_xh, 0,
                  TBL[tl][5] + c.draw_main_y, BL_OR, 0);
            break;
        case TT_WALL:
            if (c.palace && (c.modifier_left & 0x80) == 0)
                addOp(c, 0, 6, 84, c.draw_xh + 3, 0, c.draw_main_y - 27, BL_NONE, 0);
            addOp(c, 0, 7, 1, c.draw_xh, 0, TBL[tl][5] + c.draw_main_y, BL_OR, 0);
            break;
    }
}

static void drawGateBack(RoomCtx& c) {
    int gateTopY = c.draw_bottom_y - 62;
    int gateOpenness = std::min(c.modifier_left, 188) >> 2;
    gateOpenness += 1;
    int gateBottomY = c.draw_main_y - gateOpenness;
    if (gateBottomY + 12 < c.draw_main_y) {
        addOp(c, 0, 6, 50, c.draw_xh, 0, gateBottomY, BL_NONE, 0);
    } else {
        addOp(c, 0, 6, TBL[TT_GATE][3], c.draw_xh, 0, TBL[TT_GATE][5] + c.draw_main_y, BL_NONE, 0);
        drawTileTopright(c);
        drawTileBottomT(c, 0);
        drawLooseT(c);
        drawTileBaseT(c);
        addOp(c, 0, 6, 51, c.draw_xh, 0, gateBottomY - 2, BL_TRANS, 0);
    }
    int ybottom = gateBottomY - 12;
    if (ybottom < 192) {
        for (; ybottom >= 0 && ybottom > 7 && ybottom - 7 > gateTopY; ybottom -= 8)
            addOp(c, 0, 6, 52, c.draw_xh, 0, ybottom, BL_NONE, 0);
    }
    int gateFrame = ybottom - gateTopY + 1;
    if (gateFrame > 0 && gateFrame < 9)
        addOp(c, 0, 6, door_fram_slice[gateFrame], c.draw_xh, 0, ybottom, BL_NONE, 0);
}

static void drawGateFore(RoomCtx& c) {
    int gateTopY = c.draw_bottom_y - 62;
    int gateOpenness = std::min(c.modifier_left, 188) >> 2;
    gateOpenness += 1;
    int gateBottomY = c.draw_main_y - gateOpenness;
    addOp(c, 2, 6, 51, c.draw_xh, 0, gateBottomY - 2, BL_TRANS, 0);
    int ybottom = gateBottomY - 12;
    if (ybottom < 192) {
        for (; ybottom >= 0 && ybottom > 7 && ybottom - 7 > gateTopY; ybottom -= 8)
            addOp(c, 2, 6, 52, c.draw_xh, 0, ybottom, BL_TRANS, 0);
    }
}

static void drawLeveldoor(RoomCtx& c) {
    int ybottom = c.draw_main_y - 13;
    addOp(c, 0, 6, 99, c.draw_xh + 1, 0, ybottom, BL_NONE, 0);
    if (c.modifier_left) {
        if (c.lv->start_room != c.drawn_room) {
            addOp(c, 0, 6, 144, c.draw_xh + 1, 0, ybottom - 4, BL_NONE, 0);
        } else {
            int width = c.palace ? 48 : 39;
            int xLow = c.palace ? 0 : 2;
            addWipe(c, 0, 8 * (c.draw_xh + 1) + xLow, ybottom - 4, 45, width, 0);
        }
    }
    int leveldoorYbottom = ybottom - (c.modifier_left & 3) - 48;
    int y = ybottom - c.modifier_left;
    for (;;) {
        addOp(c, 0, 6, 33, c.draw_xh + 1, 0, leveldoorYbottom, BL_NONE, 0);
        if (y > leveldoorYbottom) leveldoorYbottom += 4;
        else break;
    }
    addOp(c, 0, 6, 34, c.draw_xh + 1, 0, c.draw_main_y - 64, BL_NONE, 0);
}

static void drawTileAnimRight(RoomCtx& c) {
    int tl = c.tile_left & 0x1F;
    switch (tl) {
        case TT_SPIKE:
            addOp(c, 0, 6, spikes_fram_right[getSpikeFrame(c.modifier_left)], c.draw_xh, 0,
                  c.draw_main_y - 7, BL_TRANS, 0);
            break;
        case TT_GATE:
            drawGateBack(c);
            break;
        case TT_LOOSE:
            addOp(c, 0, 6, loose_fram_right[getLooseFrame(c.modifier_left)], c.draw_xh, 0,
                  c.draw_bottom_y - 1, BL_OR, 0);
            break;
        case TT_LEVELDOOR_L:
            drawLeveldoor(c);
            break;
        case TT_TORCH:
        case TT_TORCH_DEBRIS:
            if (c.modifier_left < 9)
                addOp(c, 0, 1, c.modifier_left + 1, c.draw_xh + 1, 0, c.draw_main_y - 40, BL_NONE, 0);
            break;
    }
}

static void drawTileBottomT(RoomCtx& c, int extraF) {
    int id = 0;
    int blit = BL_NONE;
    int chtabId = 6;
    switch (c.curr_tile & 0x1F) {
        case TT_WALL:
            id = wall_fram_bottom[c.curr_modifier & 0x7F];
            chtabId = 7;
            break;
        case TT_DOORTOP:
            blit = BL_OR;
            // fallthrough
        default:
            id = TBL[c.curr_tile & 0x1F][8]; // bottom_id
            break;
    }
    if (addOp(c, 0, chtabId, id, c.draw_xh, 0, c.draw_bottom_y, blit, 0) && extraF)
        addOp(c, 2, chtabId, id, c.draw_xh, 0, c.draw_bottom_y, blit, 0);
    if (chtabId == 7)
        wallPattern(c, 0, 0);
}

static void drawLooseT(RoomCtx& c) {
    if ((c.curr_tile & 0x1F) == TT_LOOSE) {
        int id = loose_fram_bottom[getLooseFrame(c.curr_modifier)];
        addOp(c, 0, 6, id, c.draw_xh, 0, c.draw_bottom_y, BL_NONE, 0);
        addOp(c, 2, 6, id, c.draw_xh, 0, c.draw_bottom_y, BL_NONE, 0);
    }
}

static void drawTileBaseT(RoomCtx& c) {
    int ybottom = c.draw_main_y;
    int id;
    if (c.tile_left == TT_LATT_DOWN && c.curr_tile == TT_DOORTOP) {
        id = 6;
        ybottom += 3;
    } else if ((c.curr_tile & 0x1F) == TT_LOOSE) {
        id = loose_fram_left[getLooseFrame(c.curr_modifier)];
    } else if ((c.curr_tile & 0x1F) == TT_OPENER && c.tile_left == TT_EMPTY && !c.palace) {
        id = 148;
    } else {
        id = TBL[c.curr_tile & 0x1F][0];
    }
    addOp(c, 0, 6, id, c.draw_xh, 0, TBL[c.curr_tile & 0x1F][2] + ybottom, BL_TRANS, 0);
}

static void drawTileAnim(RoomCtx& c) {
    int potSize = 0;
    int color = 12;
    switch (c.curr_tile & 0x1F) {
        case TT_SPIKE:
            addOp(c, 0, 6, spikes_fram_left[getSpikeFrame(c.curr_modifier)], c.draw_xh, 0,
                  c.draw_main_y - 2, BL_TRANS, 0);
            break;
        case TT_POTION: {
            int ptype = (c.curr_modifier & 0xF8) >> 3;
            if (ptype == 0) return;
            if (ptype == 5 || ptype == 6) color = 9;
            else if (ptype == 3 || ptype == 4) color = 10;
            if (ptype == 2 || ptype == 3 || ptype == 4) potSize = 1;
            addOp(c, 0, 1, 23, c.draw_xh + 3, 1, c.draw_main_y - (potSize << 2) - 14, BL_MONO, 0);
            addOp(c, 2, 1, potion_fram_bubb[c.curr_modifier & 7], c.draw_xh + 3, 1,
                  c.draw_main_y - (potSize << 2) - 14, BL_MONO, color);
            break;
        }
        case TT_SWORD:
            addOp(c, 1, 1, (c.curr_modifier == 1) + 10, c.draw_xh, 0, c.draw_main_y - 3, BL_TRANS, 0);
            break;
        case TT_CHOMPER: {
            int cn = chomper_fram1[std::min(c.curr_modifier & 0x7F, 6)];
            addOp(c, 0, 6, chomper_fram_bot[cn], c.draw_xh, 0, c.draw_main_y, BL_TRANS, 0);
            if (c.curr_modifier & 0x80)
                addOp(c, 0, 6, cn + 114, c.draw_xh + 1, 4, c.draw_main_y - 6, BL_MONO, 12);
            addOp(c, 0, 6, chomper_fram_top[cn], c.draw_xh, 0,
                  c.draw_main_y - chomper_fram_y[cn], BL_TRANS, 0);
            break;
        }
    }
}

static void drawTileFore(RoomCtx& c) {
    if ((c.tile_left & 0x1F) == TT_GATE) {
        drawGateFore(c);
    }
    switch (c.curr_tile & 0x1F) {
        case TT_SPIKE:
            addOp(c, 2, 6, spikes_fram_fore[getSpikeFrame(c.curr_modifier)], c.draw_xh, 0,
                  c.draw_main_y - 2, BL_TRANS, 0);
            break;
        case TT_CHOMPER: {
            int cn = chomper_fram1[std::min(c.curr_modifier & 0x7F, 6)];
            addOp(c, 2, 6, chomper_fram_for[cn], c.draw_xh, 0, c.draw_main_y, BL_TRANS, 0);
            if (c.curr_modifier & 0x80)
                addOp(c, 2, 6, cn + 119, c.draw_xh + 1, 4, c.draw_main_y - 6, BL_MONO, 12);
            break;
        }
        case TT_WALL:
            if (!c.palace)
                addOp(c, 2, 7, wall_fram_main[c.curr_modifier & 0x7F], c.draw_xh, 0,
                      c.draw_main_y, BL_NONE, 0);
            wallPattern(c, 1, 1);
            break;
        default: {
            int id = TBL[c.curr_tile & 0x1F][9]; // fore_id
            if (id == 0) return;
            if ((c.curr_tile & 0x1F) == TT_POTION) {
                int ptype = (c.curr_modifier & 0xF8) >> 3;
                if (ptype < 5 && ptype >= 2) id = 13;
            }
            int xh = TBL[c.curr_tile & 0x1F][10] + c.draw_xh;
            int ybottom = TBL[c.curr_tile & 0x1F][11] + c.draw_main_y;
            if ((c.curr_tile & 0x1F) == TT_POTION) {
                if (c.palace) id += 2;
                addOp(c, 2, 1, id, xh, 6, ybottom, BL_TRANS, 0);
            } else {
                int ct = c.curr_tile & 0x1F;
                int ntrans = (ct == TT_PILLAR && !c.palace)
                    || (ct >= TT_LATT_SMALL && ct < TT_TORCH_DEBRIS);
                addOp(c, 2, 6, id, xh, 0, ybottom, ntrans ? BL_NONE : BL_TRANS, 0);
            }
            break;
        }
    }
}

static void drawTileFull(RoomCtx& c) {
    drawTileFloorright(c);
    drawTileAnimTopright(c);
    drawTileRight(c);
    drawTileAnimRight(c);
    drawTileBottomT(c, 0);
    drawLooseT(c);
    drawTileBaseT(c);
    drawTileAnim(c);
    drawTileFore(c);
}

static void drawTileAboveroom(RoomCtx& c) {
    drawTileFloorright(c);
    drawTileAnimTopright(c);
    drawTileRight(c);
    drawTileBottomT(c, 0);
    drawLooseT(c);
    drawTileFore(c);
}

// ---------------- render del buffer ----------------
static unsigned rgbOf(const uint8_t v[3]) {
    return 0xFF000000u | ((unsigned)(v[0] << 2) << 16) | ((unsigned)(v[1] << 2) << 8) | (unsigned)(v[2] << 2);
}

static void fillRectCol(SDL_Surface* buf, int x, int y, int w, int h, unsigned rgb) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            setPixel(buf, i, j, rgb);
}

static void drawOps(SDL_Surface* buf, const RoomCtx& c, const vector<RoomCtx::Op>& ops) {
    const uint8_t(*pal)[3] = c.g->envVga;
    for (size_t k = 0; k < ops.size(); k++) {
        const RoomCtx::Op& o = ops[k];
        const Image& im = *o.im;
        int mode = o.mode;
        bool mono = (mode >= BL_MONO);
        for (int yy = 0; yy < im.h; yy++) {
            int dy = o.y + yy;
            if (dy < 0 || dy >= buf->h) continue;
            for (int xx = 0; xx < im.w; xx++) {
                int cidx = im.pixels[yy * im.w + xx];
                if (cidx <= 0) continue;
                unsigned rgb;
                if (mode == BL_BLACK) {
                    rgb = 0xFF000000u;
                } else if (mono) {
                    rgb = c.monoPal[o.color & 0x0F];
                } else {
                    rgb = rgbOf(pal[cidx]);
                    if (mode == BL_NONE && cidx == 0) rgb = 0xFF000000u;
                }
                setPixel(buf, o.x + xx, dy, rgb);
            }
        }
    }
}

// ---------------- aplicacion de modificadores ----------------
void applyAlterMods(Level& lv) {
    int usedRooms = lv.used_rooms;
    if (usedRooms > 24) usedRooms = 24;
    for (int room = 1; room <= usedRooms; room++) {
        int roomL = lv.roomlinks[(room - 1) * 4 + 0];
        int roomR = lv.roomlinks[(room - 1) * 4 + 1];
        for (int tilepos = 0; tilepos < 30; tilepos++) {
            uint8_t* mod = &lv.bg[(room - 1) * 30 + tilepos];
            int tiletype = lv.fg[(room - 1) * 30 + tilepos] & 0x1F;
            switch (tiletype) {
                case TT_GATE:
                    *mod = (*mod == 1) ? 188 : 0;
                    break;
                case TT_LOOSE:
                    *mod = 0;
                    break;
                case TT_POTION:
                    *mod <<= 3;
                    break;
                case TT_WALL: {
                    uint8_t stored = *mod;
                    if (stored == 1) *mod = 0x80;
                    else *mod = (stored << 4);
                    bool wallL = true, wallR = true;
                    int adjIdx;
                    if (tilepos % 10 == 0) {
                        if (roomL) { adjIdx = 30 * (roomL - 1) + tilepos + 9; wallL = (lv.fg[adjIdx] & 0x1F) == TT_WALL; }
                    } else {
                        adjIdx = tilepos - 1;
                        wallL = (lv.fg[(room - 1) * 30 + adjIdx] & 0x1F) == TT_WALL;
                    }
                    if (tilepos % 10 == 9) {
                        if (roomR) { adjIdx = 30 * (roomR - 1) + tilepos - 9; wallR = (lv.fg[adjIdx] & 0x1F) == TT_WALL; }
                    } else {
                        adjIdx = tilepos + 1;
                        wallR = (lv.fg[(room - 1) * 30 + adjIdx] & 0x1F) == TT_WALL;
                    }
                    if (wallL && wallR) *mod |= 3;
                    else if (wallL) *mod |= 2;
                    else if (wallR) *mod |= 1;
                    break;
                }
            }
        }
    }
}

// ---------------- carga de chtabs ----------------
static void loadRange(const vector<uint8_t>& d, const vector<DatEntry>& es, int base, int count,
                      vector<Image>& out) {
    out.assign(count + 1, Image());
    for (int id = 1; id <= count; id++) {
        vector<uint8_t> res;
        if (findRes(d, es, (uint16_t)(base + id), res)) {
            Image im = decodeImage(res);
            if (im.ok) out[id] = std::move(im);
        }
    }
}

TileGfx loadEnvGfx(const char* datPath, const char* princePath) {
    TileGfx g;
    vector<uint8_t> d = loadFile(datPath);
    if (d.empty()) return g;
    vector<DatEntry> es;
    uint32_t off = 0;
    if (!parseDat(d, es, off)) return g;

    vector<uint8_t> pal;
    if (findRes(d, es, 200, pal)) parsePalette(pal, g.envVga);
    if (findRes(d, es, 360, pal)) parsePalette(pal, g.wallVga);

    loadRange(d, es, 200, 151, g.env);
    loadRange(d, es, 360, 17, g.wall);

    if (princePath) {
        vector<uint8_t> pd = loadFile(princePath);
        if (!pd.empty()) {
            vector<DatEntry> pes;
            uint32_t poff = 0;
            if (parseDat(pd, pes, poff)) {
                loadRange(pd, pes, 150, 23, g.fire);
                g.hasFire = !g.fire.empty();
            }
        }
    }

    g.ok = !g.env.empty() && g.env.size() > 1;
    return g;
}

bool drawRoomTiles(SDL_Surface* buf, const TileGfx& g, const Level& lv, int room, int levelClass) {
    if (!g.ok) return false;
    RoomCtx c;
    c.g = &g;
    c.lv = &lv;
    c.palace = (levelClass != 0);
    for (int i = 0; i < 16; i++) c.monoPal[i] = rgbOf(g.wallVga[i]);
    c.monoPal[6] = 0xFF000000u | ((0x30u << 2) << 16) | ((0x26u << 2) << 8) | (0x14u << 2);
    c.monoPal[12] = 0xFF000000u | ((0x38u << 2) << 16) | (0u << 8) | (0x0Cu << 2);

    c.drawn_room = room;
    loadRoomLinks(c, room);
    loadLeftroom(c);

    for (c.drawn_row = 2; c.drawn_row >= 0; --c.drawn_row) {
        loadRowbelow(c);
        c.draw_bottom_y = 63 * c.drawn_row + 65;
        c.draw_main_y = c.draw_bottom_y - 3;
        for (c.drawn_col = 0; c.drawn_col < 10; c.drawn_col++) {
            loadCurrAndLeft(c);
            drawTileFull(c);
        }
    }

    int savedRoom = c.drawn_room;
    c.drawn_room = c.room_A;
    loadRoomLinks(c, c.drawn_room);
    loadLeftroom(c);
    c.drawn_row = 2;
    loadRowbelow(c);
    for (c.drawn_col = 0; c.drawn_col < 10; c.drawn_col++) {
        loadCurrAndLeft(c);
        c.draw_main_y = -1;
        c.draw_bottom_y = 2;
        drawTileAboveroom(c);
    }
    c.drawn_room = savedRoom;
    loadRoomLinks(c, savedRoom);

    // capas: wipes(0), back, mid, wipes(1), fore
    for (size_t k = 0; k < c.wipes.size(); k++) {
        if (c.wipes[k].layer != 0) continue;
        const RoomCtx::Wip& w = c.wipes[k];
        fillRectCol(buf, w.x, w.y, w.w, w.h, w.color == 0 ? 0xFF000000u : rgbOf(g.wallVga[w.color & 0x0F]));
    }
    drawOps(buf, c, c.back);
    drawOps(buf, c, c.mid);
    for (size_t k = 0; k < c.wipes.size(); k++) {
        if (c.wipes[k].layer != 1) continue;
        const RoomCtx::Wip& w = c.wipes[k];
        fillRectCol(buf, w.x, w.y, w.w, w.h, w.color == 0 ? 0xFF000000u : rgbOf(g.wallVga[w.color & 0x0F]));
    }
    drawOps(buf, c, c.fore);
    return true;
}