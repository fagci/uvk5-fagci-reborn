#include "lootlist.h"
#include "../dcs.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/bandlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "../ui/statusline.h"
#include "apps.h"
#include <string.h>

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

static void exportLootList() {
  UART_printf("--- 8< ---\n");
  UART_printf("F,duration,ct,cd,rssi,noise\n");
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *v = LOOT_Item(i);
    if (!v->blacklist) {
      UART_printf("%u.%05u,%u,%u,%u,%u\n", v->f / 100000, v->f % 100000,
                  v->duration, v->ct, v->cd, v->rssi);
    }
  }
  UART_printf("--- >8 ---\n");
  UART_flush();
}

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
  const uint32_t ago = (elapsedMilliseconds - item->lastTimeOpen) / 1000;
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

void LOOTLIST_render() {
  UI_ClearScreen();
  UI_ShowMenuEx(shortList ? getLootItemShort : getLootItem, LOOT_Size(),
                menuIndex, shortList ? 5 : 3);
}

void LOOTLIST_init() {
  gRedrawScreen = true;
  sortType = SORT_F;
  sort(SORT_LOT);
  if (LOOT_Size()) {
    Loot *item = LOOT_Item(menuIndex);
    RADIO_TuneTo(item->f);
  }
}

static void saveAllToFreeChannels() {
  uint16_t chnum = 0;
  for (uint8_t i = 0; i < LOOT_Size(); ++i) {
    Loot *loot = LOOT_Item(i);
    if (!loot->blacklist) {
      while (CHANNELS_Existing(chnum)) {
        chnum++;
        if (chnum >= CHANNELS_GetCountMax()) {
          return;
        }
      }
      CH ch;
      ch.f = loot->f;
      snprintf(ch.name, 9, "%lu.%05lu", ch.f / 100000, ch.f % 100000);

      CHANNELS_Save(chnum, &ch);
      loot->blacklist = true;
      chnum++;
    }
  }
}

void LOOTLIST_update() {}

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
      LOOT_RemoveBlacklisted();
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
      exportLootList();
      return true;
    case KEY_5:
      radio->f = item->f;
      radio->codeTX = item->cd != 0xFF ? item->cd : item->ct;
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


static App meta = {
    .id = APP_LOOT_LIST,
    .name = "LOOTLIST",
    .runnable = true,
    .init = LOOTLIST_init,
    .update = LOOTLIST_update,
    .render = LOOTLIST_render,
    .key = LOOTLIST_key,
};
App *LOOTLIST_Meta() { return &meta; }
