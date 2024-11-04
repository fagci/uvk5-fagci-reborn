#include "svc_fastscan.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "helper/lootlist.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"

static FreqScanTime T = F_SC_T_0_2s;
static uint32_t scanF = 0;
static bool lower = true;
static uint32_t bound;

static uint32_t lastSwitch = 0;

static bool scanning = false;

static uint32_t delta(uint32_t f1, uint32_t f2) {
  if (f1 > f2) {
    return f1 - f2;
  }
  return f2 - f1;
}

void SVC_FC_Init(void) {
  SVC_Toggle(SVC_LISTEN, false, 10);
  BK4819_StopScan();
  BK4819_EnableFrequencyScanEx(T);
  scanning = true;
  bound = SETTINGS_GetFilterBound();
}

static void gotF(uint32_t f) {
  uint32_t step = 100;
  uint32_t sd = f % step;
  if (sd > step / 2) {
    f += step - sd;
  } else {
    f -= sd;
  }

  SVC_FC_Deinit();
  RADIO_TuneTo(f);
  RADIO_ToggleRX(true);

  Loot msm = {.f = f, .open = true};
  LOOT_Update(&msm);
  gRedrawScreen = true;
}

static void switchBand() {
  lower = !lower;
  BK4819_SelectFilter(lower ? 14500000 : 43300000);
  SVC_FC_Init();
}

void SVC_FC_Update(void) {
  if (gIsListening ||
      (gLastActiveLoot && Now() - gLastActiveLoot->lastTimeOpen < 500)) {
    return;
  }

  if (Now() - lastSwitch >= 700) {
    switchBand();
    lastSwitch = Now();
    return;
  }

  uint32_t f = 0;
  if (scanning && !BK4819_GetFrequencyScanResult(&f)) {
    return;
  }

  SVC_Toggle(SVC_LISTEN, false, 10);
  BK4819_DisableFrequencyScan();
  BK4819_EnableFrequencyScanEx(T);
  scanning = true;

  if (!f) {
    return;
  }

  if (f >= 8800000 && f < 10800000) {
    return;
  }

  if ((lower && f >= bound) || (!lower && f < bound)) {
    return;
  }

  Loot *loot = LOOT_Get(f);
  if (loot && (loot->blacklist || loot->whitelist)) {
    return;
  }

  if (delta(f, scanF) < 100) {
    gotF(f);
  } else {
    // new freq
    scanF = f;
  }
}

void SVC_FC_Deinit(void) {
  scanning = false;
  BK4819_StopScan();
  RADIO_SetupByCurrentVFO();
  SVC_Toggle(SVC_LISTEN, true, 10);
  BK4819_RX_TurnOn();
}
