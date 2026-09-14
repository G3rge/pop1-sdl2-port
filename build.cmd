@echo off
rem Compilar el port (PoP 320x200) con MSYS2/mingw64 + SDL2
rem Modulos: main + dat + img + text + level + player + tiles
"C:\msys64\mingw64\bin\g++.exe" main.cpp dat.cpp img.cpp text.cpp level.cpp player.cpp tiles.cpp -o dat_view.exe -Ic:/msys64/mingw64/include/SDL2 -Lc:/msys64/mingw64/lib -lmingw32 -lSDL2main -lSDL2
if %errorlevel%==0 (echo COMPILADO OK -> dat_view.exe) else (echo ERROR de compilacion)
pause