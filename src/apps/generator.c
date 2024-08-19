#include "generator.h"
#include "../driver/bk4819.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static const char *TYPES[] = {"Sine", "Square"};
uint16_t tone1Freq = 1000;
uint16_t tone2Freq = 0;
uint8_t toneType = 0;

static void setTone1Freq(uint32_t f) { tone1Freq = f / 100; }
static void setTone2Freq(uint32_t f) { tone2Freq = f / 100; }

void GENERATOR_init() {}
void GENERATOR_update() {}
bool GENERATOR_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (key == KEY_PTT) {
    if (!bKeyPressed) {
      BK4819_ExitDTMF_TX(false);
    }
    RADIO_ToggleTX(bKeyHeld);
    if (bKeyHeld) {
      /* BK4819_EnterTxMute();
      BK4819_SetAF(BK4819_AF_BEEP);

      BK4819_WriteRegister(BK4819_REG_70,
                           BK4819_REG_70_ENABLE_TONE1 |
                               (66u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));

      BK4819_EnableTXLink(); */
      BK4819_EnterDTMF_TX(true);

      BK4819_SetToneFrequency(tone1Freq);
      BK4819_SetTone2Frequency(tone2Freq);
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
}
