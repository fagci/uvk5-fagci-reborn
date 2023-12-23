#include "test.h"
#include "../driver/st7565.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/graphics.h"
#include "apps.h"

void TEST_Init() {}

void TEST_Update() { gRedrawScreen = true; }

static uint32_t counter = 1024;
static bool kp = false;
static bool kh = false;
static KEY_Code_t key = KEY_INVALID;

void TEST_Render() {
  UI_ClearScreen();

  for (uint8_t i = 0; i < TASKS_MAX; i++) {
    if (tasks[i].handler) {
      PrintSmall((i / 8) * 64, i % 8 * 6 + 8 + 5, "%u(%s): %u", i,
                 tasks[i].name, tasks[i].countdown);
    }
  }

  PrintMedium(0, 42, "Key:%u, P:%u, H:%u, V:%lu", key, kp, kh, counter);
}

bool TEST_key(KEY_Code_t k, bool p, bool h) {
  kh = h;
  kp = p;
  key = k;
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  case KEY_UP:
    if (!p) {
      counter++;
    }
    return true;
  case KEY_DOWN:
    if (!p) {
      counter--;
    }
    return true;
  case KEY_MENU:
    return false;
  default:
    break;
  }
  return true;
}
