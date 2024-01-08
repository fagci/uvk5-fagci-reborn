#include "vfocfg.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"
#include "textinput.h"

typedef enum {
  M_F_RX, // uint32_t fRX : 32;
  M_F_TX, // uint32_t fTX : 32;
  // uint8_t memoryBanks : 8;
  M_STEP,       // uint8_t step : 8;
  M_MODULATION, // uint8_t modulation : 4;
  M_BW,         // uint8_t bw : 2;
                // uint8_t power : 2;
                // uint8_t codeRx : 8;
                // uint8_t codeTx : 8;
                // uint8_t codeTypeRx : 4;
                // uint8_t codeTypeTx : 4;
  M_SQ,
  M_SQ_TYPE,
  M_SAVE,
} VfoCfgMenu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"RX freq", M_F_RX},
    {"TX freq", M_F_TX},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"SQ level", M_SQ, 10},
    {"Save", M_SAVE},
};

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    subMenuIndex = gCurrentPreset->band.bw;
    break;
  case M_MODULATION:
    subMenuIndex = gCurrentVFO->modulation;
    break;
  case M_STEP:
    subMenuIndex = gCurrentPreset->band.step;
    break;
  case M_SQ_TYPE:
    subMenuIndex = gCurrentPreset->band.squelchType;
    break;
  case M_SQ:
    subMenuIndex = gCurrentPreset->band.squelch;
    break;
  default:
    subMenuIndex = 0;
    break;
  }
}

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    gCurrentPreset->band.bw = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_MODULATION:
    gCurrentVFO->modulation = subMenuIndex;
    RADIO_SaveCurrentVFO();
    break;
  case M_STEP:
    gCurrentPreset->band.step = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_SQ_TYPE:
    gCurrentPreset->band.squelchType = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  case M_SQ:
    gCurrentPreset->band.squelch = subMenuIndex;
    PRESETS_SaveCurrent();
    break;
  default:
    break;
  }
}

static char Output[16];
static const char *getValue(VfoCfgMenu type) {
  switch (type) {
  case M_F_RX:
    sprintf(Output, "%u.%05u", gCurrentVFO->fRX / 100000,
            gCurrentVFO->fRX % 100000);
    return Output;
  case M_F_TX:
    sprintf(Output, "%u.%05u", gCurrentVFO->fTX / 100000,
            gCurrentVFO->fTX % 100000);
    return Output;
  case M_BW:
    return bwNames[gCurrentPreset->band.bw];
  case M_MODULATION:
    return modulationTypeOptions[gCurrentPreset->band.modulation];
  case M_STEP:
    sprintf(Output, "%d.%02dKHz",
            StepFrequencyTable[gCurrentPreset->band.step] / 100,
            StepFrequencyTable[gCurrentPreset->band.step] % 100);
    return Output;
  case M_SQ_TYPE:
    return sqTypeNames[gCurrentPreset->band.squelchType];
  case M_SQ:
    sprintf(Output, "%u", gCurrentPreset->band.squelch);
    return Output;
  default:
    break;
  }
  return "";
}

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, menu[index].name, 31);
}

static void getStepText(uint16_t i, char *name) {
  sprintf(name, "%d.%02dKHz", StepFrequencyTable[i] / 100,
          StepFrequencyTable[i] % 100);
}

static void getSquelchValueText(uint16_t i, char *name) {
  sprintf(name, "%u", i);
}

static void getModulationTypeText(uint16_t index, char *name) {
  strncpy(name, modulationTypeOptions[index], 31);
}

static void getBWName(uint16_t index, char *name) {
  strncpy(name, bwNames[index], 31);
}

static void getSQTypeName(uint16_t index, char *name) {
  strncpy(name, sqTypeNames[index], 31);
}

static void showSubmenu(VfoCfgMenu m) {
  const MenuItem *item = &menu[menuIndex];
  switch (m) {
  case M_MODULATION:
    UI_ShowMenu(getModulationTypeText, item->size, subMenuIndex);
    return;
  case M_BW:
    UI_ShowMenu(getBWName, item->size, subMenuIndex);
    return;
  case M_STEP:
    UI_ShowMenu(getStepText, item->size, subMenuIndex);
    return;
  case M_SQ_TYPE:
    UI_ShowMenu(getSQTypeName, item->size, subMenuIndex);
    return;
  case M_SQ:
    UI_ShowMenu(getSquelchValueText, item->size, subMenuIndex);
    return;
  default:
    break;
  }
}

static void setTXF(uint32_t f) {
  gCurrentVFO->fTX = f;
  RADIO_SaveCurrentVFO();
}

void VFOCFG_init() {
  gRedrawScreen = true;
  menu[3].size -= (RADIO_IsBK1080Range(gCurrentVFO->fRX) ? 0 : 1);
}
void VFOCFG_update() {}
bool VFOCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const MenuItem *item = &menu[menuIndex];
  const uint8_t MENU_SIZE = ARRAY_SIZE(menu);
  const uint8_t SUBMENU_SIZE = item->size;
  switch (key) {
  case KEY_UP:
    if (isSubMenu) {
      IncDec8(&subMenuIndex, 0, SUBMENU_SIZE, -1);
    } else {
      IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    }
    return true;
  case KEY_DOWN:
    if (isSubMenu) {
      IncDec8(&subMenuIndex, 0, SUBMENU_SIZE, 1);
    } else {
      IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    }
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_F_RX:
      gFInputCallback = RADIO_TuneTo;
      APPS_run(APP_FINPUT);
      return true;
    case M_F_TX:
      gFInputCallback = setTXF;
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
      setInitialSubmenuIndex();
    }
    return true;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_exit();
    }
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
    STATUSLINE_SetText(item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    PrintMediumEx(LCD_WIDTH / 2, LCD_HEIGHT - 2, POS_C, C_FILL,
                  getValue(item->type));
  }
}
