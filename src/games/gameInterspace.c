#include "cglp.h"

static char *title = "INTERSPACE";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

static Vector p;
static Vector s;
static Vector wp;

static void update() {
  if (!ticks) {
    vectorSet(&p, 50, 10);
    vectorSet(&s, 7, 7);
    vectorSet(&wp, 50, 99);
  }
  float sc = difficulty * 0.5 + (s.y - 7) * 0.3;
  wp.y -= sc;
  addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (wp.y < 0) {
    wp.x = wrap(wp.x + rnd(-9, 9), 9, 90);
    wp.y = 99;
    play(COIN);
  }
  color = RED;
  rect(0, wp.y, wp.x - 7, 9);
  rect(wp.x + 7, wp.y, 99, 9);
  p.x = clamp(input.pos.x, 0, 99);
  color = TRANSPARENT;
  Collision c1 = box(VEC_XY(p), 7, 99);
  if (c1.isColliding.rect[RED]) {
    p.y += (10 - p.y) * 0.1;
    s.y += (7 - s.y) * 0.4;
  } else {
    p.y += (30 - p.y) * 0.1;
    s.y += (30 - s.y) * 0.1;
  }
  color = GREEN;
  Collision c2 = box(VEC_XY(p), VEC_XY(s));
  if (c2.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameInterspace() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
