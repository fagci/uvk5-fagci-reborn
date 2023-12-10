#ifndef GRAPHICS_H

#define GRAPHICS_H

#include "gfxfont.h"
#include <stdint.h>

void DrawOnStatus(bool on);

void PutPixel(uint8_t x, uint8_t y, uint8_t fill);

void DrawVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void DrawHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintSmallC(uint8_t x, uint8_t y, uint8_t color, const char *pattern, ...);
void PrintSmallRight(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMedium(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMediumCentered(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMediumRight(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMediumBold(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintBigDigits(uint8_t x, uint8_t y, const char *pattern, ...);

void DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   uint16_t color);
void FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   uint16_t color);

#endif /* end of include guard: GRAPHICS_H */
