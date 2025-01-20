#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "external/FreeRTOS/include/FreeRTOS.h"
#include "external/FreeRTOS/include/projdefs.h"
#include "external/FreeRTOS/include/task.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint32_t Now(void);

void SetTimeout(uint32_t *v, uint32_t t);
bool CheckTimeout(uint32_t *v);

#endif /* end of include guard: SCHEDULER_H */
