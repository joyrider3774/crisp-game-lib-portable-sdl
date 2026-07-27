#ifndef MENU_H
#define MENU_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#include "cglp.h"

EXTERNC int gameCount;
EXTERNC void addGame(char *title, char *description,
                     char (*characters)[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1],
                     int charactersCount, Options options, int usesMouse,
                     void (*update)(void));
EXTERNC Game getGame(int index);
EXTERNC void addMenu();
EXTERNC void addGames();

#endif
