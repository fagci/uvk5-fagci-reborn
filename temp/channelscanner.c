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
  if (item->blacklist) {
    PrintMediumEx(2, y + 8, POS_L, C_INVERT, "X");
  }
  if (gIsListening && item->f == currentChannel.rx.f) {
    PrintSymbolsEx(LCD_WIDTH - 24, y + 8, POS_R, C_INVERT, "%c", SYM_BEEP);
  }
  PrintMediumEx(8, y + 8, POS_L, C_INVERT, "%s", ch.name);
  /* PrintSmallEx(LCD_WIDTH - 5, y + 8, POS_R, C_INVERT, "R%u",
               item->rssi); // TODO: antenna */
  FillRect(LCD_WIDTH - 15, y + 2, Rssi2PX(item->rssi, 0, 10), 6, C_INVERT);
}

static void scanFn(bool forward) {
  IncDecI32(&scanIndex, 0, gScanlistSize, forward ? 1 : -1);
  int32_t chNum = gScanlist[scanIndex];
  radio->channel = chNum;
  RADIO_VfoLoadCH(gSettings.activeVFO);
  RADIO_SetupByCurrentVFO();
}

void CHSCANNER_init(void) {
  currentIndex = 0;
  scanIndex = 0;
  LOOT_Clear();
  CHANNELS_LoadScanlist(gSettings.currentScanlist);
  for (uint16_t i = 0; i < gScanlistSize; ++i) {
    CH ch;
    int32_t num = gScanlist[i];
    CHANNELS_Load(num, &ch);
    Loot *loot = LOOT_AddEx(ch.rx.f, false);
    loot->open = false;
    loot->lastTimeOpen = 0;
  }

  /* if (gScanlistSize) {
    RADIO_TuneToPure(LOOT_Item(scanIndex)->f, true);
  } */

  gScanFn = scanFn;
  SVC_Toggle(SVC_SCAN, true, gSettings.scanTimeout);
}

void CHSCANNER_deinit(void) { SVC_Toggle(SVC_SCAN, false, 0); }

bool CHSCANNER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
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
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
      CHSCANNER_deinit();
      gSettings.currentScanlist = Key - KEY_1;
      CHSCANNER_init();
      currentIndex = 0;
      return true;
    case KEY_0:
      gSettings.currentScanlist = 15;
      CHSCANNER_init();
      currentIndex = 0;
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
      RADIO_SaveCurrentVFO();
      gVfo1ProMode = true;
      APPS_run(APP_VFO1);
      return true;
    case KEY_5:
      RADIO_TuneToCH(gScanlist[currentIndex]);
      RADIO_SaveCurrentVFO();
      APPS_run(APP_ANALYZER);
      return true;
    case KEY_SIDE1:
      LOOT_Item(currentIndex)->blacklist = !LOOT_Item(currentIndex)->blacklist;
      return true;

    default:
      break;
    }
  }
  return false;
}

void CHSCANNER_update(void) {
  if (gIsListening && LOOT_Item(scanIndex)->f != currentChannel.rx.f) {
    CHANNELS_Load(gScanlist[scanIndex], &currentChannel);
  }
}

void CHSCANNER_render(void) {
  UI_ClearScreen();

  if (gScanlistSize == 0) {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL,
                      "Scanlist empty.");
    return;
  }
  PrintMediumBoldEx(LCD_XCENTER, MENU_Y + 7, POS_C, C_FILL, "%s",
                    gIsListening ? currentChannel.name : "Scan...");

  UI_ShowMenuEx(showItem, gScanlistSize, currentIndex, 4);
}
