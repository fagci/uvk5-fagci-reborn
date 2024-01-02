#include "components.h"
#include "../driver/st7565.h"
#include "../external/printf/printf.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "graphics.h"
#include <stdarg.h>
#include <string.h>

static const uint8_t MENU_ITEM_H = 11;
static const uint8_t MENU_LINES_TO_SHOW = 4;

void UI_Battery(uint8_t Level) {
  DrawRect(0, 0, 12, 5, C_FILL);
  FillRect(1, 1, ConvertDomain(Level, 0, 5, 0, 10), 3, C_FILL);
  DrawVLine(12, 1, 3, C_FILL);

  if (Level > 5) {
    DrawHLine(5, 1, 5, C_INVERT);
    PutPixel(6, 2, C_INVERT);
    DrawHLine(2, 3, 6, C_INVERT);
  }
}

void UI_RSSIBar(int16_t rssi, uint32_t f, uint8_t y) {
  const uint8_t BAR_LEFT_MARGIN = 32;
  const uint8_t BAR_BASE = y + 7;

  int dBm = Rssi2DBm(rssi);
  uint8_t s = DBm2S(dBm, f >= 3000000);

  FillRect(0, y, LCD_WIDTH, 8, C_CLEAR);

  for (uint8_t i = 0; i < s; ++i) {
    if (i >= 9) {
      FillRect(BAR_LEFT_MARGIN + i * 5, y + 2, 4, 6, C_FILL);
    } else {
      FillRect(BAR_LEFT_MARGIN + i * 5, y + 3, 4, 4, C_FILL);
    }
  }

  PrintMediumEx(LCD_WIDTH - 1, BAR_BASE, 2, true, "%d", dBm);
  if (s < 10) {
    PrintMedium(0, BAR_BASE, "S%u", s);
  } else {
    PrintMedium(0, BAR_BASE, "S9+%u0", s - 9);
  }
}

void UI_FSmall(uint32_t f) {
  PrintSmallEx(LCD_WIDTH - 1, 15, 2, true,
               modulationTypeOptions[gCurrentPreset->band.modulation]);
  PrintSmallEx(LCD_WIDTH - 1, 21, 2, true, bwNames[gCurrentPreset->band.bw]);

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

void UI_ShowMenu(void (*getItemText)(uint16_t index, char *name), uint16_t size,
                 uint16_t currentIndex) {
  const uint16_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);
  char name[32] = {0};

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    getItemText(itemIndex, name);
    UI_ShowMenuItem(i, name, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}

void UI_ShowMenuEx(void (*showItem)(uint16_t i, uint16_t index, bool isCurrent),
                   uint16_t size, uint16_t currentIndex, uint16_t linesMax) {
  const uint16_t maxItems = size < linesMax ? size : linesMax;
  const uint16_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  for (uint16_t i = 0; i < maxItems; ++i) {
    uint16_t itemIndex = i + offset;
    showItem(i, itemIndex, currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, linesMax);
}

void drawTicks(uint8_t x1, uint8_t x2, uint8_t y, uint32_t fs, uint32_t fe,
               uint32_t div, uint8_t h) {
  for (uint32_t f = fs - (fs % div) + div; f < fe; f += div) {
    uint8_t x = ConvertDomain(f, fs, fe, x1, x2);
    DrawVLine(x, y, h, C_FILL);
  }
}

void UI_DrawTicks(uint8_t x1, uint8_t x2, uint8_t y, Band *band) {
  // TODO: automatic ticks size determination

  uint32_t fs = band->bounds.start;
  uint32_t fe = band->bounds.end;
  uint32_t bw = fe - fs;

  if (bw > 5000000) {
    drawTicks(x1, x2, y, fs, fe, 1000000, 3);
    drawTicks(x1, x2, y, fs, fe, 500000, 2);
  } else if (bw > 1000000) {
    drawTicks(x1, x2, y, fs, fe, 500000, 3);
    drawTicks(x1, x2, y, fs, fe, 100000, 2);
  } else if (bw > 500000) {
    drawTicks(x1, x2, y, fs, fe, 500000, 3);
    drawTicks(x1, x2, y, fs, fe, 100000, 2);
  } else {
    drawTicks(x1, x2, y, fs, fe, 100000, 3);
    drawTicks(x1, x2, y, fs, fe, 50000, 2);
  }
}
