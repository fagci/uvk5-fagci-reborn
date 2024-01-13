#include "presetcfg.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "finput.h"
#include "textinput.h"
#include <stdint.h>
#include <stdio.h>

typedef enum {
  M_START,
  M_END,
  M_NAME,       // char name[16];
  M_STEP,       // uint8_t step : 8;
  M_MODULATION, // uint8_t modulation : 4;
  M_BW,         // uint8_t bw : 2;
  M_SQ,
  M_SQ_TYPE,
  M_GAIN,
  // M_SAVE,
} PresetCfgMenu;

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"Start freq", M_START},
    {"End freq", M_END},
    {"Name", M_NAME},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"SQ level", M_SQ, 10},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"Gain", M_GAIN, ARRAY_SIZE(gainTable)},
    // {"Save", M_SAVE},
};

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    subMenuIndex = gCurrentPreset->band.bw;
    break;
  case M_MODULATION:
    subMenuIndex = gCurrentPreset->band.modulation;
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
  case M_GAIN:
    subMenuIndex = gCurrentPreset->band.gainIndex;
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
    PRESETS_SaveCurrent();
    break;
  case M_MODULATION:
    gCurrentPreset->band.modulation = subMenuIndex;
    PRESETS_SaveCurrent();
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
  case M_GAIN:
    gCurrentPreset->band.gainIndex = subMenuIndex;
    PRESETS_SaveCurrent();
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
  case M_START:
    sprintf(Output, "%lu.%03lu", fs / 100000, fs / 100 % 1000);
    return Output;
  case M_END:
    sprintf(Output, "%lu.%03lu", fe / 100000, fe / 100 % 1000);
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

static void getGainText(uint16_t i, char *name) {
  sprintf(name, "%ddB%s", gainTable[i].gainDb, i == 90 ? "(def)" : "");
}

static void showSubmenu(PresetCfgMenu menu) {
  switch (menu) {
  case M_MODULATION:
    UI_ShowMenu(getModulationTypeText, ARRAY_SIZE(modulationTypeOptions),
                subMenuIndex);
    return;
  case M_BW:
    UI_ShowMenu(getBWName, ARRAY_SIZE(bwNames), subMenuIndex);
    return;
  case M_SQ_TYPE:
    UI_ShowMenu(getSQTypeName, ARRAY_SIZE(sqTypeNames), subMenuIndex);
    return;
  case M_SQ:
    UI_ShowMenu(getSquelchValueText, 10, subMenuIndex);
    return;
  case M_STEP:
    UI_ShowMenu(getStepText, ARRAY_SIZE(StepFrequencyTable), subMenuIndex);
    return;
  case M_GAIN:
    UI_ShowMenu(getGainText, ARRAY_SIZE(gainTable), subMenuIndex);
    return;
  default:
    break;
  }
}

static void setUpperBound(uint32_t f) {
  gCurrentPreset->band.bounds.end = f;
  PRESETS_SaveCurrent();
}

static void setLowerBound(uint32_t f) {
  gCurrentPreset->band.bounds.start = f;
  PRESETS_SaveCurrent();
}

void PRESETCFG_init() { gRedrawScreen = true; }
void PRESETCFG_update() {}
bool PRESETCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
      gTextInputSize = 9;
      gTextinputText = gCurrentPreset->band.name;
      gTextInputCallback = PRESETS_SaveCurrent;
      APPS_run(APP_TEXTINPUT);
      return true;
    case M_START:
      gFInputCallback = setLowerBound;
      APPS_run(APP_FINPUT);
      return true;
    case M_END:
      gFInputCallback = setUpperBound;
      APPS_run(APP_FINPUT);
      return true;
    /* case M_SAVE:
      APPS_run(APP_SAVECH);
      return true; */
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

void PRESETCFG_render() {
  UI_ClearScreen();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    showSubmenu(item->type);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 2, POS_C, C_FILL, item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 2, POS_C, C_FILL,
                  getValue(item->type));
  }
}
