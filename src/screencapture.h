#include "string.h"

#include "external/printf/printf.h"

#include "driver/uart.h"
#include "driver/st7565.h"

static inline void LogUart(const char *const str)
{
    UART_Send(str, strlen(str));
}

static inline void captureScreen(void)
{
    char str[2] = "";

    LogUart("P1\n");
    LogUart("128 64\n");

    for(uint8_t l = 0; l < 8; l++)
    {
        for(uint8_t b = 0; b < 8; b++)
        {
            for(uint8_t i = 0; i < 128; i++)
            {
                sprintf(str, "%d ", ((gFrameBuffer[l][i] >> b)  & 0x01));
                LogUart(str);
            }
        }

        LogUart("\n");
    }
    
    LogUart("\n");
}