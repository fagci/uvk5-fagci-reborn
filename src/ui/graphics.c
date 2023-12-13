#include "graphics.h"
// #include "fonts/DSEG7_Classic_Mini_Regular_11.h"
#include "fonts/NumbersStepanv3.h"
#include "fonts/NumbersStepanv4.h"
// #include "fonts/NumbersSoftAce6.h"
#include "fonts/Minecraft8.h"
#include "fonts/TomThumb.h"
#include "fonts/muHeavy8ptBold.h"
// #include "fonts/muMatrix8ptRegular.h"
#include <stdlib.h>

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }
#endif

static uint8_t cursor_x = 0, cursor_y = 0;

static const GFXfont *fontSmall = &TomThumb;
static const GFXfont *fontMedium = &Minecraft_Rus_NEW4pt7b;
static const GFXfont *fontMediumBold = &muHeavy8ptBold;
static const GFXfont *fontBig = &dig_11;
static const GFXfont *fontBiggest = &dig_14;

void UI_ClearStatus() { memset(gFrameBuffer[0], 0, sizeof(gFrameBuffer[0])); }
void UI_ClearScreen() { memset(gFrameBuffer, 0, sizeof(gFrameBuffer)); }

void PutPixel(uint8_t x, uint8_t y, uint8_t fill) {
  if (fill == 1) {
    gFrameBuffer[y >> 3][x] |= 1 << (y & 7);
  } else if (fill == 2) {
    gFrameBuffer[y >> 3][x] ^= 1 << (y & 7);
  } else {
    gFrameBuffer[y >> 3][x] &= ~(1 << (y & 7));
  }
}

static void DrawALine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                      int16_t color) {
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    _swap_int16_t(x0, y0);
    _swap_int16_t(x1, y1);
  }

  if (x0 > x1) {
    _swap_int16_t(x0, x1);
    _swap_int16_t(y0, y1);
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
      _swap_int16_t(y0, y1);
    DrawVLine(x0, y0, y1 - y0 + 1, color);
  } else if (y0 == y1) {
    if (x0 > x1)
      _swap_int16_t(x0, x1);
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
                      uint16_t bg, uint8_t size_x, uint8_t size_y,
                      const GFXfont *gfxFont) {
  c -= gfxFont->first;
  GFXglyph *glyph = &gfxFont->glyph[c];
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
      GFXglyph *glyph = &gfxFont->glyph[c - first];
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
           Color color, uint8_t bg, const GFXfont *gfxFont) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y += (int16_t)textsize_y * gfxFont->yAdvance;
  } else if (c != '\r') {
    uint8_t first = gfxFont->first;
    if ((c >= first) && (c <= gfxFont->last)) {
      GFXglyph *glyph = &gfxFont->glyph[c - first];
      uint8_t w = glyph->width, h = glyph->height;
      if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
        int16_t xo = glyph->xOffset;
        if (wrap && ((cursor_x + textsize_x * (xo + w)) > LCD_WIDTH)) {
          cursor_x = 0;
          cursor_y += (int16_t)textsize_y * gfxFont->yAdvance;
        }
        m_putchar(cursor_x, cursor_y, c, color, bg, textsize_x, textsize_y,
                  gfxFont);
      }
      cursor_x += glyph->xAdvance * (int16_t)textsize_x;
    }
  }
}

void moveTo(uint8_t x, uint8_t y) {
  cursor_x = x;
  cursor_y = y;
}

static void printString(const GFXfont *gfxFont, uint8_t x, uint8_t y,
                        Color color, TextPos posLCR, const char *pattern,
                        va_list args) {
  char String[256];
  vsnprintf(String, 255, pattern, args);

  int16_t x1, y1;
  uint16_t w, h;
  getTextBounds(String, x, y, &x1, &y1, &w, &h, false,
                gfxFont); // calc width of new string
  if (posLCR == 1) {      // centered
    x = x - w / 2;
  } else if (posLCR == 2) {
    x = x - w;
  }
  moveTo(x, y);
  for (uint8_t i = 0; i < strlen(String); i++) {
    write(String[i], 1, 1, true, color, false, gfxFont);
  }
}

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontSmall, x, y, true, 0, pattern, args);
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
  printString(fontMedium, x, y, true, 0, pattern, args);
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
  printString(fontMediumBold, x, y, true, 0, pattern, args);
  va_end(args);
}

void PrintBigDigits(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBig, x, y, true, 0, pattern, args);
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
  printString(fontBiggest, x, y, true, 0, pattern, args);
  va_end(args);
}

void PrintBiggestDigitsEx(uint8_t x, uint8_t y, TextPos posLCR, Color color,
                          const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(fontBiggest, x, y, color, posLCR, pattern, args);
  va_end(args);
}

static void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners,
                             int16_t delta, Color color) {

  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;
  int16_t px = x;
  int16_t py = y;

  delta++; // Avoid some +1's in the loop

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    // These checks avoid double-drawing certain lines, important
    // for the SSD1306 library which has an INVERT drawing mode.
    if (x < (y + 1)) {
      if (corners & 1)
        DrawVLine(x0 + x, y0 - y, 2 * y + delta, color);
      if (corners & 2)
        DrawVLine(x0 - x, y0 - y, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1)
        DrawVLine(x0 + py, y0 - px, 2 * px + delta, color);
      if (corners & 2)
        DrawVLine(x0 - py, y0 - px, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

static void drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
                             uint8_t cornername, Color color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    if (cornername & 0x4) {
      PutPixel(x0 + x, y0 + y, color);
      PutPixel(x0 + y, y0 + x, color);
    }
    if (cornername & 0x2) {
      PutPixel(x0 + x, y0 - y, color);
      PutPixel(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      PutPixel(x0 - y, y0 + x, color);
      PutPixel(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      PutPixel(x0 - y, y0 - x, color);
      PutPixel(x0 - x, y0 - y, color);
    }
  }
}

void DrawCircle(int16_t x0, int16_t y0, int16_t r, Color color) {
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  PutPixel(x0, y0 + r, color);
  PutPixel(x0, y0 - r, color);
  PutPixel(x0 + r, y0, color);
  PutPixel(x0 - r, y0, color);

  while (x < y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    PutPixel(x0 + x, y0 + y, color);
    PutPixel(x0 - x, y0 + y, color);
    PutPixel(x0 + x, y0 - y, color);
    PutPixel(x0 - x, y0 - y, color);
    PutPixel(x0 + y, y0 + x, color);
    PutPixel(x0 - y, y0 + x, color);
    PutPixel(x0 + y, y0 - x, color);
    PutPixel(x0 - y, y0 - x, color);
  }
}

void FillCircle(int16_t x0, int16_t y0, int16_t r, Color color) {
  DrawVLine(x0, y0 - r, 2 * r + 1, color);
  fillCircleHelper(x0, y0, r, 3, 0, color);
}

void DrawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  DrawHLine(x + r, y, w - 2 * r, color);         // Top
  DrawHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
  DrawVLine(x, y + r, h - 2 * r, color);         // Left
  DrawVLine(x + w - 1, y + r, h - 2 * r, color); // Right
  // draw four corners
  drawCircleHelper(x + r, y + r, r, 1, color);
  drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
  drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void FillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r,
                   Color color) {
  int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
  if (r > max_radius)
    r = max_radius;
  // smarter version
  FillRect(x + r, y, w - 2 * r, h, color);
  // draw four corners
  fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}
