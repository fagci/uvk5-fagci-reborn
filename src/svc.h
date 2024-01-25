#ifndef SVC_H
#define SVC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
  SVC_LISTEN,
  SVC_SCAN,
  SVC_BAT_SAVE,
} Svc;

bool SVC_Running(Svc svc);
void SVC_Toggle(Svc svc, bool on, uint16_t interval);

#endif /* end of include guard: SVC_H */
