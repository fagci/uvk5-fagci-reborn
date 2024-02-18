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

static const uint8_t MENU_SIZE = ARRAY_SIZE(appsAvailableToRun);

static uint8_t menuIndex = 0;

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, apps[appsAvailableToRun[index]].name, 31);
}

void APPSLIST_render(void) {
  UI_ClearScreen();

  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText(apps[APP_APPS_LIST].name);
  }

  UI_ShowMenu(getMenuItemText, MENU_SIZE, menuIndex);
}

static void setMenuIndexAndRun(uint16_t v) {
  menuIndex = v - 1;
  APPS_exit();
  APPS_runManual(appsAvailableToRun[menuIndex]);
}

void APPSLIST_init(void) { gRedrawScreen = true; }
void APPSLIST_update(void) {}
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key >= KEY_0 && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      menuIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }

  AppType_t app = appsAvailableToRun[menuIndex];
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  case KEY_MENU:
    APPS_exit();
    if (app == APP_PRESETS_LIST || app == APP_LOOT_LIST ||
        app == APP_SCANLISTS) {
      APPS_run(app);
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
