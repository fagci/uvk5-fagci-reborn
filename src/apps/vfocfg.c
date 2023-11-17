#include "vfocfg.h"
#include "../driver/st7565.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include "textinput.h"

typedef enum {
  M_F_RX, // uint32_t fRX : 32;
  M_F_TX, // uint32_t fTX : 32;
  M_NAME, // char name[16];
  // uint8_t memoryBanks : 8;
  // uint8_t step : 8;
  // uint8_t modulation : 4;
  // uint8_t bw : 2;
  // uint8_t power : 2;
  // uint8_t codeRx : 8;
  // uint8_t codeTx : 8;
  // uint8_t codeTypeRx : 4;
  // uint8_t codeTypeTx : 4;
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

MenuItem menu[] = {
    {"RX freq", M_F_RX},
    {"TX freq", M_F_TX},
    {"Name", M_NAME},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  /* case M_UPCONVERTER: {
    uint32_t f = GetScreenF(gCurrentVfo.fRX);
    gUpconverterType = subMenuIndex;
    RADIO_TuneTo(GetTuneF(f), true);
    isSubMenu = false;
  }; break; */
  default:
    break;
  }
}

char Output[16];
static const char *getValue(Menu type) {
  switch (type) {
  case M_F_RX:
    sprintf(Output, "%u.%05u", gCurrentVfo.fRX / 100000,
            gCurrentVfo.fRX % 100000);
    return Output;
  case M_F_TX:
    sprintf(Output, "%u.%05u", gCurrentVfo.fTX / 100000,
            gCurrentVfo.fTX % 100000);
    return Output;
  case M_NAME:
    return gCurrentVfo.name;
  default:
    break;
  }
  return "";
}

/* static void showSubmenu(Menu menu) {
  const char **items;
  uint8_t size;
  MenuItemType type;

  switch (menu) {
  case M_UPCONVERTER:
    ITEMS(upConverterFreqNames);
    break;
  default:
    break;
  }

  switch (type) {
  case MT_ITEMS:
    UI_ShowItems(items, size, subMenuIndex);
    break;
  default:
    break;
  }
} */

void VFOCFG_init() {}
void VFOCFG_update() {}
void VFOCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
    case M_NAME:
      gTextinputText = gCurrentVfo.name;
      APPS_run(APP_TEXTINPUT);
      return;
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
void VFOCFG_render() {
  memset(gStatusLine, 0, sizeof(gStatusLine));
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    // showSubmenu(item->type);
    UI_PrintStringSmallest(item->name, 0, 0, true, true);
  } else {
    UI_ShowMenu(menu, ARRAY_SIZE(menu), menuIndex);
    UI_PrintStringSmall(getValue(item->type), 1, 126, 6);
  }
}
