#include "cglp.h"

static char *title = "DARK SORT";
static char *description = "[Hold] Light gate";

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
  float x;
  float y;
  float vx;
  bool isLight;
} Gate;
static Gate gate;

typedef struct {
  float x;
  float y;
  bool isWhite;
  float speed;
  bool isAlive;
} Orb;
// Spawns every 8-30 ticks (shrinks toward 8 as difficulty rises), 1-2 orbs
// per spawn, each falling across ~105px at 0.8*sqrt(difficulty) px/tick -
// worked out to roughly 5 concurrent orbs at worst, sized generously above.
#define DARKSORT_MAX_ORB_COUNT 32
static Orb orbs[DARKSORT_MAX_ORB_COUNT];
static int orbIndex;

static int spawnTimer;
static float visionRadius;
static int combo;

static void update() {
  // Never reads a Collision result - catches are decided by distance math, not hitboxes.
  hasCollision = false;
  if (!ticks) {
    gate.x = 50;
    gate.y = 92;
    gate.vx = 1;
    gate.isLight = false;
    INIT_UNALIVED_ARRAY(orbs);
    orbIndex = 0;
    spawnTimer = 0;
    visionRadius = 55;
    combo = 0;
  }

  gate.isLight = input.isPressed;

  spawnTimer--;
  if (spawnTimer <= 0) {
    int spawnInterval = 30 - (int)floor(difficulty);
    if (spawnInterval < 8) {
      spawnInterval = 8;
    }
    spawnTimer = spawnInterval;

    int numOrbs = 1;
    if (difficulty > 5) {
      if (rnd(0, 1) < 0.3) {
        numOrbs = 2;
      } else {
        numOrbs = 1;
      }
    }

    TIMES(numOrbs, i) {
      ASSIGN_ARRAY_ITEM(orbs, orbIndex, Orb, o);
      o->x = rnd(12, 88);
      o->y = -5 - i * 15;
      o->isWhite = rnd(0, 1) < 0.5;
      o->speed = 0.8 * sqrt(difficulty);
      o->isAlive = true;
      orbIndex = wrap(orbIndex + 1, 0, DARKSORT_MAX_ORB_COUNT);
    }
  }

  color = LIGHT_BLACK;
  rect(0, 0, 100, 100);

  color = WHITE;
  thickness = 6;
  arc(gate.x, gate.y - 25, visionRadius, 0, M_PI * 2);

  gate.x += gate.vx * sqrt(difficulty);
  if ((gate.x > 90 && gate.vx > 0) ||
      (gate.x < 10 && gate.vx < 0)) {
    gate.vx *= -1;
  }
  if (gate.isLight) {
    color = WHITE;
  } else {
    color = BLACK;
  }
  box(gate.x, gate.y, 24, 5);

  bool shouldEnd = false;

  FOR_EACH(orbs, i) {
    ASSIGN_ARRAY_ITEM(orbs, i, Orb, o);
    SKIP_IS_NOT_ALIVE(o);
    o->y += o->speed;

    float dx = o->x - gate.x;
    float dy = o->y - (gate.y - 25);
    bool inVision = sqrt(dx * dx + dy * dy) < visionRadius;

    if (inVision) {
      if (o->isWhite) {
        color = WHITE;
      } else {
        color = BLACK;
      }
      box(o->x, o->y, 6, 6);
    }

    if (o->y >= gate.y - 2 && o->y <= gate.y + 3 &&
        fabs(o->x - gate.x) < 14) {
      if ((o->isWhite && gate.isLight) ||
          (!o->isWhite && !gate.isLight)) {
        play(COIN);
        combo++;
        addScore(combo, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        o->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        shouldEnd = true;
      }
    }

    if (o->y > 100) {
      play(HIT);
      combo = 0;
      o->isAlive = false;
      continue;
    }
  }

  if (shouldEnd) {
    gameOver();
  }
}

void addGameDarkSort() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
