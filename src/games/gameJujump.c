#include "cglp.h"

static char *title = "JUJUMP";
static char *description = "[Press] Jump";

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
static Vector v;

typedef struct {
  Vector pos;
  bool isAlive;
} JujumpFloor;
#define JUJUMP_MAX_FLOOR_COUNT 16
static JujumpFloor floors[JUJUMP_MAX_FLOOR_COUNT];
static int floorIndex;

static float jumpWay;
static float jumpPower;
static float floorAppDist;

static void update() {
  if (!ticks) {
    vectorSet(&p, 50, 50);
    vectorSet(&v, 0, 0);
    INIT_UNALIVED_ARRAY(floors);
    vectorSet(&floors[0].pos, 50, 70);
    floors[0].isAlive = true;
    floorIndex = 1;
    jumpWay = 1;
    jumpPower = 1;
    floorAppDist = 1;
  }
  vectorAdd(&p, v.x, v.y);
  if (input.isPressed) {
    v.y += 0.05;
  } else {
    v.y += 0.1;
  }
  float scr;
  if (p.y < 30) {
    scr = (30 - p.y) * 0.1;
  } else {
    scr = 0;
  }
  scr += difficulty * 0.1;
  score += scr;
  floorAppDist -= scr;
  if (floorAppDist < 0) {
    floorAppDist = rnd(0, 99);
    ASSIGN_ARRAY_ITEM(floors, floorIndex, JujumpFloor, nf);
    vectorSet(&nf->pos, rnd(0, 99), -9);
    nf->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, JUJUMP_MAX_FLOOR_COUNT);
  }
  p.y += scr;
  color = BLUE;
  FOR_EACH(floors, i) {
    ASSIGN_ARRAY_ITEM(floors, i, JujumpFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.y += scr;
    box(VEC_XY(f->pos), 33, 7);
    if (f->pos.y >= 99) {
      f->isAlive = false;
    }
  }
  color = TRANSPARENT;
  while (true) {
    Collision scratch = box(VEC_XY(p), 7, 7);
    if (!scratch.isColliding.rect[BLUE]) {
      break;
    }
    p.y--;
    vectorSet(&v, 0, 0);
    jumpPower = 1;
  }
  color = GREEN;
  box(VEC_XY(p), 7, 7);
  if (input.isJustPressed) {
    play(JUMP);
    jumpWay *= -1;
    v.x = jumpWay;
    v.y = -3 * jumpPower;
    jumpPower *= 0.7;
  }
  if (p.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJujump() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
