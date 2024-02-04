#include "svc_render.h"
#include "apps/apps.h"
#include "driver/st7565.h"
#include "ui/statusline.h"

void SVC_RENDER_Init(void) {}
void SVC_RENDER_Update(void) {
  if (gRedrawScreen) {
    STATUSLINE_render();
    APPS_render();
    ST7565_Render();
    gRedrawScreen = false;
  }
}
void SVC_RENDER_Deinit(void) {}
