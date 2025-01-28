SRC_DIR = src 
OBJ_DIR = ./obj
EXE=cglpsdl2

SRC=$(wildcard *.c $(foreach fd, $(SRC_DIR), $(fd)/*.c)) 
OBJS=$(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))

SDL_CONFIG=sdl2-config
CC = gcc
OPT_LEVEL ?= -O2 
CFLAGS ?= -g -Wall -Wextra `$(SDL_CONFIG) --cflags` -Wno-write-strings
LDFLAGS ?= -g
LDLIBS ?=  `$(SDL_CONFIG) --libs` -lm 

ifeq ($(OS),Windows_NT)
LDLIBS += -mconsole
endif

#MINGW does not have X11 and does not require it
#dont know about cygwin
ifneq ($(OS),Windows_NT)
ifeq ($(NOX11),)
LDLIBS += -lX11
endif
endif

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $(OBJS) -o $@ $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)