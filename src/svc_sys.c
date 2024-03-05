#include "svc_sys.h"
#include "driver/backlight.h"
#include "ui/statusline.h"

void SVC_SYS_Init() {}
void SVC_SYS_Update() {
  STATUSLINE_update();
  BACKLIGHT_Update();
}
void SVC_SYS_Deinit() {}
