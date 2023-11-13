#ifndef APPS_H
#define APPS_H

#include "../driver/keyboard.h"

typedef enum {
  APP_TEST,
  APP_SPECTRUM,
  APP_STILL,
  APP_FINPUT,
  APP_SCANLIST,
  APP_AB_SCANNER,
} AppType_t;

typedef struct App {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*render)(void);
  void (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
} App;

extern const App apps[5];
extern AppType_t gCurrentApp;

void APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(AppType_t app);
void APPS_update(void);
void APPS_render(void);
void APPS_run(AppType_t app);

#endif /* end of include guard: APPS_H */
