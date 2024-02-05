#include "test.h"
#include "../driver/eeprom.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "apps.h"

void TEST_Init(void) {}

void TEST_Update(void) { gRedrawScreen = true; }

static uint16_t page = 0;

void TEST_Render(void) {
  UI_ClearScreen();
  const uint8_t PAGE_SZ = 64;
  uint8_t buf[64] = {0};
  EEPROM_ReadBuffer(page * PAGE_SZ, buf, PAGE_SZ);

  for (uint8_t i = 0; i < PAGE_SZ; ++i) {
    uint16_t offset = i + page * PAGE_SZ;
    if (i % 8 == 0) {
      PrintSmall(0, (i / 8) * 6 + 8 + 5, "%u", page * PAGE_SZ + i);
    }

    PrintSmall(16 + (i % 8) * 9, (i / 8) * 6 + 8 + 5, "%02x", buf[i]);
    PrintSmall(88 + (i % 8) * 5, (i / 8) * 6 + 8 + 5, "%c",
               buf[i] >= 32 && buf[i] < 128 ? buf[i] : '.');
    if (offset == SETTINGS_OFFSET || offset == VFOS_OFFSET ||
        offset == PRESETS_OFFSET ||
        (offset >= PRESETS_OFFSET &&
         (offset - PRESETS_OFFSET) % PRESET_SIZE == 0)) {
      FillRect(16 + (i % 8) * 9 - 1, (i / 8) * 6 + 8, 9, 7, C_INVERT);
    }
  }
}

bool TEST_key(KEY_Code_t k, bool p, bool h) {
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_UP:
    IncDec16(&page, 0, 128, -1);
    return true;
  case KEY_DOWN:
    IncDec16(&page, 0, 128, 1);
    return true;
  case KEY_MENU:
    return false;
  default:
    break;
  }
  return true;
}
