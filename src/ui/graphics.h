#ifndef GRAPHICS_H

#define GRAPHICS_H

#include "gfxfont.h"
#include <stdint.h>

void DrawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void DrawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMedium(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMediumBold(uint8_t x, uint8_t y, const char *pattern, ...);

#endif /* end of include guard: GRAPHICS_H */
