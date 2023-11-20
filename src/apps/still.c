#include "still.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"

static uint8_t menuState = 0;
static bool monitorMode = false;

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

void STILL_update() {}

static void update() {
  rssi = BK4819_GetRSSI();
  RADIO_ToggleRX(monitorMode || BK4819_IsSquelchOpen());
}

static void render() { gRedrawScreen = true; }

void STILL_init() {
  BK4819_Squelch(3, gCurrentVfo.fRX);
  TaskAdd("Update still", update, 10, true);
  TaskAdd("Redraw still", render, 1000, true);
}

void STILL_deinit() {
  TaskRemove(update);
  TaskRemove(render);
}

bool STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  switch (key) {
  case KEY_1:
    RADIO_UpdateStep(true);
    gRedrawScreen = true;
    return true;
  case KEY_7:
    RADIO_UpdateStep(false);
    gRedrawScreen = true;
    return true;
#ifdef ENABLE_ALL_REGISTERS
  case KEY_2:
    menuState = 0;
    if (hiddenMenuState <= 1) {
      hiddenMenuState = ARRAY_SIZE(hiddenRegisterSpecs) - 1;
    } else {
      hiddenMenuState--;
    }
    redrawScreen = true;
    return true;
  case KEY_8:
    menuState = 0;
    if (hiddenMenuState == ARRAY_SIZE(hiddenRegisterSpecs) - 1) {
      hiddenMenuState = 1;
    } else {
      hiddenMenuState++;
    }
    redrawScreen = true;
    return true;
#endif
  case KEY_UP:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], true);
      return true;
    }
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], true);
      return true;
    }
#endif
    UpdateCurrentFreqStill(true);
    return true;
  case KEY_DOWN:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], false);
      return true;
    }
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], false);
      return true;
    }
#endif
    UpdateCurrentFreqStill(false);
    return true;
  case KEY_F:
    APPS_run(APP_VFO_CFG);
    return true;
  case KEY_5:
    APPS_run(APP_FINPUT);
    return true;
  case KEY_0:
    RADIO_ToggleModulation();
    return true;
  case KEY_6:
    RADIO_ToggleListeningBW();
    return true;
  case KEY_SIDE1:
    monitorMode = !monitorMode;
    gRedrawScreen = true;
    return true;
  case KEY_SIDE2:
    // ToggleBacklight();
    return true;
  case KEY_MENU:
    if (!bKeyHeld) {
      if (menuState == ARRAY_SIZE(registerSpecs) - 1) {
        menuState = 1;
      } else {
        menuState++;
      }
      gRedrawScreen = true;
      return true;
    }
    break;
  case KEY_EXIT:
    if (menuState) {
      menuState = 0;
      gRedrawScreen = true;
      return true;
    }
    APPS_exit();
    monitorMode = false;
    return true;
  default:
    break;
  }
  return false;
  gRedrawStatus = true;
}

static void DrawRegs() {
  const uint8_t PAD_LEFT = 4;
  const uint8_t CELL_WIDTH = 30;
  uint8_t row = 3;

  for (uint8_t i = 0, idx = 1; idx < ARRAY_SIZE(registerSpecs); ++i, ++idx) {
    RegisterSpec rs = registerSpecs[idx];
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
    sprintf(String, "%s", rs.name);
    UI_PrintStringSmallest(String, offset + 2, row * 8 + 2, false,
                           menuState != idx);
    sprintf(String, "%u", BK4819_GetRegValue(rs));
    UI_PrintStringSmallest(String, offset + 2, (row + 1) * 8 + 1, false,
                           menuState != idx);
  }
}

void STILL_render() {
  UI_ClearScreen();
  UI_FSmall(GetScreenF(gCurrentVfo.fRX));
  UI_RSSIBar(rssi, gCurrentVfo.fRX, 2);
  DrawRegs();
}
