#include "cglp.h"

static char *title = "TWHOLS";
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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector p;
  float v;
} Wall;
#define TWHOLS_WALL_COUNT 2
static Wall walls[TWHOLS_WALL_COUNT];

static Vector p;
static Vector v;

static void update() {
  if (!ticks) {
    vectorSet(&p, 50, 50);
    vectorSet(&v, 1, 1);
    TIMES(TWHOLS_WALL_COUNT, i) {
      vectorSet(&walls[i].p, 0, 9 + i * 70);
      walls[i].v = 1;
    }
  }
  color = BLUE;
  TIMES(TWHOLS_WALL_COUNT, wi) {
    Wall *w = &walls[wi];
    rect(w->p.x, w->p.y, -99, 5);
    rect(w->p.x + 40, w->p.y, 99, 5);
    w->p.x -= w->v;
    if (w->p.x < -40) {
      w->p.x = 99;
      w->v = rnd(1, 2) * difficulty;
    }
  }
  color = GREEN;
  Vector dv;
  vectorSet(&dv, v.x, v.y);
  vectorMul(&dv, difficulty);
  vectorAdd(&p, dv.x, dv.y);
  Collision c = box(p.x, p.y, 5, 5);
  if (c.isColliding.rect[BLUE]) {
    play(HIT);
    if (p.y < 50) {
      v.y = 1;
    } else {
      v.y = -1;
    }
    score++;
  }
  if (input.isJustPressed || p.x < 0 || p.x > 99) {
    v.x *= -1;
  }
  if (p.y < 0 || p.y > 99) {
    play(RANDOM);
    gameOver();
  }
}

void addGameTwhols() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
