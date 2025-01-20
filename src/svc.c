#include "svc.h"
#include "driver/uart.h"
#include "external/CMSIS_5/Device/ARM/ARMCM0/Include/ARMCM0.h"
#include "external/FreeRTOS/include/FreeRTOS.h"
#include "external/FreeRTOS/include/projdefs.h"
#include "external/FreeRTOS/include/task.h"
#include "scheduler.h"
#include "svc_apps.h"
#include "svc_bat_save.h"
#include "svc_beacon.h"
#include "svc_fastscan.h"
#include "svc_keyboard.h"
#include "svc_listening.h"
#include "svc_render.h"
#include "svc_scan.h"
#include "svc_sys.h"

typedef struct {
  const char *name;
  void (*init)(void);
  void (*update)(void);
  void (*deinit)(void);
  bool running;
} Service;

Service services[] = {
    {"KBD", SVC_KEYBOARD_Init, SVC_KEYBOARD_Update, SVC_KEYBOARD_Deinit},
    {"Listen", SVC_LISTEN_Init, SVC_LISTEN_Update, SVC_LISTEN_Deinit},
    {"Scan", SVC_SCAN_Init, SVC_SCAN_Update, SVC_SCAN_Deinit},
    {"FC", SVC_FC_Init, SVC_FC_Update, SVC_FC_Deinit},
    {"PING", SVC_BEACON_Init, SVC_BEACON_Update, SVC_BEACON_Deinit},
    /* {"Bat save", SVC_BAT_SAVE_Init, SVC_BAT_SAVE_Update,
       SVC_BAT_SAVE_Deinit}, */
    {"Apps", SVC_APPS_Init, SVC_APPS_Update, SVC_APPS_Deinit},
    // {"Sys", SVC_SYS_Init, SVC_SYS_Update, SVC_SYS_Deinit},
    {"RNDR", SVC_RENDER_Init, SVC_RENDER_Update, SVC_RENDER_Deinit},
    // {"UART", NULL, uartHandle, NULL},
};

bool SVC_Running(Svc svc) { return services[svc].running; }

void SVC_Toggle(Svc svc, bool on, uint16_t interval) {
  Service *s = &services[svc];
  if (on) {
    s->init();
    s->running = true;
  } else {
    s->deinit();
    s->running = false;
  }
}
