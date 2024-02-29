#include "apps.h"
#include "../driver/st7565.h"
#include "../ui/statusline.h"
#include <stddef.h>

#define APPS_STACK_SIZE 8

App *gCurrentApp;

uint8_t appsCount = 0;
uint8_t appsToRunCount = 0;
App *apps[256];
App *appsAvailableToRun[256];

static App *appsStack[APPS_STACK_SIZE];
static int8_t stackIndex = -1;

static bool pushApp(App *app) {
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

static App *popApp(void) {
  if (stackIndex > 0) {
    return appsStack[stackIndex--]; // Do not care about existing value
  }
  return appsStack[stackIndex];
}

App *APPS_Peek(void) {
  if (stackIndex >= 0) {
    return appsStack[stackIndex];
  }
  return NULL;
}

/* [APP_NONE] = {.name = "None"},
[APP_TEST] = DECL_TEST,
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
{"Text input", TEXTINPUT_init, TEXTINPUT_update, TEXTINPUT_render,
 TEXTINPUT_key, TEXTINPUT_deinit},
{"VFO config", VFOCFG_init, VFOCFG_update, VFOCFG_render, VFOCFG_key},
{"Preset config", PRESETCFG_init, PRESETCFG_update, PRESETCFG_render,
 PRESETCFG_key},
{"Scanlists", SCANLISTS_init, SCANLISTS_update, SCANLISTS_render,
 SCANLISTS_key},
{"Save to channel", SAVECH_init, SAVECH_update, SAVECH_render, SAVECH_key},
{"Settings", SETTINGS_init, SETTINGS_update, SETTINGS_render, SETTINGS_key},
{"1 VFO", VFO1_init, VFO1_update, VFO1_render, VFO1_key, VFO1_deinit},
{"2 VFO", VFO2_init, VFO2_update, VFO2_render, VFO2_key, VFO2_deinit},
{"ABOUT", ABOUT_Init, ABOUT_Update, ABOUT_Render, ABOUT_key, ABOUT_Deinit},
{"Antenna len", ANTENNA_init, ANTENNA_update, ANTENNA_render, ANTENNA_key,
 ANTENNA_deinit},
{"Task manager", TASKMAN_Init, NULL, TASKMAN_Render, TASKMAN_Key}, */

bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (gCurrentApp->key) {
    return gCurrentApp->key(Key, bKeyPressed, bKeyHeld);
  }
  return false;
}
void APPS_init(App *app) {
  gCurrentApp = app;

  STATUSLINE_SetText("%s", gCurrentApp->name);
  gRedrawScreen = true;

  if (gCurrentApp->init) {
    gCurrentApp->init();
  }
}
void APPS_update(void) {
  if (gCurrentApp->update) {
    gCurrentApp->update();
  }
}
void APPS_render(void) {
  if (gCurrentApp->render) {
    gCurrentApp->render();
  }
}
void APPS_deinit(void) {
  if (gCurrentApp->deinit) {
    gCurrentApp->deinit();
  }
}

void APPS_RunPure(App *app) {
  if (appsStack[stackIndex] == app) {
    return;
  }
  APPS_deinit();
  pushApp(app);
  APPS_init(app);
}

void APPS_run(AppType_t id) {
  for (AppType_t i = 0; i < appsCount; i++) {
    if (apps[i]->id == id) {
      APPS_RunPure(apps[i]);
      return;
    }
  }
}

void APPS_runManual(App *app) {
  APPS_RunPure(app);
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

void APPS_Register(App *app) {
  apps[appsCount++] = app;
  if (app->runnable) {
    appsAvailableToRun[appsToRunCount++] = app;
  }
}
