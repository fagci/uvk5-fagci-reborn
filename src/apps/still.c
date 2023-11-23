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
#include "finput.h"

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
    gCurrentVfo.fRX += offset;
  } else if (!inc && gCurrentVfo.fRX > F_MIN) {
    gCurrentVfo.fRX -= offset;
  } else {
    return;
  }
  RADIO_TuneTo(gCurrentVfo.fRX, false);
  gRedrawScreen = true;
}

static void update() {
  rssi = BK4819_GetRSSI();
  RADIO_ToggleRX(monitorMode || BK4819_IsSquelchOpen());
}

static void render() { gRedrawScreen = true; }

void STILL_init() {
  /* RegisterSpec sqType = {"SQ type", 0x77, 8, 0xFF, 1};
  BK4819_SetRegValue(sqType, 0xFF); */
  BK4819_Squelch(gCurrentVfo.squelch, gCurrentVfo.fRX);
  BK4819_SetFilterBandwidth(gCurrentVfo.bw);
  BK4819_SetModulation(gCurrentVfo.modulation);

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
  if (menuState) {
    switch (key) {
    case KEY_1:
    case KEY_2:
    case KEY_3:
      menuState = key - KEY_0;
      return true;
    case KEY_4:
    case KEY_5:
    case KEY_6:
      menuState = key - KEY_0 + 1;
      return true;
    case KEY_0:
      menuState = 8;
      return true;
    case KEY_STAR:
      menuState = 4;
      return true;
    default:
      break;
    }
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
  case KEY_3:
    gCurrentVfo.squelch++;
    RADIO_SetSquelch(gCurrentVfo.squelch);
    return true;
  case KEY_9:
    gCurrentVfo.squelch--;
    RADIO_SetSquelch(gCurrentVfo.squelch);
    return true;
  case KEY_UP:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], true);
      return true;
    }
    UpdateCurrentFreqStill(true);
    return true;
  case KEY_DOWN:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], false);
      return true;
    }
    UpdateCurrentFreqStill(false);
    return true;
  case KEY_F:
    APPS_run(APP_VFO_CFG);
    return true;
  case KEY_5:
    gFInputValue = &gCurrentVfo.fRX;
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
  case KEY_8:
    if (!bKeyHeld) {
      menuState =
          menuState == ARRAY_SIZE(registerSpecs) - 1 ? 1 : menuState + 1;
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
