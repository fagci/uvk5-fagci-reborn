#include "antenna.h"
#include "../misc.h"
#include "../radio.h"
#include "../ui/graphics.h"
#include "apps.h"
#include "finput.h"

static uint32_t f = 0;
static uint16_t CHINEESE_BNC[7] = {105, 190, 276, 360, 443, 525, 610};

static void setF(uint32_t nf) {
  f = nf;
  gRedrawScreen = true;
}

void ANTENNA_init() {
  RADIO_LoadCurrentCH();
  f = GetScreenF(radio->f);
}

void ANTENNA_update() {}

bool ANTENNA_key(KEY_Code_t key, bool bKeyPressed, bool bKeyHeld) {
  if (!bKeyPressed && !bKeyHeld) {
    switch (key) {
    case KEY_5:
      gFInputCallback = setF;
      APPS_run(APP_FINPUT);
      return true;
    case KEY_EXIT:
      APPS_exit();
      return true;
    default:
      break;
    }
  }
  return false;
}

/**
 * @return uint8_t segmentsCount * 10 + rest(1/10)
 */
uint8_t calcLen(uint16_t *ant, uint8_t n, uint16_t lenMm) {
  uint16_t prevSegmentSizeMm = 0;

  for (uint8_t i = 0; i < n; ++i) {
    uint16_t segmentSizeMm = ant[i];
    if (segmentSizeMm >= lenMm) {
      uint16_t full10 = i * 10;
      uint16_t restSegMm = lenMm - prevSegmentSizeMm;
      uint16_t lastSegMm = segmentSizeMm - prevSegmentSizeMm;
      uint16_t part = restSegMm * 10 / lastSegMm;
      return full10 + part;
    }
    prevSegmentSizeMm = segmentSizeMm;
  }
  return 255;
}

void ANTENNA_render() {
  uint32_t lambda = 29979246 / (f / 100);
  uint16_t quarterCm = lambda / 4;
  uint16_t segmentsCount =
      calcLen(CHINEESE_BNC, ARRAY_SIZE(CHINEESE_BNC), quarterCm * 10);

  UI_ClearScreen();
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER - 9 * 2, POS_C, C_FILL,
                    "f=%u.%05uMHz", f / 100000, f % 100000);
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER - 9, POS_C, C_FILL, "L=%u.%02um",
                    lambda / 100, lambda % 100);
  PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER, POS_C, C_FILL, "1/4=%ucm",
                    quarterCm);

  if (segmentsCount == 0) {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER + 11, POS_C, C_FILL,
                      "Too short seg");
  } else if (segmentsCount == 255) {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER + 11, POS_C, C_FILL,
                      "Too short ant");
  } else {
    PrintMediumBoldEx(LCD_XCENTER, LCD_YCENTER + 11, POS_C, C_FILL,
                      "%u + %u/10 segm", segmentsCount / 10,
                      segmentsCount % 10);
  }
}

void ANTENNA_deinit() {}


static App meta = {
    .id = APP_ANT,
    .name = "ANTENNA",
    .runnable = true,
    .init = ANTENNA_init,
    .update = ANTENNA_update,
    .render = ANTENNA_render,
    .key = ANTENNA_key,
    .deinit = ANTENNA_deinit,
};

App *ANTENNA_Meta() { return &meta; }
