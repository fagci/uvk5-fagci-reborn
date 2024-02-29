#ifndef APPS_H
#define APPS_H

#include "../driver/keyboard.h"
#include "../settings.h"

#define APPS_COUNT 23
#define RUN_APPS_COUNT 14

typedef enum {
  APP_TAG_SPECTRUM = 1,
  APP_TAG_VFO = 2,
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
  APP_PRESETS_LIST,
  APP_RESET,
  APP_TEXTINPUT,
  APP_VFO_CFG,
  APP_PRESET_CFG,
  APP_SCANLISTS,
  APP_SAVECH,
  APP_SETTINGS,
  APP_VFO1,
  APP_VFO2,
  APP_ABOUT,
  APP_ANT,
  APP_TASKMAN,
} AppType_t;

typedef struct {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*render)(void);
  bool (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
  void (*deinit)(void);
  VFO *vfo;
  void *cfg;
  bool runnable;
  AppCategory tags;
  AppType_t id;
} App;

extern App *apps[256];
extern App *appsAvailableToRun[256];
extern App *gCurrentApp;

App *APPS_Peek();
bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(App *app);
void APPS_update(void);
void APPS_render(void);
void APPS_RunPure(App *app);
void APPS_run(AppType_t id);
void APPS_runManual(App *app);
bool APPS_exit(void);
void APPS_Register(App *app);

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#define REGISTER_APP(...)                                                      \
  __attribute__((constructor)) static void registerApp(void) {                        \
    APPS_Register(&(App)__VA_ARGS__);                                          \
  }

#endif /* end of include guard: APPS_H */
