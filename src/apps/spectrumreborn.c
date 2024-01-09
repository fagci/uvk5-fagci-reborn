#include "spectrumreborn.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"
#include <string.h>

#define DATA_LEN 84

static const uint16_t U16_MAX = 65535;

static const uint8_t S_HEIGHT = 40;

static const uint8_t SPECTRUM_Y = 16;
static const uint8_t S_BOTTOM = SPECTRUM_Y + S_HEIGHT;

static uint16_t rssiHistory[DATA_LEN] = {0};
static uint16_t noiseHistory[DATA_LEN] = {0};
static bool markers[DATA_LEN] = {0};

static uint8_t x;

static Band *currentBand;

static uint32_t currentStepSize;
static uint8_t exLen;
static uint16_t stepsCount;
static uint16_t currentStep;
static uint32_t bandwidth;

static bool newScan = true;

static uint8_t ro = 255;
static uint8_t rc = 255;
static uint8_t no = 0;
static uint8_t nc = 0;

static uint8_t msmDelay = 10;

static bool noListen = false;

static uint16_t oldPresetIndex = 255;

static bool bandFilled = false;

static uint32_t lastRender = 0;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static void resetRssiHistory() {
  for (uint8_t x = 0; x < DATA_LEN; ++x) {
    rssiHistory[x] = 0;
    noiseHistory[x] = 0;
    markers[x] = false;
  }
}

static Loot msm = {0};

static bool isSquelchOpen() {
  bool open = msm.rssi > ro && msm.noise < no;

  if (msm.rssi < rc || msm.noise > nc) {
    open = false;
  }
  return open;
}

static void updateMeasurements() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();

  msm.open = isSquelchOpen();

  LOOT_Update(&msm);

  if (exLen) {
    for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
      x = DATA_LEN * currentStep / stepsCount + exIndex;
      rssiHistory[x] = msm.rssi;
      noiseHistory[x] = msm.noise;
      markers[x] = msm.open;
    }
  } else {
    x = DATA_LEN * currentStep / stepsCount;
    if (msm.rssi > rssiHistory[x]) {
      rssiHistory[x] = msm.rssi;
    }
    if (msm.noise < noiseHistory[x]) {
      noiseHistory[x] = msm.noise;
    }
    if (markers[x] == false && msm.open) {
      markers[x] = true;
    }
  }
}

static void writeRssi() {
  updateMeasurements();

  if (bandFilled && !noListen) {
    RADIO_ToggleRX(msm.open);
  }

  if (gIsListening) {
    return;
  }

  msm.f += currentStepSize;
  currentStep++;
}

static void setF() {
  msm.rssi = 0;
  msm.blacklist = false;
  msm.noise = U16_MAX;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t lx = DATA_LEN * currentStep / stepsCount + exIndex;
    noiseHistory[lx] = U16_MAX;
    rssiHistory[lx] = 0;
    markers[lx] = false;
  }
  // need to run when task activated, coz of another tasks exists between
  /* BK4819_SetFrequency(msm.f);
  BK4819_WriteRegister(BK4819_REG_30, 0x200);
  BK4819_WriteRegister(BK4819_REG_30, 0xBFF1); */
  BK4819_TuneTo(msm.f);
  SYSTEM_DelayMs(msmDelay); // (X_X)
  writeRssi();
}

static void step() { setF(); }

static void updateStats() {
  /* const uint16_t noiseFloor = Std(rssiHistory, x);
  const uint16_t noiseMax = Max(noiseHistory, x); */
}

static void startNewScan() {
  currentStep = 0;
  currentBand = &gCurrentPreset->band;
  currentStepSize = StepFrequencyTable[currentBand->step];

  bandwidth = currentBand->bounds.end - currentBand->bounds.start;

  stepsCount = bandwidth / currentStepSize;
  exLen = ceilDiv(DATA_LEN, stepsCount);

  msm.f = currentBand->bounds.start;

  if (gSettings.activePreset != oldPresetIndex) {
    resetRssiHistory();
    LOOT_Standby();
    RADIO_SetupBandParams(currentBand);
    uint8_t band = msm.f > VHF_UHF_BOUND ? 1 : 0;
    uint8_t sql = gCurrentPreset->band.squelch;
    ro = SQ[band][0][sql];
    rc = SQ[band][1][sql];
    no = SQ[band][2][sql];
    nc = SQ[band][3][sql];

    oldPresetIndex = gSettings.activePreset;
    gRedrawScreen = true;
    bandFilled = false;
  } else {
    bandFilled = true;
  }
}

void SPECTRUM_init(void) {
  newScan = true;
  step();
}

void SPECTRUM_deinit() { RADIO_ToggleRX(false); }

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    if (Key == KEY_SIDE1) {
      noListen = !noListen;
      RADIO_ToggleRX(false);
      return true;
    }
    if (Key == KEY_0) {
      LOOT_Clear();
      return true;
    }
  }

  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_UP:
      PRESETS_SelectPresetRelative(true);
      newScan = true;
      return true;
    case KEY_DOWN:
      PRESETS_SelectPresetRelative(false);
      newScan = true;
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      return true;
    case KEY_SIDE2:
      LOOT_GoodKnownLast();
      return true;
    case KEY_F:
      APPS_run(APP_PRESET_CFG);
      return true;
    case KEY_0:
      APPS_run(APP_PRESETS_LIST);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_5:
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      resetRssiHistory();
      newScan = true;
      oldPresetIndex = 255;
      bandFilled = false;
      return true;
    case KEY_9:
      if (gCurrentPreset->band.squelch > 1) {
        RADIO_UpdateSquelchLevel(false);
      }
      resetRssiHistory();
      newScan = true;
      oldPresetIndex = 255;
      bandFilled = false;
      return true;
    case KEY_2:
      IncDec8(&msmDelay, 0, 20, 1);
      resetRssiHistory();
      newScan = true;
      bandFilled = false;
      return true;
    case KEY_8:
      IncDec8(&msmDelay, 0, 20, -1);
      resetRssiHistory();
      newScan = true;
      bandFilled = false;
      return true;
    case KEY_PTT:
      RADIO_TuneToSave(gLastActiveLoot->f);
      APPS_run(APP_STILL);
      return true;
    case KEY_1:
      UART_ToggleLog(true);
      return true;
    case KEY_7:
      UART_ToggleLog(false);
      return true;
    default:
      break;
    }
  }
  return false;
}

void SPECTRUM_update(void) {
  if (msm.rssi == 0) {
    return;
  }
  if (newScan || gSettings.activePreset != oldPresetIndex) {
    newScan = false;
    startNewScan();
  }
  if (gIsListening) {
    updateMeasurements();
    gRedrawScreen = true;
    if (!msm.open) {
      RADIO_ToggleRX(false);
    }
    return;
  }

  if (!bandFilled && elapsedMilliseconds - lastRender >= 1000) {
    lastRender = elapsedMilliseconds;
    gRedrawScreen = true;
  }

  if (msm.f >= currentBand->bounds.end) {
    updateStats();
    gRedrawScreen = true;
    newScan = true;
    return;
  }

  step();
}

static int RssiMin(uint16_t *array, uint8_t n) {
  uint8_t min = array[0];
  for (uint8_t i = 1; i < n; ++i) {
    if (array[i] == 0) {
      // UART_logf(1, "MIN=0 at %i", i);
      continue;
    }
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

void SPECTRUM_render(void) {
  UI_ClearScreen();
  STATUSLINE_SetText(gCurrentPreset->band.name);
  DrawVLine(DATA_LEN - 1, 8, LCD_HEIGHT - 8, C_FILL);

  UI_DrawTicks(0, DATA_LEN - 1, 56, &gCurrentPreset->band);

  const uint8_t xMax = bandFilled ? DATA_LEN - 1 : x;

  const uint16_t rssiMin = RssiMin(rssiHistory, xMax);
  const uint16_t rssiMax = Max(rssiHistory, xMax);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  for (uint8_t xx = 0; xx < xMax; ++xx) {
    uint8_t yVal = ConvertDomain(rssiHistory[xx], vMin, vMax, 0, S_HEIGHT);
    DrawVLine(xx, S_BOTTOM - yVal, yVal, C_FILL);
    if (markers[xx]) {
      DrawVLine(xx, S_BOTTOM + 6, 2, C_FILL);
    }
  }

  LOOT_Sort(LOOT_SortByLastOpenTime, false);

  const uint8_t LOOT_BL = 13;

  uint8_t ni = 0;
  for (uint8_t i = 0; ni < 8 && i < LOOT_Size(); i++) {
    Loot *p = LOOT_Item(i);
    if (p->blacklist) {
      continue;
    }

    const uint8_t ybl = ni * 6 + LOOT_BL;
    ni++;

    if (p->open) {
      PrintSmall(DATA_LEN + 1, ybl, ">");
    } else if (p->goodKnown) {
      PrintSmall(DATA_LEN + 1, ybl, "+");
    }

    PrintSmallEx(LCD_WIDTH - 1, ybl, POS_R, C_FILL, "%u.%05u", p->f / 100000,
                 p->f % 100000);
  }

  PrintSmallEx(0, SPECTRUM_Y - 3, POS_L, C_FILL, "%ums", msmDelay);
  PrintSmallEx(DATA_LEN - 2, SPECTRUM_Y - 3, POS_R, C_FILL, "SQ:%u",
               gCurrentPreset->band.squelch);
  if (noListen) {
    PrintSmallEx(0, SPECTRUM_Y - 3 + 6, POS_L, C_FILL, "No listen");
  }
}
