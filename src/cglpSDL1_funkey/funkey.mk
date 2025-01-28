CC = /opt/funkey-sdk/usr/bin/arm-linux-gcc
PREFIX = /opt/funkey-sdk/arm-funkey-linux-musleabihf/sysroot/usr
SDL_CONFIG = $(PREFIX)/bin/sdl-config
CFLAGS += -march=armv7-a+neon-vfpv4 -mtune=cortex-a7 -mfpu=neon-vfpv4 -DFUNKEY=1