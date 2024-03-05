#include "scheduler.h"
#include "driver/uart.h"

volatile Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;
volatile uint32_t elapsedMilliseconds = 0;

Task *TaskAdd(const char *name, void (*handler)(), uint16_t interval,
              bool continuous, uint8_t priority) {
  UART_logf(3, "TaskAdd(%s)", name);
  if (tasksCount == TASKS_MAX) {
    return NULL;
  }

  uint8_t insertI;
  for (insertI = 0; insertI < tasksCount; ++insertI) {
    if (tasks[insertI].priority > priority) {
      break;
    }
  }

  if (insertI < tasksCount) {
    for (uint8_t i = tasksCount; i > insertI; --i) {
      tasks[i] = tasks[i - 1];
    }
  }

  tasks[insertI] =
      (Task){name, handler, interval, interval, continuous, priority, false};
  tasksCount++;
  return &tasks[insertI];
}

void TaskRemove(void (*handler)()) {
  uint8_t i;
  Task *t;
  for (i = 0; i < tasksCount; ++i) {
    t = &tasks[i];
    if (t->handler == handler) {
      t->handler = NULL;
      UART_logf(3, "TaskRemove(%s)", t->name);
      tasksCount--;
      break;
    }
  }
  for (; i < tasksCount; ++i) {
    if (tasks[i].handler == NULL && tasks[i + 1].handler != NULL) {
      tasks[i] = tasks[i + 1];
      tasks[i + 1].handler = NULL;
    }
  }
}

bool TaskExists(void (*handler)()) {
  uint8_t i;
  Task *t;
  for (i = 0; i < tasksCount; ++i) {
    t = &tasks[i];
    if (t->handler == handler) {
      return true;
    }
  }
  return false;
}

void TaskTouch(void (*handler)()) {
  Task *t;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    t = &tasks[i];
    if (t->handler == handler) {
      t->countdown = 0;
      UART_logf(3, "TaskTouch(%s)", t->name);
      return;
    }
  }
}

static void handle(Task *task) {
  UART_logf(3, "%s::handle() start", task->name);
  task->handler();
  UART_logf(3, "%s::handle() end", task->name);
}

void TasksUpdate() {
  Task *task;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    tasks[i].active = true;
  }
  for (uint8_t i = 0; i < tasksCount; ++i) {
    task = &tasks[i];
    if (task->handler && !task->countdown) {
      handle(task);
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        TaskRemove(task->handler);
      }
    }
  }
}

void SystickHandler() {
  Task *task;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    task = &tasks[i];
    if (task->active && task->handler && task->countdown) {
      --task->countdown;
    }
  }
  elapsedMilliseconds++;
}

uint32_t Now() { return elapsedMilliseconds; }

void SetTimeout(uint32_t *v, uint32_t t) {
  uint32_t max = (uint32_t)0 - 1;
  if (t == max) {
    *v = max;
    return;
  }
  *v = Now() + t;
}

bool CheckTimeout(uint32_t *v) { return Now() >= *v; }
