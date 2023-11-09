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
#include "audio.h"
#include "helper/battery.h"
#include "misc.h"

#define TASKS_MAX 16

typedef struct {
  void (*handler)(void);
  uint16_t interval;
  uint16_t t;
  bool continuous;
} Task;

Task tasks[TASKS_MAX];
uint8_t tasksCount = 0;

bool TaskAdd(void *handler, uint16_t interval, bool continuous) {
  if (tasksCount == TASKS_MAX) {
    return false;
  }
  uint8_t freeIndex = tasksCount > 0 ? tasksCount : 0;
  tasks[freeIndex] = (Task){handler, interval, 0, continuous};
  tasksCount++;
  return true;
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
    }
  }
}

void SystickHandler(void);

void SystickHandler(void) {
  for (uint8_t i = 0; i < 8; ++i) {
    Task *task = &tasks[i];
    if (task->handler && ++task->t >= task->interval) {
      if (task->continuous) {
        task->handler();
        task->t = 0;
      } else {
        TaskRemove(task->handler);
      }
    }
  }
}
