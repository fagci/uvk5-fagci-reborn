TARGET = firmware

BSP_DEFINITIONS := $(wildcard hardware/*/*.def)
BSP_HEADERS := $(patsubst hardware/%,bsp/%,$(BSP_DEFINITIONS))
BSP_HEADERS := $(patsubst %.def,%.h,$(BSP_HEADERS))

OBJS =
# Startup files
OBJS += start.o
OBJS += init.o
OBJS += external/printf/printf.o

# Drivers
OBJS += driver/adc.o
OBJS += driver/backlight.o
OBJS += driver/bk4819.o
OBJS += driver/eeprom.o
OBJS += driver/gpio.o
OBJS += driver/i2c.o
OBJS += driver/keyboard.o
OBJS += driver/spi.o
OBJS += driver/st7565.o
OBJS += driver/system.o
OBJS += driver/systick.o

# Main

# OBJS += audio.o
# OBJS += bitmaps.o
OBJS += board.o
OBJS += dcs.o
OBJS += font.o
# OBJS += frequencies.o
OBJS += helper/battery.o
OBJS += helper/measurements.o
OBJS += misc.o
OBJS += scheduler.o
OBJS += settings.o
OBJS += version.o

OBJS += main.o

ifeq ($(OS),Windows_NT)
TOP := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
else
TOP := $(shell pwd)
endif

AS = arm-none-eabi-gcc
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

GIT_HASH := $(shell git rev-parse --short HEAD)

ASFLAGS = -c -mcpu=cortex-m0
CFLAGS = -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -std=c11 -MMD
CFLAGS += -DPRINTF_INCLUDE_CONFIG_H
CFLAGS += -DGIT_HASH=\"$(GIT_HASH)\"
LDFLAGS = -mcpu=cortex-m0 -nostartfiles -Wl,-T,firmware.ld

ifeq ($(DEBUG),1)
ASFLAGS += -g
CFLAGS += -g
LDFLAGS += -g
endif

INC =
INC += -I $(TOP)
INC += -I $(TOP)/external/CMSIS_5/CMSIS/Core/Include/
INC += -I $(TOP)/external/CMSIS_5/Device/ARM/ARMCM0/Include

LIBS =

DEPS = $(OBJS:.o=.d)

all: $(TARGET)
	$(OBJCOPY) -O binary $< $<.bin
	-python fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	-python3 fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	$(SIZE) $<

debug:
	/opt/openocd/bin/openocd -c "bindto 0.0.0.0" -f interface/jlink.cfg -f dp32g030.cfg

flash:
	/opt/openocd/bin/openocd -c "bindto 0.0.0.0" -f interface/jlink.cfg -f dp32g030.cfg -c "write_image firmware.bin 0; shutdown;"

version.o: .FORCE

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

bsp/dp32g030/%.h: hardware/dp32g030/%.def

%.o: %.c | $(BSP_HEADERS)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

%.o: %.S
	$(AS) $(ASFLAGS) $< -o $@

.FORCE:

-include $(DEPS)

clean:
	rm -f $(TARGET).bin $(TARGET).packed.bin $(TARGET) $(OBJS) $(DEPS)
