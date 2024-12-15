#include "chlist.h"
#include "../driver/uart.h"
#include "../helper/numnav.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "chcfg.h"
#include "textinput.h"
#include "vfo1.h"

typedef enum {
  MODE_INFO,
  MODE_TX,
  MODE_SCANLIST,
  MODE_SCANLIST_SELECT,
  // MODE_TYPE,
  // MODE_SELECT,
} CHLIST_ViewMode;

static char *VIEW_MODE_NAMES[] = {
    "INFO",   //
    "TX",     //
    "SL",     //
    "SL SEL", //
              // "TYPE",     //
              // "CH SEL",   //
};

// TODO:
// filter
// - scanlist

bool gChSaveMode = false;
CHType gChListFilter = TYPE_ALL;

static uint16_t channelIndex = 0;
static uint8_t viewMode = MODE_INFO;
static CH ch;
static char tempName[9] = {0};

static const Symbol typeIcons[] = {
    [TYPE_CH] = SYM_CH,         [TYPE_PRESET] = SYM_PRESET,
    [TYPE_VFO] = SYM_VFO,       [TYPE_SETTING] = SYM_SETTING,
    [TYPE_FILE] = SYM_FILE,     [TYPE_MELODY] = SYM_MELODY,
    [TYPE_FOLDER] = SYM_FOLDER, [TYPE_EMPTY] = SYM_MISC2,
};

static inline uint16_t getChannelNumber(uint16_t menuIndex) {
  return gScanlist[menuIndex];
}

static void getChItem(uint16_t i, uint16_t index, bool isCurrent) {
  index = getChannelNumber(index);
  CHANNELS_Load(index, &ch);
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (ch.meta.type) {
    PrintSymbolsEx(2, y + 8, POS_L, C_INVERT, "%c", typeIcons[ch.meta.type]);
    PrintMediumEx(13, y + 8, POS_L, C_INVERT, "%s", ch.name);
  } else {
    PrintMediumEx(13, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
  }
  switch (viewMode) {
  case MODE_INFO:
    if (CHANNELS_IsFreqable(ch.meta.type)) {
      PrintSmallEx(LCD_WIDTH - 5, y + 5, POS_R, C_INVERT, "%u.%03u %u.%03u",
                   ch.rxF / MHZ, ch.rxF / 100 % 1000, ch.txF / MHZ,
                   ch.txF / 100 % 1000);
    }
    break;
  case MODE_SCANLIST:
    if (CHANNELS_IsScanlistable(ch.meta.type)) {
      UI_Scanlists(LCD_WIDTH - 32, y + 3, ch.scanlists);
    }
    break;
  case MODE_TX:
    PrintSmallEx(LCD_WIDTH - 5, y + 7, POS_R, C_INVERT, "%s",
                 ch.allowTx ? "ON" : "OFF");
    break;
    /* case MODE_SELECT:
      break;
    case MODE_TYPE:
      break; */
  }
}

static void setMenuIndex(uint16_t i) { channelIndex = i - 1; }

static void saveNamed() {
  strncpy(gChEd.name, gTextinputText, 9);
  gChEd.scanlists = 0;
  if (gChEd.meta.type == TYPE_VFO) {
    gChEd.meta.type = TYPE_CH;
  }
  CHANNELS_Save(getChannelNumber(channelIndex), &gChEd);
  RADIO_LoadCurrentVFO();
}

void CHLIST_init() {
  CHANNELS_LoadScanlist(gChListFilter, gSettings.currentScanlist);
}

void CHLIST_deinit() { gChSaveMode = false; }

bool CHLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key == KEY_STAR) {
      NUMNAV_Init(channelIndex + 1, 1, gScanlistSize);
      gNumNavCallback = setMenuIndex;
      return true;
    }
    if (gIsNumNavInput) {
      channelIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }

  uint16_t chNum = getChannelNumber(channelIndex);
  bool longHeld = bKeyHeld && bKeyPressed && !gRepeatHeld;
  bool simpleKeypress = !bKeyPressed && !bKeyHeld;

  if (viewMode == MODE_SCANLIST || viewMode == MODE_SCANLIST_SELECT) {
    if ((longHeld || simpleKeypress) && (key > KEY_0 && key < KEY_9)) {
      if (viewMode == MODE_SCANLIST_SELECT) {
        gSettings.currentScanlist = CHANNELS_ScanlistByKey(
            gSettings.currentScanlist, key, longHeld && !simpleKeypress);
        SETTINGS_DelayedSave();
      } else {
        CHANNELS_Load(chNum, &ch);
        ch.scanlists = CHANNELS_ScanlistByKey(ch.scanlists, key, longHeld);
        CHANNELS_Save(chNum, &ch);
      }
      return true;
    }
  }

  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_STAR:
      IncDec8(&viewMode, 0, ARRAY_SIZE(VIEW_MODE_NAMES), 1);
      return true;
    default:
      break;
    }
  }

  if (bKeyPressed || !bKeyHeld) {
    switch (key) {
    case KEY_UP:
      IncDec16(&channelIndex, 0, gScanlistSize, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&channelIndex, 0, gScanlistSize, 1);
      return true;
    default:
      break;
    }
  }
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_0:
      if (gChListFilter == TYPE_ALL) {
        gChListFilter = TYPE_EMPTY;
      } else {
        gChListFilter++;
      }
      CHANNELS_LoadScanlist(gChListFilter, gSettings.currentScanlist);
      return true;
    case KEY_MENU:
      if (gChSaveMode) {
        gTextinputText = tempName;
        snprintf(gTextinputText, 9, "%lu.%05lu", gChEd.rxF / MHZ,
                 gChEd.rxF % MHZ);
        gTextInputSize = 9;
        gTextInputCallback = saveNamed;
        APPS_run(APP_TEXTINPUT);
        return true;
      }
      RADIO_TuneToCH(chNum);
      APPS_exit();
      return true;
    case KEY_PTT:
      RADIO_TuneToCH(chNum);
      gVfo1ProMode = true;
      APPS_run(APP_VFO1);
      return true;
    case KEY_F:
      CHANNELS_Load(chNum, &ch);
      gChEd = ch;
      gChNum = chNum;
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_EXIT:
      if (gIsNumNavInput) {
        NUMNAV_Deinit();
        return true;
      }
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void CHLIST_render() {
  UI_ShowMenuEx(getChItem, gScanlistSize, channelIndex, MENU_LINES_TO_SHOW + 1);
  STATUSLINE_SetText("%s %s", CH_TYPE_NAMES[gChListFilter],
                     VIEW_MODE_NAMES[viewMode]);
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
}
