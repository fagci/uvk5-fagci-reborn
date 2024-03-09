#include "apps.h"
#include "../driver/st7565.h"
#include "../driver/uart.h"
#include "../helper/vfo.h"
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
  if (gCurrentApp->key) {
    return gCurrentApp->key(Key, bKeyPressed, bKeyHeld);
  }
  return false;
}
static void loadVfo(App *app) {
  AppVFOSlots *slots = app->vfoSlots;
  if (slots) {
    slots->count = 0;
    for (uint8_t i = 0; i < VFO_GetVfosCount(); ++i) {
      if (slots->count >= slots->maxCount) {
        break;
      }
      CH *vfo = VFO_GetVfo(i);
      if (vfo->vfo.app == app->id) {
        slots->slots[slots->count++] = vfo;
      }
    }
  }
}
void APPS_init(App *app) {
  gCurrentApp = app;

  loadVfo(app);

  STATUSLINE_SetText("%s", gCurrentApp->name);
  gRedrawScreen = true;

  if (gCurrentApp->init) {
    gCurrentApp->init();
  }
}
void APPS_update() {
  if (gCurrentApp->update) {
    gCurrentApp->update();
  }
}
void APPS_render() {
  if (gCurrentApp->render) {
    gCurrentApp->render();
  }
}
void APPS_deinit() {
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
  Log("APPS_run %u...", id);
  App *app = APPS_GetById(id);
  APPS_RunPure(app);
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
