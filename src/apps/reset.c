#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../helper/measurements.h"
#include "../ui/helper.h"
#include "ARMCM0.h"
#include <string.h>

static const uint16_t BYTES_TOTAL = 0x2000;

static uint16_t bytesErased = 0;
static uint8_t buf[8];

void RESET_Init() {
  bytesErased = 0;
  memset(buf, 0xFF, sizeof(buf));
}

void RESET_Update() {
  EEPROM_WriteBuffer(bytesErased, buf, 8);
  bytesErased += 8;
  if (bytesErased >= BYTES_TOTAL) {
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render() {
  char String[16];
  UI_ClearScreen();
  sprintf(String, "%u%", bytesErased * 100 / BYTES_TOTAL);
  UI_PrintStringSmall(String, 0, 0, 2);

  memset(gFrameBuffer[3], 0b00111100,
         ConvertDomain(bytesErased, 0, 8196, 0, LCD_WIDTH));
}

bool RESET_key(KEY_Code_t k, bool p, bool h) {
  return false;
}
