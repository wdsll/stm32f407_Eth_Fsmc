CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
CFLAGS = -mcpu=cortex-m4 -mthumb -Os -ffreestanding -fdata-sections -ffunction-sections \
         -Wall -Wextra -Werror -std=c11 -Iinclude
LDFLAGS = -Tgd32f303ret6.ld -nostdlib -Wl,--gc-sections,-Map=build/charger.map
SRC = src/main.c src/charger_control.c startup_gd32f303.S

.PHONY: all clean host-test
all: build/charger.bin
build/charger.elf: $(SRC) gd32f303ret6.ld | build
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@
build/charger.bin: build/charger.elf
	$(OBJCOPY) -O binary $< $@
build:
	mkdir -p $@
host-test: | build
	cc -std=c11 -Wall -Wextra -Werror -Iinclude tests/test_control.c src/charger_control.c -o build/test_control
	./build/test_control
clean:
	rm -rf build
