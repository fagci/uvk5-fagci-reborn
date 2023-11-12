#include "apps.h"
#include "../scheduler.h"
#include "spectrum.h"

AppType_t currentApp = APP_SPECTRUM;

void TInit() {}
void TUpdate() { gRedrawScreen = true; }
void TRender() {
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
  char String[8];
  /* sprintf(String, "%u", var);
  UI_PrintStringSmallest(String, 0, 0, false, true); */

  for (uint8_t i = 0; i < TASKS_MAX; i++) {
    if (tasks[i].handler) {
      sprintf(String, "%u(%s): %u", i, tasks[i].name, tasks[i].t);
      UI_PrintStringSmallest(String, i / 8 * 64, i % 8 * 6, false, true);
    }
  }
#include "../driver/backlight.h"
  sprintf(String, "%u %u %u", countdown, duration, state);
  UI_PrintStringSmallest(String, 0, 48, false, true);
}
void TKey(KEY_Code_t k, bool p, bool h) {
  gRedrawScreen = true;
  // SYSTEM_DelayMs(1000);
}

const App apps[5] = {
    {"Test", TInit, TUpdate, TRender, TKey},
    {"Spectrum", SPECTRUM_init, SPECTRUM_update, SPECTRUM_render, SPECTRUM_key},
    /* {"Scanlist", NULL, SCANLIST_update, SCANLIST_render, SCANLIST_key},
    {"A to B scanner", ABSCANNER_init, ABSCANNER_update, ABSCANNER_render,
     ABSCANNER_key}, */
};

void APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (apps[currentApp].key) {
    apps[currentApp].key(Key, bKeyPressed, bKeyHeld);
  }
}
void APPS_init(void) {
  if (apps[currentApp].init) {
    apps[currentApp].init();
  }
}
void APPS_update(void) {
  if (apps[currentApp].update) {
    apps[currentApp].update();
  }
}
void APPS_render(void) {
  if (apps[currentApp].render) {
    apps[currentApp].render();
  }
  ST7565_Render();
}
void APPS_run(AppType_t app) {
  currentApp = app;
  APPS_init();
}
