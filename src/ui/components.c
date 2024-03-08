#include "components.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "globals.h"
#include "graphics.h"

void UI_Battery(uint8_t Level) {
  DrawRect(LCD_WIDTH - 13, 0, 12, 5, C_FILL);
  FillRect(LCD_WIDTH - 12, 1, Level, 3, C_FILL);
  DrawVLine(LCD_WIDTH - 1, 1, 3, C_FILL);

  if (Level > 10) {
    DrawHLine(LCD_WIDTH - 1 - 3, 1, 3, C_INVERT);
    DrawHLine(LCD_WIDTH - 1 - 7, 1, 5, C_INVERT);
    DrawHLine(LCD_WIDTH - 1 - 3, 3, 3, C_INVERT);
  }
}

void UI_RSSIBar(uint16_t rssi, uint32_t f, uint8_t y) {
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
               modulationTypeOptions[radio->modulation]);
  PrintSmallEx(LCD_WIDTH - 1, 21, 2, true, BW_NAMES[radio->bw]);

  uint16_t step = StepFrequencyTable[radio->step];

  PrintSmall(0, 21, "%u.%02uk", step / 100, step % 100);

  if (gSettings.upconverter) {
    UI_FSmallest(radio->f, 32, 21);
  }

  PrintSmall(74, 21, "SQ:%u", radio->sq.level);

  PrintMediumEx(64, 15, 1, true, "%u.%05u", f / 100000, f % 100000);
}

void UI_FSmallest(uint32_t f, uint8_t x, uint8_t y) {
  PrintSmall(x, y, "%u.%05u", f / 100000, f % 100000);
}

void drawTicks(uint8_t x1, uint8_t x2, uint8_t y, uint32_t fs, uint32_t fe,
               uint32_t div, uint8_t h) {
  for (uint32_t f = fs - (fs % div) + div; f < fe; f += div) {
    uint8_t x = ConvertDomain(f, fs, fe, x1, x2);
    DrawVLine(x, y, h, C_FILL);
  }
}

void UI_DrawTicks(uint8_t x1, uint8_t x2, uint8_t y, FRange *range) {
  // TODO: automatic ticks size determination

  uint32_t fs = range->start;
  uint32_t fe = range->end;
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
