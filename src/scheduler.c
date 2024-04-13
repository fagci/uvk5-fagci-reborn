#include "scheduler.h"

Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;
static uint32_t elapsedMilliseconds = 0;

static void handle(Task *task) { task->handler(); }

static int8_t taskIndex(void (*handler)(void)) {
  for (int8_t i = 0; i < tasksCount; ++i) {
    if ((&tasks[i])->handler == handler) {
      return i;
    }
  }
  return -1;
}

Task *TaskAdd(const char *name, void (*handler)(void), uint16_t interval,
              bool continuous, uint8_t priority) {
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

  tasks[insertI] = (Task){
      .name = name,
      .handler = handler,
      .interval = interval,
      .countdown = interval,
      .continuous = continuous,
      .priority = priority,
      .active = false,
  };
  tasksCount++;
  return &tasks[insertI];
}

void TaskRemove(void (*handler)(void)) {
  int8_t i = taskIndex(handler);
  if (i == -1) {
    return;
  }
  (&tasks[i])->handler = NULL;
  tasksCount--;
  for (; i < tasksCount; ++i) {
    if (tasks[i].handler == NULL && tasks[i + 1].handler != NULL) {
      tasks[i] = tasks[i + 1];
      tasks[i + 1].handler = NULL;
    }
  }
}

bool TaskExists(void (*handler)(void)) { return taskIndex(handler) != -1; }

void TaskTouch(void (*handler)(void)) {
  int8_t i = taskIndex(handler);
  if (i != -1) {
    (&tasks[i])->countdown = 0;
  }
}

void TasksUpdate(void) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    (&tasks[i])->active = true;
  }
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
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

void SystickHandler(void) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->active && task->handler && task->countdown) {
      --task->countdown;
    }
  }
  elapsedMilliseconds++;
}

uint32_t Now(void) { return elapsedMilliseconds; }

void SetTimeout(uint32_t *v, uint32_t t) {
  if (t == UINT32_MAX) {
    *v = UINT32_MAX;
    return;
  }
  *v = Now() + t;
}

bool CheckTimeout(uint32_t *v) { return Now() >= *v; }
