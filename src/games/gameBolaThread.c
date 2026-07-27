#include "cglp.h"

static char *title = "BOLA THREAD";
static char *description = "[Hold] Contract\n[Release] Expand";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

static Vector playerPos;
static Vector balls[2];
static float ang;
static float r;
static float nextWallX;
static float nextBeaconT;
static int hitstop;
static float shakeMag;
static int shakeT;

typedef struct {
  Vector pos;
  float gapY;
  float gapH;
  bool nub;
  bool passed;
  bool isAlive;
} Obstacle;
#define BOLATHREAD_MAX_OBSTACLE_COUNT 20
static Obstacle obstacles[BOLATHREAD_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  Vector pos;
  bool isAlive;
} Item;
#define BOLATHREAD_MAX_ITEM_COUNT 12
static Item items[BOLATHREAD_MAX_ITEM_COUNT];
static int itemIndex;

static void update() {
  if (!ticks) {
    vectorSet(&playerPos, 30, 50);
    vectorSet(&balls[0], 0, 0);
    vectorSet(&balls[1], 0, 0);
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    INIT_UNALIVED_ARRAY(items);
    itemIndex = 0;
    ang = 0;
    r = 20;
    nextWallX = 60;
    nextBeaconT = 100;
    hitstop = 0;
    shakeMag = 0;
    shakeT = 0;
  }

  bool frozen = hitstop > 0;
  if (frozen) {
    hitstop--;
  }

  float spd = 0.45 + difficulty * 0.1;
  float dAngNow = 0.02 + difficulty * 0.002;
  float dr = 0;
  float dAng = 0;
  if (!frozen) {
    float prevR = r;
    if (input.isPressed) {
      r = fmax(6, r - 0.6);
    } else {
      r = fmin(26, r + 0.45);
    }
    dr = r - prevR;
    dAng = dAngNow;
    ang += dAng;
  }

  balls[0].x = 30 + cos(ang) * r;
  balls[0].y = 50 + sin(ang) * r;
  balls[1].x = 30 - cos(ang) * r;
  balls[1].y = 50 - sin(ang) * r;

  if (!frozen) {
    nextWallX -= spd;
    if (nextWallX < 0) {
      bool nub = rnd(0, 1) < 0.36;
      float ticksToArrival = (108 - 30) / spd;
      float predictedAng = ang + dAngNow * ticksToArrival;
      bool nubSafe = fabs(sin(predictedAng)) > 0.8;
      bool finalNub = nub && nubSafe;
      ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
      vectorSet(&no->pos, 108, 50);
      no->gapY = rnd(42, 58);
      if (finalNub) {
        no->gapH = rnd(42, 54);
      } else {
        no->gapH = rnd(25, 40);
      }
      no->nub = finalNub;
      no->passed = false;
      no->isAlive = true;
      obstacleIndex = wrap(obstacleIndex + 1, 0, BOLATHREAD_MAX_OBSTACLE_COUNT);
      nextWallX = rnd(65, 90);
    }
    nextBeaconT--;
    if (nextBeaconT < 0) {
      ASSIGN_ARRAY_ITEM(items, itemIndex, Item, ni);
      vectorSet(&ni->pos, 108, rnd(20, 80));
      ni->isAlive = true;
      itemIndex = wrap(itemIndex + 1, 0, BOLATHREAD_MAX_ITEM_COUNT);
      nextBeaconT = rnd(90, 150);
    }
  }

  float shakeX = 0;
  float shakeY = 0;
  if (shakeT > 0) {
    shakeT--;
    float m = shakeMag * (shakeT / 8.0);
    shakeX = rnd(-m, m);
    shakeY = rnd(-m, m);
  }

  color = LIGHT_BLACK;
  thickness = 1;
  line(balls[0].x + shakeX, balls[0].y + shakeY, balls[1].x + shakeX,
       balls[1].y + shakeY);
  color = LIGHT_BLUE;
  box(playerPos.x + shakeX, playerPos.y + shakeY, 3, 3);

  float tvx = dr * cos(ang) - r * dAng * sin(ang);
  float tvy = dr * sin(ang) + r * dAng * cos(ang);
  if (tvx * tvx + tvy * tvy > 1) {
    color = LIGHT_CYAN;
    box(balls[0].x - tvx * 1.4, balls[0].y - tvy * 1.4, 3, 3);
    box(balls[1].x + tvx * 1.4, balls[1].y + tvy * 1.4, 3, 3);
    color = LIGHT_BLACK;
    box(balls[0].x - tvx * 2.8, balls[0].y - tvy * 2.8, 2, 2);
    box(balls[1].x + tvx * 2.8, balls[1].y + tvy * 2.8, 2, 2);
  }

  color = CYAN;
  box(balls[0].x, balls[0].y, 4, 4);
  box(balls[1].x, balls[1].y, 4, 4);

  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    if (!frozen) {
      o->pos.x -= spd;
    }
    float x = o->pos.x;
    color = RED;
    Collision top = rect(x - 2, 0, 4, o->gapY - o->gapH / 2);
    Collision bot = rect(x - 2, o->gapY + o->gapH / 2, 4, 100 - o->gapY - o->gapH / 2);
    bool hit = top.isColliding.rect[CYAN] || bot.isColliding.rect[CYAN];
    if (o->nub) {
      Collision nb = box(x, 50, 4, 8);
      if (nb.isColliding.rect[CYAN]) {
        hit = true;
      }
    }
    if (hit) {
      play(EXPLOSION);
      particle(x, 50, 25, 3, 0, M_PI * 2);
      gameOver();
    }
    if (!o->passed && x < 30) {
      o->passed = true;
      int bonus = (int)fmax(floor((r - 6) / 3), 1);
      addScore(bonus, x, o->gapY);
      play(SELECT);
      int tier;
      if (bonus >= 5) {
        tier = 2;
      } else if (bonus >= 3) {
        tier = 1;
      } else {
        tier = 0;
      }
      hitstop = (int)fmax(hitstop, clamp(2 + tier * 2, 2, 6));
      shakeMag = fmax(shakeMag, clamp(1 + tier, 1, 3));
      shakeT = 8;
      if (tier == 2) {
        color = YELLOW;
      } else if (tier == 1) {
        color = CYAN;
      } else {
        color = LIGHT_CYAN;
      }
      particle(x, o->gapY, 8 + tier * 6, 1.5 + tier * 0.5, 0, M_PI * 2);
    }
    o->isAlive = x >= -5;
  }

  FOR_EACH(items, ii) {
    ASSIGN_ARRAY_ITEM(items, ii, Item, g);
    SKIP_IS_NOT_ALIVE(g);
    if (!frozen) {
      g->pos.x -= spd;
    }
    color = YELLOW;
    Collision c = box(g->pos.x, g->pos.y, 5, 5);
    if (c.isColliding.rect[CYAN]) {
      addScore(8, g->pos.x, g->pos.y);
      play(COIN);
      particle(g->pos.x, g->pos.y, 12, 2, 0, M_PI * 2);
      g->isAlive = false;
      continue;
    }
    g->isAlive = g->pos.x >= -5;
  }
}

void addGameBolaThread() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
