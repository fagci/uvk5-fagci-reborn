#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool TaskAdd(void *handler, uint16_t interval, bool continuous);
void TaskRemove(void *handler);

#endif /* end of include guard: SCHEDULER_H */
