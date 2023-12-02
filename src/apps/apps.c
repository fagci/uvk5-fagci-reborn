#include "apps.h"
#include "../driver/st7565.h"
#include "../ui/helper.h"
#include "finput.h"
#include "mainmenu.h"
#include "reset.h"
#include "savech.h"
#include "settings.h"
#include "spectrumreborn.h"
#include "still.h"
#include "test.h"
#include "textinput.h"
#include "vfocfg.h"

#define APPS_STACK_SIZE 8

AppType_t gCurrentApp = APP_NONE;

static AppType_t appsStack[APPS_STACK_SIZE] = {0};
static int8_t stackIndex = -1;

static bool pushApp(AppType_t app) {
  if (stackIndex < APPS_STACK_SIZE) {
    appsStack[++stackIndex] = app;
    return true;
  }
  return false;
}

static AppType_t popApp() {
  if (stackIndex > 0) {
    return appsStack[stackIndex--]; // Do not care about existing value
  }
  return appsStack[stackIndex];
}

static AppType_t peek() {
  if (stackIndex >= 0) {
    return appsStack[stackIndex];
  }
  return APP_NONE;
}

const App apps[APPS_COUNT] = {
    {"None"},
    {"Test", TEST_Init, TEST_Update, TEST_Render, TEST_key},
    {"Spectrum", SPECTRUM_init, SPECTRUM_update, SPECTRUM_render, SPECTRUM_key},
    {"Still", STILL_init, NULL, STILL_render, STILL_key, STILL_deinit},
    {"Frequency input", FINPUT_init, NULL, FINPUT_render, FINPUT_key,
     FINPUT_deinit},
    {"Main menu", MAINMENU_init, NULL, MAINMENU_render, MAINMENU_key},
    {"Reset", RESET_Init, RESET_Update, RESET_Render, RESET_key},
    {"Text input", TEXTINPUT_init, TEXTINPUT_update, TEXTINPUT_render,
     TEXTINPUT_key, TEXTINPUT_deinit},
    {"VFO config", VFOCFG_init, VFOCFG_update, VFOCFG_render, VFOCFG_key},
    {"Save to channel", SAVECH_init, SAVECH_update, SAVECH_render, SAVECH_key},
    {"Settings", SETTINGS_init, SETTINGS_update, SETTINGS_render, SETTINGS_key},
    // {"Scanlist", NULL, SCANLIST_update, SCANLIST_render, SCANLIST_key},
    /* {"A to B scanner", ABSCANNER_init, ABSCANNER_update, ABSCANNER_render,
     ABSCANNER_key}, */

};

bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (apps[gCurrentApp].key) {
    return apps[gCurrentApp].key(Key, bKeyPressed, bKeyHeld);
  }
  return false;
}
void APPS_init(AppType_t app) {
  char String[16] = "";
  gCurrentApp = app;
  UI_ClearScreen();
  UI_ClearStatus();
  for (uint8_t i = 0; i <= stackIndex; i++) {
    sprintf(String, "%s>%u", String, appsStack[i]);
  }
  UI_PrintStringSmallest(String, 0, 0, true, true);
  gRedrawStatus = true;
  // UI_PrintStringSmallest(apps[gCurrentApp].name, 0, 0, true, true);

  if (apps[app].init) {
    apps[app].init();
  }
}
void APPS_update(void) {
  if (apps[gCurrentApp].update) {
    apps[gCurrentApp].update();
  }
}
void APPS_render(void) {
  if (apps[gCurrentApp].render) {
    apps[gCurrentApp].render();
  }
}
void APPS_deinit(void) {
  if (apps[gCurrentApp].deinit) {
    apps[gCurrentApp].deinit();
  }
}
void APPS_run(AppType_t app) {
  if (appsStack[stackIndex] == app) {
    return;
  }
  if (pushApp(app)) {
    APPS_deinit();
    APPS_init(app);
  }
}
void APPS_exit() {
  if (stackIndex == 0) {
    return;
  }
  APPS_deinit();
  popApp();
  APPS_init(peek());
}
