#include "cglp.h"

static char *title = "GOLFME";
static char *description = "[Hold]    Change angle\n[Release] Jump";

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
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

static Vector p;
static Vector v;
static bool isJumping;
static float angle;
static float width;
static float space;

static void update() {
  if (!ticks) {
    vectorSet(&p, 50, 85);
    isJumping = false;
    angle = 0;
    width = 0;
    space = 0;
  }
  if (width + space < 0) {
    width = 200;
    space = rnd(50, 150);
  }
  color = BLUE;
  rect(0, 90, width, 9);
  rect(width + space, 90, 200, 9);
  color = GREEN;
  Collision scratch = box(VEC_XY(p), 9, 9);
  if (p.x < 0 || p.y > 99) {
    play(RANDOM);
    gameOver();
  }
  if (isJumping) {
    vectorAdd(&p, v.x, v.y);
    v.y += 0.1;
    if (scratch.isColliding.rect[BLUE]) {
      isJumping = false;
      angle = 0;
      p.y = 85;
    }
  } else {
    if (input.isPressed) {
      angle -= 0.05;
      thickness = 3;
      barCenterPosRatio = 0;
      bar(p.x, p.y, 20, angle);
    }
    if (input.isJustReleased) {
      play(JUMP);
      isJumping = true;
      vectorSet(&v, 4, 0);
      rotate(&v, angle);
    }
  }
  float scr = clamp(p.x - 50, 0, 99) * 0.1 + difficulty;
  p.x -= scr;
  width -= scr;
  score += scr;
}

void addGameGolfme() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
