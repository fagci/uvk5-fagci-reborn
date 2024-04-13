#include "scanlists.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "textinput.h"
#include <stdbool.h>

static uint16_t count = 0;

static uint16_t currentIndex = 0;
static CH ch;
static uint16_t chNum = 0;

static void getChItem(uint16_t i, uint16_t index, bool isCurrent) {
  CH ch;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  CHANNELS_Load(index, &ch);
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (IsReadable(ch.name)) {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  } else {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
    return;
  }
  char scanlistsStr[9] = "";
  for (uint8_t i = 0; i < 8; ++i) {
    scanlistsStr[i] = ch.memoryBanks & (1 << i) ? '1' + i : '-';
  }
  PrintSmallEx(LCD_WIDTH - 1, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void getScanlistItem(uint16_t i, uint16_t index, bool isCurrent) {
  uint16_t chNum = gScanlist[index];
  CH ch;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  CHANNELS_Load(chNum, &ch);
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (IsReadable(ch.name)) {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  } else {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
    return;
  }
  char scanlistsStr[9] = "";
  for (uint8_t i = 0; i < 8; ++i) {
    scanlistsStr[i] = ch.memoryBanks & (1 << i) ? '1' + i : '-';
  }
  PrintSmallEx(LCD_WIDTH - 1 - 3, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void toggleScanlist(uint8_t n) {
  CH ch;
  CHANNELS_Load(gScanlist[currentIndex], &ch);
  ch.memoryBanks ^= 1 << n;
  CHANNELS_Save(gScanlist[currentIndex], &ch);
}

void SCANLISTS_init() {
  gRedrawScreen = true;
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  count = gScanlistSize;
}

void SCANLISTS_update() {}

static void saveRenamed() { CHANNELS_Save(chNum, &ch); }

bool SCANLISTS_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  chNum = gScanlist[currentIndex];
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      IncDec16(&currentIndex, 0, count, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&currentIndex, 0, count, 1);
      return true;
    default:
      break;
    }
  }
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
      CHANNELS_LoadScanlist(key - KEY_1);
      count = gScanlistSize;
      currentIndex = 0;
      return true;
    case KEY_0:
      CHANNELS_LoadScanlist(15);
      count = gScanlistSize;
      currentIndex = 0;
      return true;
    case KEY_UP:
      IncDec16(&currentIndex, 0, count, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&currentIndex, 0, count, 1);
      return true;
    default:
      break;
    }
  }
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
      toggleScanlist(key - KEY_1);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_F:
      CHANNELS_Load(chNum, &ch);
      gTextinputText = ch.name;
      gTextInputSize = 9;
      gTextInputCallback = saveRenamed;
      APPS_run(APP_TEXTINPUT);

      return true;
    default:
      return false;
    }
  }

  return false;
}

void SCANLISTS_render() {
  UI_ClearScreen();
  if (gSettings.currentScanlist == 15) {
    STATUSLINE_SetText("CH scanlists");
    UI_ShowMenuEx(getChItem, count, currentIndex, MENU_LINES_TO_SHOW + 1);
  } else {
    STATUSLINE_SetText("CH scanlist #%u", gSettings.currentScanlist + 1);
    UI_ShowMenuEx(getScanlistItem, count, currentIndex, MENU_LINES_TO_SHOW + 1);
  }
}
