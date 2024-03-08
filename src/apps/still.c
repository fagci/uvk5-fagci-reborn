#include "still.h"
#include "../driver/audio.h"
#include "../driver/bk4819.h"
#include "../driver/st7565.h"
#include "../helper/lootlist.h"
#include "../helper/measurements.h"
#include "../helper/bandlist.h"
#include "../misc.h"
#include "../radio.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "apps.h"
#include "finput.h"

static uint8_t menuState = 0;
uint32_t lastUpdate = 0;

static char String[16];

static const RegisterSpec registerSpecs[] = {
    {},
    {"ATT", BK4819_REG_13, 0, 0xFFFF, 1},
    /* {"RF", BK4819_REG_43, 12, 0b111, 1},
    {"RFwe", BK4819_REG_43, 9, 0b111, 1}, */

    {"IF", 0x3D, 0, 0xFFFF, 100},
    // TODO: 7 values:
    /* 0: Zero IF
    0x2aab: 8.46 kHz IF
    0x4924: 7.25 kHz IF
    0x6800: 6.35 kHz IF
    0x871c: 5.64 kHz IF
    0xa666: 5.08 kHz IF
    0xc5d1: 4.62 kHz IF
    0xe555: 4.23 kHz IF
    If REG_43<5> = 1, IF = IF*2. */

    {"DEV", 0x40, 0, 0xFFF, 10},
    {"CMP", 0x31, 3, 1, 1},
    {"MIC", 0x7D, 0, 0xF, 1},
};

static void UpdateRegMenuValue(RegisterSpec s, bool add) {
  uint16_t v, maxValue;

  if (s.num == BK4819_REG_13) {
    v = gCurrentBand->band.gainIndex;
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
  } else {
    BK4819_SetRegValue(s, v);
  }
}

void STILL_init() {
  RADIO_LoadCurrentCH();
  gRedrawScreen = true;
}

void STILL_deinit() { RADIO_ToggleRX(false); }

void STILL_update() {
  RADIO_UpdateMeasurementsEx(gCurrentLoot);

  if (elapsedMilliseconds - lastUpdate >= 500) {
    gRedrawScreen = true;
    lastUpdate = elapsedMilliseconds;
  }
}

bool STILL_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTX(bKeyHeld);
    return true;
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

  if (!bKeyHeld) {
    switch (key) {
    case KEY_1:
      RADIO_UpdateStep(true);
      return true;
    case KEY_7:
      RADIO_UpdateStep(false);
      return true;
    case KEY_3:
      RADIO_UpdateSquelchLevel(true);
      return true;
    case KEY_9:
      RADIO_UpdateSquelchLevel(false);
      return true;
    case KEY_0:
      RADIO_ToggleModulation();
      return true;
    case KEY_6:
      RADIO_ToggleListeningBW();
      return true;
    case KEY_SIDE1:
      gMonitorMode = !gMonitorMode;
      return true;
    case KEY_F:
      APPS_run(APP_CH_CFG);
      return true;
    case KEY_5:
      gFInputCallback = RADIO_TuneTo;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_SIDE2:
      return true;
    case KEY_8:
      if (!gIsBK1080) {
        IncDec8(&menuState, 1, ARRAY_SIZE(registerSpecs), 1);
      }
      return true;
    default:
      break;
    }
  }

  switch (key) {
  case KEY_UP:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], true);
      return true;
    }
    RADIO_NextFreq(true);
    return true;
  case KEY_DOWN:
    if (menuState) {
      UpdateRegMenuValue(registerSpecs[menuState], false);
      return true;
    }
    RADIO_NextFreq(false);
    return true;
  case KEY_EXIT:
    if (menuState) {
      menuState = 0;
      return true;
    }
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
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
      sprintf(String, "%ddB", gainTable[gCurrentBand->band.gainIndex].gainDb);
    } else {
      sprintf(String, "%u", BK4819_GetRegValue(rs));
    }

    PrintSmallEx(textX, offsetY + 6, POS_C, C_INVERT, "%s", rs.name);
    PrintSmallEx(textX, offsetY + CELL_HEIGHT - 4, POS_C, C_INVERT, String);
  }
}

void STILL_render() {
  UI_ClearScreen();
  STATUSLINE_SetText(gCurrentBand->band.name);
  UI_FSmall(gTxState == TX_ON ? RADIO_GetTXF() : GetScreenF(radio->f));
  UI_RSSIBar(gLoot[gSettings.activeCH].rssi, radio->f, 23);

  if (!gIsBK1080) {
    DrawRegs();
  }
}


static App meta = {
    .id = APP_STILL,
    .name = "STILL",
    .runnable = true,
    .init = STILL_init,
    .update = STILL_update,
    .render = STILL_render,
    .key = STILL_key,
    .deinit = STILL_deinit,
};

App *STILL_Meta() { return &meta; }
