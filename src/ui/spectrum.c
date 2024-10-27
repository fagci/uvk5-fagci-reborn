#include "spectrum.h"
#include "../helper/measurements.h"
#include "components.h"
#include "graphics.h"

#define MAX_POINTS 128
static const uint8_t U8_MAX = 255;
static const uint16_t U16_MAX = 65535;

static uint16_t rssiHistory[MAX_POINTS] = {0};
static uint8_t noiseHistory[MAX_POINTS] = {0};
// static bool markers[MAX_POINTS] = {0};
static uint8_t x;
static uint8_t historySize;
static uint8_t filledPoints;

static uint16_t stepsCount;
static uint16_t currentStep;

static uint16_t minRssi(uint16_t *array, uint8_t n) {
  uint16_t min = U16_MAX;
  for (uint8_t i = 0; i < n; ++i) {
    if (array[i] && array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

static uint8_t maxNoise(uint8_t *array, uint8_t n) {
  uint16_t max = 0;
  for (uint8_t i = 0; i < n; ++i) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}

void SP_ResetHistory(void) {
  for (uint8_t i = 0; i < MAX_POINTS; ++i) {
    rssiHistory[i] = 0;
    noiseHistory[x] = U8_MAX;
    // markers[i] = false;
  }
  filledPoints = 0;
  currentStep = 0;
}

void SP_Begin(void) { currentStep = 0; }

void SP_Next(void) {
  if (currentStep < stepsCount - 1) {
    currentStep++;
  }
}

void SP_Init(uint16_t steps, uint8_t width) {
  stepsCount = steps;
  historySize = width;
  SP_ResetHistory();
  SP_Begin();
}

static uint8_t ox = U8_MAX;
void SP_AddPoint(Loot *msm) {
  uint8_t xs = historySize * currentStep / stepsCount;
  uint8_t xe = historySize * (currentStep + 1) / stepsCount;
  if (xe > MAX_POINTS) {
    xe = MAX_POINTS;
  }
  for (x = xs; x < xe; ++x) {
    if (ox != x) {
      ox = x;
      rssiHistory[x] = 0;
      // markers[x] = 0;
      noiseHistory[x] = U8_MAX;
    }
    if (msm->rssi > rssiHistory[x]) {
      rssiHistory[x] = msm->rssi;
    }
    if (msm->noise < noiseHistory[x]) {
      noiseHistory[x] = msm->noise;
    }
    /* if (markers[x] == false && msm->open) {
      markers[x] = msm->open;
    } */
  }
  if (x > filledPoints) {
    filledPoints = x + 1;
  }
  if (filledPoints > MAX_POINTS) {
    filledPoints = MAX_POINTS;
  }
}

typedef struct {
  uint16_t vMin;
  uint16_t vMax;
} VMinMax;

static VMinMax getV() {
  const uint16_t rssiMin = minRssi(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t noiseFloor = SP_GetNoiseFloor();
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax =
      rssiMax + Clamp((rssiMax - noiseFloor), 35, rssiMax - noiseFloor);
  return (VMinMax){vMin, vMax};
}

void SP_Render(Preset *p, uint8_t sx, uint8_t sy, uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const VMinMax v = getV();

  if (p) {
    UI_DrawTicks(sx, sx + historySize - 1, S_BOTTOM, &p->band);
  }

  DrawHLine(sx, S_BOTTOM, historySize, C_FILL);

  for (uint8_t i = 0; i < filledPoints; ++i) {
    uint8_t yVal =
        ConvertDomain(rssiHistory[i] * 2, v.vMin * 2, v.vMax * 2, 0, sh);
    DrawVLine(i, S_BOTTOM - yVal, yVal, C_FILL);
    /* if (markers[i]) {
      DrawVLine(i, S_BOTTOM + 6, 2, C_FILL);
    } */
  }
}

void SP_RenderArrow(Preset *p, uint32_t f, uint8_t sx, uint8_t sy, uint8_t sh) {
  uint8_t cx = ConvertDomain(f, p->band.bounds.start, p->band.bounds.end, sx,
                             sx + historySize - 1);
  DrawVLine(cx, sy, 4, C_FILL);
  FillRect(cx - 2, sy, 5, 2, C_FILL);
}

void SP_RenderRssi(uint16_t rssi, char *text, bool top, uint8_t sx, uint8_t sy,
                   uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const VMinMax v = getV();

  uint8_t yVal = ConvertDomain(rssi, v.vMin, v.vMax, 0, sh);
  DrawHLine(sx, S_BOTTOM - yVal, sx + filledPoints, C_FILL);
  PrintSmallEx(sx, S_BOTTOM - yVal + (top ? -2 : 6), POS_L, C_FILL, "%s %d",
               text, Rssi2DBm(rssi));
}

void SP_RenderLine(uint16_t rssi, uint8_t sx, uint8_t sy, uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const VMinMax v = getV();

  uint8_t yVal = ConvertDomain(rssi, v.vMin, v.vMax, 0, sh);
  DrawHLine(sx, S_BOTTOM - yVal, sx + filledPoints, C_FILL);
}

uint16_t SP_GetNoiseFloor() { return Std(rssiHistory, filledPoints); }
uint8_t SP_GetNoiseMax() { return maxNoise(noiseHistory, filledPoints); }
uint16_t SP_GetRssiMax() { return Max(rssiHistory, filledPoints); }

void SP_RenderGraph(uint8_t sx, uint8_t sy, uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const VMinMax v = getV();

  uint8_t oVal =
      ConvertDomain(rssiHistory[0] * 2, v.vMin * 2, v.vMax * 2, 0, sh);

  for (uint8_t i = 1; i < MAX_POINTS; ++i) {
    uint8_t yVal =
        ConvertDomain(rssiHistory[i] * 2, v.vMin * 2, v.vMax * 2, 0, sh);
    DrawLine(i - 1, S_BOTTOM - oVal, i, S_BOTTOM - yVal, C_FILL);
    oVal = yVal;
  }
}

void SP_AddGraphPoint(Loot *msm) {
  rssiHistory[MAX_POINTS - 1] = msm->rssi;
  noiseHistory[MAX_POINTS - 1] = msm->noise;
  historySize = filledPoints = MAX_POINTS;
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
      noiseHistory[0] = U8_MAX;
    }
  } else {
    while (n++ < 0) {
      for (int16_t i = 0; i < MAX_POINTS - 1; ++i) {
        rssiHistory[i] = rssiHistory[i + 1];
        noiseHistory[i] = noiseHistory[i + 1];
      }
      rssiHistory[MAX_POINTS - 1] = 0;
      noiseHistory[MAX_POINTS - 1] = U8_MAX;
    }
  }
}
