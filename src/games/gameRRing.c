#include "cglp.h"
#include "string.h"

static char *title = "R RING";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " c c  ",
        "blblb ",
        "lblbl ",
        " lbl  ",
        " l l  ",
        " l l  ",
    },
    {
        " bb   ",
        "bccb  ",
        "cllc  ",
        "cllc  ",
        "bccb  ",
        " bb   ",
    },
    {
        " lrl  ",
        "r   r ",
        " lrl  ",
        "L L L ",
        " LLL  ",
            "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

static int starColors[3] = {LIGHT_CYAN, LIGHT_PURPLE, LIGHT_BLACK};

typedef struct {
  Vector pos;
  Vector targetPos;
  float angle;
  float stopTicks;
  float moveTicks;
  float fireTicks;
} Ship;
static Ship ship;

// Ship trail used to place "option" follower glyphs a fixed number of frames
// behind the ship - a ring buffer of the last 99 positions reproduces the
// JS array's unshift()/pop() (front=newest, capped length) exactly.
static Vector shipHistory[99];
static int historyCount;
static int historyHead;

static int optionCount;
static int maxOptionCount;

typedef struct {
  Vector pos;
  float vy;
  float radius;
  bool isHit;
  bool isAlive;
} Ring;
// Fire interval is 60/difficulty (scales with difficulty directly) while each ring's lifetime only
// shrinks as 1/sqrt(difficulty) (vy=sqrt(difficulty)), and up to 5 rings fire per trigger once all 4
// options are active - concurrent count grows unboundedly (~7*sqrt(difficulty)) over a long session.
#define MAX_RING_COUNT 512
static Ring rings[MAX_RING_COUNT];
static int ringIndex;

typedef struct {
  Vector pos;
  float vy;
  int color;
} Star;
static Star stars[20];

static int multiplier;
static float nextPowerUpTicks;

static void fire(float x, float y) {
  ASSIGN_ARRAY_ITEM(rings, ringIndex, Ring, nr);
  vectorSet(&nr->pos, x, y);
  nr->vy = sqrt(difficulty);
  nr->radius = 1;
  nr->isHit = false;
  nr->isAlive = true;
  ringIndex = wrap(ringIndex + 1, 0, MAX_RING_COUNT);
}

static void update() {
  if (!ticks) {
    vectorSet(&ship.pos, 20, 20);
    ship.angle = 0;
    vectorSet(&ship.targetPos, 20, 20);
    ship.stopTicks = 0;
    ship.moveTicks = 0;
    ship.fireTicks = 0;
    historyCount = 0;
    historyHead = 0;
    optionCount = 0;
    INIT_UNALIVED_ARRAY(rings);
    ringIndex = 0;
    TIMES(20, si) {
      vectorSet(&stars[si].pos, rnd(0, 99), rnd(0, 99));
      stars[si].vy = rnd(0.5, 1);
      stars[si].color = starColors[rndi(0, 3)];
    }
    multiplier = 1;
    nextPowerUpTicks = 60;
    maxOptionCount = 1;
  }
  float sd = sqrt(difficulty);
  TIMES(20, si2) {
    Star *s = &stars[si2];
    s->pos.y += s->vy;
    if (s->pos.y > 99) {
      s->pos.x = rnd(0, 99);
      s->pos.y = 0;
    }
    color = s->color;
    rect(VEC_XY(s->pos), 1, 1);
  }
  ship.stopTicks--;
  if (ship.stopTicks < 0) {
    ship.moveTicks = rnd(120, 180) / sd;
    ship.stopTicks = ship.moveTicks + rnd(40, 60) / sd;
  }
  ship.moveTicks--;
  if (ship.moveTicks > 0) {
    if (distanceTo(&ship.pos, VEC_XY(ship.targetPos)) < 5) {
      vectorSet(&ship.targetPos, rnd(10, 90), rnd(3, 30));
    }
    float ta = angleTo(&ship.pos, VEC_XY(ship.targetPos));
    float oa = wrap(ta - ship.angle, -M_PI, M_PI);
    float va = 0.1 * difficulty;
    if (fabs(oa) < va) {
      ship.angle = ta;
    } else {
      if (oa > 0) {
        ship.angle += va;
      } else {
        ship.angle -= va;
      }
    }
    addWithAngle(&ship.pos, ship.angle, 0.5 * difficulty);
    ship.pos.x = clamp(ship.pos.x, 0, 99);
    ship.pos.y = clamp(ship.pos.y, 0, 50);
    historyHead = (historyHead - 1 + 99) % 99;
    shipHistory[historyHead] = ship.pos;
    if (historyCount < 99) {
      historyCount++;
    }
  }
  color = BLACK;
  character("a", VEC_XY(ship.pos));
  ship.fireTicks--;
  if (ship.fireTicks < 0) {
    play(HIT);
    fire(VEC_XY(ship.pos));
  }
  nextPowerUpTicks--;
  if (nextPowerUpTicks < 0) {
    play(POWER_UP);
    optionCount++;
    if (optionCount > maxOptionCount) {
      optionCount = 0;
      maxOptionCount = clamp(maxOptionCount + 1, 1, 4);
    }
    if (optionCount == 0) {
      nextPowerUpTicks = 60 / sd;
    } else {
      nextPowerUpTicks = (300.0 / optionCount) / sd;
    }
  }
  TIMES(optionCount, oi) {
    int histIdx = (oi + 1) * 24;
    if (histIdx < historyCount) {
      Vector hp = shipHistory[(historyHead + histIdx) % 99];
      // Vircon32 port note: JS's per-call {scale:{x:s,y:s}} pulsing option
      // glyph size has no engine equivalent here - drawn at normal size.
      character("b", VEC_XY(hp));
      if (ship.fireTicks < 0) {
        fire(VEC_XY(hp));
      }
    }
  }
  if (ship.fireTicks < 0) {
    ship.fireTicks = 60 / difficulty;
  }
  color = PURPLE;
  thickness = 3;
  FOR_EACH(rings, ri1) {
    ASSIGN_ARRAY_ITEM(rings, ri1, Ring, r);
    SKIP_IS_NOT_ALIVE(r);
    r->pos.y += r->vy;
    r->radius += r->vy * 0.5;
    arc(r->pos.x, r->pos.y - r->radius * 0.3, r->radius * 0.6, M_PI / 4, (M_PI / 4) * 3);
  }
  color = RED;
  thickness = 3;
  FOR_EACH(rings, ri2) {
    ASSIGN_ARRAY_ITEM(rings, ri2, Ring, r2);
    SKIP_IS_NOT_ALIVE(r2);
    arc(r2->pos.x - r2->radius * 0.32, r2->pos.y, r2->radius * 0.2, (M_PI / 6) * 5,
        (M_PI / 6) * 7);
    arc(r2->pos.x + r2->radius * 0.32, r2->pos.y, r2->radius * 0.2, -(M_PI / 6), M_PI / 6);
  }
  color = BLACK;
  Collision cColl = character("c", clamp(input.pos.x, 0, 99), 95);
  if (cColl.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(rings, ri3) {
    ASSIGN_ARRAY_ITEM(rings, ri3, Ring, r3);
    SKIP_IS_NOT_ALIVE(r3);
    if (r3->isHit) {
      color = LIGHT_PURPLE;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    Collision arcColl = arc(r3->pos.x, r3->pos.y + r3->radius * 0.3, r3->radius * 0.6,
                             -M_PI / 4, -(M_PI / 4) * 3);
    if (arcColl.isColliding.character['c'] && !r3->isHit) {
      play(COIN);
      addScore(multiplier, VEC_XY(r3->pos));
      multiplier++;
      r3->isHit = true;
    }
    if (r3->pos.y > 110) {
      if (!r3->isHit && multiplier > 1) {
        multiplier--;
      }
      r3->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "+");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameRRing() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
