#include "mainmenu.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include <string.h>

typedef enum {
  M_NONE,
  M_SPECTRUM,
  M_STILL,
  M_TASK_MANAGER,
} Menu;

#define ITEMS(value)                                                           \
  for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                            \
    strncpy(items[i], value[i], 15);                                           \
  }                                                                            \
  size = ARRAY_SIZE(value);                                                    \
  type = MT_ITEMS;

static uint8_t menuIndex = 0;

static const MenuItem menu[] = {
    {"Spectrum", M_SPECTRUM},
    {"Still", M_STILL},
    {"Task manager", M_TASK_MANAGER},
};

void MAINMENU_render() {
  UI_ClearScreen();
  UI_ShowMenu(menu, ARRAY_SIZE(menu), menuIndex);
}

void MAINMENU_init() {
    gRedrawScreen = true;
}
void MAINMENU_update() {}
bool MAINMENU_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return false;
  }
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  switch (key) {
  case KEY_UP:
    menuIndex = menuIndex == 0 ? MENU_SIZE - 1 : menuIndex - 1;
    gRedrawScreen = true;
    return true;
  case KEY_DOWN:
    menuIndex = menuIndex == MENU_SIZE - 1 ? 0 : menuIndex + 1;
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
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
    gRedrawScreen = true;
    gRedrawScreen = true;
    return true;
  case KEY_EXIT:
    APPS_exit();
    gRedrawScreen = true;
    gRedrawScreen = true;
    return true;
  default:
    break;
  }
  return false;
}
