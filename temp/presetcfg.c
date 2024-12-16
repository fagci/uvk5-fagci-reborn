#include "bandcfg.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "finput.h"
#include "textinput.h"
#include <stdint.h>

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"Name", M_NAME, 0},
    {"F start", M_START, 0},
    {"F end", M_END, 0},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions) - 1},
    {"BW", M_BW, 4},
    {"Gain", M_GAIN, ARRAY_SIZE(gainTable)},
    {"SQ level", M_SQ, 10},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"Radio", M_RADIO, ARRAY_SIZE(radioNames) - 1},
    {"Enable TX", M_TX, 2},
};

static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

static void setInitialSubmenuIndex(void) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_RADIO:
    subMenuIndex = gCurrentBand.radio;
    break;
  case M_BW:
    subMenuIndex = gCurrentBand.bw;
    break;
  case M_MODULATION:
    subMenuIndex = gCurrentBand.modulation;
    break;
  case M_STEP:
    subMenuIndex = gCurrentBand.step;
    break;
  case M_SQ_TYPE:
    subMenuIndex = gCurrentBand.squelch.type;
    break;
  case M_SQ:
    subMenuIndex = gCurrentBand.squelch.value;
    break;
  case M_GAIN:
    subMenuIndex = gCurrentBand.gainIndex;
    break;
  case M_TX:
    subMenuIndex = gCurrentBand.allowTx;
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
  case M_RADIO:
    strncpy(name, radioNames[index], 31);
    return;
  case M_MODULATION:
    strncpy(name, modulationTypeOptions[index], 31);
    return;
  case M_BW:
    strncpy(name, RADIO_GetBWName(index), 31);
    return;
  case M_SQ_TYPE:
    strncpy(name, sqTypeNames[index], 31);
    return;
  case M_SQ:
    sprintf(name, "%u", index);
    return;
  case M_STEP:
    sprintf(name, "%d.%02dKHz", StepFrequencyTable[index] / 100,
            StepFrequencyTable[index] % 100);
    return;
  case M_GAIN:
    sprintf(name, index == AUTO_GAIN_INDEX ? "auto" : "%+ddB",
            -gainTable[index].gainDb + 33);
    return;
  case M_TX:
    strncpy(name, yesNo[index], 31);
    return;
  default:
    break;
  }
}

static void setUpperBound(uint32_t f) {
  gCurrentBand.txF = f;
  if (gCurrentBand.misc.lastUsedFreq > f) {
    gCurrentBand.misc.lastUsedFreq = f;
    RADIO_TuneToSave(f);
  }
  BANDS_SaveCurrent();
}

static void setLowerBound(uint32_t f) {
  gCurrentBand.rxF = f;
  if (gCurrentBand.misc.lastUsedFreq < f) {
    gCurrentBand.misc.lastUsedFreq = f;
    RADIO_TuneToSave(f);
  }
  BANDS_SaveCurrent();
}

void BANDCFG_init(void) {}

void BANDCFG_update(void) {}

static void upDown(uint8_t inc) {
  if (isSubMenu) {
    IncDec8(&subMenuIndex, 0, menu[menuIndex].size, inc);
    OnRadioSubmenuChange(&menu[menuIndex], subMenuIndex);
  } else {
    IncDec8(&menuIndex, 0, MENU_SIZE, inc);
  }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
bool BANDCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  const MenuItem *item = &menu[menuIndex];
  if ((key == KEY_UP || key == KEY_DOWN) && isSubMenu && item->type == M_GAIN) {
    RADIO_SetGain(subMenuIndex);
  }
  switch (key) {
  case KEY_UP:
    upDown(-1);
    return true;
  case KEY_DOWN:
    upDown(1);
    return true;
  case KEY_MENU:
    // RUN APPS HERE
    switch (item->type) {
    case M_NAME:
      gTextInputSize = 9;
      gTextinputText = gCurrentBand.name;
      gTextInputCallback = BANDS_SaveCurrent;
      APPS_run(APP_TEXTINPUT);
      return true;
    case M_START:
      gFInputCallback = setLowerBound;
      gFInputTempFreq = gCurrentBand.rxF;
      APPS_run(APP_FINPUT);
      return true;
    case M_END:
      gFInputCallback = setUpperBound;
      gFInputTempFreq = gCurrentBand.txF;
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

void BANDCFG_render(void) {
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
