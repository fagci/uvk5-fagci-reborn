#include "appslist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../misc.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <string.h>

static uint8_t menuIndex = 0;

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, appsAvailableToRun[index]->name, 31);
}

void APPSLIST_render() {
  UI_ClearScreen();

  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }

  UI_ShowMenu(getMenuItemText, appsToRunCount, menuIndex);
}

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  APPS_exit();
  APPS_runManual(appsAvailableToRun[menuIndex]);
}

void APPSLIST_init() { gRedrawScreen = true; }
void APPSLIST_update() {}
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, appsToRunCount);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }

  App *app = appsAvailableToRun[menuIndex];
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, appsToRunCount, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, appsToRunCount, 1);
    return true;
  case KEY_MENU:
    APPS_exit();
    if (appid == APP_BANDS_LIST || appid == APP_LOOT_LIST ||
        appid == APP_SCANLISTS) {
      APPS_RunPure(app);
    } else {
      APPS_runManual(app);
    }
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
}


static App meta = {
    .id = APP_APPSLIST,
    .name = "Apps",
    .init = APPSLIST_init,
    .update = APPSLIST_update,
    .render = APPSLIST_render,
    .key = APPSLIST_key,
};

App *APPSLIST_Meta() { return &meta; }
