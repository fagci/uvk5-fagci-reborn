#include "apps.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../ui/statusline.h"
#include <stddef.h>

#define APPS_STACK_SIZE 8

App *gCurrentApp;

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

static App *popApp() {
  if (stackIndex > 0) {
    return appsStack[stackIndex--]; // Do not care about existing value
  }
  return appsStack[stackIndex];
}

App *APPS_Peek() {
  if (stackIndex >= 0) {
    return appsStack[stackIndex];
  }
  return NULL;
}

bool APPS_key(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld) {
  if (gCurrentAppkey) {
    return gCurrentAppkey(Key, bKeyPressed, bKeyHeld);
  }
  return false;
}
void APPS_init(App *app) {
  gCurrentApp = app;

  STATUSLINE_SetText("%s", gCurrentAppname);
  gRedrawScreen = true;

  if (gCurrentAppinit) {
    gCurrentAppinit();
  }
}
void APPS_update() {
  if (gCurrentAppupdate) {
    gCurrentAppupdate();
  }
}
void APPS_render() {
  if (gCurrentApprender) {
    gCurrentApprender();
  }
}
void APPS_deinit() {
  if (gCurrentAppdeinit) {
    gCurrentAppdeinit();
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
  Log("APPS_run %u...", id);
  for (uint8_t i = 0; i < appsCount; i++) {
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

bool APPS_exit() {
  if (stackIndex == 0) {
    return false;
  }
  APPS_deinit();
  popApp();
  APPS_init(APPS_Peek());
  return true;
}
