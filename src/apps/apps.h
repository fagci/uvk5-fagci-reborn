#ifndef APPS_H
#define APPS_H

#include "../driver/keyboard.h"

#define APPS_COUNT 15

typedef enum {
  APP_NONE,
  APP_TASK_MANAGER,
  APP_SPECTRUM,
  APP_STILL,
  APP_FINPUT,
  APP_APPS_LIST,
  APP_LOOT_LIST,
  APP_PRESETS_LIST,
  APP_RESET,
  APP_TEXTINPUT,
  APP_VFO_CFG,
  APP_PRESET_CFG,
  APP_SAVECH,
  APP_SETTINGS,
  APP_VFO,
  // APP_SCANLIST,
  // APP_AB_SCANNER,
} AppType_t;

typedef struct App {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*render)(void);
  bool (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
  void (*deinit)(void);
} App;

extern const App apps[APPS_COUNT];
extern AppType_t gPreviousApp;
extern AppType_t gCurrentApp;

bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(AppType_t app);
void APPS_update(void);
void APPS_render(void);
void APPS_run(AppType_t app);
void APPS_exit(void);

#endif /* end of include guard: APPS_H */
