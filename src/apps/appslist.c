#include "appslist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

static const uint8_t MENU_SIZE = ARRAY_SIZE(appsAvailableToRun);

static uint8_t menuIndex = 0;

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, apps[appsAvailableToRun[index]].name, 31);
}

void APPSLIST_render() {
  UI_ClearScreen();
  UI_ShowMenu(getMenuItemText, MENU_SIZE, menuIndex);
}

void APPSLIST_init() { gRedrawScreen = true; }
void APPSLIST_update() {}
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  case KEY_MENU:
    APPS_exit();
    APPS_run(appsAvailableToRun[menuIndex]);
    return true;
  case KEY_EXIT:
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
}
