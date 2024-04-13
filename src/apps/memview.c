#include "memview.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "apps.h"

static uint32_t page = 0;
static const uint8_t PAGE_SZ = 64;
static uint16_t pagesCount;

void MEMVIEW_Init(void) { pagesCount = SETTINGS_GetEEPROMSize() / PAGE_SZ; }

void MEMVIEW_Update(void) {}

void MEMVIEW_Render(void) {
  uint8_t buf[64] = {0};
  EEPROM_ReadBuffer(page * PAGE_SZ, buf, PAGE_SZ);

  UI_ClearScreen();
  for (uint8_t i = 0; i < PAGE_SZ; ++i) {
    uint32_t offset = i + page * PAGE_SZ;
    uint8_t col = i % 8;
    uint8_t row = i / 8;
    uint8_t rowYBL = row * 6 + 8 + 5;

    if (i % 8 == 0) {
      PrintSmall(0, rowYBL, "%u", page * PAGE_SZ + i);
    }

    PrintSmall(16 + col * 9, rowYBL, "%02x", buf[i]);
    PrintSmall(88 + col * 5, rowYBL, "%c",
               buf[i] >= 32 && buf[i] < 128 ? buf[i] : '.');
    if (offset == SETTINGS_OFFSET || offset == VFOS_OFFSET ||
        offset == PRESETS_OFFSET ||
        (offset >= PRESETS_OFFSET &&
         (offset - PRESETS_OFFSET) % PRESET_SIZE == 0)) {
      FillRect(16 + col * 9 - 1, row * 6 + 8, 9, 7, C_INVERT);
    }
  }
}

bool MEMVIEW_key(KEY_Code_t k, bool p, bool h) {
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_UP:
    IncDec32(&page, 0, pagesCount, -1);
    return true;
  case KEY_DOWN:
    IncDec32(&page, 0, pagesCount, 1);
    return true;
  case KEY_MENU:
    return false;
  default:
    break;
  }
  return true;
}
