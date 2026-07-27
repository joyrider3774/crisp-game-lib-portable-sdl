#include "cglp.h"

static char *title = "RPS";
static char *description = "[Tap]\n Go right &\n Change rock\n        paper\n        scissors";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " lll  ",
        "lllll ",
        "llllll",
        "lllll ",
        " lll  ",
        " lll  ",
    },
    {
        "llll  ",
        "llll  ",
        "llll l",
        "llllll",
        "lllll ",
        " lll  ",
    },
    {
        " l l  ",
        " l l  ",
        "ll l  ",
        "llllll",
        " llll ",
        " lll  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 60, .isDarkColor = false};

static int handColors[3] = {CYAN, PURPLE, YELLOW};

typedef struct {
  float x;
  int handType;
  float nextTicks;
} Lane;
static Lane lanes[4];

typedef struct {
  int laneIndex;
  float y;
  float my;
  float baseMy;
  int type;
  bool isDestroyed;
  bool isAlive;
} Hand;
#define RPS_MAX_HAND_COUNT 64
static Hand hands[RPS_MAX_HAND_COUNT];
static int handIndex;

typedef struct {
  int laneIndex;
  Vector pos;
  float ty;
  float vy;
  int type;
  float freezeTicks;
} MyHand;
static MyHand myHand;

static int multiplier;

static void update() {
  // Never reads a Collision result anywhere in this file - hand-vs-hand
  // and hand-vs-my-hand overlap are direct same-lane fabs(y-difference)
  // checks (see the "< 6" / "< 5" comparisons below), so the engine's own
  // O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(hands);
    handIndex = 0;
    TIMES(4, li) {
      lanes[li].x = li * 20 + 20;
      lanes[li].handType = rndi(0, 3);
      lanes[li].nextTicks = rnd(0, 99);
    }
    myHand.laneIndex = 0;
    vectorSet(&myHand.pos, 20, 97);
    myHand.ty = 97;
    myHand.vy = -1;
    myHand.type = 0;
    myHand.freezeTicks = 0;
    multiplier = 1;
  }
  color = LIGHT_BLACK;
  TIMES(4, li2) {
    Lane *l = &lanes[li2];
    l->nextTicks--;
    if (l->nextTicks < 0) {
      float my = rnd(1, difficulty) * 0.1;
      ASSIGN_ARRAY_ITEM(hands, handIndex, Hand, nh);
      nh->laneIndex = li2;
      nh->y = -9;
      nh->my = my;
      nh->baseMy = my;
      nh->type = l->handType;
      nh->isDestroyed = false;
      nh->isAlive = true;
      handIndex = wrap(handIndex + 1, 0, RPS_MAX_HAND_COUNT);
      l->nextTicks = rnd(200, 300) / sqrt(difficulty);
      if (rnd(0, 1) < 0.4) {
        l->handType = rndi(0, 3);
      }
    }
    rect(l->x - 5, 0, 1, 100);
    rect(l->x + 4, 0, 1, 100);
  }
  FOR_EACH(hands, hi) {
    ASSIGN_ARRAY_ITEM(hands, hi, Hand, h);
    SKIP_IS_NOT_ALIVE(h);
    color = handColors[h->type];
    if (h->isDestroyed) {
      particle(lanes[h->laneIndex].x, h->y, 16, 1, 0, M_PI * 2);
      h->isAlive = false;
      continue;
    }
    h->y += h->my;
    float factor;
    if (h->y > 90) {
      factor = 2;
    } else {
      factor = 1;
    }
    h->my += (h->baseMy * factor - h->my) * 0.1;
    FOR_EACH(hands, ohi) {
      if (ohi == hi) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(hands, ohi, Hand, oh);
      SKIP_IS_NOT_ALIVE(oh);
      if (oh->laneIndex == h->laneIndex && fabs(oh->y - h->y) < 6) {
        float cy = (oh->y + h->y) / 2;
        float t = oh->my;
        oh->my = h->my * 0.4 - oh->my * 0.2;
        h->my = t * 0.4 - h->my * 0.2;
        if (oh->y < h->y) {
          oh->y = cy - 3;
          h->y = cy + 3;
        } else {
          oh->y = cy + 3;
          h->y = cy - 3;
        }
      }
    }
    char hc[2];
    hc[0] = 'a' + h->type;
    hc[1] = 0;
    characterOptions.rotation = 2;
    character(hc, lanes[h->laneIndex].x, h->y);
    characterOptions.rotation = 0;
    if (h->y > 99) {
      play(EXPLOSION);
      gameOver();
    }
  }
  myHand.freezeTicks--;
  if (myHand.freezeTicks < 0 && input.isJustPressed) {
    play(SELECT);
    myHand.laneIndex = (int)wrap(myHand.laneIndex + 1, 0, 4);
    myHand.type = (int)wrap(myHand.type + 1, 0, 3);
    myHand.ty = 97;
    myHand.vy = -sqrt(difficulty);
  }
  float vyTarget;
  if (myHand.freezeTicks < 0) {
    vyTarget = -sqrt(difficulty);
  } else {
    vyTarget = 0;
  }
  myHand.vy += (vyTarget - myHand.vy) * 0.05;
  myHand.ty = clamp(myHand.ty + myHand.vy, 10, 97);
  float ox = lanes[myHand.laneIndex].x - myHand.pos.x;
  myHand.pos.x += ox * 0.5;
  if (ox < 1) {
    myHand.pos.x = lanes[myHand.laneIndex].x;
  }
  myHand.pos.y += (myHand.ty - myHand.pos.y) * 0.5;
  color = handColors[myHand.type];
  char mhc[2];
  mhc[0] = 'a' + myHand.type;
  mhc[1] = 0;
  character(mhc, VEC_XY(myHand.pos));
  FOR_EACH(hands, hi2) {
    ASSIGN_ARRAY_ITEM(hands, hi2, Hand, h2);
    SKIP_IS_NOT_ALIVE(h2);
    if (h2->laneIndex == myHand.laneIndex && fabs(myHand.ty - h2->y) < 5) {
      int o = (int)wrap(myHand.type - h2->type, 0, 3);
      if (o == 0) {
        play(LASER);
        myHand.vy = 0;
        h2->my -= sqrt(difficulty) * 4;
        h2->y = myHand.ty - 3;
      } else if (o == 1) {
        play(COIN);
        addScore(multiplier, VEC_XY(myHand.pos));
        multiplier++;
        h2->isDestroyed = true;
      } else if (o == 2) {
        play(HIT);
        multiplier = 1;
        myHand.vy = sqrt(difficulty) * 5;
        h2->my -= sqrt(difficulty) * 0.5;
        myHand.ty = h2->y + 3;
        myHand.freezeTicks = 60 / sqrt(difficulty);
      }
    }
  }
}

void addGameRps() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
