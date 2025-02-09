DEBUG = 0
SRC_DIR = ./src/cglpSDL2  
LIB_DIR = ./src/lib
GME_DIR = ./src/games
STATIC_LIBS = 0

OBJ_DIR = obj
EXE=cglpsdl2

SRC=$(wildcard *.c $(foreach fd, $(SRC_DIR), $(fd)/*.c)) 
OBJS=$(addprefix $(OBJ_DIR)/, $(SRC:.c=.o))

LIB_DIR_SRC=$(wildcard *.c $(foreach fd, $(LIB_DIR), $(fd)/*.c)) 
OBJS+=$(addprefix $(OBJ_DIR)/, $(LIB_DIR_SRC:.c=.o))

GME_DIR_SRC=$(wildcard *.c $(foreach fd, $(GME_DIR), $(fd)/*.c)) 
OBJS+=$(addprefix $(OBJ_DIR)/, $(GME_DIR_SRC:.c=.o))


INCS = -I$(SRC_DIR) -I$(LIB_DIR) -I$(GME_DIR)
SDL_CONFIG = sdl2-config
CC = gcc
CFLAGS ?= -Wall -Wextra `$(SDL_CONFIG) --cflags`
LDFLAGS ?=
ifeq ($(STATIC_LIBS),1)
LDLIBS ?= `$(SDL_CONFIG) --static-libs`
else
LDLIBS ?= `$(SDL_CONFIG) --libs` 
endif
LDLIBS += -lm 

ifeq ($(DEBUG),0)
OPT_LEVEL ?= -O2
else
CFLAGS += -g -DDEBUG
LDFLAGS += -g
endif

ifeq ($(OS),Windows_NT)
ifneq ($(DEBUG),0)
LDLIBS += -mconsole
endif
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
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv *~ $(OBJS) $(EXE)