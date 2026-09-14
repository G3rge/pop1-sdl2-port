// img.cpp - descompresores (RLE y LZG, equivalentes a seg009.c de SDLPoP)
//                e imagenes con cabecera de 6 bytes -> pixels 8bpp
#include "img.h"
#include <cstddef>

static std::vector<uint8_t> decompRleL(const std::vector<uint8_t>& src, int destLen) {
    std::vector<uint8_t> out;
    out.reserve(destLen);
    size_t sp = 0;
    while ((int)out.size() < destLen) {
        int count = src[sp++];
        if (count < 0x80) {              // copia count+1 bytes
            int n = count + 1;
            for (int i = 0; i < n && (int)out.size() < destLen; i++)
                out.push_back(src[sp++]);
        } else {                          // repite un byte (0x100-count) veces
            uint8_t al = src[sp++];
            int n = 0x100 - count;
            for (int i = 0; i < n && (int)out.size() < destLen; i++)
                out.push_back(al);
        }
    }
    return out;
}

static std::vector<uint8_t> decompRleU(const std::vector<uint8_t>& src, int destLen,
                                       int width, int height) {
    std::vector<uint8_t> out(destLen, 0);
    int destPos = 0, remHeight = height, remLength = destLen;
    int end = destLen - 1, w = width - 1;
    size_t sp = 0;
    while (remLength) {
        int count = src[sp++];
        if (count < 0x80) {               // copia count+1 bytes
            int n = count + 1;
            while (n && remLength) {
                out[destPos] = src[sp++];
                destPos += 1 + w;
                if (--remHeight == 0) { destPos -= end; remHeight = height; }
                --remLength; --n;
            }
        } else {                           // repite un byte (0x100-count) veces
            uint8_t al = src[sp++];
            int n = 0x100 - count;
            while (n && remLength) {
                out[destPos] = al;
                destPos += 1 + w;
                if (--remHeight == 0) { destPos -= end; remHeight = height; }
                --remLength; --n;
            }
        }
    }
    return out;
}

static std::vector<uint8_t> decompLzgL(const std::vector<uint8_t>& src, int destLen) {
    std::vector<uint8_t> window(0x400, 0);
    std::vector<uint8_t> dest(destLen, 0);
    int wp = 0x400 - 0x42, dp = 0;
    size_t sp = 0;
    uint16_t mask = 0;
    while (dp < destLen) {
        mask >>= 1;
        if ((mask & 0xFF00) == 0) mask = (uint16_t)(src[sp++] | 0xFF00);
        if (mask & 1) {                    // literal
            uint8_t b = src[sp++];
            window[wp & 0x3FF] = b;
            wp = (wp + 1) & 0x3FF;
            dest[dp++] = b;
        } else {                           // copia desde ventana
            uint16_t info = (uint16_t)((src[sp] << 8) | src[sp + 1]); sp += 2;
            int cs = info & 0x3FF;
            int cl = (info >> 10) + 3;
            while (cl-- && dp < destLen) {
                uint8_t b = window[cs & 0x3FF];
                window[wp & 0x3FF] = b;
                wp = (wp + 1) & 0x3FF;
                cs = (cs + 1) & 0x3FF;
                dest[dp++] = b;
            }
        }
    }
    return dest;
}

static std::vector<uint8_t> decompLzgU(const std::vector<uint8_t>& src, int destLen,
                                       int stride, int height) {
    std::vector<uint8_t> window(0x400, 0);
    std::vector<uint8_t> dest(destLen, 0);
    int wp = 0x400 - 0x42, dp = 0, remaining = height, remBytes = destLen;
    size_t sp = 0;
    uint16_t mask = 0;
    int destEnd = destLen - 1;
    while (remBytes > 0) {
        mask >>= 1;
        if ((mask & 0xFF00) == 0) mask = (uint16_t)(src[sp++] | 0xFF00);
        if (mask & 1) {                    // literal
            uint8_t b = src[sp++];
            window[wp & 0x3FF] = dest[dp] = b;
            wp = (wp + 1) & 0x3FF;
            dp += stride;
            if (--remaining == 0) { dp -= destEnd; remaining = height; }
            --remBytes;
        } else {                           // copia desde ventana
            uint16_t info = (uint16_t)((src[sp] << 8) | src[sp + 1]); sp += 2;
            int cs = info & 0x3FF;
            int cl = (info >> 10) + 3;
            while (cl > 0 && remBytes > 0) {
                uint8_t b = window[cs & 0x3FF];
                window[wp & 0x3FF] = dest[dp] = b;
                wp = (wp + 1) & 0x3FF;
                cs = (cs + 1) & 0x3FF;
                dp += stride;
                if (--remaining == 0) { dp -= destEnd; remaining = height; }
                --remBytes; --cl;
            }
        }
    }
    return dest;
}

Image decodeImage(const std::vector<uint8_t>& data) {
    Image im;
    if (data.size() < 6) return im;
    int height = data[0] | (data[1] << 8);
    int width  = data[2] | (data[3] << 8);
    uint16_t flags = (uint16_t)(data[4] | (data[5] << 8));
    if (height <= 0 || width <= 0 || height > 400 || width > 400) return im;

    std::vector<uint8_t> body(data.begin() + 6, data.end());
    body.insert(body.end(), 4096, 0);      // margen anti-overread

    int depth   = ((flags >> 12) & 7) + 1;
    int cmeth   = (flags >> 8) & 0x0F;
    int stride  = (depth * width + 7) / 8;
    int destLen = stride * height;

    std::vector<uint8_t> raw;
    switch (cmeth) {
        case 0: raw.assign(body.begin(), body.begin() + destLen); break;
        case 1: raw = decompRleL(body, destLen); break;
        case 2: raw = decompRleU(body, destLen, stride, height); break;
        case 3: raw = decompLzgL(body, destLen); break;
        case 4: raw = decompLzgU(body, destLen, stride, height); break;
        default: return im;
    }
    if ((int)raw.size() < destLen) raw.resize(destLen, 0);

    std::vector<uint8_t> px((size_t)width * height, 0);
    int pmask = (1 << depth) - 1;
    int ppb   = 8 / depth;
    for (int y = 0; y < height; y++) {
        int xp = 0;
        for (int xb = 0; xb < stride; xb++) {
            uint8_t v = raw[y * stride + xb];
            int shift = 8;
            for (int k = 0; k < ppb; k++) {
                if (xp >= width) break;
                shift -= depth;
                px[y * width + xp] = (uint8_t)((v >> shift) & pmask);
                xp++;
            }
        }
    }
    im.w = width; im.h = height; im.depth = depth;
    im.pixels = std::move(px); im.ok = true;
    return im;
}

bool parsePalette(const std::vector<uint8_t>& d, uint8_t vga[16][3]) {
    if (d.size() < 52) return false;
    for (int i = 0; i < 16; i++) {
        vga[i][0] = d[4 + 3 * i];
        vga[i][1] = d[4 + 3 * i + 1];
        vga[i][2] = d[4 + 3 * i + 2];
    }
    return true;
}