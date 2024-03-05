#ifndef APPSREGISTRY_H
#define APPSREGISTRY_H

#include "../driver/keyboard.h"
#include "../settings.h"
#include <stdint.h>

typedef enum {
  APP_TAG_SPECTRUM = 1,
  APP_TAG_CH = 2,
} AppCategory;

typedef enum {
  APP_NONE,
  APP_TEST,
  APP_SPECTRUM,
  APP_ANALYZER,
  APP_CH_SCANNER,
  APP_FASTSCAN,
  APP_STILL,
  APP_FINPUT,
  APP_APPSLIST,
  APP_LOOT_LIST,
  APP_BANDS_LIST,
  APP_RESET,
  APP_TEXTINPUT,
  APP_CH_CFG,
  APP_BAND_CFG,
  APP_SCANLISTS,
  APP_SAVECH,
  APP_SETTINGS,
  APP_MULTIVFO,
  APP_ABOUT,
  APP_ANT,
  APP_TASKMAN,
  APP_MESSENGER,
} AppType_t;

typedef struct {
  const char *name;
  void (*init)();
  void (*update)();
  void (*render)();
  bool (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
  void (*deinit)();
  CH *vfo;
  void *cfg;
  bool runnable;
  AppCategory tags;
  AppType_t id;
} App;

void APPS_Register(App *app);
void APPS_RegisterAll();

extern App *apps[256];
extern App *appsAvailableToRun[256];
extern uint8_t appsCount;
extern uint8_t appsToRunCount;

#endif /* end of include guard: APPSREGISTRY_H */
