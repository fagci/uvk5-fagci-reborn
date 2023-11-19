#include "settings.h"
#include "../driver/st7565.h"
#include "../misc.h"
#include "../radio.h"
#include "../settings.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"

typedef enum {
  M_NONE,
  M_UPCONVERTER,
  M_RESET,
} Menu;

#define ITEMS(value)                                                           \
  for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                            \
    strncpy(items[i], value[i], 15);                                           \
  }                                                                            \
  size = ARRAY_SIZE(value);                                                    \
  type = MT_ITEMS;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static const MenuItem menu[] = {
    {"Upconverter", M_UPCONVERTER, 3},
    {"EEPROM reset", M_RESET},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_UPCONVERTER: {
    uint32_t f = GetScreenF(gCurrentVfo.fRX);
    gSettings.upconverter = subMenuIndex;
    RADIO_TuneTo(GetTuneF(f), true);
    SETTINGS_Save();
    isSubMenu = false;
  }; break;
  default:
    break;
  }
}

static const char *getValue(Menu type) {
  switch (type) {
  case M_UPCONVERTER:
    return upConverterFreqNames[gSettings.upconverter];
  default:
    break;
  }
  return "";
}

static void subUpconverter() {
  char items[5][16] = {0};
  for (uint8_t i = 0; i < ARRAY_SIZE(upConverterFreqNames); ++i) {
    strncpy(items[i], upConverterFreqNames[i], 15);
  }
  UI_ShowItems(items, ARRAY_SIZE(upConverterFreqNames), subMenuIndex);
}

static void showSubmenu(Menu menu) {
  switch (menu) {
  case M_UPCONVERTER:
    subUpconverter();
    break;
  default:
    break;
  }
}

void SETTINGS_init() {}
void SETTINGS_update() {}
bool SETTINGS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {

  if (!bKeyPressed || bKeyHeld) {
    return;
  }
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  const uint8_t SUBMENU_SIZE = item->size;
  switch (key) {
  case KEY_UP:
    if (isSubMenu) {
      subMenuIndex = subMenuIndex == 0 ? SUBMENU_SIZE - 1 : subMenuIndex - 1;
    } else {
      menuIndex = menuIndex == 0 ? MENU_SIZE - 1 : menuIndex - 1;
    }
    gRedrawScreen = true;
    break;
  case KEY_DOWN:
    if (isSubMenu) {
      subMenuIndex = subMenuIndex == SUBMENU_SIZE - 1 ? 0 : subMenuIndex + 1;
    } else {
      menuIndex = menuIndex == MENU_SIZE - 1 ? 0 : menuIndex + 1;
    }
    gRedrawScreen = true;
    break;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_RESET:
      APPS_run(APP_RESET);
      break;
    default:
      break;
    }
    if (isSubMenu) {
      accept();
    } else {
      isSubMenu = true;
    }
    gRedrawStatus = true;
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_run(gPreviousApp);
    }
    gRedrawScreen = true;
    gRedrawStatus = true;
    break;
  default:
    break;
  }
}
void SETTINGS_render() {
  UI_ClearScreen();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    showSubmenu(item->type);
    UI_PrintStringSmallest(item->name, 0, 0, true, true);
  } else {
    UI_ShowMenu(menu, ARRAY_SIZE(menu), menuIndex);
    UI_PrintStringSmall(getValue(item->type), 1, 126, 6);
  }
}
