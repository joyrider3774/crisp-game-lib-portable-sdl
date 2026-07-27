#include "cglp.h"
#include "string.h"

static char *title = "UP 1 WAY";
static char *description = "[Tap] Go up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        "  yy  ",
        "  YY  ",
        " yyyy ",
        " YYYY ",
        "yyyyyy",
        "YYYYYY",
    },
    {
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        " r  r ",
        " rRRr ",
        " rRRr ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        "r RR r",
        "rRRRRr",
        " rRRr ",
        "  rr  ",
    },
    {
        "  rr  ",
        " rRRr ",
        " r  r ",
        " rRRr ",
        " rRRr ",
        "  rr  ",
    },
    {
        "yyyy  ",
        "y  y  ",
        "yyyy  ",
        "y Y YY",
        "y Y YY",
        "y Y YY",
    },
};
static int charactersCount = 8;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 12, .isDarkColor = false};

#define UP1WAY_FLOOR_COUNT 6
#define UP1WAY_MAX_HOLE_COUNT 24
#define UP1WAY_MAX_BAMBOO_COUNT 48
#define UP1WAY_MAX_SKULL_COUNT 24
#define UP1WAY_MAX_POW_COUNT 8

static int skullFrameOffsets[4] = {0, 1, 2, 1};

typedef struct {
  float x;
  bool isAlive;
} Hole;
typedef struct {
  float x;
  bool isAlive;
} Bamboo;
typedef struct {
  float x;
  bool isAlive;
} Skull;
typedef struct {
  float x;
  bool isAlive;
} Pow;

typedef struct {
  float y;
  Hole holes[UP1WAY_MAX_HOLE_COUNT];
  int holeIndex;
  float nextHoleDist;
  Bamboo bamboos[UP1WAY_MAX_BAMBOO_COUNT];
  int bambooIndex;
  Skull skulls[UP1WAY_MAX_SKULL_COUNT];
  int skullIndex;
  Pow pows[UP1WAY_MAX_POW_COUNT];
  int powIndex;
} Floor;
static Floor floors[UP1WAY_FLOOR_COUNT];

static float nextBambooDist;
static int nextBambooFloorIndex;
static float nextSkullDist;
static float nextPowDist;

typedef struct {
  Vector pos;
  int floorIndex;
  int targetFi;
} Player;
static Player player;

static float animTicks;

static float floorIndexToY(int i) {
  return 16 + i * 15;
}

static bool checkHole(int fi, float x) {
  Floor *f = &floors[fi];
  FOR_EACH(f->holes, i) {
    ASSIGN_ARRAY_ITEM(f->holes, i, Hole, h);
    SKIP_IS_NOT_ALIVE(h);
    if (x > h->x + 3 && x < h->x + 6) {
      return true;
    }
  }
  return false;
}

// Converts skulls on every floor, not just the current one - matches upstream.
static void convertSkullsToBamboo() {
  TIMES(UP1WAY_FLOOR_COUNT, fi) {
    Floor *f = &floors[fi];
    FOR_EACH(f->skulls, si) {
      ASSIGN_ARRAY_ITEM(f->skulls, si, Skull, sk);
      SKIP_IS_NOT_ALIVE(sk);
      ASSIGN_ARRAY_ITEM(f->bamboos, f->bambooIndex, Bamboo, nb);
      nb->x = sk->x;
      nb->isAlive = true;
      f->bambooIndex = wrap(f->bambooIndex + 1, 0, UP1WAY_MAX_BAMBOO_COUNT);
      sk->isAlive = false;
    }
  }
}

static void update() {
  if (!ticks) {
    TIMES(UP1WAY_FLOOR_COUNT, i) {
      Floor *f = &floors[i];
      f->y = floorIndexToY(i);
      INIT_UNALIVED_ARRAY(f->holes);
      f->holeIndex = 0;
      if (i == 5) {
        f->nextHoleDist = 999999999;
      } else {
        ASSIGN_ARRAY_ITEM(f->holes, 0, Hole, h0);
        h0->x = rnd(99, 180);
        h0->isAlive = true;
        f->holeIndex = 1;
        f->nextHoleDist = rnd(0, 99);
      }
      INIT_UNALIVED_ARRAY(f->bamboos);
      f->bambooIndex = 0;
      INIT_UNALIVED_ARRAY(f->skulls);
      f->skullIndex = 0;
      INIT_UNALIVED_ARRAY(f->pows);
      f->powIndex = 0;
    }
    nextBambooDist = 0;
    nextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
    nextSkullDist = rnd(49, 99);
    nextPowDist = 999;
    player.floorIndex = 5;
    vectorSet(&player.pos, 20, floorIndexToY(5));
    player.targetFi = -1;
    animTicks = 0;
  }

  animTicks += difficulty;
  if (player.targetFi != -1) {
    float ty = floorIndexToY(player.targetFi);
    float vy = 1;
    if (ty <= player.pos.y) {
      vy = -1;
    }
    player.pos.y += vy * difficulty * 3;
    if ((player.pos.y - ty) * vy > 0) {
      player.pos.y = ty;
      player.floorIndex = player.targetFi;
      player.targetFi = -1;
    }
  }
  if (player.targetFi == -1) {
    if (input.isJustPressed && player.floorIndex > 0) {
      play(JUMP);
      player.targetFi = player.floorIndex - 1;
    } else if (checkHole(player.floorIndex, player.pos.x)) {
      play(CLICK);
      player.targetFi = player.floorIndex + 1;
    }
  }
  color = BLACK;
  char playerChar[2];
  playerChar[0] = 'a' + ((int)(animTicks / 20) % 2);
  playerChar[1] = 0;
  character(playerChar, player.pos.x, player.pos.y - 5);

  float scr = difficulty;

  nextBambooDist -= scr;
  if (nextBambooDist < 0) {
    Floor *nbf = &floors[nextBambooFloorIndex];
    if (nbf->nextHoleDist < 9) {
      nextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
    } else {
      ASSIGN_ARRAY_ITEM(nbf->bamboos, nbf->bambooIndex, Bamboo, nb);
      nb->x = 209;
      nb->isAlive = true;
      nbf->bambooIndex = wrap(nbf->bambooIndex + 1, 0, UP1WAY_MAX_BAMBOO_COUNT);
      if (rnd(0, 1) < 0.3) {
        nextBambooDist = 6;
      } else {
        nextBambooDist = rnd(200, 300);
        nextBambooFloorIndex = rndi(0, UP1WAY_FLOOR_COUNT);
      }
    }
  }

  nextSkullDist -= scr;
  if (nextSkullDist < 0) {
    int fi = rndi(0, UP1WAY_FLOOR_COUNT);
    if (floors[fi].nextHoleDist > 9 && nextBambooDist > 9) {
      Floor *sf = &floors[fi];
      ASSIGN_ARRAY_ITEM(sf->skulls, sf->skullIndex, Skull, nsk);
      nsk->x = 209;
      nsk->isAlive = true;
      sf->skullIndex = wrap(sf->skullIndex + 1, 0, UP1WAY_MAX_SKULL_COUNT);
    }
    nextSkullDist += rnd(30, 50);
  }

  nextPowDist -= scr;
  if (nextPowDist < 0) {
    int pfi = rndi(0, UP1WAY_FLOOR_COUNT);
    Floor *pf = &floors[pfi];
    ASSIGN_ARRAY_ITEM(pf->pows, pf->powIndex, Pow, np);
    np->x = 209;
    np->isAlive = true;
    pf->powIndex = wrap(pf->powIndex + 1, 0, UP1WAY_MAX_POW_COUNT);
    nextPowDist = 999;
  }

  color = LIGHT_BLUE;
  rect(0, 97, 200, 3);

  TIMES(UP1WAY_FLOOR_COUNT, fidx) {
    Floor *f = &floors[fidx];

    f->nextHoleDist -= scr;
    if (f->nextHoleDist < 0) {
      ASSIGN_ARRAY_ITEM(f->holes, f->holeIndex, Hole, nh);
      nh->x = 200;
      nh->isAlive = true;
      f->holeIndex = wrap(f->holeIndex + 1, 0, UP1WAY_MAX_HOLE_COUNT);
      f->nextHoleDist = rnd(32, 99);
    }

    // See the file header port note: scan for the smallest remaining alive
    // hole x each step, rather than relying on ring-buffer index order.
    float fx = 0;
    bool holeUsed[UP1WAY_MAX_HOLE_COUNT];
    memset(holeUsed, 0, sizeof(holeUsed));
    TIMES(UP1WAY_MAX_HOLE_COUNT, step) {
      int bestIdx = -1;
      float bestX = 100000;
      FOR_EACH(f->holes, hi) {
        if (f->holes[hi].isAlive && !holeUsed[hi] && f->holes[hi].x < bestX) {
          bestX = f->holes[hi].x;
          bestIdx = hi;
        }
      }
      if (bestIdx < 0) {
        break;
      }
      holeUsed[bestIdx] = true;
      if (bestX > fx) {
        color = GREEN;
        rect(fx, f->y - 2, bestX - fx, 2);
        color = LIGHT_BLACK;
        rect(fx, f->y, bestX - fx, 3);
      }
      fx = bestX + 9;
    }
    if (fx < 200) {
      color = GREEN;
      rect(fx, f->y - 2, 200 - fx, 2);
      color = LIGHT_BLACK;
      rect(fx, f->y, 200 - fx, 3);
    }

    color = BLACK;
    FOR_EACH(f->holes, hi2) {
      ASSIGN_ARRAY_ITEM(f->holes, hi2, Hole, h);
      SKIP_IS_NOT_ALIVE(h);
      h->x -= scr;
      if (h->x < -9) {
        h->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->bamboos, bi) {
      ASSIGN_ARRAY_ITEM(f->bamboos, bi, Bamboo, b);
      SKIP_IS_NOT_ALIVE(b);
      Collision c = character("c", b->x, f->y - 5);
      if (c.isColliding.character['a'] || c.isColliding.character['b']) {
        play(COIN);
        addScore(1, b->x, f->y - 5);
        b->x = -9;
      }
      b->x -= scr;
      if (b->x < -3) {
        b->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->pows, powi) {
      ASSIGN_ARRAY_ITEM(f->pows, powi, Pow, p);
      SKIP_IS_NOT_ALIVE(p);
      Collision c = character("h", p->x, f->y - 5);
      if (c.isColliding.character['a'] || c.isColliding.character['b']) {
        play(POWER_UP);
        convertSkullsToBamboo();
        p->x = -9;
      }
      p->x -= scr;
      if (p->x < -3) {
        p->isAlive = false;
        continue;
      }
    }

    FOR_EACH(f->skulls, si) {
      ASSIGN_ARRAY_ITEM(f->skulls, si, Skull, sk);
      SKIP_IS_NOT_ALIVE(sk);
      int frame = skullFrameOffsets[(int)(animTicks / 15) % 4];
      char skullChar[2];
      skullChar[0] = 'd' + frame;
      skullChar[1] = 0;
      Collision c = character(skullChar, sk->x, f->y - 5);
      if (c.isColliding.character['c'] || c.isColliding.character['h']) {
        sk->x = -9;
      }
      if (c.isColliding.character['a'] || c.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
      sk->x -= scr;
      if (sk->x < -3) {
        sk->isAlive = false;
        continue;
      }
    }
  }
}

void addGameUp1Way() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
