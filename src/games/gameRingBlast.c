#include "cglp.h"

static char *title = "RING BLAST";
static char *description = "[Slide]\n Change\n angle/speed";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " bbbb ",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        " bbbb ",
    },
    {
        " rrrr ",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        " rrrr ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 150, .soundSeed = 3, .isDarkColor = false};

#define ANGLE_STEP 0.03
#define SPEED_STEP 0.05

typedef struct {
  Vector pos;
  Vector vel;
  int side; // 0 = player, 1 = enemy, 2 = blasting (about to become a ring)
  int blastTicks;
  bool isAlive;
} Stone;
#define MAX_STONE_COUNT 64
static Stone stones[MAX_STONE_COUNT];
static int stoneIndex;

typedef struct {
  float radius;
  float angle;
  float angleWidth;
  float angleWidthVel;
  bool isAlive;
} Ring;
#define MAX_RING_COUNT 32
static Ring rings[MAX_RING_COUNT];
static int ringIndex;

static float nextStoneTime;
static int stoneCount;
static float stoneAngle;
static float stoneSpeed;
static float enemyStoneAngle;
static float enemyStoneAngleVel;
static float enemyStoneSpeed;
static float enemyStoneSpeedVel;
static Vector centerPos = {50, 60};
static int multiplier;

static void addCollidingVelocity(Stone *s, Stone *as, Vector *v) {
  float a = angleTo(&s->pos, VEC_XY(as->pos));
  float pr = fabs(cos(vectorAngle(v) - a)) * 0.7;
  addWithAngle(&as->vel, a, vectorLength(v) * pr);
  addWithAngle(&s->vel, a, -vectorLength(v) * pr);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(stones);
    stoneIndex = 0;
    INIT_UNALIVED_ARRAY(rings);
    ringIndex = 0;
    nextStoneTime = 1;
    stoneCount = 10;
    stoneAngle = -M_PI / 2;
    stoneSpeed = 1;
    enemyStoneAngle = M_PI / 2;
    enemyStoneAngleVel = 1;
    enemyStoneSpeed = 1;
    enemyStoneSpeedVel = 1;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  color = LIGHT_PURPLE;
  rect(0, 0, 10, 150);
  rect(90, 0, 10, 150);
  color = LIGHT_BLACK;
  rect(10, 119, 80, 1);
  box(50, 60, 3, 3);
  rect(0, 144, (stoneCount + nextStoneTime) * 6, 6);
  color = BLACK;
  if (ticks < 99) {
    text("Use D-pad", 22, 130);
  }
  TIMES(stoneCount, ii) {
    character("a", ii * 6 + 3, 147);
  }

  // Upstream aims via mouse-drag; redesigned to d-pad (left/right = angle, up/down = speed).
  if (input.left.isPressed) {
    stoneAngle -= ANGLE_STEP;
  }
  if (input.right.isPressed) {
    stoneAngle += ANGLE_STEP;
  }
  stoneAngle = clamp(stoneAngle, -M_PI / 2 - M_PI / 3, -M_PI / 2 + M_PI / 3);
  if (input.up.isPressed) {
    stoneSpeed += SPEED_STEP;
  }
  if (input.down.isPressed) {
    stoneSpeed -= SPEED_STEP;
  }
  stoneSpeed = clamp(stoneSpeed, 1, 4);
  thickness = 2;
  barCenterPosRatio = 0;
  bar(50, 120, stoneSpeed * 5, stoneAngle);
  if (stoneCount > 0) {
    character("a", 50, 120);
  }

  enemyStoneAngle += enemyStoneAngleVel * rnd(0.02, 0.03) * sd;
  if ((enemyStoneAngle > M_PI / 2 + M_PI / 3 && enemyStoneAngleVel > 0) ||
      (enemyStoneAngle < M_PI / 2 - M_PI / 3 && enemyStoneAngleVel < 0)) {
    enemyStoneAngleVel *= -1;
  }
  enemyStoneSpeed += enemyStoneSpeedVel * rnd(0.03, 0.04) * sd;
  if ((enemyStoneSpeed > 4 && enemyStoneSpeedVel > 0) ||
      (enemyStoneSpeed < 1 && enemyStoneSpeedVel < 0)) {
    enemyStoneSpeedVel *= -1;
  }
  thickness = 2;
  barCenterPosRatio = 0;
  bar(50, 0, enemyStoneSpeed * 5, enemyStoneAngle);
  character("b", 50, 0);

  int blastStoneCount = 0;
  FOR_EACH(stones, bi) {
    ASSIGN_ARRAY_ITEM(stones, bi, Stone, bs);
    SKIP_IS_NOT_ALIVE(bs);
    if (bs->side == 2) {
      blastStoneCount++;
    }
  }

  nextStoneTime -= 0.01 * sd;
  if (nextStoneTime <= 0) {
    if (stoneCount == 0) {
      COUNT_IS_ALIVE(rings, ringsAliveCount);
      if (ringsAliveCount == 0 && blastStoneCount == 0) {
        play(EXPLOSION);
        gameOver();
      } else {
        nextStoneTime = 0;
      }
    } else {
      play(LASER);
      nextStoneTime = 1;
      stoneCount--;
      ASSIGN_ARRAY_ITEM(stones, stoneIndex, Stone, ps);
      vectorSet(&ps->pos, 50, 120);
      vectorSet(&ps->vel, stoneSpeed, 0);
      rotate(&ps->vel, stoneAngle);
      ps->side = 0;
      ps->blastTicks = 60;
      ps->isAlive = true;
      stoneIndex = wrap(stoneIndex + 1, 0, MAX_STONE_COUNT);
      ASSIGN_ARRAY_ITEM(stones, stoneIndex, Stone, es);
      vectorSet(&es->pos, 50, 0);
      vectorSet(&es->vel, enemyStoneSpeed, 0);
      rotate(&es->vel, enemyStoneAngle);
      es->side = 1;
      es->blastTicks = 60;
      es->isAlive = true;
      stoneIndex = wrap(stoneIndex + 1, 0, MAX_STONE_COUNT);
    }
  }

  color = PURPLE;
  FOR_EACH(rings, ri) {
    ASSIGN_ARRAY_ITEM(rings, ri, Ring, r);
    SKIP_IS_NOT_ALIVE(r);
    r->angleWidth += r->angleWidthVel;
    thickness = 3;
    arc(VEC_XY(centerPos), r->radius, r->angle - r->angleWidth, r->angle + r->angleWidth);
    r->radius += 2;
    if (r->radius > 99) {
      r->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  FOR_EACH(stones, si) {
    ASSIGN_ARRAY_ITEM(stones, si, Stone, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, VEC_XY(s->vel));
    vectorMul(&s->vel, 0.98);
    if ((s->pos.x < 13 && s->vel.x < 0) || (s->pos.x > 87 && s->vel.x > 0)) {
      s->vel.x *= -1;
    }
    if ((s->pos.y < 3 && s->vel.y < 0) || (s->pos.y > 117 && s->vel.y > 0)) {
      s->vel.y *= -1;
    }
    int ss = s->side;
    if (ss == 2) {
      s->blastTicks--;
      if (s->blastTicks < 1) {
        play(COIN);
        float radius = distanceTo(&s->pos, VEC_XY(centerPos));
        ASSIGN_ARRAY_ITEM(rings, ringIndex, Ring, nr);
        nr->radius = radius;
        nr->angle = angleTo(&centerPos, VEC_XY(s->pos));
        nr->angleWidth = 0;
        nr->angleWidthVel = 1.0 / (radius + 1);
        nr->isAlive = true;
        ringIndex = wrap(ringIndex + 1, 0, MAX_RING_COUNT);
        s->isAlive = false;
        continue;
      }
      ss = (ticks / s->blastTicks) % 2;
    }
    char stoneChar[2];
    stoneChar[0] = 'a' + ss;
    stoneChar[1] = 0;
    Collision c = character(stoneChar, VEC_XY(s->pos));
    if (c.isColliding.rect[PURPLE] && s->side < 2) {
      play(POWER_UP);
      if (s->side == 0) {
        stoneCount++;
      }
      if (s->side == 1) {
        color = RED;
      } else {
        color = BLUE;
      }
      particle(VEC_XY(s->pos), 16, 1, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(s->pos));
      multiplier++;
      s->isAlive = false;
      continue;
    }
    FOR_EACH(stones, sj) {
      if (sj == si) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(stones, sj, Stone, as);
      SKIP_IS_NOT_ALIVE(as);
      if (distanceTo(&s->pos, VEC_XY(as->pos)) < 6) {
        Vector v;
        v = s->vel;
        Vector asVel;
        asVel = as->vel;
        addCollidingVelocity(s, as, &asVel);
        addCollidingVelocity(as, s, &v);
        vectorAdd(&s->pos, VEC_XY(s->vel));
        vectorAdd(&s->pos, VEC_XY(s->vel));
        vectorAdd(&as->pos, VEC_XY(as->vel));
        vectorAdd(&as->pos, VEC_XY(as->vel));
        if (s->side == 1 && as->side == 0) {
          play(CLICK);
          s->side = 2;
        } else if (as->side == 1 && s->side == 0) {
          play(CLICK);
          as->side = 2;
        } else {
          play(HIT);
        }
      }
    }
  }
  COUNT_IS_ALIVE(rings, ringsAliveEnd);
  if (ringsAliveEnd == 0) {
    multiplier = 1;
  }
}

void addGameRingBlast() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
