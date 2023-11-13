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

static uint32_t initialFreq;
static char String[32];

bool monitorMode = false;
bool redrawStatus = true;
bool redrawScreen = false;
bool newScanStart = true;
bool preventKeypress = true;

bool isListening = false;
bool isTransmitting = false;

PeakInfo peak;
ScanInfo scanInfo;

SpectrumSettings settings = {
    .stepsCount = STEPS_64,
    .scanStepIndex = STEP_25_0kHz,
    .frequencyChangeStep = 80000,
    .rssiTriggerLevel = 150,
    .backlightState = true,
    .listenBw = BK4819_FILTER_BW_WIDE,
    .modulationType = MOD_FM,
    .delayMS = 3,
};

uint32_t fMeasure = 0;
uint32_t fTx = 0;
uint32_t currentFreq;
uint16_t rssiHistory[128] = {0};
bool blacklist[128] = {false};

static uint16_t registersBackup[128];
static const uint8_t registersToBackup[] = {
    0x13, 0x30, 0x31, 0x37, 0x3D, 0x40, 0x43, 0x47, 0x48, 0x7D, 0x7E,
};

static MovingAverage mov = {{128}, {}, 255, 128, 0, 0};
static const uint8_t MOV_N = ARRAY_SIZE(mov.buf);

#ifdef ENABLE_ALL_REGISTERS
uint8_t hiddenMenuState = 0;
#endif

uint16_t listenT = 0;

bool isMovingInitialized = false;
uint8_t lastStepsCount = 0;

VfoState_t txAllowState;

// Utility functions

KEY_Code_t GetKey() {
  KEY_Code_t btn = KEYBOARD_Poll();
  if (btn == KEY_INVALID && !GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT)) {
    btn = KEY_PTT;
  }
  return btn;
}

// Radio functions

static void SetF(uint32_t f, bool precise) {
  BK4819_TuneTo(fMeasure = f, precise);
}
// static void SetTxF(uint32_t f) { BK4819_TuneTo(fTx = f, true); }

// Spectrum related

bool IsPeakOverLevel() { return peak.rssi >= settings.rssiTriggerLevel; }

static void ResetPeak() {
  peak.t = 0;
  peak.rssi = 0;
}

bool IsCenterMode() { return settings.scanStepIndex < STEP_1_0kHz; }
uint8_t GetStepsCount() { return 128 >> settings.stepsCount; }
uint16_t GetScanStep() { return StepFrequencyTable[settings.scanStepIndex]; }
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
  SetF(scanInfo.f, true);
}

uint16_t GetBWRegValueForScan() { return 0b0000000110111100; }

uint16_t GetBWRegValueForListen() { return BWRegValues[settings.listenBw]; }

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

// static void ToggleTX(bool);

/* uint16_t registersVault[128] = {0};

static void RegBackupSet(uint8_t num, uint16_t value) {
  registersVault[num] = BK4819_ReadRegister(num);
  BK4819_WriteRegister(num, value);
}

static void RegRestore(uint8_t num) {
  BK4819_WriteRegister(num, registersVault[num]);
} */

/* static void ToggleTX(bool on) {
  if (isTransmitting == on) {
    return;
  }
  isTransmitting = on;
  if (on) {
    ToggleRX(false);
  }

  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, on);

  if (on) {
    ToggleAudio(false);

    SetTxF(fMeasure);

    RegBackupSet(BK4819_REG_47, 0x6040);
    RegBackupSet(BK4819_REG_7E, 0x302E);
    RegBackupSet(BK4819_REG_50, 0x3B20);
    RegBackupSet(BK4819_REG_37, 0x1D0F);
    RegBackupSet(BK4819_REG_52, 0x028F);
    RegBackupSet(BK4819_REG_30, 0x0000);
    BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
    RegBackupSet(BK4819_REG_51, 0x0000);

    BK4819_SetupPowerAmplifier(fTx, gCurrentVfo->TXP_CalculatedSetting);
  } else {
    // RADIO_SendEndOfTransmission();

    BK4819_SetupPowerAmplifier(0, 0);

    RegRestore(BK4819_REG_51);
    BK4819_WriteRegister(BK4819_REG_30, 0);
    RegRestore(BK4819_REG_30);
    RegRestore(BK4819_REG_52);
    RegRestore(BK4819_REG_37);
    RegRestore(BK4819_REG_50);
    RegRestore(BK4819_REG_7E);
    RegRestore(BK4819_REG_47);

    SetF(fMeasure, true);
  }
  BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, !on);
  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, on);
} */

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
  // SYSTEM_DelayMs(10);
}

static void ApplyPreset(FreqPreset p) {
  currentFreq = GetTuneF(p.fStart);
  settings.scanStepIndex = p.stepSizeIndex;
  settings.listenBw = p.listenBW;
  settings.modulationType = p.modulationType;
  settings.stepsCount = p.stepsCountIndex;
  BK4819_SetModulation(settings.modulationType);
  RelaunchScan();
  ResetBlacklist();
  redrawScreen = true;
  settings.frequencyChangeStep = GetBW();
}

static void SelectNearestPreset(bool inc) {
  FreqPreset p;
  uint32_t f = GetScreenF(currentFreq);
  const uint8_t SZ = ARRAY_SIZE(freqPresets);
  if (inc) {
    for (uint8_t i = 0; i < SZ; ++i) {
      p = freqPresets[i];
      if (f < p.fStart) {
        ApplyPreset(p);
        return;
      }
    }
  } else {
    for (int i = SZ - 1; i >= 0; --i) {
      p = freqPresets[i];
      if (f > p.fEnd) {
        ApplyPreset(p);
        return;
      }
    }
  }
  ApplyPreset(p);
}

static void UpdateScanStep(bool inc) {
  if (inc && settings.scanStepIndex < STEP_100_0kHz) {
    ++settings.scanStepIndex;
  } else if (!inc && settings.scanStepIndex > 0) {
    --settings.scanStepIndex;
  } else {
    return;
  }
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
    const FreqPreset *p = NULL;
    uint32_t f = GetScreenF(currentFreq);
    for (uint8_t i = 0; i < ARRAY_SIZE(freqPresets); ++i) {
      if (f >= freqPresets[i].fStart && f < freqPresets[i].fEnd) {
        p = &freqPresets[i];
      }
    }
    if (p != NULL) {
      UI_PrintStringSmallest(p->name, 0, 0, true, true);
    }

    sprintf(String, "D: %ums", settings.delayMS);
    UI_PrintStringSmallest(String, 64, 0, true, true);
#ifdef ENABLE_ALL_REGISTERS
  }
#endif
}

static void DrawF(uint32_t f) {
  UI_PrintStringSmallest(modulationTypeOptions[settings.modulationType], 116, 2,
                         false, true);
  UI_PrintStringSmallest(bwNames[settings.listenBw], 108, 8, false, true);

#ifdef ENABLE_ALL_REGISTERS
  if (currentState == SPECTRUM) {
    sprintf(String, "R%03u S%03u A%03u", scanInfo.rssi,
            BK4819_GetRegValue((RegisterSpec){"snr_out", 0x61, 8, 0xFF, 1}),
            BK4819_GetRegValue((RegisterSpec){"agc_rssi", 0x62, 8, 0xFF, 1}));
    UI_PrintStringSmallest(String, 36, 8, false, true);
  }
#endif

  sprintf(String, "%u.%05u", f / 100000, f % 100000);

  /* if (currentState == STILL && kbd.current == KEY_PTT) {
    if (txAllowState) {
      sprintf(String, vfoStateNames[txAllowState]);
    } else if (isTransmitting) {
      f = GetOffsetedF(gCurrentVfo, f);
      sprintf(String, "TX %u.%05u", f / 100000, f % 100000);
    }
  } */
  UI_PrintStringSmall(String, 8, 127, 0);
}

static void DrawNums() {
  sprintf(String, "%ux", GetStepsCount());
  UI_PrintStringSmallest(String, 0, 2, false, true);
  sprintf(String, "%u.%02uk", GetScanStep() / 100, GetScanStep() % 100);
  UI_PrintStringSmallest(String, 0, 8, false, true);

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

static void OnKeyDown(uint8_t key) {
  switch (key) {
  case KEY_3:
    if (0)
      SelectNearestPreset(true);
    settings.delayMS++;
    // SYSTEM_DelayMs(100);
    redrawStatus = true;
    break;
  case KEY_9:
    if (0)
      SelectNearestPreset(false);
    settings.delayMS--;
    redrawStatus = true;
    break;
  case KEY_1:
    UpdateScanStep(true);
    break;
  case KEY_7:
    UpdateScanStep(false);
    break;
  case KEY_2:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      if (hiddenMenuState <= 1) {
        hiddenMenuState = ARRAY_SIZE(hiddenRegisterSpecs) - 1;
      } else {
        hiddenMenuState--;
      }
      redrawStatus = true;
      break;
    }
#endif
    UpdateFreqChangeStep(true);
    break;
  case KEY_8:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      if (hiddenMenuState == ARRAY_SIZE(hiddenRegisterSpecs) - 1) {
        hiddenMenuState = 1;
      } else {
        hiddenMenuState++;
      }
      redrawStatus = true;
      break;
    }
#endif
    UpdateFreqChangeStep(false);
    break;
  case KEY_UP:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], true);
      redrawStatus = true;
      break;
    }
#endif
    UpdateCurrentFreq(true);
    break;
  case KEY_DOWN:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      UpdateRegMenuValue(hiddenRegisterSpecs[hiddenMenuState], false);
      redrawStatus = true;
      break;
    }
#endif
    UpdateCurrentFreq(false);
    break;
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
    break;
  case KEY_STAR:
    UpdateRssiTriggerLevel(true);
    break;
  case KEY_F:
    UpdateRssiTriggerLevel(false);
    break;
  case KEY_5:
    APPS_run(APP_FINPUT);
    break;
  case KEY_0:
    ToggleModulation();
    break;
  case KEY_6:
    ToggleListeningBW();
    break;
  case KEY_4:
    ToggleStepsCount();
    break;
  case KEY_SIDE2:
    ToggleBacklight();
    break;
  case KEY_PTT:
    APPS_run(APP_STILL);
    TuneToPeak();
    settings.rssiTriggerLevel = 120;
    break;
  case KEY_MENU:
#ifdef ENABLE_ALL_REGISTERS
    hiddenMenuState = 1;
    redrawStatus = true;
#endif
    break;
  case KEY_EXIT:
#ifdef ENABLE_ALL_REGISTERS
    if (hiddenMenuState) {
      hiddenMenuState = 0;
      redrawStatus = true;
      break;
    }
#endif
    break;
  default:
    break;
  }
}

/* static void OnKeysReleased() {
  if (isTransmitting) {
    ToggleTX(false);
  }
} */

static void RenderStatus() {
  memset(gStatusLine, 0, LCD_WIDTH - 13);
  DrawStatus();
}

static void RenderSpectrum() {
  DrawTicks();
  DrawArrow(peak.i << settings.stepsCount);
  DrawSpectrum();
  DrawRssiTriggerLevel();
  DrawF(GetScreenF(peak.f));
  DrawNums();
}

static void Render() {
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  RenderSpectrum();
}

void SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (preventKeypress)
    return;

  if (!bKeyPressed || bKeyHeld)
    return;

  OnKeyDown(Key);
}

static void Scan() {
  if (blacklist[scanInfo.i]) {
    return;
  }
  SetF(scanInfo.f, true);
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
  if (!isListening) {
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
  } else if (isListening) {
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

static void AutomaticPresetChoose(uint32_t f) {
  f = GetScreenF(f);
  for (uint8_t i = 0; i < ARRAY_SIZE(freqPresets); ++i) {
    const FreqPreset *p = &freqPresets[i];
    if (f >= p->fStart && f <= p->fEnd) {
      ApplyPreset(*p);
    }
  }
}

void SPECTRUM_init() {
  initialFreq = 43400000;
  currentFreq = initialFreq;

  settings.scanStepIndex = STEP_25_0kHz;
  settings.listenBw = BK4819_FILTER_BW_WIDE;
  settings.modulationType = MOD_FM;

  AutomaticPresetChoose(currentFreq);

  redrawStatus = true;
  redrawScreen = true;
  newScanStart = true;

  ToggleRX(true), ToggleRX(false); // hack to prevent noise when squelch off
  BK4819_SetModulation(settings.modulationType);

  RelaunchScan();

  memset(rssiHistory, 0, 128);
}
