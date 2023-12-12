#ifndef GRAPHICS_H

#define GRAPHICS_H

#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "gfxfont.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef enum {
  POS_L,
  POS_C,
  POS_R,
} TextPos;

typedef enum {
  C_CLEAR,
  C_FILL,
  C_INVERT,
} Color;

void UI_ClearStatus();
void UI_ClearScreen();

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
void PrintBigDigitsEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                      const char *pattern, ...);

void DrawCircle(int16_t x0, int16_t y0, int16_t r, Color color);
void FillCircle(int16_t x0, int16_t y0, int16_t r, Color color);
void DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color);
void FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color);

#define SHOW_ITEMS(value)                                                      \
  do {                                                                         \
    char items[ARRAY_SIZE(value)][16] = {0};                                   \
    for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                          \
      strncpy(items[i], value[i], 15);                                         \
    }                                                                          \
    UI_ShowItems(items, ARRAY_SIZE(value), subMenuIndex);                      \
  } while (0)

#endif /* end of include guard: GRAPHICS_H */
