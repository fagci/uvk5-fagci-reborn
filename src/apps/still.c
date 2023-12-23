#include "still.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/measurements.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint8_t menuState = 0;
static bool monitorMode = false;

static uint16_t rssi = 0;
static char String[16];

static const RegisterSpec registerSpecs[] = {
    {},
    {"ATT", BK4819_REG_13, 0, 0xFFFF, 1},

    {"IF", 0x3D, 0, 0xFFFF, 100},
    {"DEV", 0x40, 0, 0xFFF, 10},
    {"CMP", 0x31, 3, 1, 1},
    {"MIC", 0x7D, 0, 0xF, 1},
};

static void UpdateRegMenuValue(RegisterSpec s, bool add) {
  uint16_t v, maxValue;

  if (s.num == BK4819_REG_13) {
    v = gCurrentPreset->band.gainIndex;
    maxValue = ARRAY_SIZE(gainTable) - 1;
  } else {
    v = BK4819_GetRegValue(s);
    maxValue = s.mask;
  }

  if (add && v <= maxValue - s.inc) {
    v += s.inc;
  } else if (!add && v >= 0 + s.inc) {
    v -= s.inc;
  }

  if (s.num == BK4819_REG_13) {
    RADIO_SetGain(v);
    v = gainTable[v].regValue;
  }

  BK4819_SetRegValue(s, v);
  gRedrawScreen = true;
}

static void UpdateCurrentFreqStill(bool inc) {
  const uint16_t offset = StepFrequencyTable[gCurrentPreset->band.step];
  if (inc && gCurrentVFO->fRX < F_MAX) {
    gCurrentVFO->fRX += offset;
  } else if (!inc && gCurrentVFO->fRX > F_MIN) {
    gCurrentVFO->fRX -= offset;
  } else {
    return;
  }
  RADIO_TuneTo(gCurrentVFO->fRX);
  gRedrawScreen = true;
}

static void update() {
  rssi = BK4819_GetRSSI();
  RADIO_ToggleRX(monitorMode || BK4819_IsSquelchOpen());
}

static void render() { gRedrawScreen = true; }

void STILL_init() {
  RADIO_SetupByCurrentVFO();

  TaskAdd("Update still", update, 10, true);
  TaskAdd("Redraw still", render, 1000, true);
}

void STILL_deinit() {
  TaskRemove(update);
  TaskRemove(render);
  RADIO_ToggleRX(false);
}

bool STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
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
    gCurrentPreset->band.squelch++;
    RADIO_SetSquelch(gCurrentPreset->band.squelch);
    return true;
  case KEY_9:
    gCurrentPreset->band.squelch--;
    RADIO_SetSquelch(gCurrentPreset->band.squelch);
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
    gFInputCallback = RADIO_TuneTo;
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
  gRedrawScreen = true;
}

static void DrawRegs() {
  const uint8_t PAD_LEFT = 1;
  const uint8_t PAD_TOP = 31;
  const uint8_t CELL_WIDTH = 31;
  const uint8_t CELL_HEIGHT = 16;
  uint8_t row = 0;

  for (uint8_t i = 0, idx = 1; idx < ARRAY_SIZE(registerSpecs); ++i, ++idx) {
    if (idx == 5) {
      row++;
      i = 0;
    }

    RegisterSpec rs = registerSpecs[idx];
    const uint8_t offsetX = PAD_LEFT + i * CELL_WIDTH + 2;
    const uint8_t offsetY = PAD_TOP + row * CELL_HEIGHT + 2;
    const uint8_t textX = offsetX + (CELL_WIDTH - 2) / 2;

    if (menuState == idx) {
      FillRoundRect(offsetX, offsetY, CELL_WIDTH - 2, CELL_HEIGHT - 1, 3, true);
    } else {
      DrawRoundRect(offsetX, offsetY, CELL_WIDTH - 2, CELL_HEIGHT - 1, 3, true);
    }

    if (rs.num == BK4819_REG_13) {
      sprintf(String, "%ddB", gainTable[gCurrentPreset->band.gainIndex].gainDb);
    } else {
      sprintf(String, "%u", BK4819_GetRegValue(rs));
    }

    PrintSmallEx(textX, offsetY + 6, POS_C, C_INVERT, "%s", rs.name);
    PrintSmallEx(textX, offsetY + CELL_HEIGHT - 4, POS_C, C_INVERT, String);
  }
}

void STILL_render() {
  UI_ClearScreen();
  // PrintSmall(BATTERY_W + 1, 5, "%s", gCurrentPreset->band.name);
  UI_FSmall(GetScreenF(gCurrentVFO->fRX));
  UI_RSSIBar(rssi, gCurrentVFO->fRX, 23);
  DrawRegs();
}
