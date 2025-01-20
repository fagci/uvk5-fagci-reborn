#include "svc_render.h"
#include "apps/apps.h"
#include "driver/st7565.h"
#include "driver/uart.h"
#include "misc.h"
#include "scheduler.h"
#include "ui/statusline.h"

uint32_t gLastRender = 0;

static const uint32_t RENDER_TIME = 1000 / 25;

static StaticTask_t taskBuffer;
static StackType_t taskStack[configMINIMAL_STACK_SIZE + 100];

void SVC_RENDER_Init(void) {
  xTaskCreateStatic(SVC_RENDER_Update, "RNDR", ARRAY_SIZE(taskStack), NULL, 2,
                    taskStack, &taskBuffer);
}

void SVC_RENDER_Update(void) {
  for (;;) {
    if (gRedrawScreen && Now() - gLastRender >= RENDER_TIME) {
      APPS_render();
      STATUSLINE_render(); // next after APPS_render coz of statusline update;
                           // if need to draw in statusline, do it in
                           // STATUSLINE_render
      ST7565_Render();
      gRedrawScreen = false;
      gLastRender = Now();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
void SVC_RENDER_Deinit(void) {}
