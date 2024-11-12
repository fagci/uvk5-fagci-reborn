#include "vfo2.h"
#include "../dcs.h"
#include "../helper/lootlist.h"
#include "../helper/numnav.h"
#include "../helper/presetlist.h"
#include "../misc.h"
#include "../scheduler.h"
#include "../settings.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "vfo1.h"

static void render2VFOPart(uint8_t i) {
  const uint8_t BASE = 22;
  const uint8_t bl = BASE + 34 * i;

  Preset *p = gVFOPresets[i];
  VFO *vfo = &gVFO[i];
  const bool isActive = gSettings.activeVFO == i;
  const Loot *loot = &gLoot[i];

  uint32_t f =
      gTxState == TX_ON && isActive ? RADIO_GetTXF() : GetScreenF(vfo->rx.f);

  const uint16_t fp1 = f / MHZ;
  const uint16_t fp2 = f / 100 % 1000;
  const uint8_t fp3 = f % 100;
  const char *mod =
      modulationTypeOptions[vfo->modulation == MOD_PRST ? p->modulation
                                                        : vfo->modulation];
  const uint32_t step = StepFrequencyTable[p->step];

  if (isActive && gTxState <= TX_ON) {
    FillRect(0, bl - 14, 28, 7, C_FILL);
    if (gTxState == TX_ON) {
      PrintMedium(0, bl, "TX");
    }
  }

  if (gIsListening && ((gSettings.dw != DW_OFF && gDW.activityOnVFO == i) ||
                       (gSettings.dw == DW_OFF && isActive))) {
    PrintMedium(0, bl, "RX");
    UI_RSSIBar(gLoot[i].rssi, RADIO_GetSNR(), f, 31);
  }

  if (gSettings.dw != DW_OFF && gDW.lastActiveVFO == i) {
    PrintMedium(13, bl, ">>");
  }

  if (gTxState && gTxState != TX_ON && isActive) {
    PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, "%s",
                      TX_STATE_NAMES[gTxState]);
    PrintSmallEx(LCD_XCENTER, bl - 8 + 6, POS_C, C_FILL, "%u", RADIO_GetTXF());
  } else {
    if (vfo->channel >= 0) {
      if (gSettings.chDisplayMode == CH_DISPLAY_MODE_F) {
        PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1,
                         fp2);
      } else if (gSettings.chDisplayMode == CH_DISPLAY_MODE_N) {
        PrintMediumBoldEx(LCD_XCENTER, bl - 4, POS_C, C_FILL, gVFONames[i]);
      } else {
        PrintMediumBoldEx(LCD_XCENTER, bl - 8, POS_C, C_FILL, gVFONames[i]);
        PrintMediumEx(LCD_XCENTER, bl, POS_C, C_FILL, "%4u.%03u", fp1, fp2);
      }
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "MR %03u", vfo->channel + 1);
    } else {
      PrintBigDigitsEx(LCD_WIDTH - 19, bl, POS_R, C_FILL, "%4u.%03u", fp1, fp2);
      PrintMediumBoldEx(LCD_WIDTH, bl, POS_R, C_FILL, "%02u", fp3);
      PrintSmallEx(14, bl - 9, POS_C, C_INVERT, "VFO");
    }
    PrintSmallEx(LCD_WIDTH - 1, bl - 9, POS_R, C_FILL, mod);
    if (vfo->modulation != MOD_PRST) {
      FillRect(LCD_WIDTH - 17, bl - 14, 17, 7, C_INVERT);
    }
  }

  Radio r = vfo->radio == RADIO_UNKNOWN ? p->radio : vfo->radio;

  uint32_t est = loot->lastTimeOpen ? (Now() - loot->lastTimeOpen) / 1000 : 0;
  if (r == RADIO_BK4819) {
    if (loot->ct != 0xFF) {
      PrintSmallEx(0, bl + 6, POS_L, C_FILL, "CT %u.%u",
                   CTCSS_Options[loot->ct] / 10, CTCSS_Options[loot->ct] % 10);
    } else if (loot->cd != 0xFF) {
      PrintSmallEx(0, bl + 6, POS_L, C_FILL, "D%03oN", DCS_Options[loot->cd]);
    }
  }
  PrintSmallEx(LCD_XCENTER, bl + 6, POS_C, C_FILL, "%c %c SQ%u %c %s %s",
               p->allowTx ? TX_POWER_NAMES[p->power][0] : ' ',
               "wNnW"[p->bw], p -> squelch.value,
               RADIO_GetTXFEx(vfo, p) != vfo->rx.f
                   ? (p->offsetDir ? TX_OFFSET_NAMES[p->offsetDir][0] : '*')
                   : ' ',
               vfo->code.tx.type ? TX_CODE_TYPES[vfo->code.tx.type] : "",
               shortRadioNames[r]);

  if (loot->lastTimeOpen) {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%02u:%02u %us", est / 60,
                 est % 60, loot->duration / 1000);
  } else {
    PrintSmallEx(LCD_WIDTH, bl + 6, POS_R, C_FILL, "%d.%02dk", step / 100,
                 step % 100);
  }
}

void VFO2_init(void) {
  gVfo1ProMode = false;
  RADIO_LoadCurrentVFO();
}

bool VFO2_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (VFO1_key(key, bKeyPressed, bKeyHeld)) {
    return true;
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_2:
      LOOT_Standby();
      RADIO_NextVFO();
      return true;
    default:
      break;
    }
  }

  return false;
}

void VFO2_update(void) { VFO1_update(); }

void VFO2_render(void) {
  if (gIsNumNavInput) {
    STATUSLINE_SetText("Select: %s", gNumNavInput);
  } else {
    STATUSLINE_SetText("%s:%u", gCurrentPreset->band.name,
                       PRESETS_GetChannel(gCurrentPreset, radio->rx.f) + 1);
  }

  render2VFOPart(0);
  render2VFOPart(1);
}
