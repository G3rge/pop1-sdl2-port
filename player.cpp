// player.cpp - sistema de secuencias del jugador (seg005/seg006 de SDLPoP)
// El jugador avanza por una tabla de bytes (seqtbl) interpretada a 12Hz:
//   - los comandos DX/DY mueven al personaje (unidades internas, escala KDS),
//   - las demas instrucciones controlan accion/frame/flip/salto y se procesan
//     hasta que un byte de frame (1..220) fija Char.frame y retorna.
#include "player.h"

// ---- modelo original de colision ----
// wall_type() (seg006:0FC3): que cara/s tiene el tile como pared
//   0 = sin pared, 1 = pared a la derecha, 2 = pared a la izquierda,
//   3 = chomper (pared a la izquierda), 4 = pared ambos lados
static inline int wallFace(uint8_t t) {
    uint8_t tt = t & 0x1F;
    switch (tt) {
        case 0x04: case 0x07: case 0x0C: return 1; // gate, doortop_with_floor, doortop
        case 0x0D: return 2;                        // mirror
        case 0x12: return 3;                        // chomper
        case 0x14: return 4;                        // wall
        default:   return 0;
    }
}

// tile_is_floor() (seg006:0628): lo que NO es suelo
static inline bool tileIsFloorOrig(uint8_t t) {
    uint8_t tt = t & 0x1F;
    switch (tt) {
        case 0x00: // empty
        case 0x09: // bigpillar_top
        case 0x0C: // doortop
        case 0x14: // wall
        case 0x1A: case 0x1B: case 0x1C: case 0x1D: // lattice down/small/left/right
            return false;
        default:
            return true;
    }
}

// intervalo de la pared del tile en px dentro del tile (escala 32/14 del sistema
// interno original, ver wall_dist_from_left/right en seg004.c)
static inline void wallInterval(int type, float& lo, float& hi) {
    static const int dl[6] = {0, 10, 0, -1, 0, 0};  // dist. pared izquierda
    static const int dr[6] = {0,  0, 10, 13, 0, 0}; // dist. pared derecha
    const float S = PLAYER_KDS_PX;                  // px por unidad interna
    const float tileRightPx = 13.0f * S;            // TILE_RIGHTX en px
    lo = dl[type] * S;
    hi = -dr[type] * S + tileRightPx;
}

// colision horizontal del cuerpo [x, x+w] contra las caras de los tiles
// (equivalente a check_collisions: solo tiles con wall_type != 0 bloquean)
bool blockedHoriz(const Level& lv, int room, int cellCol, int physRowTop, int physRowBot) {
    return wallFace(tileTypeAt(lv, room, cellCol, physRowTop)) != 0 ||
           wallFace(tileTypeAt(lv, room, cellCol, physRowBot)) != 0;
}

float resolveHoriz(const Level& lv, int room, const Player& p, float x) {
    int w = p.w;
    // columnas que abarca el cuerpo
    int c0 = (int)(x) / 32;
    int c1 = (int)(x + w - 1) / 32;
    if (c0 < 0) c0 = 0;
    if (c1 > 9) c1 = 9;
    int rTop = tileRowFromFootY(p.fy - p.h);
    int rBot = tileRowFromFootY(p.fy);
    if (rTop < 0) rTop = 0;
    if (rBot > 2) rBot = 2;
    float result = x;
    for (int c = c0; c <= c1; c++) {
        for (int r = rTop; r <= rBot; r++) {
            int type = wallFace(tileTypeAt(lv, room, c, r));
            if (type == 0) continue;
            float lo, hi;
            wallInterval(type, lo, hi);
            float a = c * 32.0f + lo, b = c * 32.0f + hi;
            // el cuerpo [x, x+w] interseca [a, b]?
            if (x + w > a && x < b) {
                // cara mas cercana en la direccion de movimiento
                if (p.vx > 0) {
                    float limit = a - w;
                    if (limit < result) result = limit;
                } else if (p.vx < 0) {
                    float limit = b;
                    if (limit > result) result = limit;
                }
            }
        }
    }
    return result;
}

// ============================ SEQTBL (compacta) ============================
// Solo las secuencias de run/idle/turn. Los offsets JMP (FF xx xx) son absolutos
// dentro de esta tabla (igual que "seqtbl_base + offset" del original).
// Labels originales: runstt1=7, runstt4=10, runcyc1=19, runcyc7=39, stand=50.
static const uint8_t seqtbl[] = {
    // 0 running (seq_84): act(1); jmp runcyc1
    0xF9, 0x01,
    0xFF, 0x13, 0x00,
    // 5 startrun (seq_1): act(1) | 01 02 03 04 | dx(8) 05 | dx(3) 06 | dx(3) 07(runcyc1)
    //   | dx(5) 08 | dx(1) snd(1) 09 | dx(2) 0A | dx(4) 0B | dx(5) 0C | dx(2) snd(1) 0D(runcyc7)
    //   | dx(3) 0E | dx(4) jmp runcyc1
    0xF9, 0x01,
    0x01, 0x02, 0x03, 0x04,
    0xFB, 0x08, 0x05,
    0xFB, 0x03, 0x06,
    0xFB, 0x03, 0x07,
    0xFB, 0x05, 0x08,
    0xFB, 0x01, 0xF2, 0x01, 0x09,
    0xFB, 0x02, 0x0A,
    0xFB, 0x04, 0x0B,
    0xFB, 0x05, 0x0C,
    0xFB, 0x02, 0xF2, 0x01, 0x0D,
    0xFB, 0x03, 0x0E,
    0xFB, 0x04, 0xFF, 0x13, 0x00,
    // 50 stand (seq_2): act(0) | 0F | jmp stand
    0xF9, 0x00, 0x0F, 0xFF, 0x32, 0x00,
    // 56 turn (seq_5): act(turn) | flip | dx(6) 2D | dx(1) 2E | dx(2) 2F | dx(-1) 30 |
    //   dx(1) 31 | dx(-2) 32 33 34 | jmp stand
    0xF9, 0x07, 0xFE, 0xFB, 0x06, 0x2D,
    0xFB, 0x01, 0x2E,
    0xFB, 0x02, 0x2F,
    0xFB, 0xFF, 0x30,
    0xFB, 0x01, 0x31,
    0xFB, 0xFE, 0x32, 0x33, 0x34,
    0xFF, 0x32, 0x00,
    // 82 turnrun (seq_43): act(1) | dx(-1) | jmp runstt1 (comienza a correr tras girar)
    0xF9, 0x01, 0xFB, 0xFF, 0xFF, 0x07, 0x00,
    // 89 runturn (seq_6): act(1) | dx(1) 35 | dx(1) snd(1) 36 | dx(8) 37 snd(1) 38 |
    //   dx(7) 39 | dx(3) 3A | dx(1) 3B 3C | dx(2) 3D | dx(-1) 3E 3F 40 | dx(-1) 41 |
    //   dx(-14) flip | jmp runcyc7
    0xF9, 0x01, 0xFB, 0x01, 0x35,
    0xFB, 0x01, 0xF2, 0x01, 0x36,
    0xFB, 0x08, 0x37, 0xF2, 0x01, 0x38,
    0xFB, 0x07, 0x39,
    0xFB, 0x03, 0x3A,
    0xFB, 0x01, 0x3B, 0x3C,
    0xFB, 0x02, 0x3D,
    0xFB, 0xFF, 0x3E, 0x3F, 0x40,
    0xFB, 0xFF, 0x41,
    0xFB, 0xF2, 0xFE, 0xFF, 0x27, 0x00,
    // 132 runstop (seq_13): act(1) | 35 | dx(2) snd(1) 36 | dx(7) 37 snd(1) 38 |
    //   dx(2) 31 | dx(-2) 32 33 34 | jmp stand
    0xF9, 0x01, 0x35,
    0xFB, 0x02, 0xF2, 0x01, 0x36,
    0xFB, 0x07, 0x37, 0xF2, 0x01, 0x38,
    0xFB, 0x02, 0x31,
    0xFB, 0xFE, 0x32, 0x33, 0x34,
    0xFF, 0x32, 0x00,
};

enum {
    SEQ_RUNNING = 0,
    SEQ_STARTRUN = 5,
    SEQ_STAND = 50,
    SEQ_TURN = 56,
    SEQ_TURNRUN = 82,
    SEQ_RUNTURN = 89,
    SEQ_RUNSTOP = 132
};

// seqtbl_offset_char(seq_id): offset en seqtbl de cada secuencia usada
static void startSeq(Player& p, int seqId) {
    p.seqId = seqId;
    switch (seqId) {
        case 1:  p.currSeq = SEQ_STARTRUN; break;  // seq_1_start_run
        case 2:  p.currSeq = SEQ_STAND;    break;  // seq_2_stand
        case 5:  p.currSeq = SEQ_TURN;     break;  // seq_5_turn
        case 6:  p.currSeq = SEQ_RUNTURN;  break;  // seq_6_run_turn
        case 13: p.currSeq = SEQ_RUNSTOP;  break;  // seq_13_stop_run
        case 43: p.currSeq = SEQ_TURNRUN;  break;  // seq_43_start_run_after_turn
        case 84: p.currSeq = SEQ_RUNNING;  break;  // seq_84_run
        default: break;
    }
}

// ============================ play_seq() ============================
// Procesa la instruccion en curr_seq; si es un byte de frame, lo asigna y
// retorna (una instruccion por tick de juego, 12Hz).
static void playSeq(Player& p) {
    for (;;) {
        uint8_t cmd = seqtbl[p.currSeq & 0xFF];
        p.currSeq++;
        switch (cmd) {
            case 0xFB: { // SEQ_DX: mover en la direccion de la mirada
                int8_t d = (int8_t)seqtbl[p.currSeq & 0xFF];
                p.currSeq++;
                if (p.dir < 0) d = -d;   // char_dx_forward()
                p.x += d * PLAYER_KDS_PX;
                break;
            }
            case 0xFA: { // SEQ_DY: mover en vertical (dy en px, y_land es px)
                int8_t d = (int8_t)seqtbl[p.currSeq & 0xFF];
                p.currSeq++;
                p.fy += d;
                break;
            }
            case 0xFE: // SEQ_FLIP: invertir direccion
                p.dir = -p.dir;
                break;
            case 0xF2: // SEQ_SOUND: ignorar el sonido
                p.currSeq++;
                break;
            case 0xFF: { // SEQ_JMP: salto a offset absoluto dentro de seqtbl
                uint16_t ofs = (uint16_t)(seqtbl[p.currSeq & 0xFF] |
                                          (seqtbl[(p.currSeq + 1) & 0xFF] << 8));
                p.currSeq = ofs;
                break;
            }
            case 0xF9: // SEQ_ACTION
                p.action = seqtbl[p.currSeq & 0xFF];
                p.currSeq++;
                break;
            case 0xF7: // SEQ_JMP_IF_FEATHER: 2 bytes, se ignora (no hay pluma)
            case 0xF8: // SEQ_SET_FALL: 2 bytes, se ignora (caida simple)
                p.currSeq += 2;
                break;
            case 0xF6: case 0xF5: case 0xF4: case 0xF3: case 0xF1:
                // SEQ_DIE / KNOCK_UP / KNOCK_DOWN / GET_ITEM / END_LEVEL: fuera de alcance
                return;
            default:
                // valor de frame (1..220) -> fija el frame y finaliza la instruccion
                p.frame = cmd;
                return;
        }
    }
}

// ============================ control() ============================
// control_running: parar al soltar en 7/11, girar corriendo si pulsa atras,
//                   (salto/crouch omitidos en esta fase)
static void controlRunning(Player& p, bool fwdHeld, bool bwdHeld) {
    bool fwdReleased = !fwdHeld && !bwdHeld;
    if (fwdReleased && (p.frame == 7 || p.frame == 11))
        startSeq(p, 13);                          // seq_13_stop_run
    else if (bwdHeld)
        startSeq(p, 6);                           // seq_6_run_turn
}

// control_standing: avanzar (empezar a correr) o girar hacia atras.
// Se usa estado de tecla mantenido (control_forward == CONTROL_PRESSED):
// mientras el frame esta en 15, una direccion pulsada aranca/cancela la accion.
static void controlStanding(Player& p, bool fwdHeld, bool bwdHeld) {
    if (fwdHeld)
        startSeq(p, 1);                           // seq_1_start_run
    else if (bwdHeld)
        startSeq(p, 5);                           // seq_5_turn
}

// control_turning: pulsa hacia adelante durante el giro -> arrancar a correr
static void controlTurning(Player& p, bool fwdHeld) {
    if (fwdHeld)
        startSeq(p, 43);                          // seq_43_start_run_after_turn
}

// control_startrun: sin salto por ahora (solo avanzar)
static void controlStartrun(Player&) {}

// ============================ API publica ============================

void placeAtStart(const Level& lv, Player& p) {
    int room = lv.start_room;
    int currCol = lv.start_pos % 10;
    int currRow = lv.start_pos / 10;

    auto hasFloorAt = [&](int col, int row) -> bool {
        if (col < 0 || col > 9) return false;
        return tileIsFloorOrig(tileTypeAt(lv, room, col, row));
    };

    int fCol = currCol, fRow = currRow;
    if (!hasFloorAt(currCol, currRow)) {
        bool found = false;
        // bajar en la misma columna
        for (int r = currRow; r <= 2 && !found; r++)
            if (hasFloorAt(currCol, r)) { fRow = r; found = true; }
        // si no, columnas vecinas (izq 1, der 1, izq 2, der 2...)
        for (int w = 1; w <= 4 && !found; w++) {
            for (int dc : { -w, w }) {
                int c = currCol + dc;
                if (c < 0 || c > 9) continue;
                for (int r = currRow; r <= 2 && !found; r++)
                    if (hasFloorAt(c, r)) { fCol = c; fRow = r; found = true; }
            }
        }
        if (!found) {
            // cualquier piso del room, el mas alto
            for (int r = 0; r < 3 && !found; r++)
                for (int c = 0; c < 10 && !found; c++)
                    if (hasFloorAt(c, r)) { fCol = c; fRow = r; found = true; }
        }
    }

    p.x = (float)(fCol * 32 + 7);
    p.fy = tileSurface(fRow);
    p.vy = 0;
    p.vx = 0;
    p.dir = (lv.start_dir < 0) ? -1 : 1;
    p.frame = 15;               // frame_15_stand
    p.action = 0;               // actions_0_stand
    p.seqId = 2;                // seq_2_stand
    p.currSeq = SEQ_STAND;
}

void tickPlayer(const Level& lv, int room, Player& p, bool left, bool right) {
    // entradas relativas a la direccion de la mirada (dir>=0 = derecha)
    bool facingRight = p.dir > 0;
    bool fwdHeld = facingRight ? right : left;
    bool bwdHeld = facingRight ? left : right;

    // dispatch de control segun el frame actual
    if (p.frame == 15 || (p.frame >= 50 && p.frame < 53))
        controlStanding(p, fwdHeld, bwdHeld);
    else if (p.frame == 48)
        controlTurning(p, fwdHeld);
    else if (p.frame < 4)
        controlStartrun(p);
    else if (p.frame >= 7 && p.frame < 15)
        controlRunning(p, fwdHeld, bwdHeld);

    // una instruccion de secuencia (avanza dx del frame)
    float xBefore = p.x;
    playSeq(p);

    // colision horizontal del movimiento de esta instruccion
    if (p.x != xBefore)
        p.vx = (p.x > xBefore) ? 1.0f : -1.0f;
    p.x = resolveHoriz(lv, room, p, p.x);
    if (p.x < 0) p.x = 0;
    if (p.x + p.w > 10 * 32) p.x = 10 * 32 - p.w;
}

void physicsPlayer(const Level& lv, int room, Player& p) {
    // gravedad a 60fps: vy += 3 (fall_accel), tope 33
    p.vy += 3;
    if (p.vy > 33) p.vy = 33;

    float fyOld = p.fy;
    p.fy += p.vy;
    bool landed = false;
    if (p.vy >= 0) {
        for (int r = 0; r < 3 && !landed; r++) {
            float surf = tileSurface(r);
            if (surf >= fyOld && surf <= p.fy &&
                tileIsFloorOrig(tileTypeAt(lv, room, player_col(p), r))) {
                p.fy = surf;
                p.vy = 0;
                landed = true;
            }
        }
        if (!landed && p.fy >= 263) {  // cayo fuera del nivel -> reiniciar
            placeAtStart(lv, p);
        }
    }
}

int playerFrameImage(int frame) {
    if (frame >= 1 && frame <= 52) return frame - 1;   // run/idle/turn: imagen = frame-1
    if (frame >= 53 && frame <= 65) return frame + 11; // run-turn: imagen no contigua (+11)
    return -1;
}

int playerFrameDx(int frame) {
    if (frame >= 1 && frame <= 4) { static const int d[4] = { 1, 1, 3, 4 }; return d[frame - 1]; }
    if (frame == 49) return 4;
    if (frame == 50) return 3;
    if (frame == 51) return 1;
    return 0;
}

const char* playerSeqName(int seqId) {
    switch (seqId) {
        case 84: return "running";
        case 1:  return "startrun";
        case 2:  return "stand";
        case 5:  return "turn";
        case 43: return "turnrun";
        case 6:  return "runturn";
        case 13: return "runstop";
        default: return "?";
    }
}

const char* playerActionName(int action) {
    switch (action) {
        case 0: return "stand";
        case 1: return "run";
        case 7: return "turn";
        default: return "?";
    }
}