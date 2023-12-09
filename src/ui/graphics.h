#ifndef GRAPHICS_H

#define GRAPHICS_H

#include "gfxfont.h"
#include <stdint.h>
void m_putchar(int16_t x, int16_t y, unsigned char c, uint16_t color,
                uint16_t bg, uint8_t size, const GFXfont *font);
void m_putrect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void write(uint8_t c, uint8_t textsize_x, uint8_t textsize_y, bool wrap,
           uint8_t color, uint8_t bg, const GFXfont *gfxFont);
void moveTo(uint8_t x, uint8_t y);

#endif /* end of include guard: GRAPHICS_H */
