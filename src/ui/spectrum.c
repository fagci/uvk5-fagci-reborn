#include "spectrum.h"
#include "../driver/uart.h"
#include "../helper/measurements.h"
#include "components.h"
#include "graphics.h"
#include <stdint.h>

#define MAX_POINTS 128

typedef struct {
  uint16_t vMin;
  uint16_t vMax;
} VMinMax;

const uint8_t SPECTRUM_Y = 16;
const uint8_t SPECTRUM_H = 40;

static const uint8_t S_BOTTOM = SPECTRUM_Y + SPECTRUM_H;

static uint16_t rssiHistory[MAX_POINTS] = {0};
static uint8_t noiseHistory[MAX_POINTS] = {0};

static uint8_t x = 0;
static uint8_t ox = UINT8_MAX;
static uint8_t filledPoints;

static Band *range;
static uint32_t step;

static uint16_t minRssi(const uint16_t *array, uint8_t n) {
  uint16_t min = UINT16_MAX;
  for (uint8_t i = 0; i < n; ++i) {
    if (array[i] && array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

static uint8_t maxNoise(const uint8_t *array, uint8_t n) {
  uint16_t max = 0;
  for (uint8_t i = 0; i < n; ++i) {
    if (array[i] != UINT8_MAX && array[i] > max) {
      max = array[i];
    }
    if (array[i] == UINT8_MAX) {
      Log("!!! NOISE=255 at %u", i); // appears when switching bands
    }
  }
  return max;
}

void SP_ResetHistory(void) {
  filledPoints = 0;
  for (uint8_t i = 0; i < MAX_POINTS; ++i) {
    rssiHistory[i] = 0;
    noiseHistory[i] = UINT8_MAX;
  }
}

void SP_Begin(void) {
  x = 0;
  ox = UINT8_MAX;
}

void SP_Next(void) {
  // TODO: remove
}

void SP_Init(Band *b) {
  range = b;
  step = StepFrequencyTable[b->step];
  SP_ResetHistory();
  SP_Begin();
}

#include "../driver/uart.h"
void SP_AddPoint(const Loot *msm) {
  uint32_t xs = ConvertDomain(msm->f, range->rxF, range->txF, 0, MAX_POINTS);
  uint32_t xe =
      ConvertDomain(msm->f + step, range->rxF, range->txF, 0, MAX_POINTS);

  if (xe > MAX_POINTS) {
    xe = MAX_POINTS;
  }
  for (x = xs; x < xe; ++x) {
    if (ox != x) {
      ox = x;
      rssiHistory[x] = 0;
      noiseHistory[x] = UINT8_MAX;
    }
    if (msm->rssi > rssiHistory[x]) {
      rssiHistory[x] = msm->rssi;
    }
    if (msm->noise < noiseHistory[x]) {
      noiseHistory[x] = msm->noise;
    }
  }
  if (x + 1 > filledPoints) {
    filledPoints = x + 1;
  }
  if (filledPoints > MAX_POINTS) {
    filledPoints = MAX_POINTS;
  }
}

static VMinMax getV() {
  const uint16_t rssiMin = minRssi(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t noiseFloor = SP_GetNoiseFloor();
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax =
      rssiMax + Clamp((rssiMax - noiseFloor), 35, rssiMax - noiseFloor);
  return (VMinMax){vMin, vMax};
}

void SP_Render(const Preset *p) {
  const VMinMax v = getV();

  if (p) {
    UI_DrawTicks(S_BOTTOM, p);
  }

  DrawHLine(0, S_BOTTOM, MAX_POINTS, C_FILL);

  for (uint8_t i = 0; i < filledPoints; ++i) {
    uint8_t yVal = ConvertDomain(rssiHistory[i], v.vMin, v.vMax, 0, SPECTRUM_H);
    DrawVLine(i, S_BOTTOM - yVal, yVal, C_FILL);
  }
}

void SP_RenderArrow(const Preset *p, uint32_t f) {
  uint8_t cx = ConvertDomain(f, p->rxF, p->txF, 0, MAX_POINTS - 1);
  DrawVLine(cx, SPECTRUM_Y, 4, C_FILL);
  FillRect(cx - 2, SPECTRUM_Y, 5, 2, C_FILL);
}

void SP_RenderRssi(uint16_t rssi, char *text, bool top) {
  const VMinMax v = getV();

  uint8_t yVal = ConvertDomain(rssi, v.vMin, v.vMax, 0, SPECTRUM_H);
  DrawHLine(0, S_BOTTOM - yVal, filledPoints, C_FILL);
  PrintSmallEx(0, S_BOTTOM - yVal + (top ? -2 : 6), POS_L, C_FILL, "%s %d",
               text, Rssi2DBm(rssi));
}

void SP_RenderLine(uint16_t rssi) {
  const VMinMax v = getV();

  uint8_t yVal = ConvertDomain(rssi, v.vMin, v.vMax, 0, SPECTRUM_H);
  DrawHLine(0, S_BOTTOM - yVal, filledPoints, C_FILL);
}

uint16_t SP_GetNoiseFloor() { return Std(rssiHistory, filledPoints); }
uint8_t SP_GetNoiseMax() { return maxNoise(noiseHistory, filledPoints); }
uint16_t SP_GetRssiMax() { return Max(rssiHistory, filledPoints); }

void SP_RenderGraph() {
  const VMinMax v = getV();

  uint8_t oVal =
      ConvertDomain(rssiHistory[0] * 2, v.vMin * 2, v.vMax * 2, 0, SPECTRUM_H);

  for (uint8_t i = 1; i < MAX_POINTS; ++i) {
    uint8_t yVal = ConvertDomain(rssiHistory[i] * 2, v.vMin * 2, v.vMax * 2, 0,
                                 SPECTRUM_H);
    DrawLine(i - 1, S_BOTTOM - oVal, i, S_BOTTOM - yVal, C_FILL);
    oVal = yVal;
  }
}

void SP_AddGraphPoint(const Loot *msm) {
  rssiHistory[MAX_POINTS - 1] = msm->rssi;
  noiseHistory[MAX_POINTS - 1] = msm->noise;
  filledPoints = MAX_POINTS;
}

void SP_Shift(int16_t n) {
  if (n == 0) {
    return;
  }
  if (n > 0) {
    while (n-- > 0) {
      for (int16_t i = MAX_POINTS - 2; i >= 0; --i) {
        rssiHistory[i + 1] = rssiHistory[i];
        noiseHistory[i + 1] = noiseHistory[i];
      }
      rssiHistory[0] = 0;
      noiseHistory[0] = UINT8_MAX;
    }
  } else {
    while (n++ < 0) {
      for (int16_t i = 0; i < MAX_POINTS - 1; ++i) {
        rssiHistory[i] = rssiHistory[i + 1];
        noiseHistory[i] = noiseHistory[i + 1];
      }
      rssiHistory[MAX_POINTS - 1] = 0;
      noiseHistory[MAX_POINTS - 1] = UINT8_MAX;
    }
  }
}
