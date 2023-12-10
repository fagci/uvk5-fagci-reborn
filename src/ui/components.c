#include "components.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "graphics.h"
#include "helper.h"
#include <stdarg.h>

static const uint8_t MENU_LINES_TO_SHOW = 6;

void UI_Battery(uint8_t Level) {
  const uint8_t START = BATTERY_X;
  const uint8_t WORK_START = START + 2;
  const uint8_t WORK_WIDTH = 10;
  const uint8_t WORK_END = WORK_START + WORK_WIDTH;
  uint8_t *pLine = gFrameBuffer[0];

  pLine[START] = 0b000001110;
  pLine[START + 1] = 0b000011111;
  pLine[WORK_END] = 0b000011111;

  Level <<= 1;

  for (uint8_t i = 1; i <= WORK_WIDTH; ++i) {
    pLine[WORK_END - i] = Level >= i ? 0b000011111 : 0b000010001;
  }

  // coz x2 earlier
  if (Level > 10) {
    pLine[WORK_START + 1] &= 0b11110111;
    pLine[WORK_START + 2] &= 0b11110111;
    pLine[WORK_START + 3] &= 0b11110111;
    pLine[WORK_START + 4] &= 0b11110111;
    pLine[WORK_START + 5] &= 0b11110111;

    pLine[WORK_START + 4] &= 0b11111011;

    pLine[WORK_START + 3] &= 0b11111101;
    pLine[WORK_START + 4] &= 0b11111101;
    pLine[WORK_START + 5] &= 0b11111101;
    pLine[WORK_START + 6] &= 0b11111101;
    pLine[WORK_START + 7] &= 0b11111101;
    pLine[WORK_START + 8] &= 0b11111101;
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

  PrintSmall(110, POS_Y + 5, "%d", dBm);
  if (s < 10) {
    PrintSmall(3, POS_Y + 5, "S%u", s);
  } else {
    PrintSmall(3, POS_Y + 5, "S9+%u0", s - 9);
  }
}

void UI_FSmall(uint32_t f) {
  PrintSmall(116, 7, modulationTypeOptions[gCurrentVfo.modulation]);
  PrintSmall(108, 13, bwNames[gCurrentVfo.bw]);

  uint16_t step = StepFrequencyTable[gCurrentPreset->band.step];

  PrintSmall(0, 13, "%u.%02uk", step / 100, step % 100);

  UI_FSmallest(gCurrentVfo.fRX, 32, 8);

  PrintSmall(74, 13, "SQ:%u", gCurrentPreset->band.squelch);

  PrintMedium(8, 12, "%u.%05u", f / 100000, f % 100000);
}

void UI_FSmallest(uint32_t f, uint8_t x, uint8_t y) {
  PrintSmall(x, y, "%u.%05u", f / 100000, f % 100000);
}

void UI_DrawScrollBar(const uint16_t size, const uint16_t currentIndex,
                      const uint8_t linesCount) {
  uint8_t i;
  const uint8_t scrollbarPosY =
      ConvertDomain(currentIndex, 0, size, 0, linesCount * 8 - 3);

  for (i = 0; i < linesCount; i++) {
    gFrameBuffer[i][126] = 0xFF;
  }

  for (i = 0; i < 3; i++) {
    PutPixel(127, scrollbarPosY + i, true);
    PutPixel(125, scrollbarPosY + i, true);
  }
}

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent) {
  uint8_t *pLine = gFrameBuffer[line];
  if (isCurrent) {
    pLine[0] = 0b01111111;
    pLine[1] = 0b00111110;
    pLine[2] = 0b00011100;
    pLine[3] = 0b00001000;
    PrintMediumBold(6, line * 8 + 6 + 12, name);
  } else {
    PrintMedium(6, line * 8 + 6 + 12, name);
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
