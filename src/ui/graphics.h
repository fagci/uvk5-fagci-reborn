#ifndef GRAPHICS_H

#define GRAPHICS_H

#include "gfxfont.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  POS_L,
  POS_C,
  POS_R,
} TextPos;

typedef enum {
  C_FILL,
  C_CLEAR,
  C_INVERT,
} Color;

void DrawOnStatus(bool on);

void PutPixel(uint8_t x, uint8_t y, uint8_t fill);

void DrawVLine(int16_t x, int16_t y, int16_t h, Color color);
void DrawHLine(int16_t x, int16_t y, int16_t w, Color color);
void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color);
void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color);
void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color);

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMedium(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintMediumBold(uint8_t x, uint8_t y, const char *pattern, ...);
void PrintBigDigits(uint8_t x, uint8_t y, const char *pattern, ...);

void PrintSmallEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                  const char *pattern, ...);
void PrintMediumEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                   const char *pattern, ...);

void DrawCircle(int16_t x0, int16_t y0, int16_t r, Color color);
void FillCircle(int16_t x0, int16_t y0, int16_t r, Color color);
void DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color);
void FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color);

#endif /* end of include guard: GRAPHICS_H */
