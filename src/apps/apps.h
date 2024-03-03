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
} App;

extern App *apps[256];
extern App *appsAvailableToRun[256];
extern App *gCurrentApp;

App *APPS_Peek();
bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
void APPS_init(App *app);
void APPS_update(void);
void APPS_render(void);
void APPS_run(App *app);
void APPS_runManual(App *app);
bool APPS_exit(void);
void APPS_Register(App *app);

#define REGISTER_APP(app)                                                      \
  App app##_info __attribute__((constructor));                                 \
  void register_##app(void) __attribute__((constructor));                      \
  void register_##app(void) { APPS_Register(&app##_info); }                    \
  App app##_info

#endif /* end of include guard: APPS_H */
