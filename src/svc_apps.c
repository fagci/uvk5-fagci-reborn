#include "svc_apps.h"
#include "apps/apps.h"
#include "external/FreeRTOS/include/FreeRTOS.h"
#include "external/FreeRTOS/include/projdefs.h"
#include "external/FreeRTOS/include/task.h"
#include "misc.h"

static StaticTask_t taskBuffer;
static StackType_t taskStack[configMINIMAL_STACK_SIZE + 200];

void SVC_APPS_Init(void) {
  xTaskCreateStatic(SVC_APPS_Update, "RNDR", ARRAY_SIZE(taskStack), NULL, 4,
                    taskStack, &taskBuffer);
}

void SVC_APPS_Update(void) {
  for (;;) {
    APPS_update();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
void SVC_APPS_Deinit(void) {}
