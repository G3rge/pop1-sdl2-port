// text.h - fuente 5x7 + dibujo de texto y sprites
#pragma once
#include <SDL.h>

void setPixel(SDL_Surface* s, int x, int y, uint32_t color);
void drawText(SDL_Surface* s, int x, int y, const char* text, uint32_t color, int scale = 1);