#include "channels.h"
#include "../driver/eeprom.h"
#include "../driver/uart.h"
#include "../external/printf/printf.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "vfos.h"
#include <string.h>

static uint16_t presetsSizeBytes() {
  return gSettings.presetsCount * PRESET_SIZE;
}

uint16_t CHANNELS_GetCountMax() {
  return (EEPROM_SIZE - PRESETS_OFFSET - presetsSizeBytes()) / CH_SIZE;
}

void CHANNELS_Load(uint16_t num, CH *p) {
  EEPROM_ReadBuffer(CHANNELS_OFFSET - (num + 1) * CH_SIZE, p, CH_SIZE);
}

void CHANNELS_Save(uint16_t num, CH *p) {
  EEPROM_WriteBuffer(CHANNELS_OFFSET - (num + 1) * CH_SIZE, p, CH_SIZE);
}

bool CHANNELS_Existing(uint16_t i) {
  CH v;
  uint16_t addr = CHANNELS_OFFSET - ((i + 3) * CH_SIZE);
  EEPROM_ReadBuffer(addr, &v, 4 + 4 + 1);
  return IsReadable(v.name);
}

int16_t CHANNELS_Next(int16_t base, bool next) {
  uint16_t si = base;
  uint16_t max = CHANNELS_GetCountMax();
  IncDec16(&si, 0, max, next ? 1 : -1);
  int16_t i = si;
  UART_printf("Next CH (i:%u/%u)\n", i, max);
  UART_flush();
  if (next) {
    for (; i < max; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = 0; i < base; ++i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  } else {
    for (; i >= 0; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
    for (i = max - 1; i > base; --i) {
      if (CHANNELS_Existing(i)) {
        return i;
      }
    }
  }
  return -1;
}

void CHANNELS_Delete(uint16_t i) {
  CH v = {0};
  CHANNELS_Save(i, &v);
}
