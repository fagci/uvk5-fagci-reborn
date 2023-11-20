#include "test.h"
#include "../driver/st7565.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"
#include "apps.h"

void TEST_Init() {}

void TEST_Update() { gRedrawScreen = true; }

void TEST_Render() {
  char String[16];

  UI_ClearScreen();

  for (uint8_t i = 0; i < TASKS_MAX; i++) {
    if (tasks[i].handler) {
      sprintf(String, "%u(%s): %u", i, tasks[i].name, tasks[i].countdown);
      UI_PrintStringSmallest(String, i / 8 * 64, i % 8 * 6, false, true);
    }
  }
}

bool TEST_key(KEY_Code_t k, bool p, bool h) {
  if (!p) {
    return false;
  }
  switch (k) {
  case KEY_EXIT:
    APPS_exit();
    return true;
  default:
    break;
  }
  return false;
}
