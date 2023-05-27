NAME=uip
OBJ=obj
BIN=build
APP=webserver
SRCS = $(filter-out  uip/uip-split.c apps/$(APP)/httpd-fsdata.c,\
			$(wildcard apps/$(APP)/*.c) \
			$(wildcard uip/*.c) \
			$(wildcard unix/*.c) \
		)
OBJS = $(addprefix $(OBJ)/, $(notdir $(SRCS:.c=.o)))

$(info $(SRCS))

CFLAGS = -g -Iapps/$(APP) -Iuip -Iunix

RM = rm -rf
MKDIR = @mkdir -p $(@D)

all: $(BIN)/$(NAME).elf

clean:
	-$(RM) $(OBJ)
	-$(RM) $(BIN)

#$(OBJ)/%.o: $(SRCS)%/.c
	#$(MKDIR)
	#$(CC) -o $@ $^ -c $(CFLAGS)

$(OBJ)/%.o: apps/$(APP)/%.c
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



