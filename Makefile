SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

TARGET = $(BIN_DIR)/firmware

SRC = $(wildcard $(SRC_DIR)/driver/*.c)
SRC += $(wildcard $(SRC_DIR)/external/mcufont/fonts/freesans.c)
SRC += $(wildcard $(SRC_DIR)/external/mcufont/decoder/*.c)
SRC += $(wildcard $(SRC_DIR)/helper/*.c)
SRC += $(wildcard $(SRC_DIR)/ui/*.c)
SRC += $(wildcard $(SRC_DIR)/apps/*.c)
SRC += $(wildcard $(SRC_DIR)/*.c)

OBJS = $(OBJ_DIR)/start.o
OBJS += $(OBJ_DIR)/init.o
OBJS += $(OBJ_DIR)/external/printf/printf.o

OBJS += $(OBJ_DIR)/external/FreeRTOS/list.o
OBJS += $(OBJ_DIR)/external/FreeRTOS/queue.o
OBJS += $(OBJ_DIR)/external/FreeRTOS/tasks.o
OBJS += $(OBJ_DIR)/external/FreeRTOS/timers.o
OBJS += $(OBJ_DIR)/external/FreeRTOS/portable/GCC/ARM_CM0/port.o

OBJS += $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

BSP_DEFINITIONS := $(wildcard hardware/*/*.def)
BSP_HEADERS := $(patsubst hardware/%,inc/%,$(BSP_DEFINITIONS))
BSP_HEADERS := $(patsubst %.def,%.h,$(BSP_HEADERS))

AS = arm-none-eabi-gcc
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

GIT_HASH := $(shell git rev-parse --short HEAD)
TS := $(date +'"%Y%m%d_%H%M%S"')

ASFLAGS = -c -mcpu=cortex-m0
CFLAGS = -Os -Wall -Wno-error -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -Wno-error=incompatible-pointer-types -std=c2x -MMD -flto=auto -Wextra
CFLAGS += -DPRINTF_INCLUDE_CONFIG_H
CFLAGS += -DGIT_HASH=\"$(GIT_HASH)\"
CFLAGS += -DTIME_STAMP=\"$(TS)\"


CCFLAGS += -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums -fno-delete-null-pointer-checks -MMD -g
CCFLAGS += -flto
CCFLAGS += -ftree-vectorize -funroll-loops
CCFLAGS += -Wextra -Wno-unused-function -Wno-unused-variable -Wno-unknown-pragmas 
#-Wunused-parameter -Wconversion
CCFLAGS += -fno-math-errno -pipe -ffunction-sections -fdata-sections -ffast-math
CCFLAGS += -fsingle-precision-constant -finline-functions-called-once
CCFLAGS += -Os -g3 -fno-exceptions -fno-non-call-exceptions -fno-delete-null-pointer-checks
CCFLAGS += -DARMCM0


LDFLAGS = -mcpu=cortex-m0 -nostartfiles -Wl,-T,firmware.ld
# Use newlib-nano instead of newlib
LDFLAGS += --specs=nano.specs -lc -lnosys -mthumb -mabi=aapcs -lm -fno-rtti -fno-exceptions
LDFLAGS += -Wl,--build-id=none
LDFLAGS += -z noseparate-code -z noexecstack -mcpu=cortex-m0 -nostartfiles -Wl,-L,linker -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

INC =
INC += -I ./src
INC += -I ./src/external/CMSIS_5/CMSIS/Core/Include/
INC += -I ./src/external/CMSIS_5/Device/ARM/ARMCM0/Include
INC += -I ./src/external/mcufont/decoder/
INC += -I ./src/external/mcufont/fonts/
INC += -I ./src/external/FreeRTOS/include/.
INC += -I ./src/external/FreeRTOS/portable/GCC/ARM_CM0/.

DEPS = $(OBJS:.o=.d)

.PHONY: all clean

all: $(TARGET)
	$(OBJCOPY) -O binary $< $<.bin
	-python fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	-python3 fw-pack.py $<.bin $(GIT_HASH) $<.packed.bin
	$(SIZE) $<

version.o: .FORCE

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) $^ -o $@

inc/dp32g030/%.h: hardware/dp32g030/%.def

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(BSP_HEADERS) $(OBJ_DIR)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

.FORCE:

-include $(DEPS)

clean:
	rm -f $(TARGET).bin $(TARGET).packed.bin $(TARGET) $(OBJ_DIR)/*.o $(OBJ_DIR)/*.d $(OBJ_DIR)/**/*.o $(OBJ_DIR)/**/**/*.o $(OBJ_DIR)/**/*.d
