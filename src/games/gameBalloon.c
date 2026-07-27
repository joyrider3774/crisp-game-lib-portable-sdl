#include "cglp.h"

static char *title = "BALLOON";
static char *description = "[Slide] Wind";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 27, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  float r;
  float t;
  bool isAlive;
} BalloonBalloon;
#define BALLOON_MAX_BALLOON_COUNT 8
static BalloonBalloon balloons[BALLOON_MAX_BALLOON_COUNT];
static int balloonIndex;
static float addBalloonTicks;

typedef struct {
  Vector pos;
  Vector vel;
  int balloonIndex;
  bool hasBalloon;
  bool isAlive;
} BalloonBonus;
// Raised from 32: once attached a bonus rides its balloon for the balloon's
// whole ~330-tick life (difficulty-independent), while bonus spawn interval
// is 50/difficulty ticks, so concurrent count grows ~6.6*difficulty unboundedly.
#define BALLOON_MAX_BONUS_COUNT 256
static BalloonBonus bonuses[BALLOON_MAX_BONUS_COUNT];
static int bonusIndex;
static float addBonusTicks;

static Vector prevInputPos;
static Vector wind;
static int multiplier;
static float scoreTotal;
static Vector scorePos;

static void update() {
  // Never reads a Collision result anywhere in this file - balloon/bonus
  // pickup and popping are all direct distanceTo() checks (see the
  // "< bl->r * 1.3" and "< b->r" comparisons below), so the engine's own
  // O(n^2) hitbox scan is pure waste here.
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(balloons);
    balloonIndex = 0;
    addBalloonTicks = 0;
    vectorSet(&prevInputPos, input.pos.x, input.pos.y);
    vectorSet(&wind, 0, 0);
    INIT_UNALIVED_ARRAY(bonuses);
    bonusIndex = 0;
    addBonusTicks = 0;
  }
  multiplier = 1;
  scoreTotal = 0;
  addBonusTicks--;
  if (addBonusTicks < 0) {
    ASSIGN_ARRAY_ITEM(bonuses, bonusIndex, BalloonBonus, nb);
    vectorSet(&nb->pos, rnd(10, 80), -5);
    vectorSet(&nb->vel, rnd(0, 0.2) * RNDPM(), rnd(0.1, 0.25 + difficulty * 0.05));
    nb->hasBalloon = false;
    nb->isAlive = true;
    bonusIndex = wrap(bonusIndex + 1, 0, BALLOON_MAX_BONUS_COUNT);
    addBonusTicks += 50 / difficulty;
  }
  FOR_EACH(bonuses, i) {
    ASSIGN_ARRAY_ITEM(bonuses, i, BalloonBonus, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->hasBalloon) {
      color = GREEN;
      BalloonBalloon *bl = &balloons[b->balloonIndex];
      Vector d;
      d.x = bl->pos.x - b->pos.x;
      d.y = bl->pos.y - b->pos.y;
      vectorMul(&d, 0.1);
      vectorAdd(&b->pos, d.x, d.y);
      Vector d2;
      d2.x = bl->pos.x - b->pos.x;
      d2.y = bl->pos.y - b->pos.y;
      vectorMul(&d2, 0.02);
      vectorAdd(&b->vel, d2.x, d2.y);
      vectorMul(&b->vel, 0.95);
    } else {
      color = YELLOW;
      FOR_EACH(balloons, j) {
        ASSIGN_ARRAY_ITEM(balloons, j, BalloonBalloon, bl);
        SKIP_IS_NOT_ALIVE(bl);
        if (distanceTo(&bl->pos, b->pos.x, b->pos.y) < bl->r * 1.3) {
          play(SELECT);
          b->hasBalloon = true;
          b->balloonIndex = j;
        }
      }
    }
    if ((b->pos.x < 0 && b->vel.x < 0) || (b->pos.x > 99 && b->vel.x > 0)) {
      b->vel.x *= -1;
    }
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    text("$", b->pos.x, b->pos.y);
    if (!b->hasBalloon && b->pos.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
    if (b->hasBalloon && !balloons[b->balloonIndex].isAlive) {
      scoreTotal += multiplier;
      scorePos = b->pos;
      multiplier++;
      b->isAlive = false;
      continue;
    }
  }
  if (scoreTotal > 0) {
    play(COIN);
    addScore(scoreTotal, scorePos.x, scorePos.y + 9);
  }
  color = CYAN;
  Vector o;
  o.x = input.pos.x - prevInputPos.x;
  o.y = input.pos.y - prevInputPos.y;
  if (vectorLength(&o) < 9) {
    vectorMul(&o, 0.5);
    vectorAdd(&wind, o.x, o.y);
    vectorMul(&wind, 0.5);
  }
  vectorSet(&prevInputPos, input.pos.x, input.pos.y);
  particle(input.pos.x, input.pos.y, 3, vectorLength(&wind), vectorAngle(&wind), 0.1);
  addBalloonTicks--;
  if (addBalloonTicks < 0) {
    float r = 20;
    ASSIGN_ARRAY_ITEM(balloons, balloonIndex, BalloonBalloon, nbl);
    vectorSet(&nbl->pos, rnd(r, 99 - r), 99 + r);
    vectorSet(&nbl->vel, rnd(0, 1) * RNDPM(), 0);
    nbl->r = r;
    nbl->t = 0;
    nbl->isAlive = true;
    balloonIndex = wrap(balloonIndex + 1, 0, BALLOON_MAX_BALLOON_COUNT);
    addBalloonTicks += 200;
  }
  color = GREEN;
  FOR_EACH(balloons, i) {
    ASSIGN_ARRAY_ITEM(balloons, i, BalloonBalloon, b);
    SKIP_IS_NOT_ALIVE(b);
    if (distanceTo(&input.pos, b->pos.x, b->pos.y) < b->r) {
      vectorAdd(&b->vel, wind.x, wind.y);
    }
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    if ((b->pos.x < b->r && b->vel.x < 0) || (b->pos.x > 99 - b->r && b->vel.x > 0)) {
      b->vel.x *= -0.7;
    }
    if (b->pos.y > 99 - b->r && b->vel.y > 0) {
      b->vel.y *= -0.7;
    }
    b->vel.x *= 0.9;
    b->vel.y += (-0.3 - b->vel.y) * 0.1;
    b->r *= 1 - vectorLength(&b->vel) * 0.003;
    b->t++;
    float a = b->t * 0.03;
    thickness = 3;
    TIMES(7, k) {
      Vector p;
      vectorSet(&p, b->r, 0);
      rotate(&p, a);
      vectorAdd(&p, b->pos.x, b->pos.y);
      bar(p.x, p.y, b->r * 0.3, a + M_PI_2);
      a += M_PI * 2 / 7;
    }
    if (b->pos.y < b->r || b->r < 9) {
      b->isAlive = false;
      particle(b->pos.x, b->pos.y, 9, 1, 0, M_PI * 2);
      continue;
    }
  }
}

void addGameBalloon() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
