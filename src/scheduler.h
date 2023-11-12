#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASKS_MAX 16

typedef struct {
  const char *name;
  void (*handler)(void);
  uint16_t interval;
  uint16_t t;
  bool continuous;
} Task;

Task *TaskAdd(const char *name, void *handler, uint16_t interval,
              bool continuous);
void TaskRemove(void *handler);
void TaskTouch(void *handler);
void TasksUpdate(void);

extern Task tasks[TASKS_MAX];

#endif /* end of include guard: SCHEDULER_H */
