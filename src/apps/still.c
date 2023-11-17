#include "still.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"

static uint8_t menuState = 0;
static uint16_t screenRedrawT = 0;
static uint16_t listenT = 0;
static bool monitorMode = false;
static uint16_t rssiTriggerLevel = 0;

static uint16_t rssi = 0;
static char String[16];

static const RegisterSpec registerSpecs[] = {
    {},
    {"LNAs", BK4819_REG_13, 8, 0b11, 1},
    {"LNA", BK4819_REG_13, 5, 0b111, 1},
    {"PGA", BK4819_REG_13, 0, 0b111, 1},
    {"MIX", BK4819_REG_13, 3, 0b11, 1},

    {"IF", 0x3D, 0, 0xFFFF, 100},
    {"DEV", 0x40, 0, 0xFFF, 10},
    {"CMP", 0x31, 3, 1, 1},
    {"MIC", 0x7D, 0, 0xF, 1},
};

/* static void UpdateRssiTriggerLevel(bool inc) {
  if (inc)
    rssiTriggerLevel += 2;
  else
    rssiTriggerLevel -= 2;
  gRedrawScreen = true;
} */

static void UpdateRegMenuValue(RegisterSpec s, bool add) {
  uint16_t v = BK4819_GetRegValue(s);

  if (add && v <= s.mask - s.inc) {
    v += s.inc;
  } else if (!add && v >= 0 + s.inc) {
    v -= s.inc;
  }

  BK4819_SetRegValue(s, v);
  gRedrawScreen = true;
}

static void UpdateCurrentFreqStill(bool inc) {
  const uint16_t offset = StepFrequencyTable[gCurrentVfo.step];
  if (inc && gCurrentVfo.fRX < F_MAX) {
    RADIO_TuneTo(gCurrentVfo.fRX + offset, false);
  } else if (!inc && gCurrentVfo.fRX > F_MIN) {
    RADIO_TuneTo(gCurrentVfo.fRX - offset, false);
  }
  gRedrawScreen = true;
}

static void UpdateListening() {
  if (listenT) {
    listenT--;
    return;
  }

  gRedrawScreen = true;

  rssi = BK4819_GetRSSI();

  if (rssi >= rssiTriggerLevel || monitorMode) {
    listenT = 100;
    return;
  }

  RADIO_ToggleRX(false);
}

void STILL_update() {
  if (gIsListening) {
    UpdateListening();
    return;
  }
  rssi = BK4819_GetRSSI();

  if (++screenRedrawT >= 1000) {
    screenRedrawT = 0;
    gRedrawScreen = true;
  }

  RADIO_ToggleRX(rssi >= rssiTriggerLevel || monitorMode);
}

void STILL_init() {}

void STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return;
  }
  switch (key) {
#ifdef ENABLE_ALL_REGISTERS
  case KEY_2:
    menuState = 0;
    if (hiddenMenuState <= 1) {
      hiddenMenuState = ARRAY_SIZE(hiddenRegisterSpecs) - 1;
    } else {
      hiddenMenuState--;
    }
    redrawScreen = true;
    break;
  case KEY_8:
    menuState = 0;
    if (hiddenMenuState == ARRAY_SIZE(hiddenRegisterSpecs) - 1) {
      hiddenMenuState = 1;
    } else {
      hiddenMenuState++;
    }
    redrawScreen = true;
    break;
#endif
  case KEY_UP:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], true);
      break;
    }
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], true);
      break;
    }
#endif
    UpdateCurrentFreqStill(true);
    break;
  case KEY_DOWN:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], false);
      break;
    }
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], false);
      break;
    }
#endif
    UpdateCurrentFreqStill(false);
    break;
  case KEY_STAR:
    // UpdateRssiTriggerLevel(true);
    break;
  case KEY_F:
    // UpdateRssiTriggerLevel(false);
    APPS_run(APP_VFO_CFG);
    break;
  case KEY_5:
    APPS_run(APP_FINPUT);
    break;
  case KEY_0:
    RADIO_ToggleModulation();
    break;
  case KEY_6:
    RADIO_ToggleListeningBW();
    break;
  case KEY_SIDE1:
    monitorMode = !monitorMode;
    gRedrawScreen = true;
    break;
  case KEY_SIDE2:
    // ToggleBacklight();
    break;
  case KEY_PTT:
    // start transmit
    /* UpdateBatteryInfo();
    if (gBatteryDisplayLevel == 6) {
      txAllowState = VFO_STATE_VOL_HIGH;
    } else if (IsTXAllowed(GetOffsetedF(gCurrentVfo, fMeasure))) {
      txAllowState = VFO_STATE_NORMAL;
      // ToggleTX(true);
    } else {
      txAllowState = VFO_STATE_TX_DISABLE;
    } */
    gRedrawScreen = true;
    break;
  case KEY_MENU:
    if (menuState == ARRAY_SIZE(registerSpecs) - 1) {
      menuState = 1;
    } else {
      menuState++;
    }
    // SYSTEM_DelayMs(100);
    gRedrawScreen = true;
    break;
  case KEY_EXIT:
    if (menuState) {
      menuState = 0;
      gRedrawScreen = true;
      break;
    }
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      hiddenMenuState = 0;
      gRedrawScreen = true;
      break;
    }
#endif
    APPS_run(APP_SPECTRUM);
    monitorMode = false;
    break;
  default:
    break;
  }
  gRedrawStatus = true;
}

static void DrawRegs() {
  const uint8_t PAD_LEFT = 4;
  const uint8_t CELL_WIDTH = 30;
  uint8_t row = 3;

  for (uint8_t i = 0, idx = 1; idx < ARRAY_SIZE(registerSpecs); ++i, ++idx) {
    if (idx == 5) {
      row += 2;
      i = 0;
    }
    const uint8_t offset = PAD_LEFT + i * CELL_WIDTH;
    if (menuState == idx) {
      for (int j = 0; j < CELL_WIDTH; ++j) {
        gFrameBuffer[row][j + offset] = 0xFF;
        gFrameBuffer[row + 1][j + offset] = 0xFF;
      }
    }
    RegisterSpec rs = registerSpecs[idx];
    sprintf(String, "%s", rs.name);
    UI_PrintStringSmallest(String, offset + 2, row * 8 + 2, false,
                           menuState != idx);
    sprintf(String, "%u", BK4819_GetRegValue(rs));
    UI_PrintStringSmallest(String, offset + 2, (row + 1) * 8 + 1, false,
                           menuState != idx);
  }
}

void STILL_render() {
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  UI_FSmall(GetScreenF(gCurrentVfo.fRX));
  UI_RSSIBar(rssi, 2);

#ifdef ENABLE_ALL_REGISTERS
  if (hiddenMenuState) {
    uint8_t hiddenMenuLen = ARRAY_SIZE(hiddenRegisterSpecs);
    uint8_t offset = Clamp(hiddenMenuState - 2, 1, hiddenMenuLen - 5);
    for (int i = 0; i < 5; ++i) {
      RegisterSpec rs = hiddenRegisterSpecs[i + offset];
      bool isCurrent = hiddenMenuState == i + offset;
      sprintf(String, "%s%x %s: %u", isCurrent ? ">" : " ", rs.num, rs.name,
              BK4819_GetRegValue(rs));
      UI_PrintStringSmallest(String, 0, i * 6 + 26, false, true);
    }
  } else {
#endif
    DrawRegs();
#ifdef ENABLE_ALL_REGISTERS
  }
#endif
}
