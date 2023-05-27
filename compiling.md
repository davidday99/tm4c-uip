# Compiling uIP

There aren't any instructions on compilation. There are a set of example applications and a directory called *unix* containing code for running uIP on a Linux-like OS. Let's start by compiling an executable we can run on a PC.

Try compiling just unix/main.c and resolve the resulting errors one at a time.

Included sufficient files required for compilation:

```
gcc -o uip.elf  $(find uip -mindepth 1 -name "*.c" -not -name "uip-split.c" -printf "%p ") apps/hello-world/*.c  unix/*.c -Iuip -Iunix -Iapps/hello-world
```

Now I'm gonna move this into a Makefile so that I can easily build the project.

```
NAME=uip
OBJ=obj
BIN=build
SRCS = $(filter-out  uip/uip-split.c,\
			$(wildcard apps/hello-world/*.c) \
			$(wildcard uip/*.c) \
			$(wildcard unix/*.c) \
		)
OBJS = $(addprefix $(OBJ)/, $(notdir $(SRCS:.c=.o)))

$(info $(SRCS))

CFLAGS = -Iapps/hello-world -Iuip -Iunix

RM = rm -rf
MKDIR = @mkdir -p $(@D)

all: $(BIN)/$(NAME).elf

clean:
	-$(RM) $(OBJ)
	-$(RM) $(BIN)

#$(OBJ)/%.o: $(SRCS)%/.c
	#$(MKDIR)
	#$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: apps/hello-world/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: uip/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: unix/%.c
	$(MKDIR)
	$(CC) -o $@ $^ -c $(CFLAGS)

$(BIN)/$(NAME).elf: $(OBJS)
	$(MKDIR)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: all clean

```
