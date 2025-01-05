#include "bands.h"
#include "../driver/uart.h"
#include "../external/printf/printf.h"
#include "../radio.h"
#include "channels.h"
#include "measurements.h"
#include <stdint.h>

// NOTE
// for SCAN use cached band by index
// for DISPLAY use bands in memory to select it by frequency faster

Band gCurrentBand;

// to use instead of predefined when we need to keep step, etc
Band defaultBand = {
    .meta.readonly = true,
    .meta.type = TYPE_BAND_DETACHED,
    .name = "-",
    .step = STEP_25_0kHz,
    .bw = BK4819_FILTER_BW_12k,
    .squelch =
        {
            .type = SQUELCH_RSSI_NOISE_GLITCH,
            .value = 4,
        },
    .gainIndex = 21,
    .rxF = 0,
    .txF = 130000000,
};

static DBand allBands[BANDS_COUNT_MAX];
static int16_t allBandIndex; // -1 if default is current
static uint8_t allBandsSize = 0;

static uint8_t scanlistBandIndex;

static const PowerCalibration DEFAULT_POWER_CALIB = {
    43, 68, 140}; // Standard UV-K6 Power Calibration
// static const PowerCalibration DEFAULT_POWER_CALIB = {41, 65, 140};  //
// Modified UV-K6 Power Calibrations BFU550 A + Seperated Coils static const
// PowerCalibration DEFAULT_POWER_CALIB = {40, 65, 140};  // reborn orginal

PCal POWER_CALIBRATIONS[] = {
    /*
     (PCal){.s = 14400000, .e = 14799999, .c = {38, 63, 138}},         // reborn
     original (PCal){.s = 14800000, .e = 17399999, .c = {37, 60, 130}},
     (PCal){.s = 17400000, .e = 24499999, .c = {46, 55, 140}},
     (PCal){.s = 24500000, .e = 26999999, .c = {58, 80, 140}},
     (PCal){.s = 27000000, .e = 42999999, .c = {77, 95, 140}},
     (PCal){.s = 43000000, .e = 46999999, .c = DEFAULT_POWER_CALIB},
     (PCal){.s = 47000000, .e = 61999999, .c = {50, 100, 140}},

   */

    // Standard UV-K6 Power Calibration

    (PCal){.s = 13500000, .e = 16499999, .c = {38, 65, 140}},
    (PCal){.s = 16500000, .e = 20499999, .c = {36, 52, 140}},
    (PCal){.s = 20500000, .e = 21499999, .c = {41, 64, 135}},
    (PCal){.s = 21500000, .e = 21999999, .c = {44, 46, 50}},
    //(PCal){.s = 22000000, .e = 23999999, .c = {0, 0, 0}},     // no power
    // output power
    (PCal){.s = 24000000, .e = 26499999, .c = {62, 82, 130}},
    (PCal){.s = 26500000, .e = 26999999, .c = {65, 92, 140}},
    (PCal){.s = 27000000, .e = 27499999, .c = {73, 103, 140}},
    (PCal){.s = 27500000, .e = 28499999, .c = {81, 107, 140}},
    (PCal){.s = 28500000, .e = 29499999, .c = {57, 94, 140}},
    (PCal){.s = 29500000, .e = 30499999, .c = {74, 104, 140}},
    (PCal){.s = 30500000, .e = 33499999, .c = {81, 107, 140}},
    (PCal){.s = 33500000, .e = 34499999, .c = {63, 98, 140}},
    (PCal){.s = 34500000, .e = 35499999, .c = {52, 89, 140}},
    (PCal){.s = 35500000, .e = 36499999, .c = {46, 74, 140}},
    //(PCal){.s = 36500000, .e = 46999999, .c = DEFAULT_POWER_CALIB},     // no
    // need to add this line as it will drop to the default if not found ?
    (PCal){.s = 47000000, .e = 61999999, .c = {46, 77, 140}},

    /*


       // Modified UV-K6 Power Calibrations BFU550 A + Seperated Coils

      //(PCal){.s = 14400000, .e = 19499999, .c = DEFAULT_POWER_CALIB},
        (PCal){.s = 19500000, .e = 20499999, .c = {49, 78, 140}},
        (PCal){.s = 20500000, .e = 21499999, .c = {63, 96, 140}},
        (PCal){.s = 21500000, .e = 21999999, .c = {82, 108, 140}},
        (PCal){.s = 22000000, .e = 22499999, .c = {96, 115, 140}},
        (PCal){.s = 22500000, .e = 23499999, .c = {93, 106, 120}},
      //(PCal){.s = 23500000, .e = 23999999, .c = {0, 0, 0}},       // no power
      output power (PCal){.s = 24000000, .e = 25499999, .c = {50, 78, 135}},
        (PCal){.s = 25500000, .e = 26999999, .c = {48, 62, 108}},
        (PCal){.s = 27000000, .e = 27499999, .c = {50, 73, 118}},
        (PCal){.s = 27500000, .e = 28499999, .c = {59, 85, 130}},
        (PCal){.s = 28500000, .e = 29499999, .c = {51, 90, 140}},
        (PCal){.s = 29500000, .e = 30499999, .c = {74, 106, 140}},
        (PCal){.s = 30500000, .e = 33499999, .c = {85, 109, 140}},
        (PCal){.s = 33500000, .e = 34499999, .c = {79, 106, 140}},
        (PCal){.s = 34500000, .e = 35499999, .c = {65, 98, 140}},
        (PCal){.s = 35500000, .e = 38499999, .c = {48, 85, 140}},
        (PCal){.s = 38500000, .e = 39499999, .c = {44, 68, 140}},
        (PCal){.s = 39500000, .e = 42499999, .c = {38, 59, 140}},
      //(PCal){.s = 42500000, .e = 46900000, .c = DEFAULT_POWER_CALIB},
        (PCal){.s = 47000000, .e = 61999999, .c = {46, 75, 140}},

    */

};

// MOD_AM STEP_9_0kHz
SBand BANDS_LW_MW_BCAST[] = {
    (SBand){.s = 15300, .e = 27900},
    (SBand){.s = 52200, .e = 170100},
};

// MOD_AM STEP_5_0kHz
SBand BANDS_LOW_BCAST[] = {
    (SBand){.s = 230000, .e = 249500},   //
    (SBand){.s = 320000, .e = 340000},   //
    (SBand){.s = 390000, .e = 400000},   //
    (SBand){.s = 475000, .e = 506000},   //
    (SBand){.s = 585000, .e = 635000},   //
    (SBand){.s = 720000, .e = 750000},   //
    (SBand){.s = 940000, .e = 999000},   //
    (SBand){.s = 1160000, .e = 1210000}, //
    (SBand){.s = 1350000, .e = 1387000}, //
    (SBand){.s = 1510000, .e = 1560000}, //
    (SBand){.s = 1755000, .e = 1805000}, //
    (SBand){.s = 1890000, .e = 1902000}, //
    (SBand){.s = 2145000, .e = 2185000}, //
    (SBand){.s = 2560000, .e = 2610000}, //
};

static uint8_t BC_M[] = {120, 90, 75, 60, 49, 41, 31,
                         25,  22, 19, 16, 15, 13, 11};
static uint8_t HAM_M[] = {160, 80, 40, 30, 20, 17, 15, 12, 10};

// STEP_1_0kHz
SBand BANDS_SSB_HAM[] = {
    (SBand){.s = 181000, .e = 200000},   //
    (SBand){.s = 350000, .e = 380000},   //
    (SBand){.s = 700000, .e = 719999},   //
    (SBand){.s = 1010000, .e = 1015000}, //
    (SBand){.s = 1400000, .e = 1435000}, //
    (SBand){.s = 1806800, .e = 1816800}, //
    (SBand){.s = 2100000, .e = 2145000}, //
    (SBand){.s = 2489000, .e = 2499000}, //
    (SBand){.s = 2800000, .e = 2969999}, //
};

// STEP_10_0kHz MOD_FM
SBand BANDS_CB[] = {
    (SBand){.s = 2696500, .e = 2760124}, //
    (SBand){.s = 2760125, .e = 2799125}, //
};

// STEP_100_0kHz MOD_WFM
SBand BANDS_BCAST_FM[] = {
    (SBand){.s = 8800000, .e = 10799999}, //
};

typedef struct {
  char name[9];
  uint32_t s;
  uint32_t e;
  Step step; // needed to select band by freq
} NamedBand;

NamedBand BANDS_VHF_UHF[] = {
    (NamedBand){
        .s = 11800000, .e = 13699999, .name = "Air", .step = STEP_50_0kHz},
    (NamedBand){
        .s = 14400000, .e = 14599999, .name = "HAM 2m", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 14800000, .e = 14900000, .name = "MVD A", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 15172500, .e = 15599999, .name = "Railway", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 15600000, .e = 16327500, .name = "Sea", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 17100000, .e = 17300000, .name = "MVD B/X", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 17300000, .e = 17800000, .name = "Top 173", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 24300000, .e = 26999999, .name = "SATCOM", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 30001250, .e = 30051250, .name = "River1", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 33601250, .e = 33651250, .name = "River2", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 43307500, .e = 43477500, .name = "LPD", .step = STEP_25_0kHz},
    (NamedBand){
        .s = 44600625, .e = 44619375, .name = "PMR", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 45000000, .e = 45300000, .name = "MVD 450", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 46000000, .e = 46300000, .name = "MVD 460", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 80600000, .e = 82500000, .name = "800svc1", .step = STEP_12_5kHz},
    (NamedBand){
        .s = 85100000, .e = 87000000, .name = "800svc2", .step = STEP_12_5kHz},
};

static const uint8_t OFS1 = 0;
static const uint8_t OFS2 = OFS1 + ARRAY_SIZE(BANDS_LW_MW_BCAST);
static const uint8_t OFS3 = OFS2 + ARRAY_SIZE(BANDS_LOW_BCAST);
static const uint8_t OFS4 = OFS3 + ARRAY_SIZE(BANDS_SSB_HAM);
static const uint8_t OFS5 = OFS4 + ARRAY_SIZE(BANDS_CB);
static const uint8_t OFS6 = OFS5 + ARRAY_SIZE(BANDS_BCAST_FM);
static const uint8_t OFS7 = OFS6 + ARRAY_SIZE(BANDS_VHF_UHF);

static int16_t bandIndexByFreq(uint32_t f, bool preciseStep) {
  int16_t newBandIndex = -1;
  uint32_t smallestDiff = UINT32_MAX;
  for (uint8_t i = 0; i < allBandsSize; ++i) {
    DBand *b = &allBands[i];
    if (f < b->s || f > b->e) {
      continue;
    }
    if (preciseStep & (f % StepFrequencyTable[b->step])) {
      continue;
    }
    uint32_t diff = DeltaF(b->s, f) + DeltaF(b->e, f);
    if (diff < smallestDiff) {
      smallestDiff = diff;
      newBandIndex = i;
    }
  }
  return newBandIndex;
}

uint8_t BANDS_DefaultCount() { return OFS7; }

Band BANDS_GetDefaultBand(uint8_t i) {
  Band b = {
      .meta.type = TYPE_BAND,
      .meta.readonly = false,
      .scanlists = 0,
      .offsetDir = 0,
      .step = STEP_25_0kHz,
      .modulation = MOD_FM,
      .bw = BK4819_FILTER_BW_12k,
      .gainIndex = AUTO_GAIN_INDEX + 1, // +2dB
      .allowTx = false,
      .scrambler = 0,
      .squelch.type = SQUELCH_RSSI_NOISE_GLITCH,
      .squelch.value = 3,
      .misc.bank = 0,
      .code.rx.type = 0,
      .code.rx.value = 0,
      .code.tx.type = 0,
      .code.tx.value = 0,
  };
  SBand sb;
  if (i < OFS2) {
    sb = BANDS_LW_MW_BCAST[i - OFS1];
    b.modulation = MOD_AM;
    b.step = STEP_9_0kHz;
    snprintf(b.name, 8, i == 0 ? "LW" : "MW");
  } else if (i < OFS3) {
    sb = BANDS_LOW_BCAST[i - OFS2];
    b.modulation = MOD_AM;
    b.step = STEP_5_0kHz;
    snprintf(b.name, 8, "BC %um", BC_M[i - OFS2]);
  } else if (i < OFS4) {
    sb = BANDS_SSB_HAM[i - OFS3];
    b.modulation = sb.s >= 1000000 ? MOD_USB : MOD_LSB;
    b.step = STEP_1_0kHz;
    snprintf(b.name, 8, "HAM %um", HAM_M[i - OFS3]);
  } else if (i < OFS5) {
    sb = BANDS_CB[i - OFS4];
    b.step = STEP_10_0kHz;
    snprintf(b.name, 8, "CB");
  } else if (i < OFS6) {
    sb = BANDS_BCAST_FM[i - OFS5];
    b.modulation = MOD_WFM;
    b.step = STEP_100_0kHz;
    snprintf(b.name, 8, "BcastFM");
  } else if (i < OFS7) {
    NamedBand nb = BANDS_VHF_UHF[i - OFS6];
    snprintf(b.name, 8, nb.name);
    if (nb.s == 11800000) {
      b.modulation = MOD_AM;
      // b.step = STEP_50_0kHz;
    }
    b.scanlists = nb.s == 24300000 ? 2 : 1;
    b.step = nb.step;
    sb.s = nb.s;
    sb.e = nb.e;
  }

  b.rxF = sb.s;
  b.txF = sb.e;
  b.misc.lastUsedFreq = sb.s;

  b.misc.powCalib = (PowerCalibration){0, 0, 0};

  return b;
}

void BANDS_Load(void) {
  for (int16_t chNum = 0; chNum < CHANNELS_GetCountMax() - 2; ++chNum) {
    if (CHANNELS_GetMeta(chNum).type != TYPE_BAND) {
      continue;
    }

    CH ch;
    CHANNELS_Load(chNum, &ch);
    allBands[allBandsSize] = (DBand){
        .mr = chNum,
        .s = ch.rxF,
        .e = ch.txF,
        .step = ch.step,
    };

    allBandsSize++;

    if (allBandsSize >= BANDS_COUNT_MAX) {
      break;
    }
  }
}

bool BANDS_InRange(const uint32_t f, const Band p) {
  return f >= p.rxF && f <= p.txF;
}

// Set gCurrentBand, sets internal cursor in SL
void BANDS_Select(int16_t num, bool copyToVfo) {
  CHANNELS_Load(num, &gCurrentBand);
  // Log("Load Band %s", gCurrentBand.name);
  for (int16_t i = 0; i < gScanlistSize; ++i) {
    if (gScanlist[i] == num) {
      scanlistBandIndex = i;
      allBandIndex = bandIndexByFreq(gCurrentBand.rxF, true);
      // Log("SL band index %u", i);
      break;
    }
  }
  if (copyToVfo) {
    radio->fixedBoundsMode = true;
    radio->step = gCurrentBand.step;
    radio->bw = gCurrentBand.bw;
    radio->gainIndex = gCurrentBand.gainIndex;
    radio->modulation = gCurrentBand.modulation;
    radio->squelch = gCurrentBand.squelch;
    radio->allowTx = gCurrentBand.allowTx;
    RADIO_SaveCurrentVFO();
  }
}

// Used in vfo1 to select first band from scanlist
void BANDS_SelectScan(int8_t i) {
  if (gScanlistSize) {
    scanlistBandIndex = i;
    RADIO_TuneToBand(gScanlist[i]);
  }
}

Band BANDS_ByFrequency(uint32_t f) {
  int16_t index = bandIndexByFreq(f, false);
  if (index >= 0) {
    Band b;
    CHANNELS_Load(allBands[index].mr, &b);
    return b;
  }
  return defaultBand;
}

uint8_t BANDS_GetScanlistIndex() { return scanlistBandIndex; }

/**
 * Select band, return if changed
 */
bool BANDS_SelectByFrequency(uint32_t f, bool copyToVfo) {
  int16_t newBandIndex = bandIndexByFreq(f, false);
  if (allBandIndex != newBandIndex) {
    allBandIndex = newBandIndex;
    if (allBandIndex >= 0) {
      BANDS_Select(allBands[allBandIndex].mr, copyToVfo);
    } else {
      gCurrentBand = defaultBand;
    }
    return true;
  }
  return false;
}

bool BANDS_SelectBandRelativeByScanlist(bool next) {
  if (gScanlistSize == 0) {
    return false;
  }
  uint8_t oldScanlistBandIndex = scanlistBandIndex;
  IncDec8(&scanlistBandIndex, 0, gScanlistSize, next ? 1 : -1);
  BANDS_Select(gScanlist[scanlistBandIndex], true);
  return oldScanlistBandIndex != scanlistBandIndex;
}

void BANDS_SaveCurrent(void) {
  if (allBandIndex >= 0 && gCurrentBand.meta.type == TYPE_BAND) {
    CHANNELS_Save(allBands[allBandIndex].mr, &gCurrentBand);
  }
}

PowerCalibration BANDS_GetPowerCalib(uint32_t f) {
  Band b = BANDS_ByFrequency(f);
  if (b.meta.type == TYPE_BAND &&
      b.misc.powCalib.e > 0) { // not TYPE_BAND_DETACHED
    return b.misc.powCalib;
  }
  for (uint8_t ci = 0; ci < ARRAY_SIZE(POWER_CALIBRATIONS); ++ci) {
    PCal cal = POWER_CALIBRATIONS[ci];
    if (cal.s <= f && f <= cal.e) {
      return cal.c;
      break;
    }
  }
  return DEFAULT_POWER_CALIB;
}
