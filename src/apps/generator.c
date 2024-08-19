#include "generator.h"
#include "../driver/bk4819.h"
#include "../helper/measurements.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static const char *TYPES[] = {"Sine", "Square"};
static uint16_t tone1Freq = 1000;
static uint16_t tone2Freq = 0;
static uint8_t toneType = 0;
static uint8_t power = 10;

static void setTone1Freq(uint32_t f) { tone1Freq = f / 100; }
static void setTone2Freq(uint32_t f) { tone2Freq = f / 100; }

void GENERATOR_init() {}
void GENERATOR_update() {}
bool GENERATOR_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    RADIO_ToggleTXEX(bKeyHeld, RADIO_GetTXF(), power);
    if (bKeyHeld && gTxState == TX_ON) {
      BK4819_EnterDTMF_TX(true);
      if (tone1Freq == 0 || tone2Freq == 0) {
        BK4819_SetToneFrequency(tone1Freq | tone2Freq);
        BK4819_SetTone2Frequency(tone1Freq | tone2Freq);
      } else {
        BK4819_SetToneFrequency(tone1Freq);
        BK4819_SetTone2Frequency(tone2Freq);
      }
      BK4819_ExitTxMute();
    }

    return true;
  }
  // Simple keypress
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_5:
      gFInputCallback = RADIO_TuneTo;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_SIDE1:
      gFInputCallback = setTone1Freq;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_SIDE2:
      gFInputCallback = setTone2Freq;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  // up-down keys
  if (bKeyPressed || (!bKeyPressed && !bKeyHeld)) {
    bool isSsb = RADIO_IsSSB();
    switch (key) {
    case KEY_UP:
      RADIO_NextFreqNoClicks(true);
      return true;
    case KEY_DOWN:
      RADIO_NextFreqNoClicks(false);
      return true;
    case KEY_2:
      if (power < 255) {
        power++;
      }
      return true;
    case KEY_8:
      if (power > 0) {
        power--;
      }
      return true;
    default:
      break;
    }
  }
  return false;
}
void GENERATOR_render() {
  UI_ClearScreen();
  uint32_t txf = RADIO_GetTXF();
  PrintMediumEx(LCD_XCENTER, 15, POS_C, C_FILL, "%u.%05u", txf / 100000,
                txf % 100000);
  PrintMediumEx(LCD_XCENTER, 15 + 12, POS_C, C_FILL, "Tone1: %uHz", tone1Freq);
  PrintMediumEx(LCD_XCENTER, 15 + 20, POS_C, C_FILL, "Tone2: %uHz", tone2Freq);
  PrintMediumEx(LCD_XCENTER, 15 + 28, POS_C, C_FILL, "Power: %u%s", power,
                power < 0x91 ? "" : "!!!");
}
