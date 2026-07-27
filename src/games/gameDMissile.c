#include "cglp.h"
#include "string.h"

static char *title = "D MISSILE";
static char *description = "[Tap]  Launch\n[Hold] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define DMISSILE_INIT_ANGLE ((-M_PI / 9) * 8)

typedef struct {
  Vector pos;
  Vector vel;
  Vector from;
  int ticksLeft;
  int id;
  bool isRemoving;
  bool isAlive;
} Enemy;
#define DMISSILE_MAX_ENEMY_COUNT 64
static Enemy enemies[DMISSILE_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;
static int enemyId;

typedef struct {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
} Player;
#define DMISSILE_MAX_PLAYER_COUNT 8
static Player players[DMISSILE_MAX_PLAYER_COUNT];
static int playerIndex;
static float tapTicks;
static float multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    enemyId = 0;
    INIT_UNALIVED_ARRAY(players);
    playerIndex = 0;
    tapTicks = 0;
    multiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
    COUNT_IS_ALIVE(players, alivePlayerCount);
    if (alivePlayerCount == 0) {
      tapTicks = 99;
    } else if (alivePlayerCount >= 3) {
      tapTicks = 0;
    } else {
      tapTicks = 9;
    }
  }
  tapTicks--;
  if (input.isJustReleased && tapTicks > 0) {
    play(COIN);
    ASSIGN_ARRAY_ITEM(players, playerIndex, Player, np);
    vectorSet(&np->pos, 60, 90);
    np->angle = DMISSILE_INIT_ANGLE;
    np->speed = 0.5 * sqrt(difficulty);
    np->isAlive = true;
    playerIndex = wrap(playerIndex + 1, 0, DMISSILE_MAX_PLAYER_COUNT);
    multiplier *= 0.9;
    if (multiplier < 1) {
      multiplier = 1;
    }
  }
  FOR_EACH(players, i) {
    ASSIGN_ARRAY_ITEM(players, i, Player, p);
    SKIP_IS_NOT_ALIVE(p);
    addWithAngle(&p->pos, p->angle, p->speed);
    p->speed *= 1.002;
    if (input.isPressed) {
      float rate;
      if (p->speed < 3 * sqrt(difficulty)) {
        rate = 0.1;
      } else {
        rate = 0.01;
      }
      p->angle += p->speed * rate;
    }
    color = PURPLE;
    Vector bp;
    vectorSet(&bp, p->pos.x, p->pos.y);
    addWithAngle(&bp, p->angle, -3);
    box(VEC_XY(bp), 5, 5);
    color = BLUE;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(p->pos.x, p->pos.y, 3, p->angle);
    bool pInRect = p->pos.x >= -3 && p->pos.x < 103 && p->pos.y >= -3 && p->pos.y < 90;
    p->isAlive = pInRect;
  }
  COUNT_IS_ALIVE(players, alivePlayerCount2);
  if (alivePlayerCount2 < 3) {
    color = BLUE;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(60, 90, 3, DMISSILE_INIT_ANGLE);
  }
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10);
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (aliveEnemyCount == 0) {
    nextEnemyTicks = 0;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    int c = rndi(3, 6);
    TIMES(c, i) {
      Vector pos;
      vectorSet(&pos, rnd(10, 90), -rnd(5, 25));
      float velAngle = angleTo(&pos, rnd(10, 90), 90);
      Vector vel;
      vectorSet(&vel, rnd(1, sqrt(difficulty)) * 0.1, 0);
      rotate(&vel, velAngle);
      int ticksVal;
      if (rnd(0, 1) < sqrt(difficulty)) {
        ticksVal = ceil(rnd(300, 400) / sqrt(difficulty));
      } else {
        ticksVal = 999;
      }
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      ne->pos = pos;
      ne->vel = vel;
      ne->from = pos;
      ne->ticksLeft = ticksVal;
      ne->id = enemyId;
      ne->isRemoving = false;
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, DMISSILE_MAX_ENEMY_COUNT);
      enemyId++;
    }
    nextEnemyTicks = (150 / sqrt(difficulty)) * c;
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->isRemoving) {
      e->isAlive = false;
      continue;
    }
    if (e->ticksLeft > 0) {
      vectorAdd(&e->pos, e->vel.x, e->vel.y);
      e->ticksLeft--;
      if (e->ticksLeft == 0) {
        play(HIT);
        int c2 = rndi(2, 5);
        TIMES(c2, k) {
          Vector pos2;
          vectorSet(&pos2, e->pos.x, e->pos.y);
          float velLen = vectorLength(&e->vel);
          float velAngle2 = angleTo(&pos2, rnd(10, 90), 90);
          Vector vel2;
          vectorSet(&vel2, velLen, 0);
          rotate(&vel2, velAngle2);
          ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, fe);
          fe->pos = pos2;
          fe->vel = vel2;
          fe->from = pos2;
          fe->ticksLeft = 999;
          if (k == 0) {
            fe->id = e->id;
          } else {
            fe->id = enemyId;
          }
          fe->isRemoving = false;
          fe->isAlive = true;
          enemyIndex = wrap(enemyIndex + 1, 0, DMISSILE_MAX_ENEMY_COUNT);
          enemyId++;
        }
      }
    }
    color = LIGHT_BLACK;
    thickness = 2;
    line(VEC_XY(e->from), VEC_XY(e->pos));
    if (e->ticksLeft > 0) {
      color = RED;
      Collision ec = box(VEC_XY(e->pos), 4, 4);
      if (ec.isColliding.rect[BLUE]) {
        play(POWER_UP);
        addScore(floor(multiplier), VEC_XY(e->pos));
        particle(e->pos.x, e->pos.y, 16, 1, 0, M_PI * 2);
        COUNT_IS_ALIVE(players, alivePC);
        multiplier += 1.0 / clamp(alivePC, 1, 99);
        int hitId = e->id;
        FOR_EACH(enemies, j) {
          ASSIGN_ARRAY_ITEM(enemies, j, Enemy, ae);
          if (!ae->isAlive) {
            continue;
          }
          if (ae->id == hitId) {
            ae->isRemoving = true;
          }
        }
        e->isAlive = false;
        continue;
      }
    }
    if (e->pos.y > 90) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(multiplier)));
  text(multText, 3, 9);
}

void addGameDMissile() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
