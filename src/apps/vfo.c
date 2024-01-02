#include "vfo.h"
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

static Loot msm = {0};

static bool lastOpenState = false;
static uint32_t lastUpdate = 0;
static uint32_t lastClose = 0;

static uint8_t vfoCount = 2;

static bool repeatHeld = false;

static Loot *loot;

static void handleInt(uint16_t intStatus) {
  if (intStatus & BK4819_REG_02_SQUELCH_LOST) {
    msm.open = true;
  }
  if (intStatus & BK4819_REG_02_SQUELCH_FOUND) {
    msm.open = false;
    lastClose = elapsedMilliseconds;
  }
  if (intStatus & BK4819_REG_02_CxCSS_TAIL) {
    msm.open = false;
    lastClose = elapsedMilliseconds;
  }
}

static void update() {
  msm.f = gCurrentVFO->fRX;
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  if (gMonitorMode) {
    msm.open = true;
  } else {
    BK4819_HandleInterrupts(handleInt);
    if (elapsedMilliseconds - lastClose < 250) {
      msm.open = false;
    }
  }

  if (msm.f != loot->f) {
    LOOT_ReplaceItem(gSettings.activeVFO, msm.f);
  }

  RADIO_ToggleRX(msm.open);
  if (elapsedMilliseconds - lastUpdate >= 1000) {
    LOOT_UpdateEx(loot, &msm);
    gRedrawScreen = true;
    lastUpdate = elapsedMilliseconds;
  }
  if (msm.open != lastOpenState) {
    lastOpenState = msm.open;
    gRedrawScreen = true;
  }
}

static void render() { gRedrawScreen = true; }

void VFO_init() {
  repeatHeld = false;

  RADIO_LoadCurrentVFO();
  RADIO_EnableToneDetection();

  LOOT_Clear();
  for (uint8_t i = 0; i < 2; ++i) {
    Loot *item = LOOT_AddEx(gVFO[i].fRX, false);
    item->open = false;
    item->lastTimeOpen = 0;
  }

  RADIO_SetupByCurrentVFO(); // TODO: reread from EEPROM not needed maybe

  TaskAdd("Update still", update, 10, true);
  TaskAdd("Redraw still", render, 1000, true);
  loot = LOOT_Item(gSettings.activeVFO);
  gRedrawScreen = true;
}

void VFO_deinit() {
  TaskRemove(update);
  TaskRemove(render);
  RADIO_ToggleRX(false);
  LOOT_Clear();
  BK4819_WriteRegister(BK4819_REG_3F, 0); // disable interrupts
}

void VFO_update() {}

VFO *NextVFO(bool next) {
  gSettings.activeVFO = !gSettings.activeVFO;
  loot = LOOT_Item(gSettings.activeVFO);
  return &gVFO[gSettings.activeVFO];
}

static void nextFreq(bool next) {
  int8_t dir = next ? 1 : -1;

  if (gCurrentVFO->isMrMode) {
    int16_t idx = CHANNELS_Next(gCurrentVFO->channel, next);
    if (idx > -1) {
      CH ch;
      gCurrentVFO->channel = idx;
      CHANNELS_Load(gCurrentVFO->channel, &ch);
      CH2VFO(&ch, gCurrentVFO);
      strncpy(gVFONames[gSettings.activeVFO], ch.name, 9);
    }
    return;
  }

  Preset *nextPreset = PRESET_ByFrequency(gCurrentVFO->fRX + dir);
  if (nextPreset != gCurrentPreset && nextPreset != &defaultPreset) {
    if (next) {
      RADIO_TuneTo(nextPreset->band.bounds.start);
    } else {
      RADIO_TuneTo(nextPreset->band.bounds.end);
    }
    return;
  }
  RADIO_TuneTo(gCurrentVFO->fRX +
               StepFrequencyTable[nextPreset->band.step] * dir);
}

static void toggleVfoMR() {
  if (gCurrentVFO->isMrMode) {
    gCurrentVFO->isMrMode = false;
    return;
  }
  CH ch;
  int16_t i = gCurrentVFO->channel;
  CHANNELS_Load(gCurrentVFO->channel, &ch);
  if (!IsReadable(ch.name)) {
    i = CHANNELS_Next(gCurrentVFO->channel, true);
    if (i > -1) {
      gCurrentVFO->channel = i;
      CHANNELS_Load(gCurrentVFO->channel, &ch);
    }
  }
  if (IsReadable(ch.name)) {
    CH2VFO(&ch, gCurrentVFO);
    strncpy(gVFONames[gSettings.activeVFO], ch.name, 9);
    gCurrentVFO->isMrMode = true;
  }
}

bool VFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    repeatHeld = false;
  }

  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      nextFreq(true);
      return true;
    case KEY_DOWN:
      nextFreq(false);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !repeatHeld) {
    repeatHeld = true;
    switch (key) {
    case KEY_2:
      gCurrentVFO = NextVFO(true);
      RADIO_SetupByCurrentVFO();
      LOOT_Standby();
      msm.f = gCurrentVFO->fRX;
      return true;
    case KEY_EXIT:
      return true;
    case KEY_3:
      toggleVfoMR();
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
    case KEY_5:
      vfoCount = vfoCount == 1 ? 2 : 1;
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
      msm.open = gMonitorMode;
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

static void render2VFOPart(uint8_t i) {
  const uint8_t BASE = 22;
  const uint8_t bl = BASE + 34 * i;

  VFO *vfo = &gVFO[i];
  Preset *p = PRESET_ByFrequency(vfo->fRX);
  const bool isActive = gCurrentVFO == vfo;

  const uint16_t fp1 = vfo->fRX / 100000;
  const uint16_t fp2 = vfo->fRX / 100 % 1000;
  const uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];

  if (isActive) {
    FillRect(0, bl - 14, 28, 7, C_FILL);
    if (msm.open) {
      PrintMediumEx(0, bl, POS_L, C_INVERT, "RX");
      UI_RSSIBar(msm.rssi, msm.f, 31);
    }
  }

  if (vfo->isMrMode) {
    PrintMediumBoldEx(LCD_WIDTH / 2, bl - 8, POS_C, C_FILL, gVFONames[i]);
    PrintMediumEx(LCD_WIDTH / 2, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
    PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "MR %03u", vfo->channel + 1);
  } else {
    PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
    PrintMediumBoldEx(LCD_WIDTH - 1, bl, POS_R, C_FILL, "%02u", fp3);
    PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "VFO");
  }
  PrintSmallEx(LCD_WIDTH - 1, bl - 9, POS_R, C_FILL, mod);

  Loot *stats = LOOT_Item(i);
  uint32_t est = stats->lastTimeOpen
                     ? (elapsedMilliseconds - stats->lastTimeOpen) / 1000
                     : 0;
  if (stats->ct != 0xFF) {
    PrintSmallEx(0, bl + 6, POS_L, C_FILL, "CT:%u.%uHz",
                 CTCSS_Options[stats->ct] / 10, CTCSS_Options[stats->ct] % 10);
  } else if (stats->cd != 0xFF) {
    PrintSmallEx(0, bl + 6, POS_L, C_FILL, "D%03oN(fake)",
                 DCS_Options[stats->cd]);
  }
  PrintSmallEx(LCD_WIDTH - 1, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
               est % 60, stats->duration / 1000);

  /* PrintSmallEx(LCD_WIDTH - 18, bl - 12, POS_R, C_FILL, "SQ %d",
               gCurrentPreset->band.squelch); */
}

static void render2VFO() {
  for (uint8_t i = 0; i < 2; ++i) {
    render2VFOPart(i);
  }
}

static void render1VFO() {
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[gSettings.activeVFO];
  Preset *p = PRESET_ByFrequency(vfo->fRX);

  uint16_t fp1 = vfo->fRX / 100000;
  uint16_t fp2 = vfo->fRX / 10 % 10000;
  uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[p->band.modulation];

  PrintBiggestDigitsEx(LCD_WIDTH - 19, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                       fp2);
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
  PrintSmallEx(LCD_WIDTH - 1, BASE - 8, POS_R, C_FILL, mod);
}

void VFO_render() {
  UI_ClearScreen();
  if (vfoCount == 1) {
    render1VFO();
  } else if (vfoCount == 2) {
    render2VFO();
  } else {
    PrintMediumEx(LCD_WIDTH / 2, LCD_HEIGHT / 2, POS_C, C_FILL,
                  "%u VFO not impl", vfoCount);
  }
}
