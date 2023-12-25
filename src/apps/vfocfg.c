#include "vfocfg.h"
#include "../driver/st7565.h"
#include "../helper/presetlist.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
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
    {"Name", M_NAME},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"SQ level", M_SQ, 10},
    {"Save", M_SAVE},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    gCurrentVFO->bw = subMenuIndex;
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
  case M_NAME:
    return gCurrentVFO->name;
  case M_BW:
    return bwNames[gCurrentVFO->bw];
  case M_MODULATION:
    return modulationTypeOptions[gCurrentVFO->modulation];
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

static void showStepValues() {
  char items[ARRAY_SIZE(StepFrequencyTable)][16] = {0};
  for (uint8_t i = 0; i < ARRAY_SIZE(StepFrequencyTable); ++i) {
    sprintf(items[i], "%d.%02dKHz", StepFrequencyTable[i] / 100,
            StepFrequencyTable[i] % 100);
  }
  UI_ShowItems(items, ARRAY_SIZE(StepFrequencyTable), subMenuIndex);
}

static void showSquelchValues() {
  char items[10][16] = {0};
  for (uint8_t i = 0; i < ARRAY_SIZE(items); ++i) {
    sprintf(items[i], "%u", i);
  }
  UI_ShowItems(items, ARRAY_SIZE(items), subMenuIndex);
}

static void showSubmenu(VfoCfgMenu menu) {
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
  case M_SQ_TYPE:
    SHOW_ITEMS(sqTypeNames);
    return;
  case M_SQ:
    showSquelchValues();
    return;
  default:
    break;
  }
}

static void setTXF(uint32_t f) {
  gCurrentVFO->fTX = f;
  RADIO_SaveCurrentVFO();
}

void VFOCFG_init() { gRedrawScreen = true; }
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
    case M_NAME:
      gTextinputText = gCurrentVFO->name;
      APPS_run(APP_TEXTINPUT);
      return true;
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
    UI_ShowMenu(menu, ARRAY_SIZE(menu), menuIndex);
    PrintMediumEx(LCD_WIDTH / 2, LCD_HEIGHT - 2, POS_C, C_FILL,
                  getValue(item->type));
  }
}
