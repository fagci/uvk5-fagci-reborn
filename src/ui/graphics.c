#include "../driver/st7565.h"
#include "fonts/FreeSans9pt7b.h"
#include "fonts/FreeSansBold9pt7b.h"
#include "fonts/TomThumb.h"
#include "gfxfont.h"
#include "helper.h"
#include <stdint.h>
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

void DrawVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  DrawALine(x, y, x, y + h - 1, color);
}

void DrawHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  DrawALine(x, y, x + w - 1, y, color);
}

void DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
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

void DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  DrawHLine(x, y, w, color);
  DrawHLine(x, y + h - 1, w, color);
  DrawVLine(x, y, h, color);
  DrawVLine(x + w - 1, y, h, color);
}

void FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  int16_t i;

  for (i = x; i < x + w; i++) {
    DrawVLine(i, y, h, color);
  }
}

static void m_putchar(int16_t x, int16_t y, unsigned char c, uint16_t color,
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

void write(uint8_t c, uint8_t textsize_x, uint8_t textsize_y, bool wrap,
           uint8_t color, uint8_t bg, const GFXfont *gfxFont) {
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
                        const char *pattern, va_list args) {
  char String[256];
  vsnprintf(String, 255, pattern, args);

  moveTo(x, y);
  for (uint8_t i = 0; i < strlen(String); i++) {
    write(String[i], 1, 1, true, true, false, gfxFont);
  }
}

void PrintSmall(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(&TomThumb, x, y, pattern, args);
  va_end(args);
}

void PrintMedium(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(&FreeSans9pt7b, x, y, pattern, args);
  va_end(args);
}

void PrintMediumBold(uint8_t x, uint8_t y, const char *pattern, ...) {
  va_list args;
  va_start(args, pattern);
  printString(&FreeSansBold9pt7b, x, y, pattern, args);
  va_end(args);
}
