#include "svc_sys.h"
#include "driver/backlight.h"
#include "ui/statusline.h"

void SVC_SYS_Init(void) {}
void SVC_SYS_Update(void) {
  STATUSLINE_update();
  BACKLIGHT_Update();
}
void SVC_SYS_Deinit(void) {}
