#include "chlist.h"
#include "../helper/numnav.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
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

static uint16_t channelIndex = 0;
static CHLIST_ViewMode viewMode = MODE_INFO;
static CH _ch;
static uint16_t chCount = 1024;

static void getChItem(uint16_t i, uint16_t index, bool isCurrent) {
  if (gSettings.currentScanlist != 15) {
    index = gScanlist[index];
  }
  CHANNELS_Load(index, &_ch);
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (IsReadable(_ch.name)) {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", _ch.name);
  } else {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
  }
  char scanlistsStr[9] = "";
  for (uint8_t n = 0; n < 8; ++n) {
    scanlistsStr[n] =
        _ch.memoryBanks & (1 << n) ? (n == 7 ? 'X' : '1' + n) : '-';
  }
  PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void setMenuIndex(uint16_t i) { channelIndex = i - 1; }

void CHLIST_init() {
  CHANNELS_LoadScanlist(15);
  chCount = CHANNELS_GetCountMax();
}

void CHLIST_update() {}

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

  uint16_t chNum = gScanlist[channelIndex];
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
    case KEY_PTT:
      RADIO_TuneToCH(chNum);
      RADIO_SaveCurrentVFO();
      gVfo1ProMode = true;
      APPS_run(APP_VFO1);
      return true;
    case KEY_F:
      CHANNELS_Load(chNum, &_ch);
      gChEd = _ch;
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
