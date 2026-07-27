#include "cglp.h"

static char *title = "FALL BOUNCE";
static char *description = "[Hold]\n Fall & Bounce";

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

#define FALLBOUNCE_PLATFORM_WIDTH 15

typedef struct {
  Vector pos;
  float width;
  bool isAlive;
} FallbouncePlatform;
#define FALLBOUNCE_MAX_PLATFORM_COUNT 16
static FallbouncePlatform fallbouncePlatforms[FALLBOUNCE_MAX_PLATFORM_COUNT];
static int fallbouncePlatformIndex;
static float fallbounceNextPlatformTicks;

typedef struct {
  Vector pos;
  float vx;
  float width;
  bool hasTargetY;
  float targetY;
} FallbounceWhale;
static FallbounceWhale fallbounceWhale;

static void fallbounceSpawnPlatform() {
  ASSIGN_ARRAY_ITEM(fallbouncePlatforms, fallbouncePlatformIndex, FallbouncePlatform, p);
  vectorSet(&p->pos, rnd(0, 100 - FALLBOUNCE_PLATFORM_WIDTH), -5);
  p->width = rnd(FALLBOUNCE_PLATFORM_WIDTH, 35);
  p->isAlive = true;
  fallbouncePlatformIndex =
      wrap(fallbouncePlatformIndex + 1, 0, FALLBOUNCE_MAX_PLATFORM_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(fallbouncePlatforms);
    fallbouncePlatformIndex = 0;
    TIMES(2, i) { fallbounceSpawnPlatform(); }
    vectorSet(&fallbounceWhale.pos, 50, 10);
    fallbounceWhale.vx = 1;
    fallbounceWhale.width = 10;
    fallbounceWhale.hasTargetY = false;
    fallbounceNextPlatformTicks = 0;
  }
  if (fallbounceWhale.hasTargetY) {
    fallbounceWhale.pos.y += (fallbounceWhale.targetY - fallbounceWhale.pos.y) * 0.2;
    if (fabs(fallbounceWhale.targetY - fallbounceWhale.pos.y) < 1) {
      fallbounceWhale.hasTargetY = false;
    }
  } else {
    float fallSpeed;
    if (input.isPressed) {
      fallSpeed = difficulty * 1.5;
    } else {
      fallSpeed = difficulty * 0.25;
    }
    fallbounceWhale.pos.y += fallSpeed;
  }
  fallbounceWhale.pos.x = wrap(fallbounceWhale.pos.x + fallbounceWhale.vx, 0, 100);
  if (input.isPressed) {
    color = CYAN;
  } else {
    color = LIGHT_BLACK;
  }
  box(fallbounceWhale.pos.x, fallbounceWhale.pos.y, fallbounceWhale.width, 3);
  if (fallbounceWhale.pos.y > 103) {
    play(HIT);
    gameOver();
  }
  fallbounceNextPlatformTicks--;
  if (fallbounceNextPlatformTicks < 0) {
    TIMES(2, i) { fallbounceSpawnPlatform(); }
    fallbounceNextPlatformTicks = ceil(rnd(40, 60) / difficulty);
  }
  FOR_EACH(fallbouncePlatforms, i) {
    ASSIGN_ARRAY_ITEM(fallbouncePlatforms, i, FallbouncePlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += difficulty * 1;
    color = BLUE;
    Collision c = box(VEC_XY(p->pos), p->width, 5);
    bool isLanded = c.isColliding.rect[CYAN] && !fallbounceWhale.hasTargetY;
    if (isLanded) {
      play(POWER_UP);
      addScore(fallbounceWhale.pos.y - 10, p->pos.x, p->pos.y);
      fallbounceWhale.targetY = 10;
      fallbounceWhale.hasTargetY = true;
    }
    if (p->pos.y >= 103) {
      p->isAlive = false;
      continue;
    }
  }
}

void addGameFallBounce() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
