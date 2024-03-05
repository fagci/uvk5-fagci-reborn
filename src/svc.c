#include "svc.h"
#include "driver/uart.h"
#include "scheduler.h"
#include "svc_apps.h"
#include "svc_bat_save.h"
#include "svc_keyboard.h"
#include "svc_listening.h"
#include "svc_render.h"
#include "svc_scan.h"
#include "svc_sys.h"

typedef struct {
  const char *name;
  void (*init)();
  void (*update)();
  void (*deinit)();
  uint8_t priority;
} Service;

Service services[] = {
    {"Keyboard", SVC_KEYBOARD_Init, SVC_KEYBOARD_Update, SVC_KEYBOARD_Deinit,
     0},
    {"Listen", SVC_LISTEN_Init, SVC_LISTEN_Update, SVC_LISTEN_Deinit, 50},
    {"Scan", SVC_SCAN_Init, SVC_SCAN_Update, SVC_SCAN_Deinit, 51},
    {"Bat save", SVC_BAT_SAVE_Init, SVC_BAT_SAVE_Update, SVC_BAT_SAVE_Deinit,
     52},
    {"Apps", SVC_APPS_Init, SVC_APPS_Update, SVC_APPS_Deinit, 100},
    {"Sys", SVC_SYS_Init, SVC_SYS_Update, SVC_SYS_Deinit, 150},
    {"Render", SVC_RENDER_Init, SVC_RENDER_Update, SVC_RENDER_Deinit, 255},
};

bool SVC_Running(Svc svc) { return TaskExists(services[svc].update); }

void SVC_Toggle(Svc svc, bool on, uint16_t interval) {
  Service *s = &services[svc];
  bool exists = TaskExists(s->update);
  if (on) {
    if (!exists) {
      s->init();
      TaskAdd(s->name, s->update, interval, true, s->priority);
    }
  } else {
    if (exists) {
      TaskRemove(s->update);
      s->deinit();
    }
  }
}
