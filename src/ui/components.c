#include "components.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "helper.h"

static const uint8_t MENU_LINES_TO_SHOW = 6;

void UI_Battery(uint8_t Level) {
  const uint8_t START = 115;
  const uint8_t WORK_START = START + 2;
  const uint8_t WORK_WIDTH = 10;
  const uint8_t WORK_END = WORK_START + WORK_WIDTH;

  gStatusLine[START] = 0b000001110;
  gStatusLine[START + 1] = 0b000011111;
  gStatusLine[WORK_END] = 0b000011111;

  Level <<= 1;

  for (uint8_t i = 1; i <= WORK_WIDTH; ++i) {
    if (Level >= i) {
      gStatusLine[WORK_END - i] = 0b000011111;
    } else {
      gStatusLine[WORK_END - i] = 0b000010001;
    }
  }

  // coz x2 earlier
  if (Level > 10) {
    gStatusLine[WORK_START + 1] &= 0b11110111;
    gStatusLine[WORK_START + 2] &= 0b11110111;
    gStatusLine[WORK_START + 3] &= 0b11110111;
    gStatusLine[WORK_START + 4] &= 0b11110111;
    gStatusLine[WORK_START + 5] &= 0b11110111;

    gStatusLine[WORK_START + 4] &= 0b11111011;

    gStatusLine[WORK_START + 3] &= 0b11111101;
    gStatusLine[WORK_START + 4] &= 0b11111101;
    gStatusLine[WORK_START + 5] &= 0b11111101;
    gStatusLine[WORK_START + 6] &= 0b11111101;
    gStatusLine[WORK_START + 7] &= 0b11111101;
    gStatusLine[WORK_START + 8] &= 0b11111101;
  }
}

void UI_RSSIBar(int16_t rssi, uint8_t line) {
  char String[16];

  const uint8_t BAR_LEFT_MARGIN = 24;
  const uint8_t POS_Y = line * 8 + 1;

  int dBm = Rssi2DBm(rssi);
  uint8_t s = DBm2S(dBm);
  uint8_t *ln = gFrameBuffer[line];

  memset(ln, 0, LCD_WIDTH);

  for (int i = BAR_LEFT_MARGIN, sv = 1; i < BAR_LEFT_MARGIN + s * 5;
       i += 5, sv++) {
    ln[i] = ln[i + 3] = 0b00111110;
    ln[i + 1] = ln[i + 2] = sv > 9 ? 0b00100010 : 0b00111110;
  }

  sprintf(String, "%d", dBm);
  UI_PrintStringSmallest(String, 110, POS_Y, false, true);
  if (s < 10) {
    sprintf(String, "S%u", s);
  } else {
    sprintf(String, "S9+%u0", s - 9);
  }
  UI_PrintStringSmallest(String, 3, POS_Y, false, true);
}

void UI_F(uint32_t f, uint8_t line) {
  char String[16];

  uint8_t i;

  for (i = 0; i < 9; i++) {
    uint32_t Result = f / 10U;

    String[8 - i] = f - (Result * 10U);
    f = Result;
  }

  UI_DisplayFrequency(String, 19, line, false, false);
  UI_DisplaySmallDigits(2, String + 7, 113, line + 1);
}

void UI_FSmall(uint32_t f) {
  char String[16];
  UI_PrintStringSmallest(modulationTypeOptions[gCurrentVfo.modulation], 116, 2,
                         false, true);
  UI_PrintStringSmallest(bwNames[gCurrentVfo.bw], 108, 8, false, true);

  sprintf(String, "%u.%05u", f / 100000, f % 100000);

  UI_PrintStringSmall(String, 8, 127, 0);
}

void UI_DrawScrollBar(const uint8_t size, const uint8_t currentIndex,
                      const uint8_t linesCount) {
  uint8_t i;
  const uint8_t scrollbarPosY =
      ConvertDomain(currentIndex, 0, size, 0, linesCount * 8 + 5);

  for (i = 0; i < linesCount; i++) {
    gFrameBuffer[i][126] = 0xFF;
  }

  for (i = 0; i < 3; i++) {
    PutPixel(127, scrollbarPosY + i, true);
    PutPixel(125, scrollbarPosY + i, true);
  }
}

void UI_ShowMenuItem(uint8_t line, const char *name, bool isCurrent) {
  if (isCurrent) {
    gFrameBuffer[line][0] = 0b01111111;
    gFrameBuffer[line][1] = 0b00111110;
    gFrameBuffer[line][2] = 0b00011100;
    gFrameBuffer[line][3] = 0b00001000;
    UI_PrintStringSmallBold(name, 6, 6, line);
  } else {
    UI_PrintStringSmall(name, 6, 6, line);
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
  const uint8_t maxItems =
      size < MENU_LINES_TO_SHOW ? size : MENU_LINES_TO_SHOW;
  const uint8_t offset = Clamp(currentIndex - 2, 0, size - maxItems);

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < maxItems; ++i) {
    uint8_t itemIndex = i + offset;
    UI_ShowMenuItem(i, items[itemIndex], currentIndex == itemIndex);
  }

  UI_DrawScrollBar(size, currentIndex, MENU_LINES_TO_SHOW);
}
