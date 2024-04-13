#include "apps.h"
#include "../driver/st7565.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "../ui/statusline.h"
#include "about.h"
#include "analyzer.h"
#include "antenna.h"
#include "appslist.h"
#include "channelscanner.h"
#include "fastscan.h"
#include "finput.h"
#include "lootlist.h"
#include "memview.h"
#include "presetcfg.h"
#include "presetlist.h"
#include "reset.h"
#include "savech.h"
#include "scanlists.h"
#include "settings.h"
#include "si.h"
#include "spectrumreborn.h"
#include "still.h"
#include "textinput.h"
#include "vfo1.h"
#include "vfo2.h"
#include "vfocfg.h"

#define APPS_STACK_SIZE 8

AppType_t gCurrentApp = APP_NONE;

static AppType_t appsStack[APPS_STACK_SIZE] = {0};
static int8_t stackIndex = -1;

static bool pushApp(AppType_t app) {
  if (stackIndex < APPS_STACK_SIZE - 1) {
    appsStack[++stackIndex] = app;
  } else {
    for (uint8_t i = 1; i < APPS_STACK_SIZE; ++i) {
      appsStack[i - 1] = appsStack[i];
    }
    appsStack[stackIndex] = app;
  }
  return true;
}

static AppType_t popApp(void) {
  if (stackIndex > 0) {
    return appsStack[stackIndex--]; // Do not care about existing value
  }
  return appsStack[stackIndex];
}

AppType_t APPS_Peek(void) {
  if (stackIndex >= 0) {
    return appsStack[stackIndex];
  }
  return APP_NONE;
}

const AppType_t appsAvailableToRun[RUN_APPS_COUNT] = {
    APP_VFO1,         //
    APP_STILL,        //
    APP_VFO2,         //
    APP_SI,           //
    APP_CH_SCANNER,   //
    APP_SPECTRUM,     //
    APP_ANALYZER,     //
    APP_FASTSCAN,     //
    APP_LOOT_LIST,    //
    APP_SCANLISTS,    //
    APP_PRESETS_LIST, //
    APP_ANT,          //
    APP_MEMVIEW,      //
    APP_ABOUT,        //
};

const App apps[APPS_COUNT] = {
    {"None"},
    {"EEPROM view", MEMVIEW_Init, MEMVIEW_Update, MEMVIEW_Render, MEMVIEW_key},
    {"Spectrum band", SPECTRUM_init, SPECTRUM_update, SPECTRUM_render,
     SPECTRUM_key, SPECTRUM_deinit},
    {"Spectrum analyzer", ANALYZER_init, ANALYZER_update, ANALYZER_render,
     ANALYZER_key, ANALYZER_deinit},
    {"CH Scan", CHSCANNER_init, CHSCANNER_update, CHSCANNER_render,
     CHSCANNER_key, CHSCANNER_deinit},
    {"Freq catch", FASTSCAN_init, FASTSCAN_update, FASTSCAN_render,
     FASTSCAN_key, FASTSCAN_deinit},
    {"1 VFO pro", STILL_init, STILL_update, STILL_render, STILL_key,
     STILL_deinit},
    {"Frequency input", FINPUT_init, NULL, FINPUT_render, FINPUT_key,
     FINPUT_deinit},
    {"Run app", APPSLIST_init, NULL, APPSLIST_render, APPSLIST_key},
    {"Loot", LOOTLIST_init, NULL, LOOTLIST_render, LOOTLIST_key},
    {"Presets", PRESETLIST_init, NULL, PRESETLIST_render, PRESETLIST_key},
    {"Reset", RESET_Init, RESET_Update, RESET_Render, RESET_key},
    {"Text input", TEXTINPUT_init, NULL, TEXTINPUT_render, TEXTINPUT_key,
     TEXTINPUT_deinit},
    {"VFO config", VFOCFG_init, VFOCFG_update, VFOCFG_render, VFOCFG_key},
    {"Preset config", PRESETCFG_init, PRESETCFG_update, PRESETCFG_render,
     PRESETCFG_key},
    {"Scanlists", SCANLISTS_init, SCANLISTS_update, SCANLISTS_render,
     SCANLISTS_key},
    {"Save to channel", SAVECH_init, SAVECH_update, SAVECH_render, SAVECH_key},
    {"Settings", SETTINGS_init, SETTINGS_update, SETTINGS_render, SETTINGS_key},
    {"1 VFO", VFO1_init, VFO1_update, VFO1_render, VFO1_key, VFO1_deinit},
    {"2 VFO", VFO2_init, VFO2_update, VFO2_render, VFO2_key, VFO2_deinit},
    {"SI47XX", SI_init, SI_update, SI_render, SI_key, SI_deinit},
    {"ABOUT", NULL, NULL, ABOUT_Render, ABOUT_key, NULL},
    {"Antenna len", ANTENNA_init, NULL, ANTENNA_render, ANTENNA_key,
     ANTENNA_deinit},
};

bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (apps[gCurrentApp].key) {
    return apps[gCurrentApp].key(Key, bKeyPressed, bKeyHeld);
  }
  return false;
}
void APPS_init(AppType_t app) {
  gCurrentApp = app;

  STATUSLINE_SetText("%s", apps[gCurrentApp].name);
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
void APPS_run(AppType_t app) {
  if (appsStack[stackIndex] == app) {
    return;
  }
  APPS_deinit();
  pushApp(app);
  APPS_init(app);
}

void APPS_runManual(AppType_t app) {
  APPS_run(app);
  /* APPS_deinit();
  stackIndex = 0;
  appsStack[stackIndex] = app;
  APPS_init(app); */
}

bool APPS_exit(void) {
  if (stackIndex == 0) {
    return false;
  }
  APPS_deinit();
  popApp();
  APPS_init(APPS_Peek());
  return true;
}
