#include "savech.h"
#include "../helper/channels.h"
#include "../helper/measurements.h"
#include "../helper/numnav.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "textinput.h"
#include "vfo1.h"

static uint16_t currentChannelIndex = 0;
static uint16_t chCount = 0;
static char tempName[9] = {0};
static uint16_t chNum = 0;
static CH ch;

static int16_t from = -1;
static int16_t to = -1;

static void getChItem(uint16_t i, uint16_t index, bool isCurrent) {
  if (gSettings.currentScanlist != 15) {
    index = gScanlist[index];
  }
  CH _ch;
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

static void saveNamed(void) {
  CH _ch = *radio;
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

static void moveRange() {
  UI_ShowWait();
  int16_t f = from;
  int16_t t = to;
  if (f > t) {
    SWAP(f, t);
  }

  CH _ch;
  uint16_t offset = 0;
  for (uint16_t i = f; i <= t; i++) {
    CHANNELS_Load(i, &_ch);
    CHANNELS_Save(currentChannelIndex + offset, &_ch);
    CHANNELS_Delete(i);
    offset++;
  }

  from = to = -1;
}

void SAVECH_init(void) {
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  if (gSettings.currentScanlist == 15) {
    chCount = CHANNELS_GetCountMax();
  } else {
    chCount = gScanlistSize;
  }
  if (currentChannelIndex > chCount) {
    currentChannelIndex = chCount;
  }
}

void SAVECH_update(void) {}

static void save(void) {
  gTextinputText = tempName;
  snprintf(gTextinputText, 9, "%lu.%05lu", radio->rxF / MHZ, radio->rxF % MHZ);
  gTextInputSize = 9;
  gTextInputCallback = saveNamed;
  APPS_run(APP_TEXTINPUT);
}

static void setMenuIndex(uint16_t v) { currentChannelIndex = v - 1; }

static void toggleScanlist(uint16_t idx, uint8_t n) {
  CH _ch;
  uint16_t _chNum = gScanlist[idx];
  if (gSettings.currentScanlist == 15) {
    _chNum = idx;
  }
  CHANNELS_Load(_chNum, &_ch);
  _ch.scanlists ^= 1 << n;
  CHANNELS_Save(_chNum, &_ch);
}

bool SAVECH_SelectScanlist(KEY_Code_t key) {
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
    from = -1;
    return true;
  case KEY_9:
    from = -1;
    return true;
  case KEY_0:
    CHANNELS_LoadScanlist(15);
    chCount = CHANNELS_GetCountMax();
    currentChannelIndex = 0;
    from = -1;
    return true;
  default:
    break;
  }
  return false;
}

bool SAVECH_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    if (!gIsNumNavInput && key == KEY_STAR) {
      NUMNAV_Init(currentChannelIndex + 1, 1, chCount);
      gNumNavCallback = setMenuIndex;
      return true;
    }
    if (gIsNumNavInput) {
      currentChannelIndex = NUMNAV_Input(key) - 1;
      return true;
    }
  }
  if (gSettings.currentScanlist != 15) {
    chNum = gScanlist[currentChannelIndex];
  } else {
    chNum = currentChannelIndex;
  }
  if (bKeyPressed || !bKeyHeld) {
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
    if (SAVECH_SelectScanlist(key)) {
      return true;
    }
  }
  int16_t f = from;
  int16_t t = currentChannelIndex;
  if (f > t) {
    SWAP(f, t);
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
      if (from != -1) {
        for (uint16_t i = f; i <= t; i++) {
          toggleScanlist(i, key - KEY_1);
        }
        from = -1;
        return true;
      }
      toggleScanlist(currentChannelIndex, key - KEY_1);
      return true;
    case KEY_MENU:
      save();
      from = -1;
      return true;
    case KEY_0:
      if (from != -1) {
        for (uint16_t i = f; i <= t; i++) {
          CHANNELS_Delete(i);
        }
        from = -1;
        return true;
      }
      CHANNELS_Delete(chNum);
      return true;
    case KEY_9:
      from = currentChannelIndex;
      return true;
    case KEY_PTT:
      RADIO_TuneToCH(chNum);
      RADIO_SaveCurrentVFO();
      gVfo1ProMode = true;
      APPS_run(APP_VFO1);
      from = -1;
      return true;
    case KEY_F:
      if (from != -1) {
        if (to != -1) {
          moveRange();
          return true;
        }
        to = currentChannelIndex;
        return true;
      }
      CHANNELS_Load(chNum, &_ch);
      gTextinputText = _ch.name;
      gTextInputSize = 9;
      gTextInputCallback = saveRenamed;
      APPS_run(APP_TEXTINPUT);
      from = -1;
      return true;
    case KEY_EXIT:
      APPS_exit();
      from = -1;
      return true;
    default:
      break;
    }
  }
  return false;
}

void SAVECH_render(void) {
  UI_ShowMenuEx(getChItem,
                gSettings.currentScanlist == 15 ? CHANNELS_GetCountMax()
                                                : gScanlistSize,
                currentChannelIndex, MENU_LINES_TO_SHOW + 1);
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else if (from != -1) {
    int16_t f = from + 1;
    int16_t t = to == -1 ? currentChannelIndex + 1 : to + 1;
    if (f > t) {
      SWAP(f, t);
    }
    if (to == -1) {
      STATUSLINE_SetText("%d-%d", f, t);
    } else {
      STATUSLINE_SetText("M%d-%d", f, t);
    }
  }
}
