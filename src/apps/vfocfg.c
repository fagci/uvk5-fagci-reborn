#include "vfocfg.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/bandlist.h"
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
    {"RX freq", M_F_RX, 0},
    {"TX freq", M_F_TX, 0},
    {"TX offset", M_TX_OFFSET, 0},
    {"TX offset dir", M_TX_OFFSET_DIR, ARRAY_SIZE(TX_OFFSET_NAMES)},
    {"TX power", M_F_TXP, ARRAY_SIZE(TX_POWER_NAMES)},
    {"Step", M_STEP, ARRAY_SIZE(StepFrequencyTable)},
    {"Modulation", M_MODULATION, ARRAY_SIZE(modulationTypeOptions)},
    {"BW", M_BW, ARRAY_SIZE(BW_NAMES)},
    {"SQ type", M_SQ_TYPE, ARRAY_SIZE(SQ_TYPE_NAMES)},
    {"SQ level", M_SQ, 10},
    {"Save", M_SAVE, 0},
};
static const uint8_t MENU_SIZE = ARRAY_SIZE(menu);

static void setInitialSubmenuIndex() {
  const MenuItem *item = &menu[menuIndex];
  switch (item->type) {
  case M_BW:
    subMenuIndex = radio->bw;
    break;
  case M_F_TXP:
    subMenuIndex = gCurrentBand->power;
    break;
  case M_TX_OFFSET_DIR:
    subMenuIndex = gCurrentBand->offsetDir;
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
    strncpy(name, BW_NAMES[index], 15);
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
    strncpy(name, SQ_TYPE_NAMES[index], 31);
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
  RADIO_SaveCurrentCH();
}

static void setTXOffset(uint32_t f) {
  gCurrentBand->offset = f;
  BANDS_SaveCurrent();
}

void CHCFG_init() {
  gRedrawScreen = true;
  for (uint8_t i = 0; i < MENU_SIZE; ++i) {
    if (menu[i].type == M_MODULATION) {
      menu[i].size = RADIO_IsBK1080Range(radio->f)
                         ? ARRAY_SIZE(modulationTypeOptions)
                         : ARRAY_SIZE(modulationTypeOptions) - 1;
      break;
    }
  }
}

void CHCFG_update() {}

static bool accept() {
  MenuItem *item = &menu[menuIndex];
  // RUN APPS HERE
  switch (item->type) {
  case M_F_RX:
    gFInputCallback = RADIO_TuneTo;
    gFInputTempFreq = radio->f;
    APPS_run(APP_FINPUT);
    return true;
  case M_F_TX:
    gFInputCallback = setTXF;
    gFInputTempFreq = radio->tx.f;
    APPS_run(APP_FINPUT);
    return true;
  case M_TX_OFFSET:
    gFInputCallback = setTXOffset;
    gFInputTempFreq = gCurrentBand->offset;
    APPS_run(APP_FINPUT);
    return true;
  case M_SAVE:
    APPS_run(APP_SAVECH);
    return true;
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
}

static void setMenuIndexAndRun(uint16_t v) {
  if (isSubMenu) {
    subMenuIndex = v - 1;
  } else {
    menuIndex = v - 1;
  }
  accept();
}

bool CHCFG_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key <= KEY_9) {
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

void CHCFG_render() {
  UI_ClearScreen();
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
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


static App meta = {
    .id = APP_CH_CFG,
    .name = "CH cfg",
    .init = CHCFG_init,
    .update = CHCFG_update,
    .render = CHCFG_render,
    .key = CHCFG_key,
};

App *CHCFG_Meta() { return &meta; }
