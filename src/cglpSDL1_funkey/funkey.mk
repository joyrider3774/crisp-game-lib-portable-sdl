CC = /opt/funkey-sdk/usr/bin/arm-linux-gcc
PREFIX = /opt/funkey-sdk/arm-funkey-linux-musleabihf/sysroot/usr
SDL_CONFIG = $(PREFIX)/bin/sdl-config
CCFLAGS += -DFUNKEY=1