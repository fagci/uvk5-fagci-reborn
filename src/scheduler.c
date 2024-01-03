#include "scheduler.h"
#include "driver/uart.h"
#include "helper/battery.h"
#include "misc.h"

volatile Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;
volatile uint32_t elapsedMilliseconds = 0;

Task *TaskAdd(const char *name, void (*handler)(void), uint16_t interval,
              bool continuous) {
  UART_logf(3, "TaskAdd(%s)", name);
  if (tasksCount != TASKS_MAX) {
    tasks[tasksCount] =
        (Task){name, handler, interval, interval, continuous, 128, false};
    return &tasks[tasksCount++];
  }
  return NULL;
}

void TaskRemove(void (*handler)(void)) {
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

bool TaskExists(void (*handler)(void)) {
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

void TaskTouch(void (*handler)(void)) {
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

void TaskSetPriority(void (*handler)(void), uint8_t priority) {
  Task *t;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    t = &tasks[i];
    if (t->handler == handler) {
      t->priority = priority;
      UART_logf(3, "TaskSetPrio(%s, %u)", t->name, priority);
      return;
    }
  }
}

static void handle(Task *task) {
  UART_logf(3, "%s::handle() start", task->name);
  task->handler();
  UART_logf(3, "%s::handle() end", task->name);
}

void TasksUpdate(void) {
  bool prioritized = false;
  Task *task;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    tasks[i].active = true;
  }
  for (uint8_t i = 0; i < tasksCount; ++i) {
    task = &tasks[i];
    if (task->handler && !task->countdown && task->priority == 0) {
      handle(task);
      prioritized = true;
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        TaskRemove(task->handler);
        prioritized = false;
      }
    }
  }
  if (prioritized)
    return;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    task = &tasks[i];
    if (task->handler && !task->countdown && task->priority != 0) {
      handle(task);
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        TaskRemove(task->handler);
      }
    }
  }
}

void SystickHandler(void) {
  Task *task;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    task = &tasks[i];
    if (task->active && task->handler && task->countdown) {
      --task->countdown;
    }
  }
  elapsedMilliseconds++;
}
