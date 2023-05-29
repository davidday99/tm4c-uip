# Compiling uIP

There aren't any instructions on compilation. There are a set of example applications and a directory called *unix* containing code for running uIP on a Linux-like OS. Let's start by compiling an executable we can run on a PC.

Try compiling just unix/main.c and resolve the resulting errors one at a time.

Included sufficient files required for compilation:

```
gcc -o uip.elf  $(find uip -mindepth 1 -name "*.c" -not -name "uip-split.c" -printf "%p ") apps/hello-world/*.c  unix/*.c -Iuip -Iunix -Iapps/hello-world
```

According to the code, we should expect to get something if we connect over TCP to port 1000;

```
void
hello_world_init(void)
{
  /* We start to listen for connections on TCP port 1000. */
  uip_listen(HTONS(1000));
}
```

We can use a simple Python script to test it out.

```
import socket 

HOST = "192.168.0.2"  # our host IP
PORT = 1000

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    data = s.recv(1024)

print(f'Received {data}')
```

Now if we run `uip.elf`, `client.py` will print out a message we received.

Not only that, but the device is also pingable!

# TODO: explain tap0 device

I'm gonna move this into a Makefile so that I can easily build the project.

```
NAME=uip
OBJ=obj
BIN=build
APP=hello-world
SRCS = $(filter-out  uip/uip-split.c apps/$(APP)/httpd-fsdata.c,\
			$(wildcard apps/$(APP)/*.c) \
			$(wildcard uip/*.c) \
			$(wildcard unix/*.c) \
			$(wildcard lib/*.c) \
		)
OBJS = $(addprefix $(OBJ)/, $(SRCS:.c=.o))

$(info $(SRCS))

CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix -Ilib

RM = rm -rf
MKDIR = @mkdir -p $(@D)

all: $(BIN)/$(NAME).elf

clean:
	-$(RM) $(OBJ)
	-$(RM) $(BIN)

$(OBJ)/%.o: %.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(BIN)/$(NAME).elf: $(OBJS)
	$(MKDIR)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: all clean

```

# Compiling for ARM Cortex-M

Now let's move everything over to the TM4C.

I'll start off by updating my Makefile. 

```
diff --git a/Makefile b/Makefile
index ac5f197..7838846 100644
--- a/Makefile
+++ b/Makefile
@@ -2,22 +2,32 @@ NAME=uip
 OBJ=obj
 BIN=build
 APP=hello-world
+
+CC = arm-none-eabi-gcc
+OBJCOPY = arm-none-eabi-objcopy
+
+DEV = /dev/ttyACM0
+FLASHER = lm4flash
+FLASHER_OPTS = -S
+LD_SCRIPT = tm4c/TM4C123GH6PM.ld
+
 SRCS = $(filter-out  uip/uip-split.c apps/$(APP)/httpd-fsdata.c,\
 			$(wildcard apps/$(APP)/*.c) \
 			$(wildcard uip/*.c) \
-			$(wildcard unix/*.c) \
+			$(wildcard tm4c/*.c) \
 			$(wildcard lib/*.c) \
 		)
 OBJS = $(addprefix $(OBJ)/, $(SRCS:.c=.o))
 
-CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix -Ilib
+CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix -Ilib -Itm4c -Itm4c/inc -MD -Wextra -Wall -Wno-missing-braces -std=c99 
+CFLAGS += -mcpu=cortex-m4 -mfloat-abi=softfp -nostdlib -ffreestanding 
 
 RM = rm -rf
 MKDIR = @mkdir -p $(@D)
 
-all: $(BIN)/$(NAME).elf
+all: $(BIN)/$(NAME).elf $(BIN)/$(NAME).bin
 
 clean:
 	-$(RM) $(OBJ)
@@ -31,7 +41,7 @@ $(OBJ)/%.o: uip/%.c
 	$(MKDIR)
 	$(CC) -o $@ $^ -c $(CFLAGS)
 
-$(OBJ)/%.o: unix/%.c
+$(OBJ)/%.o: tm4c/%.c
 	$(MKDIR)
 	$(CC) -o $@ $^ -c $(CFLAGS)
 
@@ -41,7 +51,15 @@ $(OBJ)/%.o: lib/%.c
 
 $(BIN)/$(NAME).elf: $(OBJS)
 	$(MKDIR)
-	$(CC) -o $@ $^ $(CFLAGS)
+	$(CC) -o $@ $^ $(CFLAGS) -Wl,-T$(LD_SCRIPT) -Wl,-eResetISR
+	$(OBJCOPY) -O binary $@ $(BIN)/$(NAME).bin
+
+$(BIN)/$(NAME).bin: $(BIN)/$(NAME).elf
+	$(OBJCOPY) -O binary $< $@
+
+flash: $(BIN)/$(NAME).bin
+	$(FLASHER) $(FLASHER_OPTS) $(DEV) $(BIN)/$(NAME).bin
+
 
 .PHONY: all clean
 
```

I've added some variables defining the tools from the `arm-none-eabi-` toolchain along with some
variables for flashing the the MCU. I'm using `lm4flash`.

I've also added some compiler flags, namely:

- `-mcpu=cortex-m4` 
- `-mfloat-abi=softfp` to ensure the compiler doesn't emit any floating point instructions
- `-nostdlib` to disable linking any of the standard library functions or startup files
- `-ffreestanding` so that we can override some of the standard functions like `memcpy` and `printf`

Finally, I've added a few other targets to the makefile.

- `$(BIN)/$(NAME).bin`, a flat binary generated from the ELF file to flash to the MCU
- `flash` to flash the MCU

Now that our Makefile is updated, we can compile our project. The next thing to do is 
implement the functions from earlier.

```
void memset(void *dst, char c, int n) {
    while (n--)
      *((char *) dst++) = c; 
}

void memcpy(void *dst, const void *src, int n) {
    while (n--)
      *((char *) dst++) = *((char *) src++);
}

int strlen(char *s) {
    int n = 0;
    while (*s++)
        n++;
    return n;
}

char *strncpy(char *dst, char *src, int sz) {
    while (*src && sz--)
      *((char *) dst++) = *((char *) src++);
    return dst;
}

int printf() {
    return 0;
}
``` 

That should do it.

# Network Interface 

The last thing we need before we can write our actual application is some means of sending and receiving data from the network. I'm going to use a breakout ENC28J60 ethernet controller from Microchip that I happen to have. It has a SPI interface for communicating with a host MCU. I wrote the driver for it using the SSI functions from `driverlib`.





