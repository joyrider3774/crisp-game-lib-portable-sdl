#include "cglp.h"

static char *title = "WIPER";
static char *description = "[Tap] Swing";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 8, .isDarkColor = true};

typedef struct {
  Vector pos;
  float size;
  bool isAlive;
} Rain;
#define WIPER_MAX_RAIN_COUNT 64
static Rain rains[WIPER_MAX_RAIN_COUNT];
static int rainIndex;
static int nextRainTicks;
static float angle;
static float va;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(rains);
    rainIndex = 0;
    nextRainTicks = 0;
    angle = (-M_PI / 5) * 4;
    va = 1;
  }
  angle += va * 0.05 * sqrt(difficulty);
  if ((angle > -M_PI / 5 && va > 0) ||
      (angle < (-M_PI / 5) * 4 && va < 0)) {
    play(HIT);
    va *= -1;
  }
  float ta;
  if (va > 0) {
    ta = -M_PI / 5;
  } else {
    ta = (-M_PI / 5) * 4;
  }
  if (angle > -M_PI / 3) {
    ta = (-M_PI / 5) * 4;
  }
  if (angle < (-M_PI / 3) * 2) {
    ta = -M_PI / 5;
  }
  color = LIGHT_BLACK;
  thickness = 2;
  barCenterPosRatio = -0.7;
  bar(50, 110, 36, ta);
  if (input.isJustPressed) {
    play(SELECT);
    angle = ta;
    va *= -1;
  }
  color = BLACK;
  thickness = 6;
  barCenterPosRatio = -0.7;
  bar(50, 110, 36, angle);
  nextRainTicks--;
  if (nextRainTicks < 0) {
    ASSIGN_ARRAY_ITEM(rains, rainIndex, Rain, r);
    vectorSet(&r->pos, rnd(10, 90), rnd(0, 20));
    r->size = 2;
    r->isAlive = true;
    rainIndex = wrap(rainIndex + 1, 0, WIPER_MAX_RAIN_COUNT);
    nextRainTicks = 10 / difficulty;
  }
  FOR_EACH(rains, i) {
    ASSIGN_ARRAY_ITEM(rains, i, Rain, r);
    SKIP_IS_NOT_ALIVE(r);
    color = TRANSPARENT;
    Collision scratch = box(r->pos.x, r->pos.y, sqrt(r->size) * 2, sqrt(r->size) * 2);
    bool isRemoved = scratch.isColliding.rect[BLACK];
    bool isAbsorbed = false;
    FOR_EACH(rains, j) {
      ASSIGN_ARRAY_ITEM(rains, j, Rain, other);
      SKIP_IS_NOT_ALIVE(other);
      if (r->pos.y > other->pos.y &&
          distanceTo(&r->pos, VEC_XY(other->pos)) < sqrt(r->size + other->size) * 2) {
        other->size += r->size;
        other->pos.x = (other->pos.x + r->pos.x) / 2;
        isAbsorbed = true;
      }
    }
    if (isAbsorbed) {
      play(LASER);
      r->isAlive = false;
      continue;
    }
    r->pos.y += sqrt(r->size - 1.9) * 0.3 * sqrt(difficulty);
    color = CYAN;
    Collision scratch2 = box(r->pos.x, r->pos.y, sqrt(r->size), sqrt(r->size));
    if (isRemoved || scratch2.isColliding.rect[BLACK]) {
      play(POWER_UP);
      addScore(ceil(r->size - 1), VEC_XY(r->pos));
      r->isAlive = false;
      continue;
    }
    if (r->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", r->pos.x, 97);
      gameOver();
    }
  }
}

void addGameWiper() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
