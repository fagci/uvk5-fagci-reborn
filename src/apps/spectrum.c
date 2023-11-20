/* Copyright 2023 fagci
 * https://github.com/fagci
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "spectrum.h"
#include "apps.h"
#include <string.h>

const uint16_t RSSI_MAX_VALUE = 65535;

static char String[32];

static bool monitorMode = false;
static bool redrawStatus = true;
static bool redrawScreen = false;
static bool newScanStart = true;
static bool preventKeypress = true;

static bool isTransmitting = false;

PeakInfo peak;
ScanInfo scanInfo;

SpectrumSettings settings = {
    .stepsCount = STEPS_64,
    .frequencyChangeStep = 80000,
    .rssiTriggerLevel = 150,
    .backlightState = true,
    .delayMS = 3,
};

static uint32_t currentFreq;
static uint16_t rssiHistory[128] = {0};
static bool blacklist[128] = {false};

static MovingAverage mov = {{128}, {}, 255, 128, 0, 0};

#ifdef ENABLE_ALL_REGISTERS
uint8_t hiddenMenuState = 0;
#endif

static uint16_t listenT = 0;

static uint8_t lastStepsCount = 0;

// Spectrum related

bool IsPeakOverLevel() { return peak.rssi >= settings.rssiTriggerLevel; }

static void ResetPeak() {
  peak.t = 0;
  peak.rssi = 0;
}

bool IsCenterMode() { return gCurrentVfo.step < STEP_1_0kHz; }
uint8_t GetStepsCount() { return 128 >> settings.stepsCount; }
uint16_t GetScanStep() { return StepFrequencyTable[gCurrentVfo.step]; }
uint32_t GetBW() { return GetStepsCount() * GetScanStep(); }
uint32_t GetFStart() {
  return IsCenterMode() ? currentFreq - (GetBW() >> 1) : currentFreq;
}
uint32_t GetFEnd() { return currentFreq + GetBW(); }

static void MovingCp(uint16_t *dst, uint16_t *src) {
  memcpy(dst, src, GetStepsCount() * sizeof(uint16_t));
}

static void ResetMoving() {
  for (uint8_t i = 0; i < MOV_N; ++i) {
    MovingCp(mov.buf[i], rssiHistory);
  }
}

static void MoveHistory() {
  const uint8_t XN = GetStepsCount();

  uint32_t midSum = 0;

  mov.min = RSSI_MAX_VALUE;
  mov.max = 0;

  if (lastStepsCount != XN) {
    ResetMoving();
    lastStepsCount = XN;
  }
  for (uint8_t i = MOV_N - 1; i > 0; --i) {
    MovingCp(mov.buf[i], mov.buf[i - 1]);
  }
  MovingCp(mov.buf[0], rssiHistory);

  uint8_t skipped = 0;

  for (uint8_t x = 0; x < XN; ++x) {
    if (blacklist[x]) {
      skipped++;
      continue;
    }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < MOV_N; ++i) {
      sum += mov.buf[i][x];
    }

    uint16_t pointV = mov.mean[x] = sum / MOV_N;

    midSum += pointV;

    if (pointV > mov.max) {
      mov.max = pointV;
    }

    if (pointV < mov.min) {
      mov.min = pointV;
    }
  }
  if (skipped == XN) {
    return;
  }

  mov.mid = midSum / (XN - skipped);
}

static void TuneToPeak() {
  scanInfo.f = peak.f;
  scanInfo.rssi = peak.rssi;
  scanInfo.i = peak.i;
  BK4819_TuneTo(scanInfo.f, true);
}

uint16_t GetBWRegValueForScan() { return 0b0000000110111100; }

uint16_t GetBWRegValueForListen() { return BWRegValues[gCurrentVfo.bw]; }

void GetRssiTask() {
  scanInfo.rssiSemaphore = false;
  rssiHistory[scanInfo.i] = scanInfo.rssi = BK4819_GetRSSI();
}

void GetRssi() {
  scanInfo.rssiSemaphore = true;
  BK4819_ResetRSSI();
  TaskAdd("Get RSSI", GetRssiTask, settings.delayMS, false);
  TaskSetPriority(GetRssiTask, 0);
}

static void ToggleRX(bool on) {
  RADIO_ToggleRX(on);

  if (on) {
#ifndef ENABLE_ALL_REGISTERS
    BK4819_WriteRegister(0x43, GetBWRegValueForListen());
#endif
  } else {
#ifndef ENABLE_ALL_REGISTERS
    BK4819_WriteRegister(0x43, GetBWRegValueForScan());
#endif
  }
}

// Scan info

static void ResetScanStats() {
  scanInfo.rssi = 0;
  scanInfo.rssiMax = 0;
  scanInfo.iPeak = 0;
  scanInfo.fPeak = 0;
}

static void InitScan() {
  ResetScanStats();
  scanInfo.i = 0;
  scanInfo.f = GetFStart();

  scanInfo.scanStep = GetScanStep();
  scanInfo.measurementsCount = GetStepsCount();
}

static void ResetBlacklist() { memset(blacklist, false, 128); }

static void RelaunchScan() {
  InitScan();
  ResetPeak();
  lastStepsCount = 0;
  ToggleRX(false);
  settings.rssiTriggerLevel = RSSI_MAX_VALUE;
  scanInfo.rssiMin = RSSI_MAX_VALUE;
  preventKeypress = true;
  redrawStatus = true;
}

static void UpdateScanInfo() {
  if (scanInfo.rssi > scanInfo.rssiMax) {
    scanInfo.rssiMax = scanInfo.rssi;
    scanInfo.fPeak = scanInfo.f;
    scanInfo.iPeak = scanInfo.i;
  }

  if (scanInfo.rssi < scanInfo.rssiMin) {
    scanInfo.rssiMin = scanInfo.rssi;
  }
}

static void AutoTriggerLevel() {
  if (settings.rssiTriggerLevel == RSSI_MAX_VALUE) {
    settings.rssiTriggerLevel = Clamp(scanInfo.rssiMax + 4, 0, RSSI_MAX_VALUE);
  }
}

static void UpdatePeakInfoForce() {
  peak.t = 0;
  peak.rssi = scanInfo.rssiMax;
  peak.f = scanInfo.fPeak;
  peak.i = scanInfo.iPeak;
  AutoTriggerLevel();
}

static void UpdatePeakInfo() {
  if (peak.f == 0 || peak.t >= 1024 || peak.rssi < scanInfo.rssiMax)
    UpdatePeakInfoForce();
}

static void Measure() {
  // rm harmonics using blacklist for now
#ifndef ENABLE_ALL_REGISTERS
  if (scanInfo.f % 1300000 == 0) {
    blacklist[scanInfo.i] = true;
    return;
  }
#endif
  GetRssi();
}

// Update things by keypress

static void UpdateRssiTriggerLevel(bool inc) {
  if (inc)
    settings.rssiTriggerLevel += 2;
  else
    settings.rssiTriggerLevel -= 2;
  redrawScreen = true;
}

static void UpdateScanStep(bool inc) {
  RADIO_UpdateStep(inc);
  settings.frequencyChangeStep = GetBW() >> 1;
  RelaunchScan();
  ResetBlacklist();
  redrawScreen = true;
}

static void UpdateCurrentFreq(bool inc) {
  if (inc && currentFreq < F_MAX) {
    currentFreq += settings.frequencyChangeStep;
  } else if (!inc && currentFreq > F_MIN) {
    currentFreq -= settings.frequencyChangeStep;
  } else {
    return;
  }
  RelaunchScan();
  ResetBlacklist();
  redrawScreen = true;
}

static void UpdateFreqChangeStep(bool inc) {
  uint16_t diff = GetScanStep() * 4;
  if (inc && settings.frequencyChangeStep < 1280000) {
    settings.frequencyChangeStep += diff;
  } else if (!inc && settings.frequencyChangeStep > 10000) {
    settings.frequencyChangeStep -= diff;
  }
  redrawScreen = true;
}

static void ToggleBacklight() {
  settings.backlightState = !settings.backlightState;
  BACKLIGHT_Toggle(settings.backlightState);
}

static void ToggleStepsCount() {
  if (settings.stepsCount == STEPS_128) {
    settings.stepsCount = STEPS_16;
  } else {
    --settings.stepsCount;
  }
  settings.frequencyChangeStep = GetBW() >> 1;
  RelaunchScan();
  ResetBlacklist();
  redrawScreen = true;
}

#ifndef ENABLE_ALL_REGISTERS
static void Blacklist() {
  blacklist[peak.i] = true;
  ResetPeak();
  ToggleRX(false);
  newScanStart = true;
  redrawScreen = true;
}
#endif

// Draw things

static uint8_t Rssi2Y(uint16_t rssi) {
  return DrawingEndY - ConvertDomain(rssi, mov.min - 2,
                                     mov.max + 20 + (mov.max - mov.min) / 2, 0,
                                     DrawingEndY);
}

static void DrawSpectrum() {
  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    uint8_t i = x >> settings.stepsCount;
    if (blacklist[i]) {
      continue;
    }
    uint16_t rssi = rssiHistory[i];
    DrawHLine(Rssi2Y(rssi), DrawingEndY, x, true);
  }
}

static void DrawStatus() {
#ifdef ENABLE_ALL_REGISTERS
  if (hiddenMenuState) {
    RegisterSpec s = hiddenRegisterSpecs[hiddenMenuState];
    sprintf(String, "%x %s: %u", s.num, s.name, BK4819_GetRegValue(s));
    UI_PrintStringSmallest(String, 0, 0, true, true);
  } else {
#endif

    sprintf(String, "D: %ums", settings.delayMS);
    UI_PrintStringSmallest(String, 64, 0, true, true);
#ifdef ENABLE_ALL_REGISTERS
  }
#endif
}

static void DrawNums() {
  sprintf(String, "%ux", GetStepsCount());
  UI_PrintStringSmallest(String, 0, 2, false, true);

  if (IsCenterMode()) {
    uint32_t cf = GetScreenF(currentFreq);
    sprintf(String, "%u.%05u \xB1%u.%02uk", cf / 100000, cf % 100000,
            settings.frequencyChangeStep / 100,
            settings.frequencyChangeStep % 100);
    UI_PrintStringSmallest(String, 36, 49, false, true);
  } else {
    uint32_t fs = GetScreenF(GetFStart());
    uint32_t fe = GetScreenF(GetFEnd());
    sprintf(String, "%u.%05u", fs / 100000, fs % 100000);
    UI_PrintStringSmallest(String, 0, 49, false, true);

    sprintf(String, "\xB1%uk", settings.frequencyChangeStep / 100);
    UI_PrintStringSmallest(String, 52, 49, false, true);

    sprintf(String, "%u.%05u", fe / 100000, fe % 100000);
    UI_PrintStringSmallest(String, 93, 49, false, true);
  }
}

static void DrawRssiTriggerLevel() {
  if (settings.rssiTriggerLevel == RSSI_MAX_VALUE || monitorMode)
    return;
  uint8_t y = Rssi2Y(settings.rssiTriggerLevel);
  for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
    PutPixel(x, y, 2);
  }
}

static void DrawTicks() {
  uint32_t f = GetFStart() % 100000;
  uint32_t step = GetScanStep();
  for (uint8_t x = 0; x < LCD_WIDTH;
       x += (1 << settings.stepsCount), f += step) {
    uint8_t barValue = 0b00000001;
    (f % 10000) < step && (barValue |= 0b00000010);
    (f % 50000) < step && (barValue |= 0b00000100);
    (f % 100000) < step && (barValue |= 0b00011000);

    gFrameBuffer[5][x] |= barValue;
  }

  // center
  if (IsCenterMode()) {
    gFrameBuffer[5][62] = 0x80;
    gFrameBuffer[5][63] = 0x80;
    gFrameBuffer[5][64] = 0xff;
    gFrameBuffer[5][65] = 0x80;
    gFrameBuffer[5][66] = 0x80;
  } else {
    gFrameBuffer[5][0] = 0xff;
    gFrameBuffer[5][1] = 0x80;
    gFrameBuffer[5][2] = 0x80;
    gFrameBuffer[5][3] = 0x80;
    gFrameBuffer[5][124] = 0x80;
    gFrameBuffer[5][125] = 0x80;
    gFrameBuffer[5][126] = 0x80;
    gFrameBuffer[5][127] = 0xff;
  }
}

static void DrawArrow(uint8_t x) {
  for (signed i = -2; i <= 2; ++i) {
    signed v = x + i;
    uint8_t a = i > 0 ? i : -i;
    if (!(v & LCD_WIDTH)) {
      gFrameBuffer[5][v] |= (0b01111000 << a) & 0b01111000;
    }
  }
}

static void RenderStatus() {
  memset(gStatusLine, 0, LCD_WIDTH - 13);
  DrawStatus();
}

static void RenderSpectrum() {
  DrawTicks();
  DrawArrow(peak.i << settings.stepsCount);
  DrawSpectrum();
  DrawRssiTriggerLevel();
  UI_FSmall(GetScreenF(peak.f));
  DrawNums();
}

static void Render() {
  UI_ClearScreen();
  RenderSpectrum();
}

bool SPECTRUM_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (preventKeypress || bKeyPressed || bKeyHeld) {
    return false;
  }
  switch (key) {
  case KEY_3:
    settings.delayMS++;
    redrawStatus = true;
    return true;
  case KEY_9:
    settings.delayMS--;
    redrawStatus = true;
    return true;
  case KEY_1:
    UpdateScanStep(true);
    return true;
  case KEY_7:
    UpdateScanStep(false);
    return true;
  case KEY_2:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      if (hiddenMenuState <= 1) {
        hiddenMenuState = ARRAY_SIZE(hiddenRegisterSpecs) - 1;
      } else {
        hiddenMenuState--;
      }
      redrawStatus = true;
      return true;
    }
#endif
    UpdateFreqChangeStep(true);
    return true;
  case KEY_8:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      if (hiddenMenuState == ARRAY_SIZE(hiddenRegisterSpecs) - 1) {
        hiddenMenuState = 1;
      } else {
        hiddenMenuState++;
      }
      redrawStatus = true;
      return true;
    }
#endif
    UpdateFreqChangeStep(false);
    return true;
  case KEY_UP:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], true);
      redrawStatus = true;
      return true;
    }
#endif
    UpdateCurrentFreq(true);
    return true;
  case KEY_DOWN:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], false);
      redrawStatus = true;
      return true;
    }
#endif
    UpdateCurrentFreq(false);
    return true;
  case KEY_SIDE1:
#ifdef ENABLE_ALL_REGISTERS
    if (settings.rssiTriggerLevel != RSSI_MAX_VALUE - 1) {
      settings.rssiTriggerLevel = RSSI_MAX_VALUE - 1;
    } else {
      settings.rssiTriggerLevel = RSSI_MAX_VALUE;
    }
    redrawScreen = true;
#else
    Blacklist();
#endif
    return true;
  case KEY_STAR:
    UpdateRssiTriggerLevel(true);
    return true;
  case KEY_F:
    UpdateRssiTriggerLevel(false);
    return true;
  case KEY_5:
    APPS_run(APP_FINPUT);
    return true;
  case KEY_0:
    RADIO_ToggleModulation();
    return true;
  case KEY_6:
    RADIO_ToggleListeningBW();
    return true;
  case KEY_4:
    ToggleStepsCount();
    return true;
  case KEY_SIDE2:
    ToggleBacklight();
    return true;
  case KEY_PTT:
    APPS_run(APP_STILL);
    TuneToPeak();
    settings.rssiTriggerLevel = 120;
    return true;
  case KEY_MENU:
#ifdef ENABLE_ALL_REGISTERS
    hiddenMenuState = 1;
    redrawStatus = true;
#endif
    return true;
  case KEY_EXIT:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      hiddenMenuState = 0;
      redrawStatus = true;
      return true;
    }
#endif
    return true;
  default:
    break;
  }
  return false;
}

static void Scan() {
  if (blacklist[scanInfo.i]) {
    return;
  }
  BK4819_TuneTo(scanInfo.f, true);
  Measure();
  UpdateScanInfo();
}

static void NextScanStep() {
  ++peak.t;
  ++scanInfo.i;
  scanInfo.f += scanInfo.scanStep;
}

static void UpdateScan() {
  if (scanInfo.rssiSemaphore) {
    return;
  }
  Scan();

  if (scanInfo.i < scanInfo.measurementsCount) {
    NextScanStep();
    return;
  }

  MoveHistory();

  redrawScreen = true;
  preventKeypress = false;

  UpdatePeakInfo();
  if (IsPeakOverLevel()) {
    ToggleRX(true);
    TuneToPeak();
    return;
  }

  newScanStart = true;
}

static void UpdateListening() {
  if (!gIsListening) {
    ToggleRX(true);
  }
  if (listenT) {
    listenT--;
    return;
  }

  redrawScreen = true;

#ifndef ENABLE_ALL_REGISTERS
  BK4819_WriteRegister(0x43, GetBWRegValueForScan());
#endif
  Measure();
#ifndef ENABLE_ALL_REGISTERS
  BK4819_WriteRegister(0x43, GetBWRegValueForListen());
#endif

  peak.rssi = scanInfo.rssi;

  MoveHistory();

  if (IsPeakOverLevel() || monitorMode) {
    listenT = 1000;
    return;
  }

  ToggleRX(false);
  newScanStart = true;
}

static void UpdateTransmitting() {}

void SPECTRUM_update() {
  if (newScanStart) {
    InitScan();
    newScanStart = false;
  }
  if (isTransmitting) {
    UpdateTransmitting();
  } else if (gIsListening) {
    UpdateListening();
  } else {
    UpdateScan();
  }
}

void SPECTRUM_render() {
  if (redrawStatus) {
    RenderStatus();
    redrawStatus = false;
    gRedrawStatus = true;
  }
  if (redrawScreen) {
    Render();
    redrawScreen = false;
    gRedrawScreen = true;
  }
}

void SPECTRUM_init() {
  currentFreq = gCurrentVfo.fRX;

  redrawStatus = true;
  redrawScreen = true;
  newScanStart = true;

  ToggleRX(true), ToggleRX(false); // hack to prevent noise when squelch off

  RelaunchScan();

  memset(rssiHistory, 0, 128);
}
