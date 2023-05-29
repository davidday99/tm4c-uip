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

Not only that, but we can also ping our server! 

# TODO: explain tap0 device

Now I'm gonna move this into a Makefile so that I can easily build the project.

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
OBJS = $(addprefix $(OBJ)/, $(notdir $(SRCS:.c=.o)))

$(info $(SRCS))

CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix -Ilib

RM = rm -rf
MKDIR = @mkdir -p $(@D)

all: $(BIN)/$(NAME).elf

clean:
	-$(RM) $(OBJ)
	-$(RM) $(BIN)

$(OBJ)/%.o: apps/$(APP)/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: uip/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: unix/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: lib/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(BIN)/$(NAME).elf: $(OBJS)
	$(MKDIR)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: all clean

```

# Compiling for ARM Cortex-M


