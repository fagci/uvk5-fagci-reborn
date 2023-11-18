#include "reset.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
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
  EEPROM_WriteBuffer(bytesErased, buf);
  bytesErased += 8;
  if (bytesErased >= BYTES_TOTAL) {
    NVIC_SystemReset();
  }
  gRedrawScreen = true;
}

void RESET_Render() {
  char String[16];
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  sprintf(String, "%u%", bytesErased * 100 / BYTES_TOTAL);
  UI_PrintStringSmall(String, 0, 0, 2);
}

void RESET_Key(KEY_Code_t k, bool p, bool h) {}
