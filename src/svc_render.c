#include "svc_render.h"
#include "apps/apps.h"
#include "driver/st7565.h"
#include "scheduler.h"
#include "ui/statusline.h"

static const uint32_t RENDER_TIME = 1000 / 25;
uint32_t gLastRender = 0;

void SVC_RENDER_Init(void) {}
void SVC_RENDER_Update(void) {
  if (gRedrawScreen && Now() - gLastRender >= RENDER_TIME) {
    APPS_render();
    STATUSLINE_render(); // next after APPS_render coz of statusline update; if
                         // need to draw in statusline, do it in
                         // STATUSLINE_render
    ST7565_Render();
    gRedrawScreen = false;
    gLastRender = Now();
  }
}
void SVC_RENDER_Deinit(void) {}
