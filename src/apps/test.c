#include "../driver/eeprom.h"
#include "../driver/keyboard.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../settings.h"
#include "../ui/graphics.h"
#include "apps.h"

void TEST_Init(void) {}

void TEST_Update(void) { gRedrawScreen = true; }

static uint32_t page = 0;
const uint8_t PAGE_SZ = 64;

static void TEST_Update(void) { gRedrawScreen = true; }

static void TEST_Render(void) {
  UI_ClearScreen();
  uint8_t buf[64] = {0};
  EEPROM_ReadBuffer(page * PAGE_SZ, buf, PAGE_SZ);

  for (uint8_t i = 0; i < PAGE_SZ; ++i) {
    uint32_t offset = i + page * PAGE_SZ;
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

static bool TEST_key(KEY_Code_t k, bool p, bool h) {
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_UP:
    IncDec32(&page, 0, SETTINGS_GetEEPROMSize() / PAGE_SZ, -1);
    return true;
  case KEY_DOWN:
    IncDec32(&page, 0, SETTINGS_GetEEPROMSize() / PAGE_SZ, 1);
    return true;
  case KEY_MENU:
    return false;
  default:
    break;
  }
  return true;
}

static VFO vfo;

REGISTER_APP({
    .id = APP_TEST,
    .name = "TEST",
    // .init = TEST_init,
    .update = TEST_Update,
    .render = TEST_Render,
    .key = TEST_key,
    // .deinit = TEST_deinit,
    .vfo = &vfo,
})
