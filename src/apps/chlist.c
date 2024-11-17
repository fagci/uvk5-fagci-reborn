#include "chlist.h"
#include "../driver/uart.h"
#include "../helper/numnav.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "chcfg.h"
#include "textinput.h"
#include "vfo1.h"

typedef enum {
  MODE_INFO,
  MODE_SCANLIST,
  MODE_TX,
  MODE_TYPE,
  MODE_SELECT,
} CHLIST_ViewMode;

// TODO:
// filter
// - scanlist
// - type

bool gChSaveMode = false;

static uint16_t channelIndex = 0;
static CHLIST_ViewMode viewMode = MODE_INFO;
static CH ch;
static uint16_t chCount = 1024;
static char tempName[9] = {0};

static const Symbol typeIcons[] = {
    [TYPE_CH] = SYM_CH,         [TYPE_PRESET] = SYM_PRESET,
    [TYPE_VFO] = SYM_VFO,       [TYPE_SETTING] = SYM_SETTING,
    [TYPE_FILE] = SYM_FILE,     [TYPE_MELODY] = SYM_MELODY,
    [TYPE_FOLDER] = SYM_FOLDER, [TYPE_EMPTY] = SYM_MISC2,
};

static uint16_t getChannelNumber(uint16_t menuIndex) {
  if (gSettings.currentScanlist != 15) {
    menuIndex = gScanlist[menuIndex];
  }
  return menuIndex;
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
    PrintMediumEx(8 + 8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  } else {
    PrintMediumEx(2, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
  }
  if (ch.meta.type == TYPE_CH || ch.meta.type == TYPE_PRESET ||
      ch.meta.type == TYPE_FOLDER) {
    char scanlistsStr[9] = "";
    for (uint8_t n = 0; n < 8; ++n) {
      scanlistsStr[n] =
          ch.scanlists & (1 << n) ? (n == 7 ? 'X' : '1' + n) : '-';
    }
    PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
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
  CHANNELS_LoadScanlist(15);
  chCount = CHANNELS_GetCountMax();
}

void CHLIST_deinit() { gChSaveMode = false; }

bool CHLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key == KEY_STAR) {
      NUMNAV_Init(channelIndex + 1, 1, chCount);
      gNumNavCallback = setMenuIndex;
      return true;
    }
    if (gIsNumNavInput) {
      channelIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }

  uint16_t chNum = getChannelNumber(channelIndex);
  if (bKeyPressed || !bKeyHeld) {
    switch (key) {
    case KEY_UP:
      IncDec16(&channelIndex, 0, chCount, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&channelIndex, 0, chCount, 1);
      return true;
    default:
      break;
    }
  }
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
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
      Log("Prepare to edit CH #%u %s", chNum, ch.name);
      gChEd = ch;
      gChNum = chNum;
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void CHLIST_render() {
  UI_ShowMenuEx(getChItem, chCount, channelIndex, MENU_LINES_TO_SHOW + 1);
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
}
