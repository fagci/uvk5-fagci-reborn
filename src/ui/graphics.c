#include "graphics.h"
#include "../misc.h"
#include "fonts/NumbersStepanv3.h"
#include "fonts/NumbersStepanv4.h"
#include "fonts/TomThumb.h"
#include "fonts/muHeavy8ptBold.h"
#include "fonts/muMatrix8ptRegular.h"
#include "fonts/symbols.h"
#include <stdlib.h>

static Cursor cursor = {0, 0};

static const GFXfont *fontSmall = &TomThumb;
static const GFXfont *fontMedium = &MuMatrix8ptRegular;
static const GFXfont *fontMediumBold = &muHeavy8ptBold;
static const GFXfont *fontBig = &dig_11;
static const GFXfont *fontBiggest = &dig_14;

void UI_ClearStatus(void) {
  memset(gFrameBuffer[0], 0, sizeof(gFrameBuffer[0]));
}

void UI_ClearScreen(void) {
  for (uint8_t i = 1; i < 8; ++i) {
    memset(gFrameBuffer[i], 0, sizeof(gFrameBuffer[i]));
  }
}

void PutPixel(uint8_t x, uint8_t y, uint8_t fill) {
  if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
    return;
  }
  if (fill == 1) {
    gFrameBuffer[y >> 3][x] |= 1 << (y & 7);
  } else if (fill == 2) {
    gFrameBuffer[y >> 3][x] ^= 1 << (y & 7);
  } else {
    gFrameBuffer[y >> 3][x] &= ~(1 << (y & 7));
  }
}

bool GetPixel(uint8_t x, uint8_t y) {
  return gFrameBuffer[y / 8][x] & (1 << (y & 7));
}

static void DrawALine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    SWAP(x0, y0);
    SWAP(x1, y1);
  }

  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0 <= x1; x0++) {
    if (steep) {
      PutPixel((uint8_t)y0, (uint8_t)x0, color);
    } else {
      PutPixel((uint8_t)x0, (uint8_t)y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void DrawVLine(int16_t x, int16_t y, int16_t h, Color color) {
  DrawALine(x, y, x, y + h - 1, color);
}

void DrawHLine(int16_t x, int16_t y, int16_t w, Color color) {
  DrawALine(x, y, x + w - 1, y, color);
}

void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color color) {
  if (x0 == x1) {
    if (y0 > y1)
      SWAP(y0, y1);
    DrawVLine(x0, y0, y1 - y0 + 1, color);
  } else if (y0 == y1) {
    if (x0 > x1)
      SWAP(x0, x1);
    DrawHLine(x0, y0, x1 - x0 + 1, color);
  } else {
    DrawALine(x0, y0, x1, y1, color);
  }
}

void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
  DrawHLine(x, y, w, color);
  DrawHLine(x, y + h - 1, w, color);
  DrawVLine(x, y, h, color);
  DrawVLine(x + w - 1, y, h, color);
}

void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, Color color) {
  for (int16_t i = x; i < x + w; i++) {
    DrawVLine(i, y, h, color);
  }
}

static void m_putchar(int16_t x, int16_t y, unsigned char c, Color color,
                      uint8_t size_x, uint8_t size_y, const GFXfont *gfxFont) {
  c -= gfxFont->first;
  const GFXglyph *glyph = &gfxFont->glyph[c];
  const uint8_t *bitmap = gfxFont->bitmap;

  uint16_t bo = glyph->bitmapOffset;
  uint8_t w = glyph->width, h = glyph->height;
  int8_t xo = glyph->xOffset, yo = glyph->yOffset;
  uint8_t xx, yy, bits = 0, bit = 0;
  int16_t xo16 = 0, yo16 = 0;

  if (size_x > 1 || size_y > 1) {
    xo16 = xo;
    yo16 = yo;
  }

  for (yy = 0; yy < h; yy++) {
    for (xx = 0; xx < w; xx++) {
      if (!(bit++ & 7)) {
        bits = bitmap[bo++];
      }
      if (bits & 0x80) {
        if (size_x == 1 && size_y == 1) {
          PutPixel(x + xo + xx, y + yo + yy, color);
        } else {
          FillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y, size_x,
                   size_y, color);
        }
      }
      bits <<= 1;
    }
  }
}

void charBounds(unsigned char c, int16_t *x, int16_t *y, int16_t *minx,
                int16_t *miny, int16_t *maxx, int16_t *maxy, uint8_t textsize_x,
                uint8_t textsize_y, bool wrap, const GFXfont *gfxFont) {

  if (c == '\n') { // Newline?
    *x = 0;        // Reset x to zero, advance y by one line
    *y += textsize_y * gfxFont->yAdvance;
  } else if (c != '\r') { // Not a carriage return; is normal char
    uint8_t first = gfxFont->first, last = gfxFont->last;
    if ((c >= first) && (c <= last)) { // Char present in this font?
      const GFXglyph *glyph = &gfxFont->glyph[c - first];
      uint8_t gw = glyph->width, gh = glyph->height, xa = glyph->xAdvance;
      int8_t xo = glyph->xOffset, yo = glyph->yOffset;
      if (wrap && ((*x + (((int16_t)xo + gw) * textsize_x)) > LCD_WIDTH)) {
        *x = 0; // Reset x to zero, advance y by one line
        *y += textsize_y * gfxFont->yAdvance;
      }
      int16_t tsx = (int16_t)textsize_x, tsy = (int16_t)textsize_y,
              x1 = *x + xo * tsx, y1 = *y + yo * tsy, x2 = x1 + gw * tsx - 1,
              y2 = y1 + gh * tsy - 1;
      if (x1 < *minx)
        *minx = x1;
      if (y1 < *miny)
        *miny = y1;
      if (x2 > *maxx)
        *maxx = x2;
      if (y2 > *maxy)
        *maxy = y2;
      *x += xa * tsx;
    }
  }
}

static void getTextBounds(const char *str, int16_t x, int16_t y, int16_t *x1,
                          int16_t *y1, uint16_t *w, uint16_t *h, bool wrap,
                          const GFXfont *gfxFont) {

  uint8_t c; // Current character
  int16_t minx = 0x7FFF, miny = 0x7FFF, maxx = -1, maxy = -1; // Bound rect
  // Bound rect is intentionally initialized inverted, so 1st char sets it

  *x1 = x; // Initial position is value passed in
  *y1 = y;
  *w = *h = 0; // Initial size is zero

  while ((c = *str++)) {
    // charBounds() modifies x/y to advance for each character,
    // and min/max x/y are updated to incrementally build bounding rect.
    charBounds(c, &x, &y, &minx, &miny, &maxx, &maxy, 1, 1, wrap, gfxFont);
  }

  if (maxx >= minx) {     // If legit string bounds were found...
    *x1 = minx;           // Update x1 to least X coord,
    *w = maxx - minx + 1; // And w to bound rect width
  }
  if (maxy >= miny) { // Same for height
    *y1 = miny;
    *h = maxy - miny + 1;
  }
}

void write(uint8_t c, uint8_t textsize_x, uint8_t textsize_y, bool wrap,
           Color color, const GFXfont *gfxFont) {
  if (c == '\n') {
    cursor.x = 0;
    cursor.y += (int16_t)textsize_y * gfxFont->yAdvance;
  } else if (c != '\r') {
    uint8_t first = gfxFont->first;
    if ((c >= first) && (c <= gfxFont->last)) {
      GFXglyph *glyph = &gfxFont->glyph[c - first];
      uint8_t w = glyph->width, h = glyph->height;
      if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
        int16_t xo = glyph->xOffset;
        if (wrap && ((cursor.x + textsize_x * (xo + w)) > LCD_WIDTH)) {
          cursor.x = 0;
          cursor.y += (int16_t)textsize_y * gfxFont->yAdvance;
        }
        m_putchar(cursor.x, cursor.y, c, color, textsize_x, textsize_y,
                  gfxFont);
      }
      cursor.x += glyph->xAdvance * (int16_t)textsize_x;
    }
  }
}

static void printString(const GFXfont *gfxFont, uint8_t x, uint8_t y,
                        Color color, TextPos posLCR, const char *pattern,
                        va_list args) {
  char String[64] = {'\0'};
  vsnprintf(String, 63, pattern, args);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds(String, x, y, &x1, &y1, &w, &h, false, gfxFont);
  if (posLCR == POS_C) {
    x = x - w / 2;
  } else if (posLCR == POS_R) {
    x = x - w;
  }
  cursor.x = x;
  cursor.y = y;
  for (uint8_t i = 0; i < strlen(String); i++) {
    write(String[i], 1, 1, true, color, gfxFont);
  }
}

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontSmall, x, y, C_FILL, POS_L, pattern, args);
  va_end(args);
}

void PrintSmallEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                  const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontSmall, x, y, color, posLCR, pattern, args);
  va_end(args);
}

void PrintMedium(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontMedium, x, y, C_FILL, POS_L, pattern, args);
  va_end(args);
}

void PrintMediumEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                   const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontMedium, x, y, color, posLCR, pattern, args);
  va_end(args);
}

void PrintMediumBold(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontMediumBold, x, y, C_FILL, POS_L, pattern, args);
  va_end(args);
}

void PrintMediumBoldEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                       const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontMediumBold, x, y, color, posLCR, pattern, args);
  va_end(args);
}

void PrintBigDigits(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBig, x, y, C_FILL, POS_L, pattern, args);
  va_end(args);
}

void PrintBigDigitsEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                      const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBig, x, y, color, posLCR, pattern, args);
  va_end(args);
}

void PrintBiggestDigits(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBiggest, x, y, C_FILL, POS_L, pattern, args);
  va_end(args);
}

void PrintBiggestDigitsEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                          const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBiggest, x, y, color, posLCR, pattern, args);
  va_end(args);
}

void PrintSymbolsEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                    const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(&Symbols, x, y, color, posLCR, pattern, args);
  va_end(args);
}
