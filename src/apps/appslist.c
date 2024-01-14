#include "appslist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"
#include <string.h>

static const uint8_t MENU_SIZE = ARRAY_SIZE(appsAvailableToRun);

static uint8_t menuIndex = 0;

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, apps[appsAvailableToRun[index]].name, 15);
}

void APPSLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenu(getMenuItemText, MENU_SIZE, menuIndex);
}

void APPSLIST_init(void) { gRedrawScreen = true; }
void APPSLIST_update(void) {}
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
