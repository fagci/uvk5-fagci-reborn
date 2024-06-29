#include "savech.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "textinput.h"

static uint16_t currentChannelIndex = 0;
static uint16_t chCount = 0;
static char tempName[9] = {0};
static uint16_t chNum = 0;
static CH ch;

static void getChItem(uint16_t i, uint16_t index, bool isCurrent) {
  CH _ch;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  CHANNELS_Load(index, &_ch);
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (IsReadable(_ch.name)) {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", _ch.name);
  } else {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
    return;
  }
  char scanlistsStr[9] = "";
  for (uint8_t i = 0; i < 8; ++i) {
    scanlistsStr[i] = _ch.memoryBanks & (1 << i) ? '1' + i : '-';
  }
  PrintSmallEx(LCD_WIDTH - 1, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void getScanlistItem(uint16_t i, uint16_t index, bool isCurrent) {
  uint16_t chNum = gScanlist[index];
  CH _ch;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  CHANNELS_Load(chNum, &_ch);
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  if (IsReadable(_ch.name)) {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", _ch.name);
  } else {
    PrintMediumEx(8, y + 8, POS_L, C_INVERT, "CH-%u", index + 1);
    return;
  }
  char scanlistsStr[9] = "";
  for (uint8_t i = 0; i < 8; ++i) {
    scanlistsStr[i] = _ch.memoryBanks & (1 << i) ? '1' + i : '-';
  }
  PrintSmallEx(LCD_WIDTH - 1 - 3, y + 8, POS_R, C_INVERT, "%s", scanlistsStr);
}

static void saveNamed(void) {
  CH _ch;
  VFO2CH(radio, gCurrentPreset, &_ch);
  _ch.memoryBanks = 1 << gSettings.currentScanlist;
  strncpy(_ch.name, tempName, 9);
  CHANNELS_Save(currentChannelIndex, &_ch);
  for (uint8_t i = 0; i < 2; ++i) {
    if (gVFO[i].channel >= 0 && gVFO[i].channel == currentChannelIndex) {
      RADIO_VfoLoadCH(i);
      break;
    }
  }
}
static void saveRenamed() { CHANNELS_Save(chNum, &ch); }

void SAVECH_init(void) {
  gRedrawScreen = true;
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  if (gSettings.currentScanlist == 15) {
    chCount = CHANNELS_GetCountMax();
  } else {
    chCount = gScanlistSize;
  }
}

void SAVECH_update(void) {}

static void save(void) {
  gTextinputText = tempName;
  snprintf(gTextinputText, 9, "%lu.%05lu", radio->rx.f / 100000,
           radio->rx.f % 100000);
  gTextInputSize = 9;
  gTextInputCallback = saveNamed;
  APPS_run(APP_TEXTINPUT);
}

static void setMenuIndexAndRun(uint16_t v) {
  currentChannelIndex = v - 1;
  save();
}
#include "../driver/uart.h"
static void toggleScanlist(uint8_t n) {
  CH _ch;
  uint16_t chNum = gScanlist[currentChannelIndex];
  if (gSettings.currentScanlist == 15) {
    chNum = currentChannelIndex;
  }
  CHANNELS_Load(chNum, &_ch);
  Log("i:%d, ch:%d, name: %s", currentChannelIndex, chNum, _ch.name);
  _ch.memoryBanks ^= 1 << n;
  CHANNELS_Save(chNum, &_ch);
}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  chNum = gScanlist[currentChannelIndex];
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key == KEY_STAR) {
      NUMNAV_Init(currentChannelIndex + 1, 1, chCount);
      gNumNavCallback = setMenuIndexAndRun;
      return true;
    }
    if (gIsNumNavInput) {
      currentChannelIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      IncDec16(&currentChannelIndex, 0, chCount, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&currentChannelIndex, 0, chCount, 1);
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
      chCount = gScanlistSize;
      currentChannelIndex = 0;
      return true;
    case KEY_0:
      CHANNELS_LoadScanlist(15);
      chCount = CHANNELS_GetCountMax();
      currentChannelIndex = 0;
      return true;
    default:
      break;
    }
  }
  CH _ch;
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
    case KEY_MENU:
      save();
      return true;
    case KEY_0:
      CHANNELS_Delete(currentChannelIndex);
      return true;
    case KEY_PTT:
      CHANNELS_Load(currentChannelIndex, &_ch);
      RADIO_TuneToSave(_ch.rx.f);
      APPS_run(APP_STILL);
      return true;
    case KEY_F:
      CHANNELS_Load(chNum, &_ch);
      gTextinputText = _ch.name;
      gTextInputSize = 9;
      gTextInputCallback = saveRenamed;
      APPS_run(APP_TEXTINPUT);
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

void SAVECH_render(void) {
  UI_ClearScreen();
  if (gSettings.currentScanlist == 15) {
    STATUSLINE_SetText(apps[APP_SAVECH].name);
    UI_ShowMenuEx(getChItem, chCount, currentChannelIndex,
                  MENU_LINES_TO_SHOW + 1);
  } else {
    STATUSLINE_SetText("Current scanlist: %u", gSettings.currentScanlist + 1);
    UI_ShowMenuEx(getScanlistItem, chCount, currentChannelIndex,
                  MENU_LINES_TO_SHOW + 1);
  }
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  }
}
