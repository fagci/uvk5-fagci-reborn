#include "vfo.h"
#include "../helper/lootlist.h"
#include "../scheduler.h"
#include "../ui/graphics.h"

void VFO_init() {
  UI_ClearScreen();

  LOOT_Clear();
  LOOT_Add(40655000);
  LOOT_Add(123456700);
}

void VFO_update() {
  // asdasd
}

bool VFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  return false;
}

void renderExample() {
  const uint8_t BASE = 38;
  PrintBigDigitsEx(LCD_WIDTH - 1 - 18, BASE, POS_R, C_FILL, "1234.567");
  PrintMediumEx(LCD_WIDTH - 1 - 18, BASE + 8, POS_R, C_FILL, "123.456");
  PrintSmallEx(LCD_WIDTH - 1, BASE + 8, POS_R, C_FILL, "78");
  PrintSmallEx(LCD_WIDTH - 1, BASE - 10, POS_R, C_FILL, "WFM");
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "89");
}

void render2VFO() {
  uint8_t BASE = 24;

  for (uint8_t i = 0; i < 2; ++i) {
    Loot *item = LOOT_Item(i);

    uint8_t bl = BASE + 32 * i;

    uint16_t fp1 = item->vfo.fRX / 100000;
    uint16_t fp2 = item->vfo.fRX / 100 % 1000;
    uint8_t fp3 = item->vfo.fRX % 100;
    const char *mod = modulationTypeOptions[item->vfo.modulation];

    PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
    PrintMediumEx(LCD_WIDTH - 1, bl, POS_R, C_FILL, "%02u", fp3);
    PrintSmallEx(LCD_WIDTH - 1, bl - 8, POS_R, C_FILL, mod);
  }
}

void render1VFO() {
  const uint8_t BASE = 38;

  Loot *item = LOOT_Item(0);

  uint16_t fp1 = item->vfo.fRX / 100000;
  uint16_t fp2 = item->vfo.fRX / 10 % 10000;
  uint8_t fp3 = item->vfo.fRX % 100;
  const char *mod = modulationTypeOptions[item->vfo.modulation];

  PrintBiggestDigitsEx(LCD_WIDTH - 19, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                       fp2);
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
  PrintSmallEx(LCD_WIDTH - 1, BASE - 8, POS_R, C_FILL, mod);
}

void VFO_render() {
  UI_ClearScreen();
  uint8_t sz = LOOT_Size();
  if (sz == 1) {
    render1VFO();
  } else if (sz == 2) {
    render2VFO();
  }
}
