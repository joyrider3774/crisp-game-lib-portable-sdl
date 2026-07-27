#include "cglp.h"

static char *title = "LEVITATION";
static char *description = "[Tap] Levitate / Fall";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " lll  ",
        "ll ll ",
        "ll ll ",
        "ll ll ",
        "ll ll ",
        "ll lll",
    },
    {
        " llll ",
        "ll  ll",
        "ll  ll",
        "ll  ll",
        "ll  ll",
        " llll ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 8, .isDarkColor = false};

#define LEVITATION_STATE_CRAWL 0
#define LEVITATION_STATE_ROLL 1

typedef struct {
  Vector pos;
  Vector vel;
  int state;
} LevitationCaterpillar;
static LevitationCaterpillar caterpillar;

typedef struct {
  Vector pos;
  float width;
  bool isAlive;
} LevitationPlatform;
#define LEVITATION_MAX_PLATFORM_COUNT 16
static LevitationPlatform platforms[LEVITATION_MAX_PLATFORM_COUNT];
static int platformIndex;
static float nextPlatformDist;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&caterpillar.pos, 40, 40);
    vectorSet(&caterpillar.vel, 1, 0);
    caterpillar.state = LEVITATION_STATE_CRAWL;
    INIT_UNALIVED_ARRAY(platforms);
    vectorSet(&platforms[0].pos, 0, 60);
    platforms[0].width = 99;
    platforms[0].isAlive = true;
    platformIndex = 1;
    nextPlatformDist = 0;
    multiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
    if (caterpillar.state == LEVITATION_STATE_CRAWL) {
      caterpillar.state = LEVITATION_STATE_ROLL;
    } else {
      caterpillar.state = LEVITATION_STATE_CRAWL;
    }
    multiplier = 1;
  }
  if (caterpillar.state == LEVITATION_STATE_CRAWL) {
    caterpillar.vel.x = -0.4 * difficulty;
    caterpillar.vel.y = 0;
  } else {
    caterpillar.vel.x = 0.2 * difficulty;
    caterpillar.vel.y += 0.2 * sqrt(difficulty);
  }
  caterpillar.vel.y *= 0.99;
  vectorAdd(&caterpillar.pos, caterpillar.vel.x, caterpillar.vel.y);
  if (caterpillar.pos.y < 0 && caterpillar.vel.y < 0) {
    caterpillar.pos.y = 0;
    caterpillar.vel.y *= -0.3;
  }
  float scrollSpeed = difficulty * 0.5;
  nextPlatformDist -= scrollSpeed;
  if (nextPlatformDist < 0) {
    float width = rnd(30, 50);
    ASSIGN_ARRAY_ITEM(platforms, platformIndex, LevitationPlatform, np);
    vectorSet(&np->pos, 100, rnd(30, 90));
    np->width = width;
    np->isAlive = true;
    platformIndex = wrap(platformIndex + 1, 0, LEVITATION_MAX_PLATFORM_COUNT);
    nextPlatformDist = width + rnd(0, 9);
  }
  if (caterpillar.pos.y > 100 || caterpillar.pos.x < -3 || caterpillar.pos.x > 103) {
    play(EXPLOSION);
    gameOver();
  }
  color = GREEN;
  char shape[2];
  if (caterpillar.state == LEVITATION_STATE_CRAWL) {
    shape[0] = 'b';
  } else {
    shape[0] = 'a';
  }
  shape[1] = 0;
  character(shape, VEC_XY(caterpillar.pos));
  color = LIGHT_BLACK;
  FOR_EACH(platforms, i) {
    ASSIGN_ARRAY_ITEM(platforms, i, LevitationPlatform, p);
    SKIP_IS_NOT_ALIVE(p);
    Collision scratch = rect(p->pos.x, p->pos.y, p->width, 5);
    if (scratch.isColliding.character['a'] && caterpillar.vel.y >= 0) {
      play(JUMP);
      addScore(multiplier, VEC_XY(caterpillar.pos));
      multiplier++;
      caterpillar.pos.y = p->pos.y - 3;
      caterpillar.vel.y *= -1.5;
    }
    p->pos.x -= scrollSpeed;
    if (p->pos.x + p->width < 0) {
      p->isAlive = false;
    }
  }
}

void addGameLevitation() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
