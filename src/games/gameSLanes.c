#include "cglp.h"

static char *title = "S LANES";
static char *description = "[Hold]\n Shot & Forward";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "rllbb ",
        "lllccb",
        "LlyL b",
            "      ",
        "      ",
        "      ",
},
    {
        "  r rr",
        "rrRrRR",
        "  grr ",
        "  grr ",
        "rrRrRR",
        "  r rr",
    },
    {
        " LLLL ",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        "LyyyyL",
        " LLLL ",
    },
    {
        "l Llll",
        "l Llll",
            "      ",
        "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

#define SLANES_LANE_WIDTH 20
#define SLANES_LANE_COUNT 4
#define SLANES_SHIP_X 3

typedef struct {
  Vector pos;
  float vx;
  int laneIndex;
  bool isAlive;
} Enemy;
// Spawn interval is rnd(60,90)/difficulty (scales with difficulty directly) while enemy speed only
// scales as sqrt(difficulty) - concurrent count grows unboundedly (~12*sqrt(difficulty)), reachable
// within roughly the first 10 minutes of play.
#define SLANES_MAX_ENEMY_COUNT 256
static Enemy enemies[SLANES_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  int laneIndex;
  bool isAlive;
} Coin;
#define SLANES_MAX_COIN_COUNT 32
static Coin coins[SLANES_MAX_COIN_COUNT];
static int coinIndex;

typedef struct {
  Vector pos;
  float vx;
  bool isAlive;
} Shot;
#define SLANES_MAX_SHOT_COUNT 32
static Shot shots[SLANES_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  int laneIndex;
  float targetY;
  float laneTicks;
  float shotTicks;
} Ship;
static Ship ship;

static float laneSpeeds[SLANES_LANE_COUNT];
static int multiplier;

static float calcY(int i) {
  return i * SLANES_LANE_WIDTH + SLANES_LANE_WIDTH / 2.0 + 12;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    ship.pos.x = SLANES_SHIP_X;
    ship.pos.y = calcY(0);
    ship.laneIndex = 0;
    ship.targetY = calcY(0);
    ship.laneTicks = 0;
    ship.shotTicks = 0;
    TIMES(SLANES_LANE_COUNT, i) { laneSpeeds[i] = 1; }
    multiplier = 1;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    play(SELECT);
    int laneIndex = rndi(0, SLANES_LANE_COUNT);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, 103, calcY(laneIndex));
    ne->vx = -rnd(1, sqrt(difficulty)) * 0.2;
    ne->laneIndex = laneIndex;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, SLANES_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(60, 90) / difficulty;
  }
  color = BLACK;
  if (input.isPressed) {
    if (input.isJustPressed) {
      ship.pos.x = SLANES_SHIP_X;
    } else {
      ship.pos.x = clamp(ship.pos.x + sqrt(difficulty) * 0.3, 0, 50);
    }
    ship.pos.y = calcY(ship.laneIndex);
    ship.targetY = ship.pos.y;
    ship.shotTicks--;
    if (ship.shotTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
      vectorSet(&ns->pos, ship.pos.x, wrap(ship.pos.y, 0, 100));
      ns->vx = difficulty * 2;
      ns->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, SLANES_MAX_SHOT_COUNT);
      ship.shotTicks = 20 / sqrt(difficulty);
      ship.laneTicks = ship.shotTicks;
    }
  } else {
    play(HIT);
    ship.laneTicks--;
    if (ship.laneTicks < 0) {
      ship.laneIndex = (int)wrap(ship.laneIndex + 1, 0, SLANES_LANE_COUNT);
      float bonus;
      if (ship.laneIndex == 0) {
        bonus = 100;
      } else {
        bonus = 0;
      }
      ship.targetY = calcY(ship.laneIndex) + bonus;
      ship.pos.y = wrap(ship.pos.y, 0, 100);
      ship.laneTicks = 20 / sqrt(difficulty);
    }
    ship.pos.x += (SLANES_SHIP_X - ship.pos.x) * 0.3;
    ship.pos.y += (ship.targetY - ship.pos.y) * 0.3;
  }
  character("a", ship.pos.x, wrap(ship.pos.y, 0, 100));
  color = LIGHT_PURPLE;
  TIMES(SLANES_LANE_COUNT, i) {
    laneSpeeds[i] += (0.9 - laneSpeeds[i]) * 0.02;
    int n = (int)ceil(laneSpeeds[i]);
    if (n > 23) {
      n = 23;
    }
    char laneText[24];
    TIMES(n, k) { laneText[k] = '<'; }
    laneText[n] = 0;
    text(laneText, 9, calcY(i));
  }
  color = BLACK;
  float cvx = -difficulty * 0.2;
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, Coin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x += cvx;
    Collision cc = character("c", VEC_XY(c->pos));
    if (cc.isColliding.character['a']) {
      play(COIN);
      addScore(multiplier, VEC_XY(c->pos));
      multiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      play(EXPLOSION);
      laneSpeeds[c->laneIndex]++;
      if (multiplier > 1) {
        multiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x += s->vx;
    character("d", VEC_XY(s->pos));
  }
  color = BLACK;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x += e->vx * laneSpeeds[e->laneIndex];
    Collision ec = character("b", VEC_XY(e->pos));
    if (ec.isColliding.character['d']) {
      play(POWER_UP);
      particle(VEC_XY(e->pos), 16, 1, 0, M_PI * 2);
      ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, nc);
      nc->pos = e->pos;
      nc->laneIndex = e->laneIndex;
      nc->isAlive = true;
      coinIndex = wrap(coinIndex + 1, 0, SLANES_MAX_COIN_COUNT);
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < 9) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      color = RED;
      text("X", 3, e->pos.y);
      gameOver();
      color = BLACK;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    if (s->pos.x > 103) {
      s->isAlive = false;
      continue;
    }
    Collision sc = character("d", VEC_XY(s->pos));
    if (sc.isColliding.character['b']) {
      s->isAlive = false;
      continue;
    }
  }
}

void addGameSLanes() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
