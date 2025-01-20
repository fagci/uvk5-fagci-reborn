#include "svc_sys.h"
#include "driver/backlight.h"
#include "driver/uart.h"
#include "external/FreeRTOS/include/FreeRTOS.h"
#include "external/FreeRTOS/include/projdefs.h"
#include "external/FreeRTOS/include/task.h"
#include "ui/statusline.h"

void SVC_SYS_Init(void) {
    Log("SYS I");
}
void SVC_SYS_Update(void) {
  for (;;) {
    Log("SYS .");
    STATUSLINE_update();
    BACKLIGHT_Update();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void SVC_SYS_Deinit(void) {}
