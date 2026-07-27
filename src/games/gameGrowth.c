#include "cglp.h"

static char *title = "GROWTH";
static char *description = "[Hold] Growth";

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
    .viewSizeX = 200, .viewSizeY = 70, .soundSeed = 30, .isDarkColor = false};

#define GROWTH_FLOOR_Y 60

typedef struct {
  float x;
  float vx;
  float size;
} Player;
static Player player;

typedef struct {
  float x;
  float size;
  bool isAlive;
} Enemy;
#define GROWTH_MAX_ENEMY_COUNT 32
static Enemy enemies[GROWTH_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;

static void update() {
  if (!ticks) {
    player.x = 9;
    player.vx = 1;
    player.size = 5;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 0;
  }
  float scr;
  if (player.x > 9) {
    scr = (player.x - 9) * 0.5;
  } else {
    scr = 0;
  }
  color = LIGHT_BLUE;
  rect(0, GROWTH_FLOOR_Y, 200, 10);
  if (input.isJustPressed) {
    play(LASER);
  }
  float targetSize;
  if (input.isPressed) {
    targetSize = 50;
  } else {
    targetSize = 5;
  }
  player.size += (targetSize - player.size) * clamp(player.vx, 1, 999) * 0.01;
  player.vx += (15 / player.size - 1) * 0.02 * sqrt(difficulty);
  player.x += player.vx - scr;
  if (player.x + player.size / 2 < 1) {
    gameOver();
  }
  color = YELLOW;
  rect(0, GROWTH_FLOOR_Y, player.x + player.size / 2, -player.size);
  nextEnemyDist -= scr;
  if (nextEnemyDist < 0) {
    float size;
    if (rnd(0, 1) < 0.8) {
      size = 3;
    } else {
      size = rnd(0, 5) * rnd(0, 5) + 3;
    }
    if (size < 7) {
      size = 3;
    }
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, e);
    e->x = 400;
    e->size = size;
    e->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, GROWTH_MAX_ENEMY_COUNT);
    nextEnemyDist += rnd(30, 50);
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    if (e->size > player.size) {
      color = RED;
    } else {
      color = CYAN;
    }
    float sc;
    if (e->x > 100) {
      sc = (e->x - 100) / 300 + 1;
    } else {
      sc = 1;
    }
    float sz = e->size / sc;
    if (rect(e->x / sc, GROWTH_FLOOR_Y, sz, -sz).isColliding.rect[YELLOW]) {
      if (e->size > player.size) {
        play(EXPLOSION);
        gameOver();
      } else {
        if (e->size < 5) {
          play(HIT);
        } else {
          play(POWER_UP);
        }
        float ss = sqrt(e->size);
        particle(e->x, GROWTH_FLOOR_Y - e->size / 2, ss, ss, 0, M_PI_2);
        addScore(ceil(clamp(player.vx, 1, 999) * e->size), e->x,
                 GROWTH_FLOOR_Y - player.size);
      }
      e->isAlive = false;
      continue;
    }
  }
}

void addGameGrowth() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
