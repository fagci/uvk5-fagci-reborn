#include "spectrum.h"
#include "../helper/measurements.h"
#include "components.h"
#include "graphics.h"

#define MAX_POINTS 128

static uint16_t rssiHistory[MAX_POINTS] = {0};
static bool markers[MAX_POINTS] = {0};
static uint8_t x;
static uint8_t historySize;
static uint8_t filledPoints;

static uint16_t stepsCount;
static uint16_t currentStep;
static uint8_t exLen;

static uint16_t ceilDiv(uint16_t a, uint16_t b) { return (a + b - 1) / b; }

void SP_ResetHistory() {
  for (uint8_t i = 0; i < MAX_POINTS; ++i) {
    rssiHistory[i] = 0;
    markers[i] = false;
  }
  filledPoints = 0;
  currentStep = 0;
}

void SP_Begin() { currentStep = 0; }

void SP_Next() {
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
  if (exLen) {
    for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
      x = historySize * currentStep / stepsCount + exIndex;
      rssiHistory[x] = msm->rssi;
      markers[x] = msm->open;
    }
  } else {
    x = historySize * currentStep / stepsCount;
    if (msm->rssi > rssiHistory[x]) {
      rssiHistory[x] = msm->rssi;
    }
    if (markers[x] == false && msm->open) {
      markers[x] = true;
    }
  }
  if (x > filledPoints && x < historySize) {
    filledPoints = x;
  }
}

void SP_ResetPoint() {
  for (uint8_t exIndex = 0; exIndex < exLen; ++exIndex) {
    uint8_t lx = historySize * currentStep / stepsCount + exIndex;
    rssiHistory[lx] = 0;
    markers[lx] = false;
  }
}

void SP_Render(FRange *p, uint8_t sx, uint8_t sy, uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const uint16_t rssiMin = Min(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  if (p) {
    UI_DrawTicks(sx, sx + historySize - 1, S_BOTTOM, p);
  }

  DrawHLine(sx, S_BOTTOM, historySize, C_FILL);

  for (uint8_t i = 0; i < filledPoints; ++i) {
    uint8_t yVal = ConvertDomain(rssiHistory[i], vMin, vMax, 0, sh);
    DrawVLine(i, S_BOTTOM - yVal, yVal, C_FILL);
    if (markers[i]) {
      DrawVLine(i, S_BOTTOM + 6, 2, C_FILL);
    }
  }
}

void SP_RenderRssi(uint16_t rssi, char *text, bool top, uint8_t sx, uint8_t sy,
                   uint8_t sh) {
  const uint8_t S_BOTTOM = sy + sh;
  const uint16_t rssiMin = Min(rssiHistory, filledPoints);
  const uint16_t rssiMax = Max(rssiHistory, filledPoints);
  const uint16_t vMin = rssiMin - 2;
  const uint16_t vMax = rssiMax + 20 + (rssiMax - rssiMin) / 2;

  uint8_t yVal = ConvertDomain(rssi, vMin, vMax, 0, sh);
  DrawHLine(sx, S_BOTTOM - yVal, sx + filledPoints, C_FILL);
  PrintSmallEx(sx, S_BOTTOM - yVal + (top ? -2 : 6), POS_L, C_FILL, "%s %u %d",
               text, rssi, Rssi2DBm(rssi));
}
