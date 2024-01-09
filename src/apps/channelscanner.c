#include "channelscanner.h"
#include "../driver/system.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "../ui/menu.h"

static uint16_t currentIndex = 0;
static uint16_t scanIndex = 0;
static const uint8_t LIST_Y = MENU_Y + 8;

Loot msm = {0};

static uint8_t ro = 255;
static uint8_t rc = 255;
static uint8_t no = 0;
static uint8_t nc = 0;

static bool isSquelchOpen() {
  bool open = msm.rssi > ro && msm.noise < no;

  if (msm.rssi < rc || msm.noise > nc) {
    open = false;
  }
  return open;
}

static void msmUpdate() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  msm.open = isSquelchOpen();

  if (msm.open != gIsListening) {
    gRedrawScreen = true;
  }

  LOOT_Update(&msm);
  RADIO_ToggleRX(msm.open);
}

static void setup() {
  Loot *item = LOOT_Item(scanIndex);
  msm.f = item->f;

  uint32_t oldF = gCurrentVFO->fRX;
  uint8_t band = msm.f > VHF_UHF_BOUND ? 1 : 0;
  gCurrentVFO->fRX = msm.f;
  RADIO_SetupByCurrentVFO();
  gCurrentVFO->fRX = oldF;

  uint8_t sql = gCurrentPreset->band.squelch;
  ro = SQ[band][0][sql];
  rc = SQ[band][1][sql];
  no = SQ[band][2][sql];
  nc = SQ[band][3][sql];
}

static void step() {
  msmUpdate();

  if (!msm.open) {
    IncDec16(&scanIndex, 0, gScanlistSize, 1);
    setup();
  }
}

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
  if (item->open) {
    FillRect(1, y + 1, 3, MENU_ITEM_H - 2, C_INVERT);
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
    LOOT_AddEx(ch.fRX, false);
  }
  setup();
}

void CHSCANNER_deinit(void) { RADIO_ToggleRX(false); }

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
    return true;
  }
  return false;
}

static uint32_t lastUpdate = 0;

void CHSCANNER_update(void) {
  if (elapsedMilliseconds - lastUpdate >= 10) {
    lastUpdate = elapsedMilliseconds;
    step();
  }
}

void CHSCANNER_render(void) {
  UI_ClearScreen();
  if (gLastActiveLootIndex >= 0) {
    CH ch;
    CHANNELS_Load(gScanlist[gLastActiveLootIndex], &ch);
    PrintMediumBoldEx(LCD_WIDTH / 2, MENU_Y + 8, POS_C, C_FILL, "%s", ch.name);
  }
  UI_ShowMenuEx(showItem, gScanlistSize, currentIndex, 4);
}
