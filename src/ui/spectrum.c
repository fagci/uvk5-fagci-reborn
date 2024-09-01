#include "spectrum.h"
#include "../helper/measurements.h"
#include "components.h"
#include "graphics.h"

#define MAX_POINTS 128
static const uint8_t U8_MAX = 255;
static const uint16_t U16_MAX = 65535;

static uint16_t rssiHistory[MAX_POINTS] = {0};
static uint16_t noiseHistory[MAX_POINTS] = {0};
static bool markers[MAX_POINTS] = {0};
static uint8_t x;
static uint8_t historySize;
static uint8_t filledPoints;

static uint16_t stepsCount;
static uint16_t currentStep;
static uint8_t exLen;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

static uint16_t minRssi(uint16_t *array, uint8_t n) {
  uint16_t min = U16_MAX;
  for (uint8_t i = 0; i < n; ++i) {
    if (array[i] && array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

void SP_ResetHistory(void) {
  for (uint8_t i = 0; i < MAX_POINTS; ++i) {
    rssiHistory[i] = 0;
    noiseHistory[x] = U8_MAX;
    markers[i] = false;
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
  exLen = ceilDiv(historySize, stepsCount);
  SP_ResetHistory();
  SP_Begin();
}

void SP_AddPoint(Loot *msm) {
  uint8_t ox = U8_MAX;
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    x = historySize * currentStep / stepsCount + exIndex;
    if (ox != x) {
      rssiHistory[x] = markers[x] = 0;
      noiseHistory[x] = U8_MAX;
      ox = x;
    }
    if (msm->rssi > rssiHistory[x]) {
      rssiHistory[x] = msm->rssi;
    }
    if (msm->noise < noiseHistory[x]) {
      noiseHistory[x] = msm->noise;
    }
    if (markers[x] == false && msm->open) {
      markers[x] = msm->open;
    }
  }
  if (x > filledPoints && x < historySize) {
    filledPoints = x + 1;
  }
}

void SP_Render(Preset *p, uint8_t sx, uint8_t sy, uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const uint16_t rssiMin = minRssi(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax =
      rssiMax + Clamp((rssiMax - rssiMin), 15, rssiMax - rssiMin);

  if (p) {
    UI_DrawTicks(sx, sx + historySize - 1, S_BOTTOM, &p->band);
  }

  DrawHLine(sx, S_BOTTOM, historySize, C_FILL);

  for (uint8_t i = 0; i < filledPoints; ++i) {
    uint8_t yVal = ConvertDomain(rssiHistory[i] * 2, vMin * 2, vMax * 2, 0, sh);
    DrawVLine(i, S_BOTTOM - yVal, yVal, C_FILL);
    if (markers[i]) {
      DrawVLine(i, S_BOTTOM + 6, 2, C_FILL);
    }
  }
}

uint32_t ClampF(uint32_t v, uint32_t min, uint32_t max) {
  return v <= min ? min : (v >= max ? max : v);
}

static uint32_t ConvertDomainF(uint32_t aValue, uint32_t aMin, uint32_t aMax,
                               uint32_t bMin, uint32_t bMax) {
  const uint32_t aRange = aMax - aMin;
  const uint32_t bRange = bMax - bMin;
  aValue = ClampF(aValue, aMin, aMax);
  return ((aValue - aMin) * bRange + aRange / 2) / aRange + bMin;
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
  const uint16_t rssiMin = Min(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  uint8_t yVal = ConvertDomainF(rssi, vMin, vMax, 0, sh);
  DrawHLine(sx, S_BOTTOM - yVal, sx + filledPoints, C_FILL);
  PrintSmallEx(sx, S_BOTTOM - yVal + (top ? -2 : 6), POS_L, C_FILL, "%s %u %d",
               text, rssi, Rssi2DBm(rssi));
}

uint16_t SP_GetNoiseFloor() { return Std(rssiHistory, filledPoints); }
uint16_t SP_GetNoiseMax() { return Max(noiseHistory, filledPoints); }
