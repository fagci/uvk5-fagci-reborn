#include "components.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "helper.h"

void UI_Battery(uint8_t Level) {
  const uint8_t START = 115;
  const uint8_t WORK_START = START + 2;
  const uint8_t WORK_WIDTH = 10;
  const uint8_t WORK_END = WORK_START + WORK_WIDTH;

  gStatusLine[START] |= 0b000001110;
  gStatusLine[START + 1] |= 0b000011111;
  gStatusLine[WORK_END] |= 0b000011111;

  Level <<= 1;

  for (uint8_t i = 1; i <= WORK_WIDTH; ++i) {
    if (Level >= i) {
      gStatusLine[WORK_END - i] |= 0b000011111;
    } else {
      gStatusLine[WORK_END - i] |= 0b000010001;
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

  memset(ln, 0, 128);

  for (int i = BAR_LEFT_MARGIN, sv = 1; i < BAR_LEFT_MARGIN + s * 4;
       i += 4, sv++) {
    ln[i] = ln[i + 2] = 0b00111110;
    ln[i + 1] = sv > 9 ? 0b00100010 : 0b00111110;
  }

  sprintf(String, "%d", dBm);
  UI_PrintStringSmallest(String, 110, POS_Y, false, true);
  if (s < 10) {
    sprintf(String, "S%u", s);
  } else {
    sprintf(String, "S9+%u0", s - 9);
  }
  UI_PrintStringSmallest(String, 3, POS_Y, false, true);
  ST7565_BlitFullScreen();
}
