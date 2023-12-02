#include "scheduler.h"
#include "driver/uart.h"
#include "helper/battery.h"
#include "misc.h"

volatile Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;
volatile uint32_t elapsedMilliseconds = 0;

Task *TaskAdd(const char *name, void (*handler)(void), uint16_t interval,
              bool continuous) {
  UART_printf("%u: TaskAdd(%s)\n", elapsedMilliseconds, name);
  if (tasksCount != TASKS_MAX) {
    tasks[tasksCount] =
        (Task){name, handler, interval, interval, continuous, 128, false};
    return &tasks[tasksCount++];
  }
  return NULL;
}

void TaskRemove(void (*handler)(void)) {
  uint8_t i;
  for (i = 0; i < tasksCount; ++i) {
    if (tasks[i].handler == handler) {
      tasks[i].handler = NULL;
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

void TaskTouch(void (*handler)(void)) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    if (tasks[i].handler == handler) {
      tasks[i].countdown = 0;
      return;
    }
  }
}

void TaskSetPriority(void (*handler)(void), uint8_t priority) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    if (tasks[i].handler == handler) {
      tasks[i].priority = priority;
      return;
    }
  }
}

void TasksUpdate(void) {
  bool prioritized = false;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    tasks[i].active = true;
  }
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->handler && !task->countdown && task->priority == 0) {
      uint32_t now = elapsedMilliseconds;
      task->handler();
      UART_printf("%u: %s() took %ums\n", elapsedMilliseconds, task->name,
                  elapsedMilliseconds - now);
      prioritized = true;
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        UART_printf("%u: TaskRemove(%s)\n", elapsedMilliseconds, task->name);
        TaskRemove(task->handler);
      }
    }
  }
  if (prioritized)
    return;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->handler && !task->countdown && task->priority != 0) {
      uint32_t now = elapsedMilliseconds;
      task->handler();
      UART_printf("%u: %s() took %ums\n", elapsedMilliseconds, task->name,
                  elapsedMilliseconds - now);
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        UART_printf("%u: TaskRemove(%s)\n", elapsedMilliseconds, task->name);
        TaskRemove(task->handler);
      }
    }
  }
}

/* void TasksUpdate(void) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->handler && !task->countdown && task->priority != 0) {
      uint32_t now = elapsedMilliseconds;
      task->handler();
      UART_printf("%u: %s() took %ums\n", elapsedMilliseconds, task->name,
                  elapsedMilliseconds - now);
      if (task->continuous) {
        task->countdown = task->interval;
      } else {
        UART_printf("%u: TaskRemove(%s)\n", elapsedMilliseconds, task->name);
        TaskRemove(task->handler);
      }
    }
  }
} */

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
