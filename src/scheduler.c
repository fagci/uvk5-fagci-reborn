#include "scheduler.h"

static uint32_t elapsedMilliseconds = 0;

Task tasks[TASKS_MAX];
static uint8_t tasksCount = 0;

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
  tasksCount--;
  for (; i < tasksCount; ++i) {
    tasks[i] = tasks[i + 1];
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
    Task *task = &tasks[i];
    task->active = true;
    if (!task->countdown) {
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
  elapsedMilliseconds++;
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->active && task->countdown) {
      --task->countdown;
    }
  }
}

uint32_t Now(void) { return elapsedMilliseconds; }

void SetTimeout(uint32_t *v, uint32_t t) {
  *v = t == UINT32_MAX ? UINT32_MAX : Now() + t;
}

bool CheckTimeout(uint32_t *v) { return Now() >= *v; }
