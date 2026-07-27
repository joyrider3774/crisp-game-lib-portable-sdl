#include "cglp.h"

static char *title = "LLAND";
static char *description = "[Hold] Thrust up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "l    l",
    " llll ",
    " l  l ",
    "l ll l",
    "ll  ll",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  float y;
  int c;
} LlandMountain;
static LlandMountain mountains[9];

static float shipY;
static float shipV;
static float offset;
static float mountainAppDist;
static int mountainIndex;
static int landingIndex;
static bool landing;
static float landY;
static bool isFirstLanded;

static void update() {
  if (!ticks) {
    TIMES(9, i) {
      if (i == 4) {
        landY = 49;
        mountains[i].y = landY;
        mountains[i].c = CYAN;
      } else {
        mountains[i].y = 90 - i;
        mountains[i].c = RED;
      }
    }
    shipY = 30;
    shipV = 0;
    offset = 0;
    mountainAppDist = 0;
    mountainIndex = 0;
    landing = false;
    isFirstLanded = false;
    landingIndex = 7;
  }
  TIMES(9, i) {
    LlandMountain *m = &mountains[i];
    color = m->c;
    rect(wrap(i * 13 + offset - 13, -13, 104), m->y, 13, 99);
  }
  color = GREEN;
  Collision shipCollision = character("a", 25, shipY);
  if (landing) {
    if (input.isJustPressed) {
      landing = false;
    } else {
      return;
    }
  }
  offset -= difficulty;
  mountainAppDist -= difficulty;
  if (mountainAppDist < 0) {
    LlandMountain *m = &mountains[(int)wrap(mountainIndex, 0, 9)];
    if (landingIndex > 7 || landingIndex == 1) {
      m->y = rnd(70, 90);
    } else if (landingIndex == 0) {
      landY = rnd(40, 70);
      m->y = landY;
    } else {
      m->y = rnd(40, 90);
    }
    landingIndex--;
    if (landingIndex < 0) {
      m->c = CYAN;
      landingIndex = 9;
    } else {
      m->c = RED;
    }
    mountainIndex++;
    mountainAppDist += 13;
  }
  if (isFirstLanded) {
    if (input.isJustPressed) {
      play(LASER);
      shipV -= 0.4;
    }
    if (input.isPressed) {
      shipV -= 0.2;
      particle(24.5, shipY + 2, 1, 1, M_PI_2, 1);
    }
  }
  shipV += 0.1;
  shipV *= 0.99;
  if (shipY < 0 && shipV < 0) {
    shipV *= -1;
  }
  shipY += shipV * difficulty;
  if (shipCollision.isColliding.rect[CYAN]) {
    play(SELECT);
    particle(24.5, shipY, 9, 1, 0, M_PI * 2);
    score++;
    landing = true;
    shipV = 0;
    shipY = landY - 3;
    TIMES(9, i) { mountains[i].c = RED; }
    isFirstLanded = true;
  }
  if (shipCollision.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  Collision leftEdge = rect(-1, 0, 1, 99);
  if (leftEdge.isColliding.rect[CYAN]) {
    color = RED;
    for (float y = landY - 4; y < 99; y += 7) {
      text("X", 2, y);
    }
    play(EXPLOSION);
    gameOver();
  }
}

void addGameLland() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
