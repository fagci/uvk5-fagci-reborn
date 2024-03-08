#include "channelscanner.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../svc.h"
#include "../svc_scan.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"
#include "apps.h"

static int32_t currentIndex = 0;
static int32_t scanIndex = 0;
static const uint8_t LIST_Y = MENU_Y + 10;

static CH currentChannel;

static void showItem(uint16_t i, uint16_t index, bool isCurrent) {
  CH ch;
  int32_t chNum = gScanlist[index];
  CHANNELS_Load(chNum, &ch);
  Loot *item = LOOT_Item(index);
  const uint8_t y = LIST_Y + i * MENU_ITEM_H;
  if (isCurrent) {
    FillRect(0, y, LCD_WIDTH - 3, MENU_ITEM_H, C_FILL);
  }
  PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "R%u",
               item->rssi); // TODO: antenna
}

static void scanFn(bool forward) {
  if (gIsListening) {
    currentIndex = scanIndex;
  } else {
    IncDecI32(&scanIndex, 0, gScanlistSize, forward ? 1 : -1);
    RADIO_TuneToPure(LOOT_Item(scanIndex)->f, true);
  }
}

void CHSCANNER_init() {
  currentIndex = 0;
  scanIndex = 0;
  LOOT_Clear();
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  for (uint16_t i = 0; i < gScanlistSize; ++i) {
    CH ch;
    int32_t num = gScanlist[i];
    CHANNELS_Load(num, &ch);
    Loot *loot = LOOT_AddEx(ch.f, false);
    loot->open = false;
    loot->lastTimeOpen = 0;
  }

  if (gScanlistSize) {
    RADIO_TuneToPure(LOOT_Item(scanIndex)->f, true);
  }

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, 10);
}

void CHSCANNER_deinit() { SVC_Toggle(SVC_SCAN, false, 0); }

bool CHSCANNER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed || (bKeyPressed && !bKeyHeld)) {
    switch (Key) {
    case KEY_UP:
      IncDecI32(&currentIndex, 0, gScanlistSize, -1);
      return true;
    case KEY_DOWN:
      IncDecI32(&currentIndex, 0, gScanlistSize, 1);
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
    case KEY_PTT:
      RADIO_TuneToCH(gScanlist[currentIndex]);
      RADIO_SaveCurrentCH();
      APPS_run(APP_STILL);
      return true;
    case KEY_5:
      RADIO_TuneToCH(gScanlist[currentIndex]);
      RADIO_SaveCurrentCH();
      APPS_run(APP_ANALYZER);
      return true;

    default:
      break;
    }
  }
  return false;
}

void CHSCANNER_update() {}

void CHSCANNER_render() {
  UI_ClearScreen();

  if (gScanlistSize == 0) {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL,
                      "Scanlist empty.");
    return;
  }

  if (gIsListening) {
    currentIndex = scanIndex; // HACK
    if (LOOT_Item(scanIndex)->f != currentChannel.f) {
      CHANNELS_Load(gScanlist[scanIndex], &currentChannel);
    }
    PrintMediumBoldEx(LCD_XCENTER, MENU_Y + 7, POS_C, C_FILL, "%s",
                      currentChannel.name);
  } else {
    PrintMediumBoldEx(LCD_XCENTER, MENU_Y + 7, POS_C, C_FILL, "Scanning...");
  }
  UI_ShowMenuEx(showItem, gScanlistSize, currentIndex, 4);
}


static App meta = {
    .id = APP_CH_SCANNER,
    .name = "CHSCANNER",
    .runnable = true,
    .init = CHSCANNER_init,
    .update = CHSCANNER_update,
    .render = CHSCANNER_render,
    .key = CHSCANNER_key,
    .deinit = CHSCANNER_deinit,
};

App *CHSCANNER_Meta() { return &meta; }
