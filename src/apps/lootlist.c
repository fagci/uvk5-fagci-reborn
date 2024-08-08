#include "lootlist.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../scheduler.h"
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

bool (*sortings[])(Loot *a, Loot *b) = {
    LOOT_SortByLastOpenTime,
    LOOT_SortByDuration,
    LOOT_SortByBlacklist,
    LOOT_SortByF,
};

static char *sortNames[] = {
    "last open time",
    "duration",
    "blacklist",
    "frequency",
};

Sort sortType = SORT_LOT;

static bool shortList = true;
static bool sortRev = false;

static void getLootItem(uint16_t i, uint16_t index, bool isCurrent) {
  Loot *item = LOOT_Item(index);
  uint32_t f = item->f;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H_LARGER;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H_LARGER, C_FILL);
  }
  PrintMediumEx(8, y + 7, POS_L, C_INVERT, "%u.%05u", f / 100000, f % 100000);
  PrintSmallEx(LCD_WIDTH - 6, y + 7, POS_R, C_INVERT, "%us",
               item->duration / 1000);

  PrintSmallEx(6, y + 7 + 6, POS_L, C_INVERT, "R:%03u", item->rssi);
  if (item->cd != 0xFF) {
    PrintSmallEx(6 + 55, y + 7 + 6, POS_L, C_INVERT, "DCS:D%03oN",
                 DCS_Options[item->cd]);
  }
  if (item->ct != 0xFF) {
    PrintSmallEx(6 + 55, y + 7 + 6, POS_L, C_INVERT, "CT:%u.%uHz",
                 CTCSS_Options[item->ct] / 10, CTCSS_Options[item->ct] % 10);
  }
  if (item->blacklist) {
    PrintSmallEx(1, y + 5, POS_L, C_INVERT, "X");
  }
  if (item->goodKnown) {
    PrintSmallEx(1, y + 5, POS_L, C_INVERT, "*");
  }
}

static void getLootItemShort(uint16_t i, uint16_t index, bool isCurrent) {
  Loot *item = LOOT_Item(index);
  uint32_t f = item->f;
  const uint8_t x = LCD_WIDTH - 6;
  const uint8_t y = MENU_Y + i * MENU_ITEM_H;
  const uint32_t ago = (Now() - item->lastTimeOpen) / 1000;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  PrintMediumEx(8, y + 7, POS_L, C_INVERT, "%u.%05u", f / 100000, f % 100000);
  switch (sortType) {
  case SORT_LOT:
    PrintSmallEx(x, y + 7, POS_R, C_INVERT, "%u:%02u", ago / 60, ago % 60);
    break;
  case SORT_DUR:
  case SORT_BL:
  case SORT_F:
    PrintSmallEx(x, y + 7, POS_R, C_INVERT, "%us", item->duration / 1000);
    break;
  }
  if (item->blacklist) {
    PrintMediumEx(1, y + 7, POS_L, C_INVERT, "-");
  }
  if (item->goodKnown) {
    PrintMediumEx(1, y + 7, POS_L, C_INVERT, "+");
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
  STATUSLINE_SetText("By %s %s", sortNames[sortType], sortRev ? "desc" : "asc");
}

void LOOTLIST_render(void) {
  UI_ClearScreen();
  UI_ShowMenuEx(shortList ? getLootItemShort : getLootItem, LOOT_Size(),
                menuIndex, shortList ? 5 : 3);
}

void LOOTLIST_init(void) {
  gRedrawScreen = true;
  sortType = SORT_F;
  sort(SORT_LOT);
  if (LOOT_Size()) {
    Loot *item = LOOT_Item(menuIndex);
    RADIO_TuneTo(item->f);
  }
}

static void saveAllToFreeChannels(void) {
  uint16_t chnum = CHANNELS_GetCountMax() - 1;
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *loot = LOOT_Item(i);
    if (loot->goodKnown) {
      while (CHANNELS_Existing(chnum)) {
        chnum--;
        if (chnum == 0) {
          return;
        }
      }
      CH ch = {0};
      ch.rx.f = loot->f;
      ch.tx.f = 0;
      if (loot->ct != 255) {
        ch.tx.codeType = CODE_TYPE_CONTINUOUS_TONE;
        ch.tx.code = loot->ct;
      } else if (loot->cd != 255) {
        ch.tx.codeType = CODE_TYPE_DIGITAL;
        ch.tx.code = loot->cd;
      }
      ch.memoryBanks = 1 << 7;
      snprintf(ch.name, 9, "%lu.%05lu", ch.rx.f / 100000, ch.rx.f % 100000);

      CHANNELS_Save(chnum, &ch);
      chnum--;
    }
  }
}

bool LOOTLIST_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  Loot *item = LOOT_Item(menuIndex);
  const uint8_t MENU_SIZE = LOOT_Size();

  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_0:
      LOOT_Clear();
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_5:
      saveAllToFreeChannels();
      return true;
    default:
      break;
    }
  }

  switch (key) {
  case KEY_UP:
    IncDec8(&menuIndex, 0, MENU_SIZE, -1);
    item = LOOT_Item(menuIndex);
    RADIO_TuneTo(item->f);
    return true;
  case KEY_DOWN:
    IncDec8(&menuIndex, 0, MENU_SIZE, 1);
    item = LOOT_Item(menuIndex);
    RADIO_TuneTo(item->f);
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
      RADIO_TuneToSave(item->f);
      APPS_run(APP_STILL);
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
      item->goodKnown = false;
      item->blacklist = !item->blacklist;
      return true;
    case KEY_SIDE2:
      item->blacklist = false;
      item->goodKnown = !item->goodKnown;
      return true;
    case KEY_7:
      shortList = !shortList;
      return true;
    case KEY_9:
      return true;
    case KEY_5:
      radio->rx.f = item->f;
      radio->tx.code = item->cd != 0xFF ? item->cd : item->ct;
      APPS_run(APP_SAVECH);
      return true;
    case KEY_0:
      LOOT_Remove(menuIndex);
      return true;
    case KEY_MENU:
      RADIO_TuneToSave(item->f);
      APPS_exit();
      return true;
    default:
      break;
    }
  }

  return false;
}
