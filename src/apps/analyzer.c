#include "analyzer.h"
#include "../apps/finput.h"
#include "../driver/st7565.h"
#include "../driver/system.h"
#include "../driver/uart.h"
#include "../helper/bands.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../svc_render.h"
#include "../ui/graphics.h"
#include "../ui/spectrum.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "vfo1.h"
#include <stdint.h>

static const uint8_t spectrumWidth = LCD_WIDTH;

static Measurement msm;
static uint32_t centerF = 0;
static uint8_t scanInterval = 2;
static uint8_t stepsCount = 128;

static uint32_t peakF = 0;
static uint32_t _peakF = 0;
static uint16_t peakRssi = 0;

static uint16_t squelchRssi = UINT16_MAX;
static bool useSquelch = false;
static uint16_t step;

static bool isStillMode = false;

static void startNewScan(bool reset) {
  _peakF = 0;
  peakRssi = 0;
  if (reset) {
    LOOT_Standby();
    msm.f = gCurrentBand.rxF;
    BK4819_TuneTo(msm.f, true);
    BK4819_SetRegValue(afcDisableRegSpec, true);
    BK4819_SetModulation(gCurrentBand.modulation);
    if (step > 5000) {
      BK4819_SetModulation(MOD_WFM);
    } else if (step >= 1300) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_26k);
    } else if (step >= 1200) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_20k);
    } else if (step >= 625) {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_12k);
    } else {
      BK4819_SetFilterBandwidth(BK4819_FILTER_BW_6k);
    }
    SP_Init(&gCurrentBand);
  } else {
    SP_Begin();
  }
}

static void setCenterF(uint32_t f) {
  step = StepFrequencyTable[gCurrentBand.step];
  const uint32_t halfBW = step * (stepsCount / 2);
  centerF = f;
  gCurrentBand.rxF = centerF - halfBW;
  gCurrentBand.txF = centerF + halfBW;
  gCurrentBand.modulation = RADIO_GetModulation();
  startNewScan(true);
}

static void shift(int8_t n) {
  _peakF = 0;
  peakRssi = 0;
  SP_Shift(-n);
  step = StepFrequencyTable[gCurrentBand.step];
  const uint32_t halfBW = step * (stepsCount / 2);
  centerF += step * n;
  gCurrentBand.rxF = centerF - halfBW;
  gCurrentBand.txF = centerF + halfBW;

  if (n < 2) {
    msm.f = gCurrentBand.rxF;
  } else {
    msm.f = gCurrentBand.rxF + step * n;
  }
}

void toggleStillMode() {
  isStillMode = !isStillMode;
  gMonitorMode = isStillMode;
  RADIO_ToggleRX(true);
  BK4819_TuneTo(peakF, true);
}

void ANALYZER_init(void) {
  SPECTRUM_Y = 6;
  SPECTRUM_H = 48;

  isStillMode = false;
  gMonitorMode = false;
  squelchRssi = UINT16_MAX;

  SVC_Toggle(SVC_SCAN, false, 0);
  SVC_Toggle(SVC_LISTEN, false, 0);

  RADIO_ToggleRX(false);
  RADIO_LoadCurrentVFO();
  radio->radio = RADIO_BK4819;
  RADIO_SwitchRadioPure();

  gCurrentBand.meta.type = TYPE_BAND_DETACHED;
  gCurrentBand.squelch.value = 0;

  setCenterF(radio->rxF);
  startNewScan(true);
}

void ANALYZER_update(void) {
  if (Now() - gLastRender >= 500) {
    gRedrawScreen = true;
  }
  if (!gIsListening) {
    BK4819_SetFrequency(msm.f);
    BK4819_WriteRegister(BK4819_REG_30, 0x0200);
    BK4819_WriteRegister(BK4819_REG_30, 0xBFF1);
    SYSTEM_DelayMs(scanInterval); // (X_X)
  }
  msm.rssi = BK4819_GetRSSI();
  if (gMonitorMode) {
    msm.open = true;
  } else {
    msm.open = msm.rssi > squelchRssi;
    LOOT_Update(&msm);
  }
  SP_AddPoint(&msm);
  if (msm.rssi > peakRssi) {
    peakRssi = msm.rssi;
    _peakF = msm.f;
  }

  RADIO_ToggleRX(msm.open);
  if (gIsListening) {
    return;
  }

  if (msm.f + step > gCurrentBand.txF) {
    msm.f = gCurrentBand.rxF;
    peakF = _peakF;
    gRedrawScreen = true;
    if (useSquelch && squelchRssi == UINT16_MAX) {
      squelchRssi = SP_GetRssiMax() + 2;
    }
  } else {
    msm.f += step;
  }

  if (msm.f == gCurrentBand.rxF) {
    startNewScan(false);
    return;
  }
}

void ANALYZER_deinit(void) { SVC_Toggle(SVC_LISTEN, true, 10); }

bool ANALYZER_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  // repeat or keyup
  if (bKeyPressed || !bKeyHeld) {
    switch (Key) {
    case KEY_1:
      IncDec8(&scanInterval, 1, 10, 1);
      return true;
    case KEY_7:
      IncDec8(&scanInterval, 1, 10, -1);
      return true;
    case KEY_3:
      if (squelchRssi < 512) {
        squelchRssi += 2;
      }
      return true;
    case KEY_9:
      if (squelchRssi > 1) {
        squelchRssi -= 2;
      }
      return true;
    default:
      break;
    }

    if (isStillMode) {
      if (Key == KEY_UP || Key == KEY_DOWN) {
        uint8_t mul = bKeyHeld ? 5 : 1;
        IncDec32(&peakF, gCurrentBand.rxF, gCurrentBand.txF,
                 step * (Key == KEY_UP ? mul : -mul));
        BK4819_TuneTo(peakF, false);
        return true;
      }
    }
  }

  // long press
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (Key) {
    case KEY_UP:
      shift(64);
      return true;
    case KEY_DOWN:
      shift(-64);
      return true;
    case KEY_SIDE1:
      gMonitorMode ^= 1;
      if (gMonitorMode) {
        RADIO_TuneToPure(centerF, true);
      }
      RADIO_ToggleRX(gMonitorMode);
      return true;
    case KEY_SIDE2:
      if (useSquelch) {
        useSquelch = false;
        squelchRssi = UINT16_MAX;
      } else {
        useSquelch = true;
      }
      return true;
    default:
      break;
    }
  }

  // just pressed
  if (!bKeyPressed && !bKeyHeld) {
    switch (Key) {
    case KEY_4:
      toggleStillMode();
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    case KEY_UP:
      shift(1);
      return true;
    case KEY_DOWN:
      shift(-1);
      return true;
    case KEY_SIDE1:
      LOOT_BlacklistLast();
      return true;
    case KEY_SIDE2:
      LOOT_WhitelistLast();
      return true;
    case KEY_2:
      if (gCurrentBand.step < STEP_500_0kHz) {
        gCurrentBand.step++;
      }
      setCenterF(centerF);
      return true;
    case KEY_8:
      if (gCurrentBand.step > 0) {
        gCurrentBand.step--;
      }
      setCenterF(centerF);
      return true;
    case KEY_F:
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_0:
      if (stepsCount > 32) {
        stepsCount >>= 1;
      } else {
        stepsCount = 128;
      }
      setCenterF(centerF);
      return true;
    case KEY_STAR:
      APPS_run(APP_LOOT_LIST);
      return true;
    case KEY_5:
      gFInputCallback = setCenterF;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_PTT:
      if (centerF == peakF) {
        RADIO_TuneToSave(centerF);
        gVfo1ProMode = true;
        APPS_run(APP_VFO1);
      } else if (peakF) {
        setCenterF(peakF);
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

void ANALYZER_render(void) {
  char sql[4] = "";
  const int16_t sq = Rssi2DBm(squelchRssi);
  const uint32_t fs = gCurrentBand.rxF;
  const uint32_t fe = gCurrentBand.txF;
  const uint32_t step = StepFrequencyTable[gCurrentBand.step];
  const uint8_t bl = 12;

  if (sq >= 255) {
    sprintf(sql, "off");
  } else {
    sprintf(sql, "%d", sq);
    SP_RenderLine(squelchRssi);
  }

  STATUSLINE_SetText("%ums %u.%02uk %s SQ %s", scanInterval, step / 100,
                     step % 100, modulationTypeOptions[radio->modulation], sql);

  for (uint8_t i = SPECTRUM_Y; i < SPECTRUM_Y + SPECTRUM_H; i += 4) {
    PutPixel(spectrumWidth / 2, i, C_FILL);
  }

  SP_Render(&gCurrentBand);
  SP_RenderArrow(&gCurrentBand, peakF);

  if (peakF) {
    PrintMediumBoldEx(LCD_XCENTER, 14, POS_C, C_FILL, "%u.%05u", peakF / MHZ,
                      peakF % MHZ);
  }

  PrintSmallEx(0, LCD_HEIGHT - 1, POS_L, C_FILL, "%u.%05u", fs / MHZ, fs % MHZ);
  PrintSmallEx(LCD_WIDTH, LCD_HEIGHT - 1, POS_R, C_FILL, "%u.%05u", fe / MHZ,
               fe % MHZ);

  PrintSmallEx(LCD_WIDTH, bl, POS_R, C_FILL, "%+3d", Rssi2DBm(SP_GetRssiMax()));
  PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%+3d",
               Rssi2DBm(SP_GetNoiseFloor()));

  if (isStillMode) {
    PrintSmallEx(0, bl + 6 * 0, POS_L, C_FILL, "r %03d", BK4819_GetRSSI());
    PrintSmallEx(0, bl + 6 * 1, POS_L, C_FILL, "n %03d", BK4819_GetNoise());
    PrintSmallEx(0, bl + 6 * 2, POS_L, C_FILL, "g %03d", BK4819_GetGlitch());
    PrintSmallEx(0, bl + 6 * 3, POS_L, C_FILL, "s %03d", BK4819_GetSNR());
  }

  PrintSmallEx(spectrumWidth / 2, LCD_HEIGHT - 1, POS_C, C_FILL, "%u.%05u",
               centerF / MHZ, centerF % MHZ);
}
