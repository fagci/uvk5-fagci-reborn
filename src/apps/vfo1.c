#include "vfo1.h"
#include "../dcs.h"
#include "../driver/uart.h"
#include "../helper/adapter.h"
#include "../helper/channels.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint32_t lastUpdate = 0;

void VFO1_init() {
  RADIO_SetupByCurrentVFO(); // TODO: reread from EEPROM not needed maybe
  gRedrawScreen = true;
}

void VFO1_deinit() {
  if (APPS_Peek() != APP_FINPUT && APPS_Peek() != APP_VFO1) {
    RADIO_ToggleRX(false);
  }
}

void VFO1_update() {
  RADIO_UpdateMeasurements();

  RADIO_ToggleRX(gMeasurements.open);
  LOOT_UpdateEx(gCurrentLoot, &gMeasurements);

  if (elapsedMilliseconds - lastUpdate >= 500) {
    gRedrawScreen = true;
    lastUpdate = elapsedMilliseconds;
  }
}

bool VFO1_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      RADIO_NextFreq(true);
      return true;
    case KEY_DOWN:
      RADIO_NextFreq(false);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    /* case KEY_2:
      LOOT_Standby();
      RADIO_NextVFO(true);
      msm.f = gCurrentVFO->fRX;
      return true; */
    case KEY_EXIT:
      return true;
    case KEY_3:
      RADIO_ToggleVfoMR();
      return true;
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_0:
    case KEY_1:
    case KEY_2:
    case KEY_3:
    case KEY_4:
    case KEY_5:
    case KEY_6:
    case KEY_7:
    case KEY_8:
    case KEY_9:
      gFInputCallback = RADIO_TuneToSave;
      APPS_run(APP_FINPUT);
      APPS_key(key, bKeyPressed, bKeyHeld);
      return true;
    case KEY_F:
      APPS_run(APP_VFO_CFG);
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      gMeasurements.open = gMonitorMode;
      return true;
    case KEY_EXIT:
      if (!APPS_exit()) {
        LOOT_Standby();
        RADIO_NextVFO(true);
        RADIO_TuneToPure(gCurrentVFO->fRX);
      }
      return true;
    default:
      break;
    }
  }
  return false;
}

void VFO1_render() {
  UI_ClearScreen();
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  Preset *p = PRESET_ByFrequency(vfo->fRX);

  uint16_t fp1 = vfo->fRX / 100000;
  uint16_t fp2 = vfo->fRX / 100 % 1000;
  uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];

  PrintBiggestDigitsEx(LCD_WIDTH - 19, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                       fp2);
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
  PrintSmallEx(LCD_WIDTH - 1, BASE - 8, POS_R, C_FILL, mod);
}
