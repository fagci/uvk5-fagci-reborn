#include "svc.h"
#include "scheduler.h"
#include "svc_bat_save.h"
#include "svc_listening.h"
#include "svc_scan.h"

typedef struct {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*deinit)(void);
} Service;

Service services[] = {
    {"Listen", SVC_LISTEN_Init, SVC_LISTEN_Update, SVC_LISTEN_Deinit},
    {"Scan", SVC_SCAN_Init, SVC_SCAN_Update, SVC_SCAN_Deinit},
    {"Bat save", SVC_BAT_SAVE_Init, SVC_BAT_SAVE_Update, SVC_BAT_SAVE_Deinit},
};

bool SVC_Running(Svc svc) { return TaskExists(services[svc].update); }

void SVC_Toggle(Svc svc, bool on, uint16_t interval) {
  Service *s = &services[svc];
  bool exists = TaskExists(s->update);
  if (on) {
    if (!exists) {
      s->init();
      TaskAdd(s->name, s->update, interval, true, 50 + (uint8_t)svc);
    }
  } else {
    if (exists) {
      TaskRemove(s->update);
      s->deinit();
    }
  }
}
