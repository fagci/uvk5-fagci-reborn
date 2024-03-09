#ifndef APPSREGISTRY_H
#define APPSREGISTRY_H

#include "../driver/keyboard.h"
#include "../globals.h"
#include <stdint.h>

typedef enum {
  APP_TAG_SPECTRUM = 1,
  APP_TAG_CH = 2,
} AppCategory;

typedef struct {
  uint8_t count;
  uint8_t maxCount;
  CH *slots[5];
} AppVFOSlots;

typedef struct {
  const char *name;
  void (*init)();
  void (*update)();
  void (*render)();
  bool (*key)(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
  void (*deinit)();
  void *cfg;
  bool runnable;
  AppCategory tags;
  AppType_t id;
  AppVFOSlots *vfoSlots;
} App;

App *APPS_GetById(AppType_t id);
void APPS_Register(App *app);
void APPS_RegisterAll();

extern App *apps[256];
extern App *appsAvailableToRun[256];
extern uint8_t appsCount;
extern uint8_t appsToRunCount;

#endif /* end of include guard: APPSREGISTRY_H */
