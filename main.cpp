// main.cpp - Port de Prince of Persia 1: bucle SDL + juego (320x200 interno)
#include <SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "dat.h"
#include "img.h"
#include "text.h"
#include "level.h"
#include "player.h"
#include "tiles.h"

static void drawSpriteInto(SDL_Surface* buf, const Image& im, const uint8_t vga[16][3],
                           int px, int py) {
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            int c = im.pixels[y * im.w + x];
            if (c <= 0) continue;
            uint8_t r = vga[c][0] << 2, g = vga[c][1] << 2, b = vga[c][2] << 2;
            setPixel(buf, px + x, py + y, SDL_MapRGBA(buf->format, r, g, b, 255));
        }
    }
}

static void drawSpriteInto(SDL_Surface* buf, const Image& im, const uint8_t vga[16][3],
                           int px, int py, bool flipX) {
    for (int y = 0; y < im.h; y++) {
        for (int x = 0; x < im.w; x++) {
            int c = im.pixels[y * im.w + (flipX ? im.w - 1 - x : x)];
            if (c <= 0) continue;
            uint8_t r = vga[c][0] << 2, g = vga[c][1] << 2, b = vga[c][2] << 2;
            setPixel(buf, px + x, py + y, SDL_MapRGBA(buf->format, r, g, b, 255));
        }
    }
}

int main(int argc, char** argv) {
    // buscar KID.DAT: argumento -> data\KID.DAT (dentro del port) -> prince\KID.DAT
    std::string datPath = (argc > 1) ? argv[1] : "data/KID.DAT";
    FILE* probe = fopen(datPath.c_str(), "rb");
    if (!probe) { datPath = "prince/KID.DAT"; probe = fopen(datPath.c_str(), "rb"); }
    if (!probe) { datPath = "data\\KID.DAT"; probe = fopen(datPath.c_str(), "rb"); }
    if (!probe) { datPath = "prince\\KID.DAT"; probe = fopen(datPath.c_str(), "rb"); }
    if (probe) fclose(probe);

    SDL_Init(SDL_INIT_VIDEO);

    const int GW = 320, GH = 200;   // resolucion original de MS-DOS
    const int SW = 3, SH = 3;       // escala de la ventana (multiple exacto) -> 960x600

    SDL_Window* win = SDL_CreateWindow("Prince of Persia - Port (320x200)",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       GW * SW, GH * SH, SDL_WINDOW_RESIZABLE);
    if (!win) { SDL_Log("no se pudo crear la ventana"); SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);

    // bufer interno del juego: 320x200, como la pantalla DOS original
    SDL_Surface* gbuf = SDL_CreateRGBSurface(0, GW, GH, 32, 0, 0, 0, 0);
    SDL_Texture* gtex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                          SDL_TEXTUREACCESS_STREAMING, GW, GH);
    if (!gbuf || !gtex) { SDL_Log("no se pudo crear el bufer 320x200"); SDL_Quit(); return 1; }

    uint32_t black = SDL_MapRGBA(gbuf->format, 0, 0, 0, 255);
    uint32_t green = SDL_MapRGBA(gbuf->format, 120, 255, 120, 255);
    uint32_t white = SDL_MapRGBA(gbuf->format, 255, 255, 255, 255);
    uint32_t cyan  = SDL_MapRGBA(gbuf->format, 120, 220, 255, 255);
    uint32_t yellow = SDL_MapRGBA(gbuf->format, 255, 255, 80, 255);

    auto present = [&]() {
        SDL_UpdateTexture(gtex, NULL, gbuf->pixels, gbuf->pitch);
        int ww, wh;
        SDL_GetWindowSize(win, &ww, &wh);
        int sc = (ww / GW) < (wh / GH) ? (ww / GW) : (wh / GH);
        if (sc < 1) sc = 1;
        SDL_Rect dst = { (ww - GW * sc) / 2, (wh - GH * sc) / 2, GW * sc, GH * sc };
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, gtex, NULL, &dst);
        SDL_RenderPresent(ren);
    };

    // ---------- pantalla de carga ----------
    char prog[64];
    SDL_FillRect(gbuf, NULL, black);
    drawText(gbuf, 92, 90, "Loading DAT KID...", green, 1);
    present();

    // ---------- cargar y decodificar KID.DAT ----------
    std::vector<uint8_t> d = loadFile(datPath.c_str());
    if (d.empty()) {
        SDL_FillRect(gbuf, NULL, black);
        drawText(gbuf, 40, 88, "No se encontro KID.DAT", white, 1);
        drawText(gbuf, 20, 100, "Uso: dat_view.exe ruta/KID.DAT", cyan, 1);
        present();
        SDL_Delay(3000);
        SDL_DestroyTexture(gtex); SDL_FreeSurface(gbuf);
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
        return 1;
    }

    std::vector<DatEntry> entries;
    uint32_t tableOff = 0;
    if (!parseDat(d, entries, tableOff)) {
        SDL_Log("KID.DAT corrupto, tabla no valida");
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
        return 1;
    }

    // paleta del player: shpl res 400
    uint8_t vga[16][3];
    {
        std::vector<uint8_t> pal;
        if (!findRes(d, entries, 400, pal) || !parsePalette(pal, vga)) {
            SDL_Log("no hay paleta 400");
            SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
            return 1;
        }
    }

    // decodificar todos los recursos-imagen del player (401..619)
    std::vector<Image> frames;
    frames.reserve(219);
    for (uint16_t rid = 401; rid <= 619; rid++) {
        std::vector<uint8_t> res;
        if (!findRes(d, entries, rid, res)) continue;
        Image im = decodeImage(res);
        if (!im.ok) continue;
        frames.push_back(std::move(im));

        // progreso en la pantalla 320x200
        int done = (int)frames.size();
        snprintf(prog, sizeof prog, "Load: %d / 219  res %d", done, rid);
        SDL_FillRect(gbuf, NULL, black);
        drawText(gbuf, 92, 90, "Loading DAT KID...", green, 1);
        drawText(gbuf, 100, 104, prog, white, 1);
        int barW = 200 * done / 219;
        SDL_Rect bar = { 60, 120, barW, 6 };
        SDL_FillRect(gbuf, &bar, SDL_MapRGBA(gbuf->format, 80, 180, 255, 255));
        present();
    }

    char title[64];
    snprintf(title, sizeof title, "Prince of Persia - Port (%zu sprites)", frames.size());
    SDL_SetWindowTitle(win, title);

    // ---------- cargar graficos de entorno (VDUNGEON / VPALACE / PRINCE.DAT) ----------
    TileGfx gfxDungeon, gfxPalace;
    {
        const char* princeCandidates[] = {
            "data/PRINCE.DAT", "prince/PRINCE.DAT",
            "data\\PRINCE.DAT", "prince\\PRINCE.DAT",
            "../prince/PRINCE.DAT", "../prince\\PRINCE.DAT",
            "..\\prince\\PRINCE.DAT"
        };
        const char* princePath = nullptr;
        for (const char* c : princeCandidates) {
            FILE* f = fopen(c, "rb"); if (f) { fclose(f); princePath = c; break; }
        }
        // dungeon
        {
            const char* dats[] = {
                "data/VDUNGEON.DAT", "prince/VDUNGEON.DAT",
                "data\\VDUNGEON.DAT", "prince\\VDUNGEON.DAT",
                "../prince/VDUNGEON.DAT", "../prince\\VDUNGEON.DAT",
                "..\\prince\\VDUNGEON.DAT"
            };
            for (const char* c : dats) {
                gfxDungeon = loadEnvGfx(c, princePath);
                if (gfxDungeon.ok) {
                    fprintf(stderr, "env dungeon: %zu env + %zu wall  (%s)\n",
                            gfxDungeon.env.size() - 1, gfxDungeon.wall.size() - 1, c);
                    break;
                }
            }
        }
        // palace
        {
            const char* dats[] = {
                "data/VPALACE.DAT", "prince/VPALACE.DAT",
                "data\\VPALACE.DAT", "prince\\VPALACE.DAT",
                "../prince/VPALACE.DAT", "../prince\\VPALACE.DAT",
                "..\\prince\\VPALACE.DAT"
            };
            for (const char* c : dats) {
                gfxPalace = loadEnvGfx(c, princePath);
                if (gfxPalace.ok) {
                    fprintf(stderr, "env palace: %zu env + %zu wall  (%s)\n",
                            gfxPalace.env.size() - 1, gfxPalace.wall.size() - 1, c);
                    break;
                }
            }
        }
        if (!gfxDungeon.ok && !gfxPalace.ok)
            fprintf(stderr, "sin graficos de entorno (rects de color)\n");
        else {
            SDL_SetWindowTitle(win, "Prince of Persia - Port (entorno cargado)");
        }
    }

    // ---------- cargar LEVELS.DAT ----------
    std::vector<uint8_t> dlev;
    {
        const char* cand[] = { "data/LEVELS.DAT", "prince/LEVELS.DAT",
                               "data\\LEVELS.DAT", "prince\\LEVELS.DAT" };
        for (const char* c : cand) { dlev = loadFile(c); if (!dlev.empty()) break; }
    }
    bool haveLevels = !dlev.empty();
    std::vector<DatEntry> lentr;
    if (haveLevels) {
        uint32_t loff = 0;
        haveLevels = parseDat(dlev, lentr, loff);
        if (haveLevels) {
            std::vector<uint8_t> res;
            if (!findRes(dlev, lentr, 2001, res) || res.size() < sizeof(Level))
                haveLevels = false;
        }
    }
    if (haveLevels)
        SDL_Log("LEVELS.DAT: %lu resources", (unsigned long)lentr.size());
    else
        SDL_Log("sin LEVELS.DAT -> modo demo de sprites");

    // ---------- bucle del juego (nivel: rects + correr + colision) ----------
    auto loadLevelRes = [&](int lvNum, Level& lv) -> bool {
        std::vector<uint8_t> res;
        if (!findRes(dlev, lentr, (uint16_t)(2000 + lvNum), res)) return false;
        return levelFromBytes(res, lv);
    };

    Level lv;
    int  currentRoom = 1;
    int  currentLevel = 1;
    Player p;
    Uint32 tick = SDL_GetTicks();

    if (loadLevelRes(currentLevel, lv)) {
        applyAlterMods(lv);
        currentRoom = lv.start_room;
        placeAtStart(lv, p);
    } else {
        SDL_Log("no pude cargar el nivel 1");
    }

    bool quit = false;
    bool autorun = false;
    bool dumpMode = false;
    bool envMode = false;
    bool showColl = false;
    bool showHud = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--autorun") == 0) autorun = true;
        if (strcmp(argv[i], "--dump") == 0) dumpMode = true;
        if (strcmp(argv[i], "--env") == 0) envMode = true;
        if (strcmp(argv[i], "--coll") == 0) showColl = true;
    }

    // ---- modo --env: browsear sprites del entorno sin aplicarlos a tiles ----
    if (envMode) {
        const char* setNames[3] = { "DUNGEON env", "PALACE env", "WALLS" };
        int curSet = 0, curI = 1;
        auto spriteAt = [&](int set, int i) -> const Image* {
            if (set == 0) return (i >= 1 && i < (int)gfxDungeon.env.size()) ? &gfxDungeon.env[i] : nullptr;
            if (set == 1) return (i >= 1 && i < (int)gfxPalace.env.size()) ? &gfxPalace.env[i] : nullptr;
            return (i >= 1 && i < (int)gfxDungeon.wall.size()) ? &gfxDungeon.wall[i] : nullptr;
        };
        auto palOf = [&](int set) -> const uint8_t(*)[3] {
            if (set == 0) return gfxDungeon.envVga;
            if (set == 1) return gfxPalace.envVga;
            return gfxDungeon.wallVga;
        };
        auto setSize = [&](int set) -> int {
            if (set == 0) return (int)gfxDungeon.env.size();   // indice base (1..N-1)
            if (set == 1) return (int)gfxPalace.env.size();
            return (int)gfxDungeon.wall.size();
        };
        while (!quit) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) quit = true;
                if (ev.type != SDL_KEYDOWN) continue;
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_RIGHT: curI++; break;
                    case SDLK_LEFT:  curI--; break;
                    case SDLK_DOWN:  curSet = (curSet + 1) % 3; curI = 1; break;
                    case SDLK_UP:    curSet = (curSet + 2) % 3; curI = 1; break;
                    default: break;
                }
                if (curI < 1) curI = setSize(curSet) - 1;
                if (curI > setSize(curSet) - 1) curI = 1;
            }
            SDL_FillRect(gbuf, NULL, black);
            const Image* im = spriteAt(curSet, curI);
            const uint8_t(*pal)[3] = palOf(curSet);
            if (im && im->ok) {
                int px = (GW - im->w) / 2, py = (GH - im->h) / 2 + 4;
                drawSpriteInto(gbuf, *im, pal, px, py);
                snprintf(prog, sizeof prog, "%s sprite %d  %dx%d",
                         setNames[curSet], curI, im->w, im->h);
                drawText(gbuf, 4, 2, prog, cyan, 1);
                drawText(gbuf, 4, GH - 10, "<- -> sprite   UP/DOWN conjunto  ESC salir", green, 1);
            } else {
                drawText(gbuf, 4, 90, "sprite vacio (placeholder del DAT)", white, 1);
                drawText(gbuf, 4, GH - 10, "<- -> sprite   UP/DOWN conjunto  ESC salir", green, 1);
            }
            present();
            SDL_Delay(20);
        }
        // al salir del browse, quit=true asi el bucle del juego no se ejecuta
    }
    // cambio de nivel (Tab / PageUp-PageDown / teclas numericas)
    auto switchLevel = [&](int lvNum) {
        if (!haveLevels) return;
        if (lvNum < 1) lvNum = 14;
        if (lvNum > 14) lvNum = 1;
        currentLevel = lvNum;
        if (loadLevelRes(currentLevel, lv)) {
            applyAlterMods(lv);
            currentRoom = lv.start_room;
            placeAtStart(lv, p);
        }
    };
    Uint32 debugTick = 0;
    int seqAccum = 0;
    const int TICKS_PER_SEQ = 5;  // 60fps / 5 = 12Hz (match.wait_time[1]=5)
    while (!quit) {
        SDL_Event ev;
        bool left = false, right = false;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) quit = true;
            if (ev.type != SDL_KEYDOWN && ev.type != SDL_KEYUP) continue;
            bool down = (ev.type == SDL_KEYDOWN);
            switch (ev.key.keysym.sym) {
                case SDLK_ESCAPE: if (down) quit = true; break;
                case SDLK_LEFT:  left  = down; break;
                case SDLK_RIGHT: right = down; break;
                case SDLK_c:     if (down) showColl = !showColl; break;
                case SDLK_d:     if (down) showHud = !showHud; break;
                case SDLK_PAGEDOWN: if (down) switchLevel(currentLevel + 1); break;
                case SDLK_PAGEUP:   if (down) switchLevel(currentLevel - 1); break;
                case SDLK_TAB:
                    if (down) switchLevel((ev.key.keysym.mod & KMOD_SHIFT)
                                              ? currentLevel - 1 : currentLevel + 1);
                    break;
                case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
                case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9:
                    if (down) switchLevel(ev.key.keysym.sym - SDLK_1 + 1);
                    break;
                case SDLK_0:     if (down) switchLevel(10); break;
                case SDLK_MINUS: if (down) switchLevel(11); break;
                case SDLK_EQUALS: if (down) switchLevel(12); break;
                default: break;
            }
        }
        if (autorun) right = true;

        // ---- fisica a 60fps + secuencia a 12Hz (cada 5 frames) ----
        bool onFloor = false;
        if (haveLevels) {
            // un tick de juego = 1 instruccion de secuencia a 12Hz
            if (++seqAccum >= TICKS_PER_SEQ) {
                seqAccum = 0;
                tickPlayer(lv, currentRoom, p, left, right);
            }
            physicsPlayer(lv, currentRoom, p);

            int feetRow = tileRowFromFootY(p.fy);
            onFloor = tileIsFloor(tileTypeAt(lv, currentRoom, player_col(p), feetRow)) &&
                      p.fy == tileSurface(feetRow);

            // ---- debug log (cada 500 ms) ----
            if (SDL_GetTicks() - debugTick >= 500) {
                debugTick = SDL_GetTicks();
                fprintf(stderr, "L%d room %d x=%d fy=%d frame=%2d act=%d(%s) seq=%d(%s) off=%d dir=%d vx=%d vy=%d col=%d row=%d tile=%02X onFloor=%d\n",
                        currentLevel, currentRoom, (int)p.x, (int)p.fy, p.frame, p.action,
                        playerActionName(p.action), p.seqId, playerSeqName(p.seqId), p.currSeq,
                        p.dir, (int)p.vx, (int)p.vy, player_col(p), feetRow,
                        tileTypeAt(lv, currentRoom, player_col(p), feetRow),
                        onFloor ? 1 : 0);
            }

            // ---- dibujo: level con sprites reales del entorno ----
            SDL_FillRect(gbuf, NULL, black);

            int lc = levelClassForLevel(currentLevel);
            TileGfx* gfx = (lc == 0) ? &gfxDungeon : &gfxPalace;
            if (gfx->ok)
                drawRoomTiles(gbuf, *gfx, lv, currentRoom, lc);
            else
                drawRoom(gbuf, lv, currentRoom);  // fallback

            // dibujar el frame de animacion del player
            if (!frames.empty()) {
                int img = playerFrameImage(p.frame);
                if (img < 0 || img >= (int)frames.size()) img = 0;
                const Image& kid = frames[img];
                bool flipX = p.dir > 0;
                int px = (int)p.x + (int)((p.dir >= 0 ? 1 : -1) * playerFrameDx(p.frame) * PLAYER_KDS_PX);
                int py = (int)p.fy - kid.h + 1;
                drawSpriteInto(gbuf, kid, vga, px, py, flipX);
            }

            // debug de colisiones (tecla C): AABB del player + celdas solidas
            if (showColl) {
                int topY = (int)p.fy - p.h;
                drawColliders(gbuf, lv, currentRoom, (int)p.x, topY, p.w, p.h);
                snprintf(prog, sizeof prog,
                         "x%d fy%d col%d row%d W%d H%d | t=%02X ct=%02X",
                         (int)p.x, (int)p.fy, (int)p.x / 32, feetRow,
                         p.w, p.h,
                         tileTypeAt(lv, currentRoom, (int)(p.x + p.w / 2) / 32, feetRow),
                         tileTypeAt(lv, currentRoom, player_col(p), feetRow));
                drawText(gbuf, 4, 2, prog, yellow, 1);
            }

            // HUD de debug (tecla D): igual que el log de terminal, en pantalla
            if (showHud) {
                SDL_Rect panel = { 0, 0, 320, 42 };
                SDL_FillRect(gbuf, &panel, SDL_MapRGBA(gbuf->format, 12, 12, 20, 255));
                char dbg[96];
                int fc = player_col(p);
                snprintf(dbg, sizeof dbg, "L%d room %d  col %d row %d  tile %02X  floor %d",
                         currentLevel, currentRoom, fc, feetRow,
                         tileTypeAt(lv, currentRoom, fc, feetRow), onFloor ? 1 : 0);
                drawText(gbuf, 4, 2, dbg, cyan, 1);
                snprintf(dbg, sizeof dbg, "x %d fy %d  vx %d vy %d  dir %s",
                         (int)p.x, (int)p.fy, (int)p.vx, (int)p.vy,
                         p.dir > 0 ? "der" : "izq");
                drawText(gbuf, 4, 11, dbg, yellow, 1);
                snprintf(dbg, sizeof dbg, "frame %d  act %d %s  seq %d %s off %d",
                         p.frame, p.action, playerActionName(p.action),
                         p.seqId, playerSeqName(p.seqId), p.currSeq);
                drawText(gbuf, 4, 20, dbg, white, 1);
                drawText(gbuf, 4, 29, "D hud  C col  Tab nivel  1-9/0 nivel  ESC salir", green, 1);
            }

            snprintf(prog, sizeof prog, "L%d room %d  x%d y%d f%d",
                     currentLevel, currentRoom, (int)p.x, (int)p.fy, p.frame);
            drawText(gbuf, 4, GH - 10, prog, cyan, 1);
        } else {
            // modo demo: recorrer sprites
            size_t frameIdx = 0;
            (void)frameIdx;
            const Image& im = frames[0];
            SDL_FillRect(gbuf, NULL, black);
            drawSpriteInto(gbuf, im, vga, (GW - im.w) / 2, (GH - im.h) / 2);
            drawText(gbuf, 4, GH - 10, "solo sprites (faltan LEVELS.DAT)", cyan, 1);
        }

        present();

        // dump: guardar primer frame y salir
        static bool dumped = false;
        if (dumpMode && !dumped) {
            SDL_SaveBMP(gbuf, "render_dump.bmp");
            fprintf(stderr, "dump guardado: render_dump.bmp\n");
            dumped = true;
        }

        // limitar ~60 fps
        Uint32 now = SDL_GetTicks();
        if (now - tick < 16) SDL_Delay(16 - (now - tick));
        tick = SDL_GetTicks();
    }

    SDL_FreeSurface(gbuf);
    SDL_DestroyTexture(gtex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}