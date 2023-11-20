#ifndef SPECTRUM_H
#define SPECTRUM_H

#include "../driver/keyboard.h"
#include "../radio.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define MOV_N 4

static const uint8_t DrawingEndY = 40;

static const uint8_t gStepSettingToIndex[] = {
    [STEP_2_5kHz] = 4,  [STEP_5_0kHz] = 5,  [STEP_6_25kHz] = 6,
    [STEP_10_0kHz] = 8, [STEP_12_5kHz] = 9, [STEP_25_0kHz] = 10,
    [STEP_8_33kHz] = 7,
};

typedef struct MovingAverage {
  uint16_t mean[128];
  uint16_t buf[MOV_N][128];
  uint16_t min, mid, max;
  uint16_t t;
} MovingAverage;

bool SPECTRUM_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void SPECTRUM_init(void);
void SPECTRUM_update(void);
void SPECTRUM_render(void);

#endif /* ifndef SPECTRUM_H */
