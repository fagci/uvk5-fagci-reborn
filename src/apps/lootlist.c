#include "lootlist.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_render.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"

static uint8_t menuIndex = 0;
static const uint8_t MENU_ITEM_H_LARGER = 15;

typedef enum {
  SORT_LOT,
  SORT_DUR,
  SORT_BL,
  SORT_F,
} Sort;

bool (*sortings[])(const Loot *a, const Loot *b) = {
    LOOT_SortByLastOpenTime,
    LOOT_SortByDuration,
    LOOT_SortByBlacklist,
    LOOT_SortByF,
};

static char *sortNames[] = {
    "last open",
    "duration",
    "blacklist",
    "freq",
};

static Sort sortType = SORT_LOT;

static bool shortList = true;
static bool sortRev = false;

static void tuneToLoot(const Loot *loot, bool save) {
  Preset *p = PRESET_ByFrequency(loot->f);
  radio->rx.f = loot->f;
  radio->tx.f = 0;
  radio->rx.codeType = radio->tx.codeType = CODE_TYPE_OFF;
  radio->rx.code = radio->tx.code = 0;

  radio->radio = p->radio;
  radio->modulation = p->band.modulation;
  radio->power = p->power;

  if (loot->cd != 0xFF) {
    radio->tx.codeType = CODE_TYPE_DIGITAL;
    radio->tx.code = loot->cd;
  } else if (loot->ct != 0xFF) {
    radio->tx.codeType = CODE_TYPE_CONTINUOUS_TONE;
    radio->tx.code = loot->ct;
  }

  if (save) {
    RADIO_TuneToSave(loot->f);
  } else {
    RADIO_TuneTo(loot->f);
  }
}

static void displayFreqBlWl(uint8_t y, const Loot *loot) {
  if (loot->blacklist) {
    PrintMediumEx(1, y + 7, POS_L, C_INVERT, "-");
  }
  if (loot->whitelist) {
    PrintMediumEx(1, y + 7, POS_L, C_INVERT, "+");
  }
  PrintMediumEx(8, y + 7, POS_L, C_INVERT, "%u.%05u", loot->f / MHZ,
                loot->f % MHZ);
  if (gIsListening && loot->f == radio->rx.f) {
    PrintSymbolsEx(LCD_WIDTH - 24, y + 7, POS_R, C_INVERT, "%c", SYM_BEEP);
  }
}

static void getLootItem(uint16_t i, uint16_t index, bool isCurrent) {
  const Loot *item = LOOT_Item(index);
  const uint8_t y = MENU_Y + i * MENU_ITEM_H_LARGER;

  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H_LARGER, C_FILL);
  }
  displayFreqBlWl(y, item);

  PrintSmallEx(LCD_WIDTH - 6, y + 7, POS_R, C_INVERT, "%us",
               item->duration / 1000);

  // PrintSmallEx(8, y + 7 + 6, POS_L, C_INVERT, "%03ddB", Rssi2DBm(item->rssi));
  if (item->ct != 0xFF) {
    PrintSmallEx(8 + 55, y + 7 + 6, POS_L, C_INVERT, "CT:%u.%uHz",
                 CTCSS_Options[item->ct] / 10, CTCSS_Options[item->ct] % 10);
  } else if (item->cd != 0xFF) {
    PrintSmallEx(8 + 55, y + 7 + 6, POS_L, C_INVERT, "DCS:D%03oN",
                 DCS_Options[item->cd]);
  }
}

static void getLootItemShort(uint16_t i, uint16_t index, bool isCurrent) {
  const Loot *loot = LOOT_Item(index);
  const uint8_t x = LCD_WIDTH - 6;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  const uint32_t ago = (Now() - loot->lastTimeOpen) / 1000;

  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  displayFreqBlWl(y, loot);

  switch (sortType) {
  case SORT_LOT:
    PrintSmallEx(x, y + 7, POS_R, C_INVERT, "%u:%02u", ago / 60, ago % 60);
    break;
  case SORT_DUR:
  case SORT_BL:
  case SORT_F:
    PrintSmallEx(x, y + 7, POS_R, C_INVERT, "%us", loot->duration / 1000);
    break;
  }
}

static void sort(Sort type) {
  if (sortType == type) {
    sortRev = !sortRev;
  } else {
    sortRev = type == SORT_DUR;
  }
  LOOT_Sort(sortings[type], sortRev);
  sortType = type;
  STATUSLINE_SetText("By %s %s", sortNames[sortType], sortRev ? "v" : "^");
}

void LOOTLIST_update() {
  if (Now() - gLastRender >= 500) {
    gRedrawScreen = true;
  }
}

void LOOTLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenuEx(shortList ? getLootItemShort : getLootItem, LOOT_Size(),
                menuIndex, shortList ? 5 : 3);
}

void LOOTLIST_init(void) {
  sortType = SORT_F;
  sort(SORT_LOT);
  if (LOOT_Size()) {
    tuneToLoot(LOOT_Item(menuIndex), false);
  }
}

static void saveLootToCh(const Loot *loot, int16_t chnum, uint8_t scanlist) {
  Preset *p = PRESET_ByFrequency(loot->f);
  CH ch = {
      .rx = {loot->f, CODE_TYPE_OFF, 0},
      .tx = {0, CODE_TYPE_OFF, 0},
      .radio = p->radio,
      .modulation = p->band.modulation,
      .memoryBanks = 1 << scanlist,
      .power = p->power,
      .bw = p->band.bw,
  };

  snprintf(ch.name, 9, "%u.%05u", ch.rx.f / MHZ, ch.rx.f % MHZ);

  if (loot->ct != 255) {
    ch.tx.codeType = CODE_TYPE_CONTINUOUS_TONE;
    ch.tx.code = loot->ct;
  } else if (loot->cd != 255) {
    ch.tx.codeType = CODE_TYPE_DIGITAL;
    ch.tx.code = loot->cd;
  }

  CHANNELS_Save(chnum, &ch);
}

static void saveToFreeChannels(bool saveWhitelist, uint8_t scanlist) {
  UI_ShowWait();
  for (uint16_t i = 0; i < LOOT_Size(); ++i) {
    uint16_t chnum = CHANNELS_GetCountMax();
    const Loot *loot = LOOT_Item(i);
    if (saveWhitelist && !loot->whitelist) {
      continue;
    }
    if (!saveWhitelist && !loot->blacklist) {
      continue;
    }

    while (chnum) {
      chnum--;
      if (CHANNELS_Existing(chnum)) {
        if (CHANNELS_GetRX(chnum).f == loot->f) {
          break;
        }
      } else {
        // save new
        saveLootToCh(loot, chnum, scanlist);
        break;
      }
    }
  }
}

bool LOOTLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  Loot *loot;
  if (SVC_Running(SVC_FC)) {
    loot = gLastActiveLoot;
  } else {
    loot = LOOT_Item(menuIndex);
  }
  const uint8_t MENU_SIZE = LOOT_Size();

  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_0:
      LOOT_Clear();
      RADIO_TuneToPure(0, true);
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_8:
      saveToFreeChannels(false, 7);
      return true;
    case KEY_5:
      saveToFreeChannels(true, gSettings.currentScanlist);
      return true;
    case KEY_4: // freq catch
      SVC_Toggle(SVC_FC, !SVC_Running(SVC_FC), 100);
      return true;
    default:
      break;
    }
  }

  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    loot = LOOT_Item(menuIndex);
    tuneToLoot(loot, false);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    loot = LOOT_Item(menuIndex);
    tuneToLoot(loot, false);
    return true;
  default:
    break;
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_PTT:
      tuneToLoot(loot, true);
      APPS_run(APP_VFOPRO);
      return true;
    case KEY_1:
      sort(SORT_LOT);
      return true;
    case KEY_2:
      sort(SORT_DUR);
      return true;
    case KEY_3:
      sort(SORT_BL);
      return true;
    case KEY_4:
      sort(SORT_F);
      return true;
    case KEY_SIDE1:
      loot->whitelist = false;
      loot->blacklist = !loot->blacklist;
      return true;
    case KEY_SIDE2:
      loot->blacklist = false;
      loot->whitelist = !loot->whitelist;
      return true;
    case KEY_7:
      shortList = !shortList;
      return true;
    case KEY_9:
      return true;
    case KEY_5:
      tuneToLoot(loot, false);
      APPS_run(APP_SAVECH);
      return true;
    case KEY_0:
      LOOT_Remove(menuIndex);
      if (menuIndex > LOOT_Size() - 1) {
        menuIndex = LOOT_Size() - 1;
      }
      loot = LOOT_Item(menuIndex);
      if (loot) {
        tuneToLoot(loot, false);
      } else {
        RADIO_TuneToPure(0, true);
      }
      return true;
    case KEY_MENU:
      tuneToLoot(loot, true);
      APPS_exit();
      return true;
    default:
      break;
    }
  }

  return false;
}
