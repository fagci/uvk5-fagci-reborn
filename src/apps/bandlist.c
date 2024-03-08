#include "bandlist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/bandlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"

static uint8_t menuIndex = 0;

static void getBandText(int32_t i, char *name) {
  Band *item = BANDS_Item(i);
  uint32_t fs = item->band.bounds.start;
  uint32_t fe = item->band.bounds.end;
  if (item->band.name[0] > 32) {
    sprintf(name, "%s", item->band.name);
  } else {
    sprintf(name, "%u.%05u-%u.%05u", fs / 100000, fs % 100000, fe / 100000,
            fe % 100000);
  }
}

void BANDLIST_render() {
  UI_ClearScreen();
  UI_ShowMenu(getBandText, BANDS_Size(), menuIndex);
}

void BANDLIST_init() {
  gRedrawScreen = true;
  menuIndex = gSettings.activeBand;
}

void BANDLIST_update() {}

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  RADIO_SelectBandSave(menuIndex);
  APPS_exit();
}

bool BANDLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const uint8_t MENU_SIZE = BANDS_Size();
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_MENU:
    RADIO_SelectBandSave(menuIndex);
    APPS_exit();
    return true;
  case KEY_F:
    BAND_Select(menuIndex);
    APPS_run(APP_BAND_CFG);
    return true;
  default:
    break;
  }
  return false;
}


static App meta = {
    .id = APP_BANDS_LIST,
    .name = "Bands",
    .runnable = true,
    .init = BANDLIST_init,
    .update = BANDLIST_update,
    .render = BANDLIST_render,
    .key = BANDLIST_key,
};

App *BANDLIST_Meta() { return &meta; }
