#include "si.h"
#include "../driver/bk4819.h"
#include "../driver/si473x.h"
#include "../helper/rds.h"
#include "../misc.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"
#include <stdint.h>

typedef enum {
  FM_BT,
  MW_BT,
  SW_BT,
  LW_BT,
} BandType;

static const char SI47XX_BW_NAMES[5][8] = {
    "6 kHz", "4 kHz", "3 kHz", "2 kHz", "1 kHz",
};

static const char SI47XX_SSB_BW_NAMES[6][8] = {
    "1.2 kHz", "2.2 kHz", "3 kHz", "4 kHz", "0.5 kHz", "1 kHz",
};

static const char SI47XX_MODE_NAMES[5][4] = {
    "FM", "AM", "LSB", "USB", "CW",
};

static SI47XX_FilterBW bw = SI47XX_BW_6_kHz;
static SI47XX_SsbFilterBW ssbBw = SI47XX_SSB_BW_3_kHz;


static uint8_t att = 0;
static uint16_t step = 10;
static uint32_t lastUpdate = 0;
static uint32_t lastRdsUpdate = 0;
static uint32_t lastSeekUpdate = 0;
static DateTime dt;
static int16_t bfo = 0;
static bool showSNR = false;

void SI_init() {
  SVC_Toggle(SVC_LISTEN, false, 0);
  BK4819_Idle();
  SI47XX_PowerUp();

  SI47XX_SetAutomaticGainControl(1, att);
}

static bool hasRDS = false;
static bool seeking = false;

void SI_update() {
  if (si4732mode == SI47XX_FM && Now() - lastRdsUpdate >= 1000) {
    hasRDS = SI47XX_GetRDS();
    lastRdsUpdate = Now();
    if (hasRDS) {
      gRedrawScreen = true;
    }
  }
  if (Now() - lastUpdate >= 1000) {
    if (showSNR) {
      RSQ_GET();
    }
    lastUpdate = Now();
    gRedrawScreen = true;
  }
  if (seeking && Now() - lastSeekUpdate >= 100) {
    bool valid = false;
    siCurrentFreq = SI47XX_getFrequency(&valid);
    if (valid) {
      seeking = false;
    }
    lastSeekUpdate = Now();
    gRedrawScreen = true;
  }
}

static uint32_t lastFreqChange = 0;

static void resetBFO() {
  if (bfo != 0) {
    bfo = 0;
    SI47XX_SetBFO(bfo);
  }
}

bool SI_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      if (Now() - lastFreqChange > 250) {
        lastFreqChange = Now();
        tune((siCurrentFreq + step) * divider);
        resetBFO();
      }
      return true;
    case KEY_DOWN:
      if (Now() - lastFreqChange > 250) {
        lastFreqChange = Now();
        tune((siCurrentFreq - step) * divider);
        resetBFO();
      }
      return true;
    case KEY_SIDE1:
      if (SI47XX_IsSSB()) {
        if (bfo < INT16_MAX - 10) {
          bfo += 10;
        }
        SI47XX_SetBFO(bfo);
      }
      return true;
    case KEY_SIDE2:
      if (SI47XX_IsSSB()) {
        if (bfo > INT16_MIN + 10) {
          bfo -= 10;
        }
        SI47XX_SetBFO(bfo);
      }
      return true;
    case KEY_2:
      if (att < 37) {
        att++;
        SI47XX_SetAutomaticGainControl(1, att);
      }
      return true;
    case KEY_8:
      if (att > 0) {
        att--;
        SI47XX_SetAutomaticGainControl(att > 0, att);
      }
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
    case KEY_STAR:
      if (SI47XX_IsSSB()) {
        return false;
      }
      if (si4732mode == SI47XX_FM) {
        SI47XX_SetSeekFmSpacing(step);
      } else {
        SI47XX_SetSeekAmSpacing(step);
      }
      SI47XX_Seek(1, 1);
      seeking = true;
      return true;
    default:
      break;
    }
  }

  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_1:
      if (step < 1000) {
        if (step == 1 || step == 10 || step == 100 || step == 1000) {
          step *= 5;
        } else {
          step *= 2;
        }
      }
      return true;
    case KEY_7:
      if (step > 1) {
        if (step == 1 || step == 10 || step == 100 || step == 1000) {
          step /= 2;
        } else {
          step /= 5;
        }
      }
      return true;
    case KEY_6:
      if (SI47XX_IsSSB()) {
        if (ssbBw == SI47XX_SSB_BW_1_0_kHz) {
          ssbBw = SI47XX_SSB_BW_1_2_kHz;
        } else {
          ssbBw++;
        }
        SI47XX_SetSsbBandwidth(ssbBw);
      } else {
        if (bw == SI47XX_BW_1_kHz) {
          bw = SI47XX_BW_6_kHz;
        } else {
          bw++;
        }
        SI47XX_SetBandwidth(bw, true);
      }
      return true;
    case KEY_4:
      showSNR = !showSNR;
      return true;
    case KEY_5:
      gFInputCallback = tune;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_0:
      divider = 100;
      if (si4732mode == SI47XX_FM) {
        SI47XX_SwitchMode(SI47XX_AM);
        SI47XX_SetBandwidth(bw, true);
        tune(720000);
        step = 5;
      } else if (si4732mode == SI47XX_AM) {
        SI47XX_SwitchMode(SI47XX_LSB);
        SI47XX_SetSsbBandwidth(ssbBw);
        tune(711300);
        step = 1;
      } else {
        divider = 1000;
        SI47XX_SwitchMode(SI47XX_FM);
        tune(10000000);
        step = 10;
      }
      resetBFO();
      return true;
    case KEY_F:
      if (SI47XX_IsSSB()) {
        SI47XX_SwitchMode(si4732mode == SI47XX_LSB ? SI47XX_USB : SI47XX_LSB);
        tune(siCurrentFreq * divider); // to apply SSB
        return true;
      }
      return false;
    case KEY_STAR:
      BK4819_Idle();
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

void SI_render() {
  UI_ClearScreen();
  const uint8_t BASE = 38;

  uint32_t f = siCurrentFreq * divider;
  uint16_t fp1 = f / 100000;
  uint16_t fp2 = f / 100 % 1000;

  // PrintSmallEx(0, 12, POS_L, C_FILL, "SNR: %u dB", rsqStatus.resp.SNR);

  PrintBiggestDigitsEx(LCD_WIDTH - 22, BASE, POS_R, C_FILL, "%3u.%03u", fp1,
                       fp2);
  PrintSmallEx(LCD_WIDTH - 1, BASE - 6, POS_R, C_FILL, "%s",
               SI47XX_MODE_NAMES[si4732mode]);
  if (SI47XX_IsSSB()) {
    PrintSmallEx(LCD_WIDTH - 1, BASE, POS_R, C_FILL, "%d", bfo);
  }

  if (si4732mode == SI47XX_FM) {
    if (rds.RDSSignal) {
      PrintSmallEx(LCD_WIDTH - 1, 12, POS_R, C_FILL, "RDS");
    }

    char genre[17];
    const char wd[8][3] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA", "SU"};
    SI47XX_GetProgramType(genre);
    PrintSmallEx(LCD_XCENTER, 14, POS_C, C_FILL, "%s", genre);

    if (SI47XX_GetLocalDateTime(&dt)) {
      PrintSmallEx(LCD_XCENTER, 22, POS_C, C_FILL,
                   "%02u.%02u.%04u, %s %02u:%02u", dt.day, dt.month, dt.year,
                   wd[dt.wday], dt.hour, dt.minute);
    }

    PrintSmall(0, LCD_HEIGHT - 8, "%s", rds.radioText);
  }

  if (si4732mode == SI47XX_FM) {
    PrintSmallEx(LCD_XCENTER, BASE + 6, POS_C, C_FILL, "STP %u ATT %u", step,
                 att);
  } else if (SI47XX_IsSSB()) {
    PrintSmallEx(LCD_XCENTER, BASE + 6, POS_C, C_FILL, "STP %u ATT %u BW %s",
                 step, att, SI47XX_SSB_BW_NAMES[ssbBw]);
  } else {
    PrintSmallEx(LCD_XCENTER, BASE + 6, POS_C, C_FILL, "STP %u ATT %u BW %s",
                 step, att, SI47XX_BW_NAMES[bw]);
  }

  if (showSNR) {
    uint8_t rssi = rsqStatus.resp.RSSI;
    if (rssi > 64) {
      rssi = 64;
    }
    FillRect(0, 8, rssi * 2, 2, C_FILL);
    PrintSmall(0, 15, "SNR %u", rsqStatus.resp.SNR);
  }
}

void SI_deinit() {
  SI47XX_PowerDown();
  BK4819_RX_TurnOn();
  SVC_Toggle(SVC_LISTEN, true, 10);
}
