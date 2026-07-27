#include "cglp.h"

static char *title = "CLEANER";
static char *description = "[Hold] Sweep";

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
    .viewSizeX = 100, .viewSizeY = 150, .soundSeed = 8, .isDarkColor = true};

static Vector headPos;
static Vector pos;
static float length;

typedef struct {
  Vector pos;
  Vector size;
  float angle;
  bool isAlive;
} Dust;
// Spawn interval scales as ~15/difficulty (ticks) but lifetime only as
// ~190/sqrt(difficulty) -> concurrent count ~12.67*sqrt(difficulty) grows
// unbounded; 256 covers difficulty up to ~400 (~400 min continuous play).
#define CLEANER_MAX_DUST_COUNT 256
static Dust dusts[CLEANER_MAX_DUST_COUNT];
static int dustIndex;
static float nextDustDist;

typedef struct {
  Vector pos;
  float size;
  bool isAlive;
} Hole;
#define CLEANER_MAX_HOLE_COUNT 16
static Hole holes[CLEANER_MAX_HOLE_COUNT];
static int holeIndex;
static float nextHoleDist;

static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&headPos, 50, 130);
    vectorSet(&pos, 50, 130);
    length = 0;
    INIT_UNALIVED_ARRAY(dusts);
    dustIndex = 0;
    INIT_UNALIVED_ARRAY(holes);
    holeIndex = 0;
    nextHoleDist = 99;
    nextDustDist = 0;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  float scrollSpeed = sd;
  headPos = pos;
  addWithAngle(&headPos, sin(ticks * 0.1 * sd) - M_PI_2, length);
  if (input.isJustPressed) {
    play(LASER);
    multiplier = 1;
  }
  if (input.isJustReleased) {
    play(CLICK);
  }
  if (input.isPressed) {
    length += (150 - length) * 0.005 * difficulty;
  } else {
    pos.x += (headPos.x - pos.x) * 0.005 * length * sd;
    length *= 1 - 0.1 * sd;
  }
  pos.x = clamp(pos.x, 0, 100);
  color = LIGHT_CYAN;
  thickness = 7;
  line(VEC_XY(headPos), VEC_XY(pos));
  color = CYAN;
  box(VEC_XY(headPos), 15, 10);
  nextDustDist -= scrollSpeed;
  if (nextDustDist < 0) {
    ASSIGN_ARRAY_ITEM(dusts, dustIndex, Dust, nd);
    vectorSet(&nd->pos, rnd(10, 90), -20);
    vectorSet(&nd->size, rnd(3, 9), rnd(3, 9));
    nd->angle = rnd(0, M_PI * 2);
    nd->isAlive = true;
    dustIndex = wrap(dustIndex + 1, 0, CLEANER_MAX_DUST_COUNT);
    nextDustDist = rnd(10, 20) / sd;
  }
  color = LIGHT_BLACK;
  FOR_EACH(dusts, i) {
    ASSIGN_ARRAY_ITEM(dusts, i, Dust, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.y += scrollSpeed;
    thickness = d->size.y;
    Collision c = bar(VEC_XY(d->pos), d->size.x, d->angle);
    if (c.isColliding.rect[CYAN] || c.isColliding.rect[LIGHT_CYAN]) {
      play(COIN);
      particle(d->pos.x, d->pos.y, 9, 1, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(d->pos));
      multiplier++;
      d->isAlive = false;
      continue;
    }
    if (d->pos.y > 170) {
      d->isAlive = false;
      continue;
    }
  }
  nextHoleDist -= scrollSpeed;
  if (nextHoleDist < 0) {
    ASSIGN_ARRAY_ITEM(holes, holeIndex, Hole, nh);
    vectorSet(&nh->pos, rnd(0, 100), -20);
    nh->size = rnd(5, 12);
    nh->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, CLEANER_MAX_HOLE_COUNT);
    nextHoleDist = rnd(99, 120);
  }
  FOR_EACH(holes, i) {
    ASSIGN_ARRAY_ITEM(holes, i, Hole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->pos.y += scrollSpeed;
    color = RED;
    thickness = 3;
    arc(VEC_XY(h->pos), h->size, 0, M_PI * 2);
    if (h->pos.y > 170) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLUE;
  thickness = 5;
  Collision c2 = arc(pos.x, pos.y + 9, 3, 0, M_PI * 2);
  if (c2.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameCleaner() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
