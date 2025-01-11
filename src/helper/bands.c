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
  radio->allowTx = gCurrentBand.allowTx;
  if (copyToVfo) {
    radio->fixedBoundsMode = true;
    radio->step = gCurrentBand.step;
    radio->bw = gCurrentBand.bw;
    radio->gainIndex = gCurrentBand.gainIndex;
    radio->modulation = gCurrentBand.modulation;
    radio->squelch = gCurrentBand.squelch;
  }
  RADIO_SaveCurrentVFO();
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
  if (allBandIndex != newBandIndex ||
      gCurrentBand.meta.type == TYPE_BAND_DETACHED) {
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
