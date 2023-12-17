#include "components.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "graphics.h"
#include <stdarg.h>
#include <string.h>

static const uint8_t MENU_ITEM_H = 11;
static const uint8_t MENU_Y = 8;
static const uint8_t MENU_LINES_TO_SHOW = 4;

void UI_Battery(uint8_t Level) {
  memset(gFrameBuffer[0], 0, 13);
  DrawRect(0, 0, 12, 5, C_FILL);
  FillRect(1, 1, ConvertDomain(Level, 0, 5, 0, 10), 3, C_FILL);
  DrawVLine(12, 1, 3, C_FILL);

  if (Level > 5) {
    DrawHLine(5, 1, 5, C_INVERT);
    PutPixel(6, 2, C_INVERT);
    DrawHLine(2, 3, 6, C_INVERT);
  }
}

void UI_RSSIBar(int16_t rssi, uint32_t f, uint8_t line) {
  const uint8_t BAR_LEFT_MARGIN = 24;
  const uint8_t POS_Y = line * 8 + 1;

  int dBm = Rssi2DBm(rssi);
  uint8_t s = DBm2S(dBm, f >= 3000000);
  uint8_t *ln = gFrameBuffer[line];

  memset(ln, 0, LCD_WIDTH);

  for (int i = BAR_LEFT_MARGIN, sv = 1; i < BAR_LEFT_MARGIN + s * 5;
       i += 5, sv++) {
    ln[i] = ln[i + 3] = 0b00111110;
    ln[i + 1] = ln[i + 2] = sv > 9 ? 0b00100010 : 0b00111110;
  }

  PrintMediumEx(LCD_WIDTH - 1, POS_Y + 5, 2, true, "%d", dBm);
  if (s < 10) {
    PrintMedium(0, POS_Y + 5, "S%u", s);
  } else {
    PrintMedium(0, POS_Y + 5, "S9+%u0", s - 9);
  }
}

void UI_FSmall(uint32_t f) {
  PrintSmallEx(LCD_WIDTH - 1, 15, 2, true,
               modulationTypeOptions[gCurrentVFO->modulation]);
  PrintSmallEx(LCD_WIDTH - 1, 21, 2, true, bwNames[gCurrentVFO->bw]);

  uint16_t step = StepFrequencyTable[gCurrentPreset->band.step];

  PrintSmall(0, 21, "%u.%02uk", step / 100, step % 100);

  UI_FSmallest(gCurrentVFO->fRX, 32, 21);

  PrintSmall(74, 21, "SQ:%u", gCurrentPreset->band.squelch);

  PrintMediumEx(64, 15, 1, true, "%u.%05u", f / 100000, f % 100000);
}

void UI_FSmallest(uint32_t f, uint8_t x, uint8_t y) {
  PrintSmall(x, y, "%u.%05u", f / 100000, f % 100000);
}

void UI_DrawScrollBar(const uint16_t size, const uint16_t iCurrent,
                      const uint8_t nLines) {
  const uint8_t sbY =
      ConvertDomain(iCurrent, 0, size, 0, nLines * MENU_ITEM_H - 3);

  DrawVLine(LCD_WIDTH - 2, MENU_Y, LCD_HEIGHT - MENU_Y, C_FILL);

  FillRect(LCD_WIDTH - 3, MENU_Y + sbY, 3, 3, C_FILL);
}

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent) {
  if (isCurrent) {
    PrintMediumBold(6, MENU_Y + line * MENU_ITEM_H + 8, name);
  } else {
    PrintMedium(6, MENU_Y + line * MENU_ITEM_H + 8, name);
  }
}

void UI_ShowMenu(const MenuItem *items, uint8_t size, uint8_t currentIndex) {

  const uint8_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint8_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < maxItems; ++i) {
    uint8_t itemIndex = i + offset;
    const MenuItem *item = &items[itemIndex];
    UI_ShowMenuItem(i, item->name, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_ShowItems(char (*items)[16], uint16_t size, uint16_t currentIndex) {
  const uint16_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    UI_ShowMenuItem(i, items[itemIndex], currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_ShowRangeItems(uint16_t size, uint16_t currentIndex) {
  char String[8];
  const uint16_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    sprintf(String, "%u", itemIndex);
    UI_ShowMenuItem(i, String, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_DrawTicks(uint8_t x1, uint8_t x2, uint8_t line, Band *band,
                  bool centerMode) {
  uint8_t width = (x2 - x1);
  uint8_t center = x1 + (width >> 1);
  uint8_t *pLine = gFrameBuffer[line];

  if (centerMode) {
    pLine[center - 2] |= 0x80;
    pLine[center - 1] |= 0x80;
    pLine[center] |= 0xff;
    pLine[center + 1] |= 0x80;
    pLine[center + 2] |= 0x80;
  } else {
    pLine[x1] |= 0xff;
    pLine[x1 + 1] |= 0x80;
    pLine[x2 - 2] |= 0x80;
    pLine[x2 - 1] |= 0xff;
  }

  uint32_t g1 = 10000;
  uint32_t g2 = 50000;
  uint32_t g3 = 100000;

  uint16_t stepSize = StepFrequencyTable[band->step];
  uint32_t bw = band->bounds.end - band->bounds.start;
  uint16_t stepsCount = bw / stepSize;

  for (uint16_t i = 0; i < stepsCount; ++i) {
    uint8_t x = width * i / stepsCount;
    uint32_t f = band->bounds.start + i * stepSize;
    (f % g1) < stepSize && (pLine[x] |= 0b00000001);
    (f % g2) < stepSize && (pLine[x] |= 0b00000011);
    (f % g3) < stepSize && (pLine[x] |= 0b00000111);
  }
}
