/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "scheduler.h"
#include "helper/battery.h"
#include "misc.h"

Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;

Task *TaskAdd(const char *name, void *handler, uint16_t interval,
              bool continuous) {
  if (tasksCount == TASKS_MAX) {
    return NULL;
  }
  Task newTask = (Task){name, handler, interval, 0, continuous};
  tasks[tasksCount++] = newTask;
  return &tasks[tasksCount - 1];
}

void TaskRemove(void *handler) {
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

void TaskTouch(void *handler) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    if (tasks[i].handler == handler) {
      tasks[i].t = 0;
      return;
    }
  }
}

void SystickHandler(void);

void SystickHandler(void) {
  for (uint8_t i = 0; i < tasksCount; ++i) {
    Task *task = &tasks[i];
    if (task->handler && ++task->t >= task->interval) {
      if (task->continuous) {
        task->handler();
        task->t = 0;
      } else {
        task->handler();
        TaskRemove(task->handler);
      }
    }
  }
}
