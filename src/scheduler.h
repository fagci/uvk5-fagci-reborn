#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TASKS_MAX 64

typedef volatile struct {
  const char *name;
  void (*handler)(void);
  uint16_t interval;
  uint16_t countdown;
  bool continuous;
  uint8_t priority;
  bool active;
} Task;
extern volatile uint32_t elapsedMilliseconds;

Task *TaskAdd(const char *name, void (*handler)(void), uint16_t interval,
              bool continuous);
void TaskSetPriority(void (*handler)(void), uint8_t priority);
void TaskRemove(void (*handler)(void));
bool TaskExists(void (*handler)(void));
void TaskTouch(void (*handler)(void));
void TasksUpdate(void);
uint32_t Now(void);

void SetTimeout(uint32_t *v, uint32_t t);
bool CheckTimeout(uint32_t *v);

extern volatile Task tasks[TASKS_MAX];

#endif /* end of include guard: SCHEDULER_H */
