#include "apps.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "spectrum.h"

AppType_t currentApp = APP_SPECTRUM;

uint16_t rssi = 0;
uint16_t taskSpawnInterval = 10;

void TInit() {}
void TUpdate() { gRedrawScreen = true; }
void TRender() {
  char String[8];

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < TASKS_MAX; i++) {
    if (tasks[i].handler) {
      sprintf(String, "%u(%s): %u", i, tasks[i].name, tasks[i].t);
      UI_PrintStringSmallest(String, i / 8 * 64, i % 8 * 6, false, true);
    }
  }
  sprintf(String, "SPWN:%u", taskSpawnInterval);
  UI_PrintStringSmallest(String, 92, 0, false, true);

  UI_RSSIBar(rssi, 6);
}
void TKey(KEY_Code_t k, bool p, bool h) {
  gRedrawScreen = true;
  if (k != KEY_INVALID && p && !h) {
    if (k == KEY_UP) {
      taskSpawnInterval++;
      return;
    }
    if (k == KEY_DOWN) {
      taskSpawnInterval--;
      return;
    }
  }
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
