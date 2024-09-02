#include "svc_fastscan.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "driver/uart.h"
#include "helper/lootlist.h"
#include "helper/presetlist.h"
#include "radio.h"
#include "scheduler.h"
#include "settings.h"
#include "svc.h"

static FreqScanTime T = F_SC_T_0_2s;
static uint32_t scanF = 0;
static uint8_t hits = 0;
static bool lower = true;

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
  hits = 0;
}

static void gotF(uint32_t f) {
  Preset *p = PRESET_ByFrequency(f);
  uint32_t step = StepFrequencyTable[p->band.step];
  uint32_t sd = f % step;
  if (sd > step / 2) {
    f += step - sd;
  } else {
    f -= sd;
  }

  Loot msm = {.f = f, .open = true};
  SVC_FC_Deinit();
  BK4819_RX_TurnOn();
  RADIO_TuneTo(f);
  // RADIO_ToggleRX(true);
  LOOT_Update(&msm);
}

void SVC_FC_Update(void) {
  uint32_t f = 0;
  if (gIsListening ||
      (gLastActiveLoot && Now() - gLastActiveLoot->lastTimeOpen < 2000)) {
    return;
  }

  if (Now() - lastSwitch >= 1000) {
    lastSwitch = Now();
    lower = !lower;
    BK4819_SelectFilter(lower ? 14500000 : 43300000);
  }

  if (scanning && !BK4819_GetFrequencyScanResult(&f)) {
    return;
  }

  uint32_t d = delta(f, scanF);

  if (d < 100) {
    if (++hits >= 1) { // 1 hit is 1 hit with same freq
      gRedrawScreen = true;

      gotF(scanF);
      return;
    }
  } else {
    if (hits) {
      hits--;
    }
  }
  BK4819_DisableFrequencyScan();
  BK4819_EnableFrequencyScanEx(T);
  scanning = true;
  uint32_t bound = SETTINGS_GetFilterBound();
  if (f && ((lower && f < bound) || (!lower && f >= bound))) {
    scanF = f;
  }
}

void SVC_FC_Deinit(void) {
  scanning = false;
  BK4819_StopScan();
  SVC_Toggle(SVC_LISTEN, true, 10);
}
