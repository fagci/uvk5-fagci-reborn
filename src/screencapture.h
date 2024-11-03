#include "string.h"

#include "external/printf/printf.h"

#include "driver/st7565.h"
#include "driver/uart.h"
#include "ui/graphics.h"

static inline void captureScreen(void) {
  LogUart("P1\n");
  LogUart("128 64\n");
  for (uint8_t y = 0; y < LCD_HEIGHT; ++y) {
    for (uint8_t x = 0; x < LCD_WIDTH; ++x) {
      LogUart(GetPixel(x, y) ? "0 " : "1 ");
    }
    LogUart("\n");
  }
  LogUart("\n");
}
