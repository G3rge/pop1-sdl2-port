// dat.cpp - parser de archivos DAT de Prince of Persia
#include "dat.h"
#include <cstdio>

std::vector<uint8_t> loadFile(const char* path) {
    std::vector<uint8_t> d;
    FILE* f = fopen(path, "rb");
    if (!f) return d;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n > 0) { d.resize((size_t)n); fread(d.data(), 1, (size_t)n, f); }
    fclose(f);
    return d;
}

bool parseDat(const std::vector<uint8_t>& d, std::vector<DatEntry>& entries, uint32_t& tableOff) {
    if (d.size() < 6) return false;
    tableOff = (uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
    if (tableOff + 2 > d.size()) return false;
    uint16_t tableSize  = (uint16_t)(d[4] | (d[5] << 8));
    uint16_t resCount   = (uint16_t)(d[tableOff] | (d[tableOff + 1] << 8));
    if (tableSize != 2 + 8 * resCount) return false;
    for (int i = 0; i < resCount; i++) {
        uint32_t b = tableOff + 2 + 8 * i;
        if (b + 8 > d.size()) return false;
        DatEntry e;
        e.rid  = (uint16_t)(d[b] | (d[b + 1] << 8));
        e.off  = (uint32_t)d[b + 2] | ((uint32_t)d[b + 3] << 8) |
                 ((uint32_t)d[b + 4] << 16) | ((uint32_t)d[b + 5] << 24);
        e.size = (uint16_t)(d[b + 6] | (d[b + 7] << 8));
        entries.push_back(e);
    }
    return true;
}

bool findRes(const std::vector<uint8_t>& d, const std::vector<DatEntry>& es,
             uint16_t rid, std::vector<uint8_t>& out) {
    for (const auto& e : es) {
        if (e.rid == rid) {
            uint32_t off = e.off + 1; // 1 byte checksum
            out.assign(d.begin() + off, d.begin() + off + e.size);
            return true;
        }
    }
    return false;
}