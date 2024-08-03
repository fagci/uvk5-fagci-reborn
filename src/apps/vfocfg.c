#include "vfocfg.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

static uint8_t menuIndex = 0;
static uint8_t subMenuIndex = 0;
static bool isSubMenu = false;

static MenuItem menu[] = {
    {"Radio", M_RADIO, ARRAY_SIZE(radioNames)},
    {"RX freq", M_F_RX, 0},
    {"TX freq", M_F_TX, 0},
    {"TX code type", M_TX_CODE_TYPE, ARRAY_SIZE(TX_CODE_TYPES)},
    {"TX code", M_TX_CODE, 0},
    {"TX offset", M_TX_OFFSET, 0},
    {"TX offset dir", M_TX_OFFSET_DIR, ARRAY_SIZE(TX_OFFSET_NAMES)},
    {"TX power", M_F_TXP, ARRAY_SIZE(TX_POWER_NAMES)},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(bwNames)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(sqTypeNames)},
    {"SQ level", M_SQ, 10},
    {"Save", M_SAVE, 0},
};
static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

static void setInitialSubmenuIndex(void) {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_RADIO:
    subMenuIndex = radio->radio;
    break;
  case M_BW:
    subMenuIndex = gCurrentPreset->band.bw;
    break;
  case M_TX_CODE_TYPE:
    subMenuIndex = radio->tx.codeType;
    break;
  case M_TX_CODE:
    subMenuIndex = radio->tx.code;
    break;
  case M_F_TXP:
    subMenuIndex = gCurrentPreset->power;
    break;
  case M_TX_OFFSET_DIR:
    subMenuIndex = gCurrentPreset->offsetDir;
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
  default:
    subMenuIndex = 0;
    break;
  }
}

static void getMenuItemText(uint16_t index, char *name) {
  strncpy(name, menu[index].name, 31);
}

static void updateTxCodeListSize() {
  for (uint8_t i = 0; i < ARRAY_SIZE(menu); ++i) {
    MenuItem *item = &menu[i];
    if (item->type == M_TX_CODE) {
      if (radio->tx.codeType == CODE_TYPE_CONTINUOUS_TONE) {
        item->size = ARRAY_SIZE(CTCSS_Options);
      } else if (radio->tx.codeType != CODE_TYPE_OFF) {
        item->size = ARRAY_SIZE(DCS_Options);
      } else {
        item->size = 0;
      }
      return;
    }
  }
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
    strncpy(name, bwNames[index], 15);
    return;
  case M_TX_CODE_TYPE:
    strncpy(name, TX_CODE_TYPES[index], 15);
    return;
  case M_TX_CODE:
    if (radio->tx.codeType) {
      if (radio->tx.codeType == CODE_TYPE_CONTINUOUS_TONE) {
        sprintf(name, "CT:%u.%uHz", CTCSS_Options[index] / 10,
                CTCSS_Options[index] % 10);
      } else if (radio->tx.codeType == CODE_TYPE_DIGITAL) {
        sprintf(name, "DCS:D%03oN", DCS_Options[index]);
      } else if (radio->tx.codeType == CODE_TYPE_REVERSE_DIGITAL) {
        sprintf(name, "DCS:D%03oI", DCS_Options[index]);
      } else {
        sprintf(name, "No code");
      }
    }
    return;
  case M_F_TXP:
    strncpy(name, TX_POWER_NAMES[index], 15);
    return;
  case M_TX_OFFSET_DIR:
    strncpy(name, TX_OFFSET_NAMES[index], 15);
    return;
  case M_STEP:
    sprintf(name, "%u.%02uKHz", StepFrequencyTable[index] / 100,
            StepFrequencyTable[index] % 100);
    return;
  case M_SQ_TYPE:
    strncpy(name, sqTypeNames[index], 31);
    return;
  case M_SQ:
    sprintf(name, "%u", index);
    return;
  default:
    break;
  }
}

static void setTXF(uint32_t f) {
  radio->tx.f = f;
  RADIO_SaveCurrentVFO();
}

static void setTXOffset(uint32_t f) {
  gCurrentPreset->offset = f;
  PRESETS_SaveCurrent();
}

void VFOCFG_init(void) {
  gRedrawScreen = true;
  updateTxCodeListSize();
  /* for (uint8_t i = 0; i < MENU_SIZE; ++i) {
    if (menu[i].type == M_MODULATION) {
      menu[i].size = RADIO_IsBK1080Range(radio->rx.f)
                         ? ARRAY_SIZE(modulationTypeOptions)
                         : ARRAY_SIZE(modulationTypeOptions) - 1;
      break;
    }
  } */
}

void VFOCFG_update(void) {}

static bool accept(void) {
  MenuItem *item = &menu[menuIndex];
  // RUN APPS HERE
  switch (item->type) {
  case M_F_RX:
    gFInputCallback = RADIO_TuneTo;
    gFInputTempFreq = radio->rx.f;
    APPS_run(APP_FINPUT);
    return true;
  case M_F_TX:
    gFInputCallback = setTXF;
    gFInputTempFreq = radio->tx.f;
    APPS_run(APP_FINPUT);
    return true;
  case M_TX_OFFSET:
    gFInputCallback = setTXOffset;
    gFInputTempFreq = gCurrentPreset->offset;
    APPS_run(APP_FINPUT);
    return true;
  case M_SAVE:
    APPS_run(APP_SAVECH);
    return true;
  default:
    break;
  }
  updateTxCodeListSize();
  if (isSubMenu) {
    AcceptRadioConfig(item, subMenuIndex);
    isSubMenu = false;
  } else {
    isSubMenu = true;
    setInitialSubmenuIndex();
  }
  return true;
}

static void setMenuIndexAndRun(uint16_t v) {
  if (isSubMenu) {
    subMenuIndex = v - 1;
  } else {
    menuIndex = v - 1;
  }
  accept();
}

bool VFOCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key >= KEY_0 && key <= KEY_9) {
      NUMNAV_Init(menuIndex + 1, 1, MENU_SIZE);
      gNumNavCallback = setMenuIndexAndRun;
    }
    if (gIsNumNavInput) {
      uint8_t v = NUMNAV_Input(key) - 1;
      if (isSubMenu) {
        subMenuIndex = v;
      } else {
        menuIndex = v;
      }
      return true;
    }
  }
  MenuItem *item = &menu[menuIndex];
  uint8_t SUBMENU_SIZE = item->size;
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
    return accept();
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

void VFOCFG_render(void) {
  UI_ClearScreen();
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText(apps[APP_VFO_CFG].name);
  }
  MenuItem *item = &menu[menuIndex];
  if (isSubMenu) {
    UI_ShowMenu(getSubmenuItemText, item->size, subMenuIndex);
    STATUSLINE_SetText(item->name);
  } else {
    UI_ShowMenu(getMenuItemText, ARRAY_SIZE(menu), menuIndex);
    char Output[32] = "";
    GetMenuItemValue(item->type, Output);
    PrintMediumEx(LCD_XCENTER, LCD_HEIGHT - 4, POS_C, C_FILL, Output);
  }
}
