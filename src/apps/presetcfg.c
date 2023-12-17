#include "presetcfg.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "finput.h"
#include "textinput.h"
#include <stdint.h>
#include <stdio.h>

typedef enum {
  M_BOUNDS,
  M_NAME,       // char name[16];
  M_STEP,       // uint8_t step : 8;
  M_MODULATION, // uint8_t modulation : 4;
  M_BW,         // uint8_t bw : 2;
  M_SQ,
  M_SQ_TYPE,
  M_GAIN,
  M_SAVE,
} PresetCfgMenu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"Bounds", M_BOUNDS},
    {"Name", M_NAME},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"SQ level", M_SQ, 10},
    {"Gain", M_GAIN, ARRAY_SIZE(gainTable)},
    {"Save", M_SAVE},
};

static void accept() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    gCurrentPreset->band.bw = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  case M_MODULATION:
    gCurrentPreset->band.modulation = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  case M_STEP:
    gCurrentPreset->band.step = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  case M_SQ_TYPE:
    gCurrentPreset->band.squelchType = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  case M_SQ:
    gCurrentPreset->band.squelch = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  case M_GAIN:
    gCurrentPreset->band.gainIndex = subMenuIndex;
    RADIO_SaveCurrentPreset();
    break;
  default:
    break;
  }
}

static char Output[16];
static const char *getValue(PresetCfgMenu type) {
  uint32_t fs = gCurrentPreset->band.bounds.start;
  uint32_t fe = gCurrentPreset->band.bounds.end;
  switch (type) {
  case M_BOUNDS:
    sprintf(Output, "%lu.%05lu-%lu.%05lu", fs / 100000, fs % 100000,
            fe / 100000, fe % 100000);
    return Output;
  case M_NAME:
    return gCurrentPreset->band.name;
  case M_BW:
    return bwNames[gCurrentPreset->band.bw];
  case M_SQ_TYPE:
    return sqTypeNames[gCurrentPreset->band.squelchType];
  case M_SQ:
    sprintf(Output, "%u", gCurrentPreset->band.squelch);
    return Output;
  case M_GAIN:
    sprintf(Output, "%ddB", gainTable[gCurrentPreset->band.gainIndex].gainDb);
    return Output;
  case M_MODULATION:
    return modulationTypeOptions[gCurrentPreset->band.modulation];
  case M_STEP:
    sprintf(Output, "%u.%02uKHz",
            StepFrequencyTable[gCurrentPreset->band.step] / 100,
            StepFrequencyTable[gCurrentPreset->band.step] % 100);
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

static void showGainValues() {
  char items[ARRAY_SIZE(gainTable)][16] = {0};
  for (uint8_t i = 0; i < ARRAY_SIZE(gainTable); ++i) {
    sprintf(items[i], "%ddB%s", gainTable[i].gainDb, i == 90 ? "(def)" : "");
  }
  UI_ShowItems(items, ARRAY_SIZE(StepFrequencyTable), subMenuIndex);
}

static void showSubmenu(PresetCfgMenu menu) {
  switch (menu) {
  case M_MODULATION:
    SHOW_ITEMS(modulationTypeOptions);
    return;
  case M_BW:
    SHOW_ITEMS(bwNames);
    return;
  case M_SQ_TYPE:
    SHOW_ITEMS(sqTypeNames);
    return;
  case M_STEP:
    showStepValues();
    return;
  case M_SQ:
    showSquelchValues();
    return;
  case M_GAIN:
    showGainValues();
    return;
  default:
    break;
  }
}

static void setUpperBound(uint32_t f) {
  gCurrentPreset->band.bounds.end = f;
  RADIO_SaveCurrentPreset();
}

static void setLowerBound(uint32_t f) {
  gCurrentPreset->band.bounds.start = f;
  gFInputCallback = setUpperBound;
  APPS_run(APP_FINPUT);
}

void PRESETCFG_init() { gRedrawScreen = true; }
void PRESETCFG_update() {}
bool PRESETCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
      gTextinputText = gCurrentVFO->name;
      APPS_run(APP_TEXTINPUT);
      return true;
    case M_BOUNDS:
      gFInputCallback = setLowerBound;
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
    gRedrawScreen = true;
    gRedrawScreen = true;
    return true;
  case KEY_EXIT:
    if (isSubMenu) {
      isSubMenu = false;
    } else {
      APPS_exit();
    }
    gRedrawScreen = true;
    gRedrawScreen = true;
    return true;
  default:
    break;
  }
  return false;
}
void PRESETCFG_render() {
  UI_ClearScreen();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    showSubmenu(item->type);
    PrintSmall(0, 0, item->name);
  } else {
    UI_ShowMenu(menu, ARRAY_SIZE(menu), menuIndex);
    PrintMedium(126, 6 * 8 + 12, getValue(item->type));
  }
}
