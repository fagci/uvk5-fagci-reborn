#include "bandcfg.h"
#include "../apps/apps.h"
#include "../helper/measurements.h"
#include "../helper/bandlist.h"
#include "../misc.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "finput.h"
#include "textinput.h"
#include <stdint.h>

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"Start freq", M_START},
    {"End freq", M_END},
    {"Name", M_NAME},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(BW_NAMES)},
    {"SQ level", M_SQ, 10},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(SQ_TYPE_NAMES)},
    {"Gain", M_GAIN, ARRAY_SIZE(gainTable)},
    {"Enable TX", M_TX, 2},
};

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    subMenuIndex = radio->bw;
    break;
  case M_MODULATION:
    subMenuIndex = radio->modulation;
    break;
  case M_STEP:
    subMenuIndex = radio->step;
    break;
  case M_SQ_TYPE:
    subMenuIndex = radio->sq.levelType;
    break;
  case M_SQ:
    subMenuIndex = radio->sq.level;
    break;
  case M_GAIN:
    subMenuIndex = gCurrentBand->band.gainIndex;
    break;
  case M_TX:
    subMenuIndex = gCurrentBand->allowTx;
    break;
  default:
    subMenuIndex = 0;
    break;
  }
}

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, menu[index].name, 31);
}

static void getSubmenuItemText(uint16_t index, char *name) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_MODULATION:
    strncpy(name, modulationTypeOptions[index], 31);
    return;
  case M_BW:
    strncpy(name, BW_NAMES[index], 31);
    return;
  case M_SQ_TYPE:
    strncpy(name, SQ_TYPE_NAMES[index], 31);
    return;
  case M_SQ:
    sprintf(name, "%u", index);
    return;
  case M_STEP:
    sprintf(name, "%d.%02dKHz", StepFrequencyTable[index] / 100,
            StepFrequencyTable[index] % 100);
    return;
  case M_GAIN:
    sprintf(name, "%ddB%s", gainTable[index].gainDb,
            index == 16 ? "(def)" : "");
    return;
  case M_TX:
    strncpy(name, YES_NO_NAMES[index], 31);
    return;
  default:
    break;
  }
}

static void setUpperBound(uint32_t f) {
  gCurrentBand->band.bounds.end = f;
  if (gCurrentBand->lastUsedFreq > f) {
    gCurrentBand->lastUsedFreq = f;
    RADIO_TuneToSave(f);
  }
  BANDS_SaveCurrent();
}

static void setLowerBound(uint32_t f) {
  gCurrentBand->band.bounds.start = f;
  if (gCurrentBand->lastUsedFreq < f) {
    gCurrentBand->lastUsedFreq = f;
    RADIO_TuneToSave(f);
  }
  BANDS_SaveCurrent();
}

void BANDCFG_init() { gRedrawScreen = true; }
void BANDCFG_update() {}
bool BANDCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
      gTextinputText = gCurrentBand->band.name;
      gTextInputCallback = BANDS_SaveCurrent;
      APPS_run(APP_TEXTINPUT);
      return true;
    case M_START:
      gFInputCallback = setLowerBound;
      gFInputTempFreq = gCurrentBand->band.bounds.start;
      APPS_run(APP_FINPUT);
      return true;
    case M_END:
      gFInputCallback = setUpperBound;
      gFInputTempFreq = gCurrentBand->band.bounds.end;
      APPS_run(APP_FINPUT);
      return true;
    /* case M_SAVE:
      APPS_run(APP_SAVECH);
      return true; */
    default:
      break;
    }
    if (isSubMenu) {
      AcceptRadioConfig(item, subMenuIndex);
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

void BANDCFG_render() {
  UI_ClearScreen();
  const MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    UI_ShowMenu(getSubmenuItemText, item->size, subMenuIndex);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 4, POS_C, C_FILL, item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    char Output[32] = "";
    GetMenuItemValue(item->type, Output);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 4, POS_C, C_FILL, Output);
  }
}


static App meta = {
    .id = APP_BAND_CFG,
    .name = "Band cfg",
    .init = BANDCFG_init,
    .update = BANDCFG_update,
    .render = BANDCFG_render,
    .key = BANDCFG_key,
};
App *BANDCFG_Meta() { return &meta; }
