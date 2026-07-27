#include "cglp.h"

static char *title = "T PUNCH";
static char *description = "[Tap]  Punch\n[Hold] Roll";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " bbbb ",
        "ppbbbb",
        "ppprrr",
        " prrr ",
            "      ",
        "      ",
},
    {
        "c   c ",
        " ccc  ",
        "ccbcc ",
        " ccc  ",
        "c   c ",
            "      ",
},
    {
        " lll  ",
        "lyyyl ",
        "l   l ",
        "l b l ",
        " lll  ",
            "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 90, .isDarkColor = false};

#define TPUNCH_ARM_COUNT 6
typedef struct {
  float angle;
  float av;
  float length;
  bool isAttacking;
  bool isAlive[TPUNCH_ARM_COUNT];
} Arm;
static Arm arm;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Enemy;
#define TPUNCH_MAX_ENEMY_COUNT 64
static Enemy enemies[TPUNCH_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
} Bonus;
#define TPUNCH_MAX_BONUS_COUNT 256
static Bonus bonuses[TPUNCH_MAX_BONUS_COUNT];
static int bonusIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    arm.angle = 0;
    arm.av = 0;
    arm.length = 0;
    arm.isAttacking = false;
    TIMES(TPUNCH_ARM_COUNT, ai) { arm.isAlive[ai] = true; }
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bonuses);
    bonusIndex = 0;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  int an = TPUNCH_ARM_COUNT;
  color = BLACK;
  character("c", 49, 49);
  if (input.isJustPressed) {
    if (arm.length < 5) {
      play(SELECT);
      arm.isAttacking = true;
    } else {
      play(LASER);
    }
    multiplier = 1;
  }
  if (!input.isPressed || arm.length > 30) {
    arm.isAttacking = false;
  }
  float lenTarget;
  if (input.isPressed) {
    lenTarget = 36;
  } else {
    lenTarget = 0;
  }
  arm.length += (lenTarget - arm.length) * 0.1 * sqrt(difficulty);
  if (input.isPressed) {
    arm.av += sqrt(difficulty) * 0.001;
  } else {
    arm.av += (0.03 * sqrt(difficulty) - arm.av) * 0.2;
  }
  arm.angle += arm.av;
  float sz = sqrt(arm.length) * 0.5 + 3;
  Vector p;
  float a = arm.angle;
  TIMES(an, ii) {
    a += (M_PI * 2) / an;
    if (!arm.isAlive[ii]) {
      continue;
    }
    float r = 5;
    float ri = arm.length / 4;
    TIMES(5, i) {
      float s = sz;
      if (i == 4 && arm.isAttacking) {
        color = RED;
        s *= 1.5;
      } else {
        color = CYAN;
      }
      vectorSet(&p, 50, 50);
      addWithAngle(&p, a, r);
      box(p.x, p.y, s, s);
      r += ri;
    }
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCheck);
  if (aliveEnemyCheck == 0) {
    nextEnemyTicks = 0;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    Vector pos;
    float py;
    if (rnd(0, 1) < 0.5) {
      py = -3;
    } else {
      py = 3;
    }
    vectorSet(&pos, rnd(0, 99), py);
    if (rnd(0, 1) < 0.5) {
      float tmp = pos.x;
      pos.x = pos.y;
      pos.y = tmp;
    }
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    ne->pos = pos;
    vectorSet(&ne->vel, rnd(0, sd) * RNDPM() * 0.3, rnd(0, sd) * RNDPM() * 0.3);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, TPUNCH_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(60, 99) / difficulty;
  }
  Vector cp;
  vectorSet(&cp, 50, 50);
  FOR_EACH(bonuses, bi2) {
    ASSIGN_ARRAY_ITEM(bonuses, bi2, Bonus, b);
    SKIP_IS_NOT_ALIVE(b);
    addWithAngle(&b->vel, angleTo(&cp, b->pos.x, b->pos.y), sd * 0.002);
    vectorMul(&b->vel, 0.98);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    float t = 60 / sd;
    if (b->ticks > t) {
      color = BLACK;
    } else {
      color = LIGHT_BLUE;
    }
    Collision bc = character("b", VEC_XY(b->pos));
    if (b->ticks > t && (bc.isColliding.rect[RED] || bc.isColliding.rect[CYAN])) {
      play(COIN);
      addScore(multiplier, VEC_XY(b->pos));
      multiplier++;
      b->isAlive = false;
      continue;
    }
    b->ticks++;
    bool inRect = b->pos.x >= -3 && b->pos.x < 103 && b->pos.y >= -3 && b->pos.y < 103;
    if (!inRect) {
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (distanceTo(&e->pos, 50, 50) > 30) {
      addWithAngle(&e->vel, angleTo(&e->pos, 50, 50), sd * 0.005);
      vectorMul(&e->vel, 0.99);
    }
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    characterOptions.isMirrorX = e->vel.x <= 0;
    Collision ec = character("a", VEC_XY(e->pos));
    characterOptions.isMirrorX = false;
    if (ec.isColliding.rect[RED]) {
      play(POWER_UP);
      TIMES(16, bqi) {
        ASSIGN_ARRAY_ITEM(bonuses, bonusIndex, Bonus, nb);
        nb->pos = e->pos;
        vectorSet(&nb->vel, bqi * 0.05, 0);
        rotate(&nb->vel, bqi);
        nb->ticks = 0;
        nb->isAlive = true;
        bonusIndex = wrap(bonusIndex + 1, 0, TPUNCH_MAX_BONUS_COUNT);
      }
      int aidx = rndi(0, an);
      TIMES(an, aloop) {
        if (!arm.isAlive[aidx]) {
          arm.isAlive[aidx] = true;
          break;
        }
        aidx = (int)wrap(aidx + 1, 0, an);
      }
      e->isAlive = false;
      continue;
    }
    if (ec.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      e->isAlive = false;
      continue;
    }
    if (ec.isColliding.character['c']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  int ac = 0;
  a = arm.angle;
  TIMES(an, jj) {
    a += (M_PI * 2) / an;
    if (!arm.isAlive[jj]) {
      continue;
    }
    ac++;
    float r = 5;
    float ri = arm.length / 4;
    TIMES(5, i2) {
      if (i2 == 4 && arm.isAttacking) {
        continue;
      }
      color = TRANSPARENT;
      vectorSet(&p, 50, 50);
      addWithAngle(&p, a, r);
      Collision pc = box(p.x, p.y, sz, sz);
      if (pc.isColliding.character['a']) {
        color = CYAN;
        particle(p.x, p.y, 9, 2, 0, M_PI * 2);
        arm.isAlive[jj] = false;
      }
      r += ri;
    }
  }
  if (ac == 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
}

void addGameTPunch() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
