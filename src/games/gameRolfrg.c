#include "cglp.h"

static char *title = "ROLFRG";
static char *description = "[Press] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  Vector p;
  float a;
} Frog;
#define FROG_COUNT 5
static Frog frogs[FROG_COUNT];

static float x;
static float w;

static void update() {
  if (!ticks) {
    TIMES(FROG_COUNT, i) {
      vectorSet(&frogs[i].p, 0, -i * 9);
      frogs[i].a = rnd(0, 9);
    }
    x = 50;
    w = 1;
  }
  color = GREEN;
  score++;
  int period = (int)ceil(30 / difficulty);
  float l;
  if (ticks % period) {
    l = 4;
  } else {
    l = 30;
  }
  TIMES(FROG_COUNT, fi) {
    Frog *f = &frogs[fi];
    if (f->p.y < 0) {
      f->p.x = rnd(0, 99);
      f->p.y += rnd(99, 120);
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(VEC_XY(f->p), l, f->a);
    f->a += difficulty / 9;
    f->p.y -= difficulty / 2;
    if (l > 9) {
      play(LASER);
      addWithAngle(&f->p, f->a, 30);
    }
  }
  color = YELLOW;
  x += w * difficulty;
  if (input.isJustPressed || x < 0 || x > 99) {
    w *= -1;
  }
  Collision c = box(x, 36, 3, 3);
  if (c.isColliding.rect[GREEN]) {
    play(RANDOM);
    gameOver();
  }
}

void addGameRolfrg() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
