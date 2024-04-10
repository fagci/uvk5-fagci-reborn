#include "si.h"
#include "../driver/si473x.h"
#include "../helper/rds.h"
#include "../scheduler.h"
#include "../svc.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

typedef enum {
  FM_BT,
  MW_BT,
  SW_BT,
  LW_BT,
} BandType;

typedef struct // Band data
{
  const char *bandName; // Bandname
  BandType bandType;    // Band type (FM, MW or SW)
  SI4732_MODE prefmod;  // Pref. modulation
  uint16_t minimumFreq; // Minimum frequency of the band
  uint16_t maximumFreq; // maximum frequency of the band
  uint16_t currentFreq; // Default frequency or current frequency
  uint8_t currentStep;  // Default step (increment and decrement)
  int lastBFO;          // Last BFO per band
  int lastmanuBFO;      // Last Manual BFO per band using X-Tal

} SIBand;

SIBand band[] = {
    {"FM", FM_BT, SI4732_FM, 6400, 10800, 9920, 10, 0, 0},    //  FM 0
    {"LW", LW_BT, SI4732_AM, 100, 514, 198, 9, 0, 0},         //  LW          1
    {"MW", MW_BT, SI4732_AM, 514, 1800, 1395, 9, 0, 0},       //  MW          2
    {"BACON", LW_BT, SI4732_AM, 280, 470, 284, 1, 0, 0},      // Ham  800M 3
    {"630M", SW_BT, SI4732_LSB, 470, 480, 475, 1, 0, 0},      // Ham  630M 4
    {"160M", SW_BT, SI4732_LSB, 1800, 2000, 1850, 1, 0, 0},   // Ham  160M    5
    {"120M", SW_BT, SI4732_AM, 2000, 3200, 2400, 5, 0, 0},    //      120M 6
    {"90M", SW_BT, SI4732_AM, 3200, 3500, 3300, 5, 0, 0},     //       90M 7
    {"80M", SW_BT, SI4732_LSB, 3500, 3900, 3630, 1, 0, 0},    // Ham   80M 8
    {"75M", SW_BT, SI4732_AM, 3900, 5300, 3950, 5, 0, 0},     //       75M 9
    {"60M", SW_BT, SI4732_USB, 5300, 5900, 5375, 1, 0, 0},    // Ham   60M   10
    {"49M", SW_BT, SI4732_AM, 5900, 7000, 6000, 5, 0, 0},     //       49M 11
    {"40M", SW_BT, SI4732_LSB, 7000, 7500, 7074, 1, 0, 0},    // Ham   40M   12
    {"41M", SW_BT, SI4732_AM, 7200, 9000, 7210, 5, 0, 0},     //       41M 13
    {"31M", SW_BT, SI4732_AM, 9000, 10000, 9600, 5, 0, 0},    //       31M   14
    {"30M", SW_BT, SI4732_USB, 10000, 10200, 10099, 1, 0, 0}, // Ham   30M   15
    {"25M", SW_BT, SI4732_AM, 10200, 13500, 11700, 5, 0, 0},  //       25M   16
    {"22M", SW_BT, SI4732_AM, 13500, 14000, 13700, 5, 0, 0},  //       22M   17
    {"20M", SW_BT, SI4732_USB, 14000, 14500, 14074, 1, 0, 0}, // Ham   20M   18
    {"19M", SW_BT, SI4732_AM, 14500, 17500, 15700, 5, 0, 0},  //       19M   19
    {"17M", SW_BT, SI4732_AM, 17500, 18000, 17600, 5, 0, 0},  //       17M   20
    {"16M", SW_BT, SI4732_USB, 18000, 18500, 18100, 1, 0, 0}, // Ham   16M   21
    {"15M", SW_BT, SI4732_AM, 18500, 21000, 18950, 5, 0, 0},  //       15M   22
    {"14M", SW_BT, SI4732_USB, 21000, 21500, 21074, 1, 0, 0}, // Ham   14M   23
    {"13M", SW_BT, SI4732_AM, 21500, 24000, 21500, 5, 0, 0},  //       13M   24
    {"12M", SW_BT, SI4732_USB, 24000, 25500, 24940, 1, 0, 0}, // Ham   12M   25
    {"11M", SW_BT, SI4732_AM, 25500, 26100, 25800, 5, 0, 0},  //       11M   26
    {"CB", SW_BT, SI4732_AM, 26100, 28000, 27200, 1, 0, 0},   // CB band 27
    {"10M", SW_BT, SI4732_USB, 28000, 30000, 28500, 1, 0, 0}, // Ham   10M   28
    {"SW", SW_BT, SI4732_AM, 100, 30000, 15500, 5, 0, 0}      // Whole SW 29
};

static uint32_t freq = 10000;
static uint32_t lastUpdate = 0;
static uint32_t lastRdsUpdate = 0;
static DateTime dt;
static Time t;

static void tune(uint32_t f) {
  f /= 1000;
  f -= f % 5;
  SI4732_SetFreq(freq = f);
}

void SI_init() {
  SVC_Toggle(SVC_LISTEN, false, 0);
  BK4819_Idle();
  SI4732_Init();
  SI4732_SetFreq(freq);
}

static bool hasRDS = false;

void SI_update() {
  if (Now() - lastRdsUpdate >= 1000) {
    hasRDS = SI4732_GetRDS();
    lastRdsUpdate = Now();
    if (hasRDS) {
      gRedrawScreen = true;
    }
  }
  if (Now() - lastUpdate >= 1000) {
    RSQ_GET();
    lastUpdate = Now();
    gRedrawScreen = true;
  }
}

bool SI_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    switch (key) {
    case KEY_UP:
      freq += 10;
      SI4732_SetFreq(freq);
      return true;
    case KEY_DOWN:
      freq -= 10;
      SI4732_SetFreq(freq);
      return true;
    default:
      break;
    }
  }

  // long held
  if (bKeyHeld && bKeyPressed && !gRepeatHeld) {
    switch (key) {
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
      gFInputCallback = tune;
      APPS_run(APP_FINPUT);
      APPS_key(key, bKeyPressed, bKeyHeld);
      return true;
    case KEY_F:
      return true;
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

  uint32_t f = freq * 1000;
  uint16_t fp1 = f / 100000;
  uint16_t fp2 = f / 100 % 1000;

  UI_RSSIBar(rsqStatus.resp.RSSI << 1, f, 42);
  char genre[17];
  SI4732_GetProgramType(genre);

  PrintSmallEx(0, 12, POS_L, C_FILL, "SNR: %u dB", rsqStatus.resp.SNR);
  if (rds.RDSSignal) {
    PrintSmallEx(LCD_WIDTH - 1, 12, POS_R, C_FILL, "RDS");
    // FillRect(LCD_WIDTH - 14, 6, 14, 7, C_INVERT);
  }

  bool hasDT = SI4732_GetLocalDateTime(&dt);
  const char wd[8][3] = {"SU", "MO", "TU", "WE", "TH", "FR", "SA", "SU"};
  PrintSmallEx(LCD_XCENTER, 14, POS_C, C_FILL, "%s", genre);

  if (hasDT) {
    PrintSmallEx(LCD_XCENTER, 22, POS_C, C_FILL, "%02u.%02u.%04u, %s %02u:%02u",
                 dt.day, dt.month, dt.year, wd[dt.wday], dt.hour, dt.minute);
  }

  PrintBiggestDigitsEx(LCD_WIDTH - 22, BASE, POS_R, C_FILL, "%3u.%03u", fp1,
                       fp2);

  PrintSmall(0, LCD_HEIGHT - 8, "%s", rds.radioText);
}
void SI_deinit() {
  SI4732_PowerDown();
  BK4819_RX_TurnOn();
  SVC_Toggle(SVC_LISTEN, true, 10);
}
