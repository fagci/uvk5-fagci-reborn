#include "test.h"
#include "../driver/st7565.h"
#include "../scheduler.h"
#include "../ui/components.h"
#include "../ui/helper.h"

uint16_t rssi = 0;
uint16_t taskSpawnInterval = 10;

void TEST_Init() {}
void TEST_Update() { gRedrawScreen = true; }
void TEST_Render() {
  char String[8];

  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  for (uint8_t i = 0; i < TASKS_MAX; i++) {
    if (tasks[i].handler) {
      sprintf(String, "%u(%s): %u", i, tasks[i].name, tasks[i].t);
      UI_PrintStringSmallest(String, i / 8 * 64, i % 8 * 6, false, true);
    }
  }
  sprintf(String, "SPWN:%u", taskSpawnInterval);
  UI_PrintStringSmallest(String, 92, 0, false, true);

  UI_RSSIBar(rssi, 6);
}
void TEST_Key(KEY_Code_t k, bool p, bool h) {
  gRedrawScreen = true;
  if (k != KEY_INVALID && p && !h) {
    if (k == KEY_UP) {
      taskSpawnInterval++;
      return;
    }
    if (k == KEY_DOWN) {
      taskSpawnInterval--;
      return;
    }
  }
}
