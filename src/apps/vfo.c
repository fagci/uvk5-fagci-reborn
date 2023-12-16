#include "vfo.h"
#include "../helper/lootlist.h"
#include "../scheduler.h"
#include "../ui/graphics.h"

void VFO_init() {
  UI_ClearScreen();

  LOOT_Clear();
  LOOT_AddVFO((VFO){40655000, .modulation = MOD_RAW});
  LOOT_AddVFO((VFO){123456700, 0, "Test CH 2", .modulation = MOD_BYP});

  gCurrentVfo = LOOT_Item(0)->vfo;

  RADIO_SetupByCurrentVFO();
}

bool lastListenState = false;

void VFO_update() {
  bool listenState = BK4819_IsSquelchOpen();
  if (listenState != lastListenState) {
    gRedrawScreen = true;
    lastListenState = listenState;
    RADIO_ToggleRX(listenState);
  }
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

void render2VFOPart(uint8_t i) {
  uint8_t BASE = 24;
  Loot *item = LOOT_Item(i);
  bool isActive = i == gSettings.activeChannel;

  uint8_t bl = BASE + 32 * i;

  uint16_t fp1 = item->vfo.fRX / 100000;
  uint16_t fp2 = item->vfo.fRX / 100 % 1000;
  uint8_t fp3 = item->vfo.fRX % 100;
  const char *mod = modulationTypeOptions[item->vfo.modulation];

  if (isActive) {
    FillRect(0, bl - 13, 16, 7, C_FILL);
  }

  if (item->vfo.name[0] < 32 || item->vfo.name[0] > 127) {
    PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
    PrintMediumEx(LCD_WIDTH - 1, bl, POS_R, C_FILL, "%02u", fp3);
    PrintSmallEx(8, bl - 8, POS_C, C_INVERT, "VFO");
  } else {
    PrintMediumBoldEx(LCD_WIDTH / 2, bl - 8, POS_C, C_FILL, item->vfo.name);
    PrintMediumEx(LCD_WIDTH / 2, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
    PrintSmallEx(8, bl - 8, POS_C, C_INVERT, "MR");
  }
  PrintSmallEx(LCD_WIDTH - 1, bl - 8, POS_R, C_FILL, mod);
}

void render2VFO() {
  for (uint8_t i = 0; i < 2; ++i) {
    render2VFOPart(i);
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
