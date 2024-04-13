#include "svc_render.h"
#include "apps/apps.h"
#include "driver/st7565.h"
#include "scheduler.h"
#include "ui/statusline.h"

static const uint32_t RENDER_TIME = 40;
static uint32_t lastRender = 0;

void SVC_RENDER_Init(void) {}
void SVC_RENDER_Update(void) {
  if (gRedrawScreen && Now() - lastRender >= RENDER_TIME) {
    APPS_render();
    STATUSLINE_render();
    ST7565_Render();
    gRedrawScreen = false;
    lastRender = Now();
  }
}
void SVC_RENDER_Deinit(void) {}
