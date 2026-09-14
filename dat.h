// dat.h - parser de archivos DAT de Prince of Persia
#pragma once
#include <cstdint>
#include <vector>

struct DatEntry { uint16_t rid; uint32_t off; uint16_t size; };

std::vector<uint8_t> loadFile(const char* path);
bool parseDat(const std::vector<uint8_t>& d, std::vector<DatEntry>& entries, uint32_t& tableOff);
bool findRes(const std::vector<uint8_t>& d, const std::vector<DatEntry>& es, uint16_t rid,
             std::vector<uint8_t>& out);