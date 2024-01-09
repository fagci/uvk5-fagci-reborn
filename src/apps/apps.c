#include "apps.h"
#include "../driver/st7565.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "about.h"
#include "appslist.h"
#include "channelscanner.h"
#include "fastscan.h"
#include "finput.h"
#include "lootlist.h"
#include "presetcfg.h"
#include "presetlist.h"
#include "reset.h"
#include "savech.h"
#include "scanlists.h"
#include "settings.h"
#include "spectrumchannel.h"
#include "spectrumreborn.h"
#include "still.h"
#include "test.h"
#include "textinput.h"
#include "vfo.h"
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

AppType_t APPS_Peek() {
  if (stackIndex >= 0) {
    return appsStack[stackIndex];
  }
  return APP_NONE;
}

const AppType_t appsAvailableToRun[RUN_APPS_COUNT] = {
    APP_VFO,   APP_SPECTRUM,  APP_SPECTRUM_CH,  APP_CH_SCANNER,   APP_FASTSCAN,
    APP_STILL, APP_SCANLISTS, APP_PRESETS_LIST, APP_TASK_MANAGER, APP_ABOUT,
};

const App apps[APPS_COUNT] = {
    {"None"},
    {"Test", TEST_Init, TEST_Update, TEST_Render, TEST_key},
    {"Spectrum", SPECTRUM_init, SPECTRUM_update, SPECTRUM_render, SPECTRUM_key},
    {"Spectrum CH", SPECTRUMCH_init, SPECTRUMCH_update, SPECTRUMCH_render,
     SPECTRUMCH_key},
    {"CH scanner", CHSCANNER_init, CHSCANNER_update, CHSCANNER_render,
     CHSCANNER_key, CHSCANNER_deinit},
    {"Freq catch", FASTSCAN_init, FASTSCAN_update, FASTSCAN_render,
     FASTSCAN_key, FASTSCAN_deinit},
    {"VFO extended", STILL_init, NULL, STILL_render, STILL_key, STILL_deinit},
    {"Frequency input", FINPUT_init, NULL, FINPUT_render, FINPUT_key,
     FINPUT_deinit},
    {"Apps", APPSLIST_init, NULL, APPSLIST_render, APPSLIST_key},
    {"Loot", LOOTLIST_init, NULL, LOOTLIST_render, LOOTLIST_key},
    {"Presets", PRESETLIST_init, NULL, PRESETLIST_render, PRESETLIST_key},
    {"Reset", RESET_Init, RESET_Update, RESET_Render, RESET_key},
    {"Text input", TEXTINPUT_init, TEXTINPUT_update, TEXTINPUT_render,
     TEXTINPUT_key, TEXTINPUT_deinit},
    {"VFO config", VFOCFG_init, VFOCFG_update, VFOCFG_render, VFOCFG_key},
    {"Preset config", PRESETCFG_init, PRESETCFG_update, PRESETCFG_render,
     PRESETCFG_key},
    {"Scanlists", SCANLISTS_init, SCANLISTS_update, SCANLISTS_render,
     SCANLISTS_key},
    {"Save to channel", SAVECH_init, SAVECH_update, SAVECH_render, SAVECH_key},
    {"Settings", SETTINGS_init, SETTINGS_update, SETTINGS_render, SETTINGS_key},
    {"VFO", VFO_init, VFO_update, VFO_render, VFO_key, VFO_deinit},
    {"ABOUT", ABOUT_Init, ABOUT_Update, ABOUT_Render, ABOUT_key, ABOUT_Deinit},
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
  char appnameShort[3];
  gCurrentApp = app;
  for (uint8_t i = 0; i <= stackIndex; i++) {
    strncpy(appnameShort, apps[appsStack[i]].name, 2);
    sprintf(String, "%s>%s", String, appnameShort);
  }
  STATUSLINE_SetText(String);
  // STATUSLINE_SetText(apps[gCurrentApp].name);
  gRedrawScreen = true;

  if (apps[gCurrentApp].init) {
    apps[gCurrentApp].init();
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
#include "../driver/uart.h"
void APPS_run(AppType_t app) {
  if (appsStack[stackIndex] == app) {
    return;
  }
  if (pushApp(app)) {
    APPS_deinit();
    APPS_init(app);
  }
}

bool APPS_exit() {
  if (stackIndex == 0) {
    return false;
  }
  APPS_deinit();
  popApp();
  APPS_init(APPS_Peek());
  return true;
}
