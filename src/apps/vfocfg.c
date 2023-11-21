#include "vfocfg.h"
#include "../driver/st7565.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"
#include "finput.h"
#include "textinput.h"

typedef enum {
  M_F_RX, // uint32_t fRX : 32;
  M_F_TX, // uint32_t fTX : 32;
  M_NAME, // char name[16];
  // uint8_t memoryBanks : 8;
  M_STEP,       // uint8_t step : 8;
  M_MODULATION, // uint8_t modulation : 4;
  M_BW,         // uint8_t bw : 2;
                // uint8_t power : 2;
                // uint8_t codeRx : 8;
                // uint8_t codeTx : 8;
                // uint8_t codeTypeRx : 4;
                // uint8_t codeTypeTx : 4;
  M_SAVE,
} Menu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

MenuItem menu[] = {
    {"RX freq", M_F_RX},
    {"TX freq", M_F_TX},
    {"Name", M_NAME},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"Save", M_SAVE},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    gCurrentVfo.bw = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_MODULATION:
    gCurrentVfo.modulation = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_STEP:
    gCurrentVfo.step = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  default:
    break;
  }
}

static char Output[16];
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
  case M_BW:
    return bwNames[gCurrentVfo.bw];
  case M_MODULATION:
    return modulationTypeOptions[gCurrentVfo.modulation];
  case M_STEP:
    sprintf(Output, "%d.%02dKHz", StepFrequencyTable[gCurrentVfo.step] / 100,
            StepFrequencyTable[gCurrentVfo.step] % 100);
    return Output;
  default:
    break;
  }
  return "";
}

#define SHOW_ITEMS(value)                                                      \
  do {                                                                         \
    char items[ARRAY_SIZE(value)][16] = {0};                                   \
    for (uint8_t i = 0; i < ARRAY_SIZE(value); ++i) {                          \
      strncpy(items[i], value[i], 15);                                         \
    }                                                                          \
    UI_ShowItems(items, ARRAY_SIZE(value), subMenuIndex);                      \
  } while (0)

static void showStepValues() {
  char items[ARRAY_SIZE(StepFrequencyTable)][16] = {0};
  for (uint8_t i = 0; i < ARRAY_SIZE(StepFrequencyTable); ++i) {
    sprintf(items[i], "%d.%02dKHz", StepFrequencyTable[i] / 100,
            StepFrequencyTable[i] % 100);
  }
  UI_ShowItems(items, ARRAY_SIZE(StepFrequencyTable), subMenuIndex);
}

static void showSubmenu(Menu menu) {
  switch (menu) {
  case M_MODULATION:
    SHOW_ITEMS(modulationTypeOptions);
    return;
  case M_BW:
    SHOW_ITEMS(bwNames);
    return;
  case M_STEP:
    showStepValues();
    return;
  default:
    break;
  }
}

void VFOCFG_init() {}
void VFOCFG_update() {}
bool VFOCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || bKeyHeld) {
    return false;
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
    return true;
  case KEY_DOWN:
    if (isSubMenu) {
      subMenuIndex = subMenuIndex == SUBMENU_SIZE - 1 ? 0 : subMenuIndex + 1;
    } else {
      menuIndex = menuIndex == MENU_SIZE - 1 ? 0 : menuIndex + 1;
    }
    gRedrawScreen = true;
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_NAME:
      gTextinputText = gCurrentVfo.name;
      APPS_run(APP_TEXTINPUT);
      return true;
    case M_F_RX:
      gFInputValue = &gCurrentVfo.fRX;
      APPS_run(APP_FINPUT);
      return true;
    case M_F_TX:
      gFInputValue = &gCurrentVfo.fTX;
      APPS_run(APP_FINPUT);
      return true;
    case M_SAVE:
      APPS_run(APP_SAVECH);
      return true;
    default:
      break;
    }
    if (isSubMenu) {
      accept();
      isSubMenu = false;
    } else {
      isSubMenu = true;
    }
    gRedrawStatus = true;
    gRedrawScreen = true;
    return true;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_exit();
    }
    gRedrawScreen = true;
    gRedrawStatus = true;
    return true;
  default:
    break;
  }
  return false;
}
void VFOCFG_render() {
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
