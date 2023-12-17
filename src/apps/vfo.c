#include "vfo.h"
#include "../helper/lootlist.h"
#include "../helper/presetlist.h"
#include "../scheduler.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static Loot msm = {0};

void VFO_init() {
  UI_ClearScreen();

  RADIO_LoadCurrentVFO();

  LOOT_Clear();
  for (uint8_t i = 0; i < 2; ++i) {
    Loot *item = LOOT_Add(gVFO[i].fRX);
    item->open = false;
    item->lastTimeOpen = 0;
  }

  RADIO_SetupByCurrentVFO(); // TODO: reread from EEPROM not needed maybe
  gRedrawScreen = true;
}

static bool lastListenState = false;
static uint32_t lastUpdate = 0;

void VFO_update() {
  msm.rssi = BK4819_GetRSSI();
  msm.noise = BK4819_GetNoise();
  msm.open = BK4819_IsSquelchOpen();
  // LOOT_Update(&msm);
  if (msm.open != lastListenState) {
    gRedrawScreen = true;
    lastListenState = msm.open;

    RADIO_ToggleRX(msm.open);
  }
  if (elapsedMilliseconds - lastUpdate >= 1000) {
    gRedrawScreen = true;
    lastUpdate = elapsedMilliseconds;
  }
}

VFO *NextVFO() {
  if (gSettings.activeChannel < LOOT_Size() - 1) {
    gSettings.activeChannel++;
  } else {
    gSettings.activeChannel = 0;
  }
  return &gVFO[gSettings.activeChannel];
}

VFO *PrevVFO() {
  if (gSettings.activeChannel > 0) {
    gSettings.activeChannel--;
  } else {
    gSettings.activeChannel = LOOT_Size() - 1;
  }
  return &gVFO[gSettings.activeChannel];
}

bool VFO_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed) {
    return false;
  }
  if (bKeyHeld) {
    switch (key) {
    case KEY_2:
      gCurrentVFO = NextVFO();
      RADIO_SetupByCurrentVFO();
      LOOT_Standby();
      msm.f = gCurrentVFO->fRX;
      return true;
    default:
      break;
    }
  } else {
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
      return true;
    case KEY_F:
      APPS_run(APP_VFO_CFG);
      return true;
    case KEY_UP:
      RADIO_TuneTo(gCurrentVFO->fRX +
                   StepFrequencyTable[gCurrentPreset->band.step]);
      return true;
    case KEY_DOWN:
      RADIO_TuneTo(gCurrentVFO->fRX -
                   StepFrequencyTable[gCurrentPreset->band.step]);
      return true;
    default:
      break;
    }
  }
  return false;
}

void renderExample() {
  const uint8_t BASE = 38;
  PrintBigDigitsEx(LCD_WIDTH - 1 - 18, BASE, POS_R, C_FILL, "1234.567");
  PrintMediumEx(LCD_WIDTH - 1 - 18, BASE + 8, POS_R, C_FILL, "123.456");
  PrintSmallEx(LCD_WIDTH - 1, BASE + 8, POS_R, C_FILL, "78");
  PrintSmallEx(LCD_WIDTH - 1, BASE - 10, POS_R, C_FILL, "WFM");
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "89");
}

void render2VFOPart(uint8_t i) {
  uint8_t BASE = 24;
  VFO *vfo = &gVFO[i];
  bool isActive = gCurrentVFO == vfo;

  uint8_t bl = BASE + 32 * i;

  uint16_t fp1 = vfo->fRX / 100000;
  uint16_t fp2 = vfo->fRX / 100 % 1000;
  uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[vfo->modulation];

  if (isActive) {
    FillRect(0, bl - 13, 16, 7, C_FILL);
    if (msm.open) {
      PrintSmallEx(0, bl, POS_C, C_INVERT, "RX");
    }
  }

  if (vfo->name[0] < 32 || vfo->name[0] > 127) {
    PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
    PrintMediumBoldEx(LCD_WIDTH - 1, bl, POS_R, C_FILL, "%02u", fp3);
    PrintSmallEx(8, bl - 8, POS_C, C_INVERT, "VFO");
  } else {
    PrintMediumBoldEx(LCD_WIDTH / 2, bl - 8, POS_C, C_FILL, vfo->name);
    PrintMediumEx(LCD_WIDTH / 2, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
    PrintSmallEx(8, bl - 8, POS_C, C_INVERT, "MR");
  }
  PrintSmallEx(LCD_WIDTH - 1, bl - 8, POS_R, C_FILL, mod);

  /* Loot *stats = LOOT_Item(i);
  uint32_t est = stats->lastTimeOpen
                     ? (elapsedMilliseconds - stats->lastTimeOpen) / 1000
                     : 0;
  PrintSmallEx(0, bl + 6, POS_L, C_FILL, "CT %d CD %d", stats->ct, stats->cd);
  PrintSmallEx(LCD_WIDTH - 1, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
               est % 60, stats->duration / 1000); */
}

void render2VFO() {
  for (uint8_t i = 0; i < 2; ++i) {
    render2VFOPart(i);
  }
}

void render1VFO() {
  const uint8_t BASE = 38;

  VFO *vfo = &gVFO[0];

  uint16_t fp1 = vfo->fRX / 100000;
  uint16_t fp2 = vfo->fRX / 10 % 10000;
  uint8_t fp3 = vfo->fRX % 100;
  const char *mod = modulationTypeOptions[vfo->modulation];

  PrintBiggestDigitsEx(LCD_WIDTH - 19, BASE, POS_R, C_FILL, "%4u.%03u", fp1,
                       fp2);
  PrintMediumEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%02u", fp3);
  PrintSmallEx(LCD_WIDTH - 1, BASE - 8, POS_R, C_FILL, mod);
}

void VFO_render() {
  UI_ClearScreen();
  uint8_t sz = LOOT_Size();
  if (sz == 1) {
    render1VFO();
  } else if (sz == 2) {
    render2VFO();
  }
}
