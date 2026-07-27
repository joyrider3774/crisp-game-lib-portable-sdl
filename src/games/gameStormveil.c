#include "cglp.h"
#include "string.h"

static char *title = "STORMVEIL";
static char *description = "[Tap] Switch Lane";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  float x;
  float y;
  float vx;
  float squash;
  float switchTimer;
} Player;
static Player player;

typedef struct {
  float x;
  float y;
  float phase;
  bool isAlive;
} Obstacle;
#define STORMVEIL_MAX_OBSTACLE_COUNT 64
static Obstacle obstacles[STORMVEIL_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  float x;
  float y;
  float pulse;
  bool isAlive;
} Coin;
#define STORMVEIL_MAX_COIN_COUNT 32
static Coin coins[STORMVEIL_MAX_COIN_COUNT];
static int coinIndex;

typedef struct {
  float x;
  float y;
  float alpha;
  bool isAlive;
} Trail;
#define STORMVEIL_MAX_TRAIL_COUNT 32
static Trail trails[STORMVEIL_MAX_TRAIL_COUNT];
static int trailIndex;

static int lane;
static float nextObstacleTicks;
static int multiplier;

static void update() {
  if (!ticks) {
    lane = 1;
    player.x = 50;
    player.y = 85;
    player.vx = 0;
    player.squash = 1;
    player.switchTimer = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    nextObstacleTicks = 0;
    multiplier = 1;
  }

  int laneX[3];
  laneX[0] = 25;
  laneX[1] = 50;
  laneX[2] = 75;
  float speed = 1.5 * sqrt(difficulty);
  float prevX = player.x;

  if (input.isJustPressed) {
    lane = wrap(lane + 1, 0, 3);
    play(JUMP);
    player.squash = 0.6;
    player.switchTimer = 10;
    color = LIGHT_BLACK;
    particle(player.x, player.y + 3, 5, 0.5, -M_PI_2, M_PI_4);
  }

  player.x += (laneX[lane] - player.x) * 0.5;
  player.vx = player.x - prevX;

  player.squash += (1 - player.squash) * 0.15;
  if (player.switchTimer > 0) {
    player.switchTimer--;
  }

  if (fabs(player.vx) > 0.5) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    nt->x = player.x;
    nt->y = player.y;
    nt->alpha = 0.5;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, STORMVEIL_MAX_TRAIL_COUNT);
  }

  nextObstacleTicks -= difficulty;
  if (nextObstacleTicks < 0) {
    nextObstacleTicks += 40;
    int blockedCount = (int)clamp(floor(rnd(1, sqrt(difficulty))), 1, 2);
    int lanesArr[3];
    lanesArr[0] = 0;
    lanesArr[1] = 1;
    lanesArr[2] = 2;
    int lanesCount = 3;
    int blocked[2];
    TIMES(blockedCount, bi) {
      int idx = (int)floor(rnd(0, lanesCount));
      blocked[bi] = lanesArr[idx];
      int k;
      for (k = idx; k < lanesCount - 1; k++) {
        lanesArr[k] = lanesArr[k + 1];
      }
      lanesCount--;
    }
    int li;
    for (li = 0; li < 3; li++) {
      bool isBlocked = false;
      int bj;
      for (bj = 0; bj < blockedCount; bj++) {
        if (blocked[bj] == li) {
          isBlocked = true;
        }
      }
      if (isBlocked) {
        ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
        no->x = laneX[li];
        no->y = -5;
        no->phase = rnd(0, M_PI * 2);
        no->isAlive = true;
        obstacleIndex = wrap(obstacleIndex + 1, 0, STORMVEIL_MAX_OBSTACLE_COUNT);
      }
    }
    if (lanesCount > 0) {
      ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, nc);
      nc->x = laneX[lanesArr[(int)floor(rnd(0, lanesCount))]];
      nc->y = -5;
      nc->pulse = 0;
      nc->isAlive = true;
      coinIndex = wrap(coinIndex + 1, 0, STORMVEIL_MAX_COIN_COUNT);
    }
  }

  color = LIGHT_BLACK;
  thickness = 1;
  int li2;
  for (li2 = 0; li2 < 3; li2++) {
    line(laneX[li2], 0, laneX[li2], 100);
  }

  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.1;
    if (t->alpha <= 0) {
      t->isAlive = false;
      continue;
    }
    color = LIGHT_CYAN;
    box(t->x, t->y, 5 * t->alpha, 5 * t->alpha);
  }

  color = CYAN;
  float breathe = 1 + sin(ticks * 0.1) * 0.05;
  float stretchX = player.squash * breathe;
  float stretchY = (2 - player.squash) * breathe;
  float tilt = -player.vx * 0.08;
  thickness = 6 * stretchX;
  bar(player.x, player.y, 6 * stretchY, tilt);

  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->y += speed;
    o->phase += 0.2;
    color = RED;
    float pulse = 1 + sin(o->phase) * 0.15;
    float sizeX = 6 * pulse;
    float sizeY = 6 * (2 - pulse);
    Collision oc = box(o->x, o->y, sizeX, sizeY);
    if (oc.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(player.x, player.y, 20, 2, 0, M_PI * 2);
      gameOver();
    }
    if (o->y > 105) {
      color = LIGHT_BLACK;
      particle(o->x, 100, 3, 0.3, M_PI_2, M_PI_4);
      o->isAlive = false;
      continue;
    }
  }

  FOR_EACH(coins, ci) {
    ASSIGN_ARRAY_ITEM(coins, ci, Coin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->y += speed;
    c->pulse += 0.12;
    float spinW = 2 + fabs(cos(c->pulse)) * 4;
    color = YELLOW;
    Collision cc = box(c->x, c->y, spinW, 6);
    if (cc.isColliding.rect[CYAN]) {
      addScore(multiplier, c->x, c->y);
      multiplier = (int)clamp(multiplier + 1, 1, 16);
      play(COIN);
      color = YELLOW;
      particle(c->x, c->y, 10, 1.5, 0, M_PI * 2);
      c->isAlive = false;
      continue;
    }
    if (c->y > 105) {
      multiplier = 1;
      c->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameStormveil() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
