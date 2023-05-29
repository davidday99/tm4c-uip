NAME=uip
OBJ=obj
BIN=build
APP=hello-world

CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

DEV = /dev/ttyACM0
FLASHER = lm4flash
FLASHER_OPTS = -S
LD_SCRIPT = tm4c/TM4C123GH6PM.ld

SRCS = $(filter-out  uip/uip-split.c apps/$(APP)/httpd-fsdata.c,\
			$(wildcard apps/$(APP)/*.c) \
			$(wildcard uip/*.c) \
			$(wildcard tm4c/*.c) \
			$(wildcard lib/*.c) \
		)
OBJS = $(addprefix $(OBJ)/, $(SRCS:.c=.o))

CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix -Ilib -Itm4c -Itm4c/inc -MD -Wextra -Wall -Wno-missing-braces -std=c99 
CFLAGS += -mcpu=cortex-m4 -mfloat-abi=softfp -nostdlib -ffreestanding 

RM = rm -rf
MKDIR = @mkdir -p $(@D)

all: $(BIN)/$(NAME).elf $(BIN)/$(NAME).bin

clean:
	-$(RM) $(OBJ)
	-$(RM) $(BIN)

$(OBJ)/%.o: %.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(BIN)/$(NAME).elf: $(OBJS)
	$(MKDIR)
	$(CC) -o $@ $^ $(CFLAGS) -Wl,-T$(LD_SCRIPT) -Wl,-eResetISR
	$(OBJCOPY) -O binary $@ $(BIN)/$(NAME).bin

$(BIN)/$(NAME).bin: $(BIN)/$(NAME).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BIN)/$(NAME).bin
	$(FLASHER) $(FLASHER_OPTS) $(DEV) $(BIN)/$(NAME).bin


.PHONY: all clean



