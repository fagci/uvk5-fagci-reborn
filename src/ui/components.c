#include "components.h"
#include "../driver/st7565.h"

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

  if (Level > 5) {
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
