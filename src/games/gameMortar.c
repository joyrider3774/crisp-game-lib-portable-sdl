#include "cglp.h"

static char *title = "MORTAR";
static char *description = "[Hold]\n Adjust distance\n[Release]\n Fire";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  ll  ",
        "  ll  ",
        "l ll l",
        "llllll",
        "ll  ll",
    },
    {
        "llllll",
        " l  l ",
        " l  l ",
        "llllll",
        "  ll  ",
        "  ll  ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} MortarEnemy;
#define MORTAR_MAX_ENEMY_COUNT 64
static MortarEnemy enemies[MORTAR_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;
static float maxEnemyY;

typedef struct {
  Vector pos;
  float vx;
  bool hasSight;
  float sightY;
} MortarCannon;
static MortarCannon cannon;

typedef struct {
  Vector pos;
  bool active;
  float width;
} MortarShot;
static MortarShot shot;

typedef struct {
  Vector pos;
  bool active;
  float targetRadius;
  float radius;
} MortarExplosion;
static MortarExplosion explosion;

static float oy;
static int endCount;
static int multiplier;
static float sightSpeedRatio;

#define MORTAR_GROUND_COUNT 99
static Vector grounds[MORTAR_GROUND_COUNT];

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, MortarEnemy, ne);
      vectorSet(&ne->pos, rnd(49, 60), rnd(-149, -140));
      ne->vy = 0.1;
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, MORTAR_MAX_ENEMY_COUNT);
    }
    nextEnemyTicks = 0;
    maxEnemyY = 0;
    vectorSet(&cannon.pos, 30, 95);
    cannon.vx = 1;
    cannon.hasSight = false;
    cannon.sightY = 0;
    shot.active = false;
    vectorSet(&shot.pos, 0, 0);
    shot.width = 0;
    explosion.active = false;
    vectorSet(&explosion.pos, 0, 0);
    oy = 0;
    endCount = 0;
    multiplier = 1;
    sightSpeedRatio = 3;
    TIMES(MORTAR_GROUND_COUNT, i) { vectorSet(&grounds[i], rnd(0, 99), rnd(-300, 100)); }
  }
  color = LIGHT_BLACK;
  TIMES(MORTAR_GROUND_COUNT, i) { box(grounds[i].x, grounds[i].y + oy, 3, 1); }
  if (explosion.active) {
    color = RED;
    explosion.radius += (explosion.targetRadius - explosion.radius) * 0.1;
    thickness = 5;
    arc(explosion.pos.x, explosion.pos.y + oy, explosion.radius, 0, M_PI * 2);
    if (explosion.targetRadius - explosion.radius < 1) {
      explosion.active = false;
    }
  } else if (cannon.hasSight) {
    cannon.sightY -= sqrt(difficulty) * 2 * sightSpeedRatio;
    float radius = 0;
    if (cannon.sightY < 0) {
      oy += (90 - cannon.sightY - oy) * (0.05 * sqrt(difficulty) * sightSpeedRatio);
      radius = clamp(-cannon.sightY * 0.3, 0, 30);
    }
    radius += 2;
    color = BLACK;
    thickness = 2;
    arc(cannon.pos.x, cannon.sightY + oy, radius, 0, M_PI * 2);
    if (input.isJustReleased || cannon.sightY < -200) {
      if (radius == 2) {
        if (!shot.active) {
          play(LASER);
          shot.pos = cannon.pos;
          shot.width = ceil((91 - cannon.sightY) * 0.2);
          shot.active = true;
        }
      } else {
        play(EXPLOSION);
        explosion.targetRadius = radius;
        explosion.radius = 0;
        vectorSet(&explosion.pos, cannon.pos.x, cannon.sightY);
        explosion.active = true;
      }
      cannon.hasSight = false;
      if (sightSpeedRatio > 1) {
        sightSpeedRatio--;
      }
    }
  } else {
    oy *= 0.8;
    cannon.pos.x += cannon.vx * difficulty;
    if ((cannon.pos.x < 3 && cannon.vx < 0) || (cannon.pos.x > 96 && cannon.vx > 0)) {
      cannon.vx *= -1;
    }
    if (input.isJustPressed) {
      play(SELECT);
      cannon.hasSight = true;
      cannon.sightY = 90;
      multiplier = 1;
    }
  }
  if (shot.active) {
    shot.pos.y -= sqrt(difficulty) * 3;
    color = RED;
    box(shot.pos.x, shot.pos.y + oy, shot.width, 6);
    if (shot.pos.y < -3) {
      shot.active = false;
    }
  }
  if (endCount > 0) {
    oy *= 0.5;
    if (endCount > 9) {
      gameOver();
    }
  }
  color = BLACK;
  character("a", cannon.pos.x, cannon.pos.y + oy);
  COUNT_IS_ALIVE(enemies, aliveEnemyCount0);
  if (aliveEnemyCount0 == 0) {
    nextEnemyTicks = 0;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    float vy = rnd(0.5, sqrt(difficulty) * 2) * 0.1;
    float x = rnd(20, 80);
    int spawnCount = rndi(5, 9);
    TIMES(spawnCount, k) {
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, MortarEnemy, ne);
      vectorSet(&ne->pos, x + rnd(0, 9) * RNDPM(), -280 + rnd(0, 9) * RNDPM());
      ne->vy = vy * rnd(0.9, 1.1);
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, MORTAR_MAX_ENEMY_COUNT);
    }
    COUNT_IS_ALIVE(enemies, aliveEnemyCountAfterSpawn);
    nextEnemyTicks = (99 * sqrt(aliveEnemyCountAfterSpawn)) / difficulty;
  }
  color = RED;
  float my = -200;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, MortarEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    Collision ec = character("b", e->pos.x, e->pos.y + oy);
    if (e->pos.y > 99) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      text("X", e->pos.x, 97);
      endCount++;
    } else {
      float vy = (e->vy * sqrt(120 - maxEnemyY)) / 4;
      if (cannon.hasSight) {
        vy *= 0.3;
      }
      e->pos.y += vy;
      if (ec.isColliding.rect[RED]) {
        play(POWER_UP);
        addScore(multiplier, e->pos.x, e->pos.y + oy);
        multiplier++;
        e->isAlive = false;
        continue;
      }
      if (e->pos.y > my) {
        my = e->pos.y;
      }
    }
  }
  maxEnemyY = my;
}

void addGameMortar() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
