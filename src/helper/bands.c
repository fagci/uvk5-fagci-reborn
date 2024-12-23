#include "bands.h"
#include "../external/printf/printf.h"
#include "../radio.h"
#include "channels.h"
#include <stdint.h>

// NOTE
// for SCAN use cached band by index
// for DISPLAY use bands in memory to select it by frequency faster

Band gCurrentBand;

// to use instead of predefined when we need to keep step, etc
Band defaultBand = {
    .name = "default",
    .step = STEP_25_0kHz,
    .bw = BK4819_FILTER_BW_12k,
    .squelch =
        {
            .type = SQUELCH_RSSI_NOISE_GLITCH,
            .value = 4,
        },
    .gainIndex = 18,
    .rxF = 0,
    .txF = 130000000,
};

static DBand defaultBands[80];
static uint8_t bandlistSize = 0;
static uint8_t activeDisplayBandIndex;

static uint8_t activeScanlistBandIndex;

static const PowerCalibration DEFAULT_POWER_CALIB = {40, 65, 140};

PCal POWER_CALIBRATIONS[] = {
    (PCal){.s = 14400000, .e = 14799999, .c = {38, 63, 138}},
    (PCal){.s = 14800000, .e = 17399999, .c = {37, 60, 130}},
    (PCal){.s = 17400000, .e = 24499999, .c = {46, 55, 140}},
    (PCal){.s = 24500000, .e = 26999999, .c = {58, 80, 140}},
    (PCal){.s = 27000000, .e = 42999999, .c = {77, 95, 140}},
    (PCal){.s = 43000000, .e = 46999999, .c = DEFAULT_POWER_CALIB},
    (PCal){.s = 47000000, .e = 61999999, .c = {50, 100, 140}},
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

// STEP_1_0kHz MOD_LSB
SBand BANDS_LSB_HAM[] = {
    (SBand){.s = 181000, .e = 200000},   //
    (SBand){.s = 350000, .e = 380000},   //
    (SBand){.s = 700000, .e = 719999},   //
    (SBand){.s = 1010000, .e = 1015000}, //
    (SBand){.s = 1400000, .e = 1435000}, //
};

// STEP_1_0kHz MOD_USB
SBand BANDS_USB_HAM[] = {
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

SBand BANDS_VHF_UHF[] = {
    (SBand){.s = 11800000, .e = 13699999}, // air
    (SBand){.s = 14400000, .e = 14599999}, // HAM
    (SBand){.s = 15172500, .e = 15599999}, // railw
    (SBand){.s = 15600000, .e = 16327500}, // sea
    (SBand){.s = 24300000, .e = 26999999}, // SATCOM
    (SBand){.s = 43307500, .e = 43477500}, // LPD
    (SBand){.s = 44600625, .e = 44619375}, // PMR
};

static const uint8_t OFS1 = 0;
static const uint8_t OFS2 = OFS1 + ARRAY_SIZE(BANDS_LW_MW_BCAST);
static const uint8_t OFS3 = OFS2 + ARRAY_SIZE(BANDS_LOW_BCAST);
static const uint8_t OFS4 = OFS3 + ARRAY_SIZE(BANDS_LSB_HAM);
static const uint8_t OFS5 = OFS4 + ARRAY_SIZE(BANDS_USB_HAM);
static const uint8_t OFS6 = OFS5 + ARRAY_SIZE(BANDS_CB);
static const uint8_t OFS7 = OFS6 + ARRAY_SIZE(BANDS_BCAST_FM);
static const uint8_t OFS8 = OFS7 + ARRAY_SIZE(BANDS_VHF_UHF);

uint8_t BANDS_DefaultCount() { return OFS8; }

Band BANDS_GetDefaultBand(uint8_t i) {
  Band b = {
      .gainIndex = AUTO_GAIN_INDEX + 1, // +2dB
      .modulation = MOD_FM,
      .allowTx = false,
      .step = STEP_12_5kHz,
      .bw = BK4819_FILTER_BW_12k,
      .code = 0,
      .squelch.type = SQUELCH_RSSI_NOISE_GLITCH,
      .squelch.value = 4,
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
    snprintf(b.name, 8, "BC %um", 30000000 / b.rxF);
  } else if (i < OFS4) {
    sb = BANDS_LSB_HAM[i - OFS3];
    b.modulation = MOD_LSB;
    b.step = STEP_1_0kHz;
    snprintf(b.name, 8, "HAM %um", 30000000 / b.rxF);
  } else if (i < OFS5) {
    sb = BANDS_USB_HAM[i - OFS4];
    b.modulation = MOD_USB;
    b.step = STEP_1_0kHz;
    snprintf(b.name, 8, "HAM %um", 30000000 / b.rxF);
  } else if (i < OFS6) {
    sb = BANDS_CB[i - OFS5];
    b.step = STEP_10_0kHz;
    snprintf(b.name, 8, "CB");
  } else if (i < OFS7) {
    sb = BANDS_BCAST_FM[i - OFS6];
    b.modulation = MOD_WFM;
    b.step = STEP_100_0kHz;
    snprintf(b.name, 8, "Bcast FM");
  } else if (i < OFS8) {
    sb = BANDS_VHF_UHF[i - OFS7];
    snprintf(b.name, 8, "%u-%u", b.rxF / MHZ, b.txF / MHZ);
    if (sb.s == 15172500 || sb.s == 44600625) {
      b.step = STEP_12_5kHz;
    }
    if (sb.s == 11800000) {
      b.modulation = MOD_AM;
    }
  }

  b.rxF = sb.s;
  b.txF = sb.e;

  b.misc.powCalib = BANDS_GetPowerCalib((b.txF - b.rxF) / 2);

  return b;
}

void BANDS_Load(void) {
  for (int16_t chNum = 0; chNum < CHANNELS_GetCountMax() - 2; ++chNum) {
    if (CHANNELS_GetMeta(chNum).type != TYPE_BAND) {
      continue;
    }

    CH ch;
    CHANNELS_Load(chNum, &ch);
    defaultBands[bandlistSize] = (DBand){
        .mr = chNum,
        .s = ch.rxF,
        .e = ch.txF,
        .step = ch.step,
    };

    bandlistSize++;

    if (bandlistSize >= BANDS_COUNT_MAX) {
      break;
    }
  }
}

// TODO: scan band select

Band BANDS_Item(int8_t i) {
  Band b;
  CHANNELS_Load(defaultBands[i].mr, &b);
  return b;
}

bool BAND_InRange(const uint32_t f, const Band p) {
  return f >= p.rxF && f <= p.txF;
}

int8_t BAND_IndexOf(Band p) {
  for (uint8_t i = 0; i < bandlistSize; ++i) {
    Band tmp = BANDS_Item(i);
    if (memcmp(&tmp, &p, sizeof(p)) == 0) {
      return i;
    }
  }
  return -1;
}

void BAND_SelectScan(int8_t i) {
  if (gScanlistSize) {
    activeScanlistBandIndex = i;
    RADIO_TuneToBand(gScanlist[i]);
  }
}

void BAND_Select(int8_t i) {
  gCurrentBand = BANDS_Item(i);
  activeDisplayBandIndex = i;
}

void BANDS_SelectBandRelative(bool next) {
  int8_t bandIndex = activeDisplayBandIndex;
  IncDecI8(&bandIndex, 0, bandlistSize, next ? 1 : -1);
  activeDisplayBandIndex = bandIndex;
  gCurrentBand = BANDS_Item(activeDisplayBandIndex);
  radio->rxF = gCurrentBand.rxF;
  SETTINGS_DelayedSave();
}

Band BAND_ByFrequency(uint32_t f) {
  uint32_t smallerBW = BK4819_F_MAX;
  int16_t index = -1;
  for (uint8_t i = 0; i < bandlistSize; ++i) {
    Band item = BANDS_Item(i);
    if (BAND_InRange(f, item)) {
      uint32_t bw = item.txF - item.rxF;
      if (bw < smallerBW) {
        smallerBW = bw;
        index = i;
      }
    }
  }
  if (index >= 0) {
    return BANDS_Item(index);
  }
  return defaultBand;
}

/**
 * Select band, return if changed
 */
bool BAND_SelectByFrequency(uint32_t f) {
  // TODO: select more precise band from list
  int16_t oldBandIndex = activeDisplayBandIndex;
  int16_t newBandIndex = -1;
  for (uint8_t i = 0; i < bandlistSize; ++i) {
    DBand *b = &defaultBands[i];
    if (b->s <= f && f <= b->e) {
      newBandIndex = b->mr;
      activeDisplayBandIndex = i;
      break;
    }
  }
  if (oldBandIndex != newBandIndex) {
    if (newBandIndex >= 0) {
      CHANNELS_Load(defaultBands[newBandIndex].mr, &gCurrentBand);
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
  uint8_t index = activeScanlistBandIndex;
  uint8_t oi = index;
  IncDec8(&index, 0, gScanlistSize, next);
  activeScanlistBandIndex = index;
  CHANNELS_Load(gScanlist[activeScanlistBandIndex], &gCurrentBand);
  return oi != index;
}

void BANDS_SaveCurrent(void) {
  CHANNELS_Save(defaultBands[activeDisplayBandIndex].mr, &gCurrentBand);
}

PowerCalibration BANDS_GetPowerCalib(uint32_t f) {
  for (uint8_t ci = 0; ci < ARRAY_SIZE(POWER_CALIBRATIONS); ++ci) {
    PCal cal = POWER_CALIBRATIONS[ci];
    if (cal.s < f && f < cal.e) {
      return cal.c;
      break;
    }
  }
  return DEFAULT_POWER_CALIB;
}
