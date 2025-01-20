#ifndef SVC_H
#define SVC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SVC_KEYBOARD,
  SVC_LISTEN,
  SVC_SCAN,
  SVC_FC,
  SVC_BEACON,
  // SVC_BAT_SAVE,
  SVC_APPS,
  // SVC_SYS,
  SVC_RENDER,
  // SVC_UART,
} Svc;

bool SVC_Running(Svc svc);
void SVC_Toggle(Svc svc, bool on, uint16_t interval);

#endif /* end of include guard: SVC_H */
