// img.h - descompresion + decodificado de imagenes (recursos TOMBI/DAT)
#pragma once
#include <cstdint>
#include <vector>

struct Image { int w = 0, h = 0, depth = 0; bool ok = false; std::vector<uint8_t> pixels; };

Image decodeImage(const std::vector<uint8_t>& data);
bool parsePalette(const std::vector<uint8_t>& d, uint8_t vga[16][3]);