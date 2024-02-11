#include "channelscanner.h"
#include "../driver/system.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"

static uint16_t currentIndex = 0;
static uint8_t scanIndex = 0;
static const uint8_t LIST_Y = MENU_Y + 10;

int16_t lastActiveLootIndex = -1;

static void showItem(uint16_t i, uint16_t index, bool isCurrent) {
  CH ch;
  uint16_t chNum = gScanlist[index];
  CHANNELS_Load(chNum, &ch);
  Loot *item = LOOT_Item(index);
  const uint8_t y = LIST_Y + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "R%u",
               item->rssi); // TODO: antenna
  if (item->open) {
    FillRect(1, y + 1, 3, MENU_ITEM_H - 2, C_INVERT);
  }
}

static void scanFn(bool forward) {
  RADIO_UpdateMeasurementsEx(LOOT_Item(scanIndex));

  if (gIsListening) {
    lastActiveLootIndex = scanIndex;
  }

  if (!gIsListening) {
    IncDec8(&scanIndex, 0, gScanlistSize, forward ? 1 : -1);
    RADIO_TuneToPure(LOOT_Item(scanIndex)->f);
  }
}

void CHSCANNER_init(void) {
  currentIndex = 0;
  scanIndex = 0;
  LOOT_Clear();
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  for (uint16_t i = 0; i < gScanlistSize; ++i) {
    CH ch;
    uint16_t num = gScanlist[i];
    CHANNELS_Load(num, &ch);
    Loot *loot = LOOT_AddEx(ch.fRX, false);
    loot->open = false;
    loot->lastTimeOpen = 0;
  }
  if (gScanlistSize) {
    RADIO_TuneToPure(LOOT_Item(scanIndex)->f);
  }

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 10);
}

void CHSCANNER_deinit(void) { SVC_Toggle(SVC_SCAN, false, 0); }

bool CHSCANNER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || (bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_UP:
      IncDec16(&currentIndex, 0, gScanlistSize, -1);
      return true;
    case KEY_DOWN:
      IncDec16(&currentIndex, 0, gScanlistSize, 1);
      return true;
    default:
      break;
    }
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

void CHSCANNER_update(void) {}

void CHSCANNER_render(void) {
  UI_ClearScreen();

  if (gScanlistSize == 0) {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL,
                      "Scanlist empty.");
    return;
  }

  if (lastActiveLootIndex >= 0) {
    CH ch;
    CHANNELS_Load(gScanlist[lastActiveLootIndex], &ch);
    PrintMediumBoldEx(LCD_XCENTER, MENU_Y + 7, POS_C, C_FILL, "%s", ch.name);
  } else {
    PrintMediumBoldEx(LCD_XCENTER, MENU_Y + 7, POS_C, C_FILL, "Scanning...");
  }
  UI_ShowMenuEx(showItem, gScanlistSize, currentIndex, 4);
}
