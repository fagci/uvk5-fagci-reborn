#include "appslist.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include <string.h>

typedef enum {
  M_NONE,
  M_VFO,
  M_SPECTRUM,
  M_STILL,
  M_TASK_MANAGER,
} Menu;

static uint8_t menuIndex = 0;

static const MenuItem menu[] = {
    {"VFO", M_VFO},
    {"Spectrum", M_SPECTRUM},
    {"Still", M_STILL},
    {"Task manager", M_TASK_MANAGER},
};

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, menu[index].name, 31);
}

void APPSLIST_render() {
  UI_ClearScreen();
  UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
}

void APPSLIST_init() { gRedrawScreen = true; }
void APPSLIST_update() {}
bool APPSLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_VFO:
      APPS_exit();
      APPS_run(APP_VFO);
      return true;
    case M_SPECTRUM:
      APPS_exit();
      APPS_run(APP_SPECTRUM);
      return true;
    case M_STILL:
      APPS_exit();
      APPS_run(APP_STILL);
      return true;
    case M_TASK_MANAGER:
      APPS_exit();
      APPS_run(APP_TASK_MANAGER);
      return true;
    default:
      return true;
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
