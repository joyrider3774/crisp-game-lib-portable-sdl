#include "cglp.h"

static char *title = "GLOOP";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " llll ",
        "llllll",
        "ll  ll",
        "ll  ll",
            "      ",
        "      ",
},
    {
        " lll  ",
        "l   l ",
        "l l l ",
        "l   l ",
        " lll  ",
            "      ",
},
    {
        "l l   ",
        " l    ",
        "l l   ",
            "      ",
        "      ",
        "      ",
},
    {
        " l l l",
        "l   l ",
        " l   l",
        "l   l ",
        " l   l",
        "l l l ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

static Vector p;
static Vector v;
static float vya;

typedef struct {
  Vector pos;
  bool isAlive;
} Pi;
#define GLOOP_MAX_PI_COUNT 7
static Pi pis[GLOOP_MAX_PI_COUNT];

typedef struct {
  Vector pos;
  bool isCleared;
  bool slotUsed;
} Sp;
#define GLOOP_MAX_SP_COUNT 32
static Sp sps[GLOOP_MAX_SP_COUNT];
static int spIndex;

static void update() {
  if (!ticks) {
    vectorSet(&p, 50, 50);
    vectorSet(&v, 0, 0);
    vya = -1;
    INIT_UNALIVED_ARRAY(pis);
    FOR_EACH(sps, spi) {
      sps[spi].isCleared = false;
      sps[spi].slotUsed = false;
    }
    spIndex = 0;
  }
  color = LIGHT_CYAN;
  for (int x = 0; x < 15; x++) {
    for (int y = 0; y < 18; y++) {
      if ((x + y) % 2 == 0) {
        character("d", x * 6 + 8, y * 6);
      }
    }
  }
  v.y += 0.02 * difficulty * vya;
  vectorMul(&v, 0.99);
  vectorAdd(&p, v.x, v.y);
  p.x = clamp(input.pos.x, 8, 92);
  p.y = wrap(p.y, 0, 99);
  color = BLACK;
  character("a", VEC_XY(p));
  TIMES(GLOOP_MAX_PI_COUNT, i) {
    if (!pis[i].isAlive) {
      vectorSet(&pis[i].pos, rnd(10, 90), rnd(10, 90));
      pis[i].isAlive = true;
    }
  }
  if (rnd(0, 1) < 0.02 * difficulty) {
    Vector pp;
    vectorSet(&pp, rnd(10, 90), rnd(10, 90));
    if (fabs(pp.x - p.x) + fabs(wrap(pp.y - p.y, -50, 50)) > 25) {
      ASSIGN_ARRAY_ITEM(sps, spIndex, Sp, nsp);
      nsp->pos = pp;
      nsp->isCleared = false;
      nsp->slotUsed = true;
      spIndex = wrap(spIndex + 1, 0, GLOOP_MAX_SP_COUNT);
    }
  }
  TIMES(GLOOP_MAX_PI_COUNT, i) {
    Pi *bumper = &pis[i];
    if (!bumper->isAlive) {
      continue;
    }
    Collision c = character("b", VEC_XY(bumper->pos));
    if (c.isColliding.character['a']) {
      if (fabs(v.y) > 1) {
        play(SELECT);
        bumper->isAlive = false;
        FOR_EACH(sps, k) {
          ASSIGN_ARRAY_ITEM(sps, k, Sp, sp);
          if (!sp->slotUsed || sp->isCleared) {
            continue;
          }
          if (distanceTo(&sp->pos, bumper->pos.x, bumper->pos.y) < 20) {
            play(COIN);
            sp->isCleared = true;
          }
        }
      } else {
        play(HIT);
      }
      v.y *= -0.3;
      vya *= -1;
      p.y = bumper->pos.y + vya * 5;
    }
  }
  int sc = 1;
  color = RED;
  FOR_EACH(sps, i) {
    ASSIGN_ARRAY_ITEM(sps, i, Sp, sp);
    if (!sp->slotUsed) {
      continue;
    }
    if (!sp->isCleared) {
      Collision c = character("c", VEC_XY(sp->pos));
      if (c.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      addScore(sc, VEC_XY(sp->pos));
      sc++;
      sp->slotUsed = false;
    }
  }
}

void addGameGloop() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
