# Prince of Persia 1 — Port (C++/SDL2)
<img width="952" height="614" alt="sh" src="https://github.com/user-attachments/assets/9f5e4295-f482-414f-bcd9-a1544169d8ff" />

A reimplementation of the original *Prince of Persia* movement engine in **C++/SDL2**.
It runs the original game loop (sequence table at 12 Hz, physics at 60 fps, tile-based
collision) but **does not include any game assets**: you must copy the required `.DAT`
files from your own legal copy of the game.

## Required assets

Place these files inside `data/` (the program also looks in `prince/`):

| File                          | Purpose                                   | Required? |
|-------------------------------|-------------------------------------------|-----------|
| `KID.DAT`                     | Player sprites and palette (resources 400–619) | **Yes** |
| `LEVELS.DAT`                  | All 14 levels (resources 2001–2014)      | **Yes** |
| `VDUNGEON.DAT` + `PRINCE.DAT` | Dungeon environment graphics             | No (falls back to colored rectangles) |
| `VPALACE.DAT` + `PRINCE.DAT`  | Palace environment graphics              | No (falls back to colored rectangles) |

The assets belong to the original game: **they are not bundled with this repository**.
Extract them from your own copy (DOS original or SDLPoP). Without `KID.DAT` the player
is not drawn; without `LEVELS.DAT` the program runs in "sprite demo mode".

## Build requirements

- **MSYS2/mingw64** with `g++` (C++17)
- **SDL2** (`pacman -S mingw-w64-x86_64-SDL2`)
- `SDL2.dll` next to the `.exe` at runtime

## Build & run

From an **MSYS2** shell (MINGW64 or UCRT64) with SDL2 installed:

```
make              # compiles dat_view.exe
make run          # compile + run (SDL2.dll must be on PATH)
make clean        # remove objects and the binary
```

Or with the batch file (from plain cmd):

```
build.cmd         (Windows/MSYS2) -> produces dat_view.exe
```

Or manually:

```
g++ main.cpp dat.cpp img.cpp text.cpp level.cpp player.cpp tiles.cpp -o dat_view.exe -I$MINGW_PREFIX/include/SDL2 -lmingw32 -lSDL2main -lSDL2
```

## Controls

| Key                        | Action                                    |
|----------------------------|-------------------------------------------|
| ← →                        | Run / turn                                |
| `C`                        | Toggle collision debug (AABB + floor lines) |
| `D`                        | Toggle on-screen debug HUD                 |
| `Tab` / `Shift+Tab`        | Next / previous level                      |
| `PageUp` / `PageDown`      | Previous / next level                      |
| `1`–`9`, `0`, `-`, `=`     | Jump directly to level 1–12                |
| `ESC`                      | Quit                                       |

Command-line options: `--autorun` (runs automatically), `--dump` (saves
`render_dump.bmp` of the first frame), `--coll` (enable collision debug),
`--env` (environment sprite browser).

## Project structure

- `main.cpp` — SDL loop, asset loading, HUD, input
- `player.*` — player sequence system (`seqtbl`, `control_*`, physics)
- `level.*` — level parser, colors, room drawing, collider debug
- `tiles.*` — environment graphics (VDUNGEON/VPALACE)
- `dat.*` / `img.*` / `text.*` — `.DAT` parser, RLE image decoding, 5×7 font
- `unpack_levels.py` — dev helper: extracts levels from `LEVELS.DAT` into `data/levels/`

## Legal notice

Prince of Persia © Jordan Mechner / Broderbund / Ubisoft. This project is a
reverse-engineering study for educational purposes and **does not include or
redistribute** the original game data. You need your own copy of the original
game files to run it.
