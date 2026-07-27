#include "cglp.h"

static char *title = "WIND POWER";
static char *description = "[Hold] Blow air";

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

// Port note: upstream initializes `player` (a Vector) but never draws or
// reads it anywhere else in update() - a vestigial leftover in the original
// source (dead code, not a translation gap). Kept here, initialized but
// otherwise unused, to stay a faithful line-for-line port rather than
// second-guessing upstream.
static Vector player;

typedef struct {
  Vector pos;
  float speed;
  float angle;
} Windmill;
static Windmill playerWindmill;
static Windmill windmill;

typedef struct {
  Vector pos;
  float size;
  float vx;
  bool isAlive;
} Obstacle;
#define WINDPOWER_MAX_OBSTACLE_COUNT 16
static Obstacle obstacles[WINDPOWER_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float nextObstacleTicks;
static float windPower;

static void update() {
  if (!ticks) {
    vectorSet(&player, 0, 50);
    vectorSet(&playerWindmill.pos, 10, 60);
    playerWindmill.speed = 0;
    playerWindmill.angle = 0;
    vectorSet(&windmill.pos, 85, 60);
    windmill.speed = 0;
    windmill.angle = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    windPower = 0;
  }
  if (input.isPressed) {
    windPower += difficulty * 0.02;
    windPower = clamp(windPower, 0, difficulty);
  } else {
    windPower *= 0.9;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isJustReleased) {
    play(CLICK);
  }
  playerWindmill.speed = windPower * 2;
  playerWindmill.angle += playerWindmill.speed * 0.2;
  color = CYAN;
  box(playerWindmill.pos.x, playerWindmill.pos.y, 5,
      sin(playerWindmill.angle) * 20);
  windmill.speed += windPower * 0.02;
  windmill.speed = fmin(windmill.speed, 10);
  windmill.speed *= 0.98;
  windmill.angle += windmill.speed * 0.2;
  color = LIGHT_BLACK;
  thickness = 4;
  barCenterPosRatio = 0;
  TIMES(3, i) {
    bar(windmill.pos.x, windmill.pos.y, 10,
        windmill.angle + M_PI * 2 / 3 * i);
  }
  if (windmill.angle > M_PI * 2) {
    play(COIN);
    windmill.angle = fmod(windmill.angle, M_PI * 2);
  }
  addScore(windmill.speed, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  nextObstacleTicks -= difficulty;
  if (nextObstacleTicks < 0) {
    play(HIT);
    float size = rnd(5, 10);
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, rnd(20, 99), -size);
    no->size = size;
    no->vx = 0;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, WINDPOWER_MAX_OBSTACLE_COUNT);
    nextObstacleTicks = rnd(75, 250);
  }
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.y += difficulty * 0.5;
    if (o->pos.y > 20 && o->pos.y < 100) {
      o->vx += windPower / o->size;
    }
    o->vx *= 0.9;
    o->pos.x += o->vx;
    color = BLACK;
    box(o->pos.x, o->pos.y, o->size, o->size);
    // Port note: upstream really does call box() twice with identical
    // arguments here - once to draw, once more just to read its collision
    // result. Kept as two separate calls (not deduped to one) to stay a
    // faithful port: this engine's collision detection uses draw order as
    // part of its semantics, so the duplicate registration is part of the
    // original observable behavior, not just an accident of translation.
    Collision oc = box(o->pos.x, o->pos.y, o->size, o->size);
    if (oc.isColliding.rect[LIGHT_BLACK]) {
      play(EXPLOSION);
      gameOver();
    }
    if (o->pos.y > 100) {
      o->isAlive = false;
      continue;
    }
  }
}

void addGameWindPower() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
