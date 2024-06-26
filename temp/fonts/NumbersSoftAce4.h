#include "../gfxfont.h"

const uint8_t SA_font4_Bitmaps[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xF0, 0xFF, 
  0xFF, 0xFF, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, 
  0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xFF, 0xFF, 0xFF, 0xF0, 0x1C, 0x0E, 
  0x07, 0x1F, 0x8F, 0xC7, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x81, 
  0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x80, 0xFF, 0xFF, 0xFF, 
  0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0xFF, 0xFF, 0xFF, 0xFC, 0x0E, 0x07, 
  0x03, 0x81, 0xC0, 0xE0, 0x7F, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xE0, 
  0x70, 0x38, 0x1C, 0x0E, 0x07, 0x1F, 0x8F, 0xC7, 0xE0, 0x70, 0x38, 0x1C, 
  0x0E, 0x07, 0x03, 0xFF, 0xFF, 0xFF, 0xF0, 0xE3, 0xF1, 0xF8, 0xFC, 0x7E, 
  0x3F, 0x1F, 0x8F, 0xC7, 0xFF, 0xFF, 0xFF, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 
  0x07, 0x03, 0x81, 0xC0, 0xE0, 0x70, 0xFF, 0xFF, 0xFF, 0xFC, 0x0E, 0x07, 
  0x03, 0x81, 0xC0, 0xFF, 0xFF, 0xFF, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 
  0x03, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFC, 0x0E, 0x07, 0x03, 
  0x81, 0xC0, 0xFF, 0xFF, 0xFF, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 
  0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 
  0x07, 0x03, 0x81, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0x81, 
  0xC0, 0xE0, 0x70, 0xFF, 0xFF, 0xFF, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 
  0xFF, 0xFF, 0xFF, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xFF, 0xFF, 
  0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFC, 0x7E, 0x3F, 0x1F, 0x8F, 0xC7, 0xFF, 
  0xFF, 0xFF, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, 0xFF, 0xFF, 0xFF, 
  0xF0
};

const GFXglyph SA_font4_Glyphs[] PROGMEM = {
  {     0,   6,  20,   7,    0,  -19 },   // 0x2D '-'
  {    15,   3,  20,   4,    0,  -19 },   // 0x2E '.'
  {     0,   0,   0,   0,    0,    0 },   // 0x2F '/'
  {    23,   9,  20,  10,    0,  -19 },   // 0x30 '0'
  {    46,   9,  20,  10,    0,  -19 },   // 0x31 '1'
  {    69,   9,  20,  10,    0,  -19 },   // 0x32 '2'
  {    92,   9,  20,  10,    0,  -19 },   // 0x33 '3'
  {   115,   9,  20,  10,    0,  -19 },   // 0x34 '4'
  {   138,   9,  20,  10,    0,  -19 },   // 0x35 '5'
  {   161,   9,  20,  10,    0,  -19 },   // 0x36 '6'
  {   184,   9,  20,  10,    0,  -19 },   // 0x37 '7'
  {   207,   9,  20,  10,    0,  -19 },   // 0x38 '8'
  {   230,   9,  20,  10,    0,  -19 }    // 0x39 '9'
};

const GFXfont SA_font4 PROGMEM = {(uint8_t *) SA_font4_Bitmaps,   (GFXglyph *)SA_font4_Glyphs, 0x2D, 0x39,   21};
