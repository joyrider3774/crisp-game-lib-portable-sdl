#include "cglp.h"

static char *title = "ELASTIC HERO";
static char *description = "[Hold] Stretch & Aim\n[Release] Launch";

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
    .viewSizeX = 100, .viewSizeY = 150, .soundSeed = 4, .isDarkColor = false};

#define ELASTICHERO_ARENA_WIDTH 100
#define ELASTICHERO_ARENA_HEIGHT 150
#define ELASTICHERO_GRAVITY 0.1
#define ELASTICHERO_MAX_STRETCH 20

typedef struct {
  Vector pos;
  Vector vel;
  float angle;
  float stretchLength;
} ElastichHero;
static ElastichHero elastichHero;

typedef struct {
  Vector pos;
  float width;
  float height;
} ElastichPlatform;
#define ELASTICHERO_MAX_PLATFORM_COUNT 8
static ElastichPlatform elastichPlatforms[ELASTICHERO_MAX_PLATFORM_COUNT];
static int elastichPlatformCount;

static float elastichWaterY;

static void elastichGeneratePlatforms() {
  elastichPlatformCount = 0;
  for (float y = ELASTICHERO_ARENA_HEIGHT - 40; y > 20; y -= 20) {
    float width = rnd(20, 30);
    elastichPlatforms[elastichPlatformCount].pos.x =
        rnd(width / 2, ELASTICHERO_ARENA_WIDTH - width / 2);
    elastichPlatforms[elastichPlatformCount].pos.y = y;
    elastichPlatforms[elastichPlatformCount].width = width;
    elastichPlatforms[elastichPlatformCount].height = 7;
    elastichPlatformCount++;
  }
}

static void elastichDrawHero() {
  color = GREEN;
  thickness = 3;
  arc(elastichHero.pos.x, elastichHero.pos.y, 5, 0, M_PI * 2);
  if (elastichHero.stretchLength > 0) {
    color = BLACK;
    Vector tip;
    vectorSet(&tip, elastichHero.stretchLength, 0);
    rotate(&tip, elastichHero.angle);
    vectorAdd(&tip, elastichHero.pos.x, elastichHero.pos.y);
    thickness = 3;
    line(elastichHero.pos.x, elastichHero.pos.y, tip.x, tip.y);
  }
}

static void elastichDrawWalls() {
  color = LIGHT_BLACK;
  rect(0, 0, 3, ELASTICHERO_ARENA_HEIGHT);
  rect(ELASTICHERO_ARENA_WIDTH - 3, 0, 3, ELASTICHERO_ARENA_HEIGHT);
  rect(0, ELASTICHERO_ARENA_HEIGHT - 3, ELASTICHERO_ARENA_WIDTH, 3);
}

static void update() {
  if (!ticks) {
    vectorSet(&elastichHero.pos, ELASTICHERO_ARENA_WIDTH / 2.0, ELASTICHERO_ARENA_HEIGHT - 20);
    vectorSet(&elastichHero.vel, 0, -1);
    elastichHero.angle = 0;
    elastichHero.stretchLength = 0;
    elastichGeneratePlatforms();
    elastichWaterY = ELASTICHERO_ARENA_HEIGHT;
  }
  vectorAdd(&elastichHero.pos, elastichHero.vel.x, elastichHero.vel.y);
  elastichHero.vel.y += ELASTICHERO_GRAVITY;
  float velMul;
  if (elastichHero.pos.y < elastichWaterY) {
    velMul = 0.99;
  } else {
    velMul = 0.9;
  }
  vectorMul(&elastichHero.vel, velMul);
  if (input.isPressed) {
    float sl = elastichHero.stretchLength + 1;
    if (sl > ELASTICHERO_MAX_STRETCH) {
      sl = ELASTICHERO_MAX_STRETCH;
    }
    elastichHero.stretchLength = sl;
    elastichHero.angle = wrap(elastichHero.angle + 0.1, 0, M_PI * 2);
  } else if (input.isJustReleased) {
    play(POWER_UP);
    vectorSet(&elastichHero.vel, elastichHero.stretchLength * 0.5, 0);
    rotate(&elastichHero.vel, elastichHero.angle);
    elastichHero.stretchLength = 0;
  } else {
    elastichHero.stretchLength = 0;
  }
  if ((elastichHero.pos.x < 3 && elastichHero.vel.x < 0) ||
      (elastichHero.pos.x > ELASTICHERO_ARENA_WIDTH - 3 && elastichHero.vel.x > 0)) {
    elastichHero.vel.x *= -0.8;
  }
  if (elastichHero.pos.y < 3) {
    elastichHero.pos.y += 150;
    addScore(floor(elastichWaterY), elastichHero.pos.x, elastichHero.pos.y);
    elastichWaterY += ELASTICHERO_ARENA_HEIGHT;
    elastichGeneratePlatforms();
    play(COIN);
  }
  if (elastichHero.pos.y > ELASTICHERO_ARENA_HEIGHT - 3 && elastichHero.vel.y > 0) {
    elastichHero.vel.y *= -0.8;
  }
  if (elastichWaterY > 140) {
    elastichWaterY += (140 - elastichWaterY) * 0.05;
  }
  elastichWaterY -= difficulty * 0.1;
  color = LIGHT_CYAN;
  rect(0, elastichWaterY, ELASTICHERO_ARENA_WIDTH, ELASTICHERO_ARENA_HEIGHT - elastichWaterY);
  if (elastichWaterY < 0) {
    play(EXPLOSION);
    gameOver();
  }
  elastichDrawHero();
  elastichDrawWalls();
  color = BLUE;
  TIMES(elastichPlatformCount, i) {
    ElastichPlatform *platform = &elastichPlatforms[i];
    Collision c = box(VEC_XY(platform->pos), platform->width, platform->height);
    if (c.isColliding.rect[GREEN]) {
      if (elastichHero.vel.y > 0 && elastichHero.pos.y < platform->pos.y) {
        elastichHero.vel.y *= -0.8;
        elastichHero.pos.y = platform->pos.y - platform->height / 2 - 3;
        play(CLICK);
      } else if (elastichHero.vel.y < 0 && elastichHero.pos.y > platform->pos.y) {
        elastichHero.vel.y *= -0.8;
        elastichHero.pos.y = platform->pos.y + platform->height / 2 + 3;
        play(CLICK);
      }
    }
  }
}

void addGameElasticHero() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
