#include "cglp.h"

static char *title = "SUB JUMP";
static char *description = "[Hold]\n Go up & \n Speed up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "   ll ",
        "   l  ",
        "  lll ",
        "l l l ",
        "llllll",
        "l lll ",
    },
    {
        "   ll ",
        "   l  ",
        "  lll ",
        "  l l ",
        "llllll",
        "  lll ",
    },
    {
        " llll ",
        "ll lll",
        "ll lll",
        "ll lll",
        "ll lll",
        " llll ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = true};

#define SUBJUMP_LANDFORM_SEA 0
#define SUBJUMP_LANDFORM_LAND 1

static Vector points[12];
static int landForm;
static float nextLandFormDist;
static float lvy;

typedef struct {
  Vector pos;
  Vector vel;
} Sub;
static Sub sub;

typedef struct {
  Vector pos;
  bool isAlive;
} Coin;
#define SUBJUMP_MAX_COIN_COUNT 64
static Coin coins[SUBJUMP_MAX_COIN_COUNT];
static int coinIndex;
static float nextCoinDist;
static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(12, ptIdx) { vectorSet(&points[ptIdx], ptIdx * 10 - 10, rnd(75, 85)); }
    landForm = SUBJUMP_LANDFORM_SEA;
    nextLandFormDist = 50;
    lvy = 0;
    vectorSet(&sub.pos, 5, 60);
    vectorSet(&sub.vel, 0, 0);
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    nextCoinDist = 0;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  float scr = 0;
  if (sub.pos.x > 10) {
    scr += (sub.pos.x - 10) * 0.2;
  }
  nextLandFormDist -= scr;
  int gpIndex = 11;
  TIMES(12, pi2) {
    Vector *p = &points[pi2];
    p->x -= scr;
    int ppIndex = (int)wrap(pi2 - 1, 0, 12);
    Vector *pp = &points[ppIndex];
    if (p->x < -10) {
      if (nextLandFormDist < 0) {
        if (landForm == SUBJUMP_LANDFORM_LAND) {
          landForm = SUBJUMP_LANDFORM_SEA;
        } else {
          landForm = SUBJUMP_LANDFORM_LAND;
        }
        float divisor;
        if (landForm == SUBJUMP_LANDFORM_LAND) {
          divisor = 7 / sqrt(difficulty);
        } else {
          divisor = 1;
        }
        nextLandFormDist = rnd(200, 300) / divisor;
      }
      p->x += 120;
      lvy += rnd(0, sd) * RNDPM() * 2;
      lvy *= 0.95;
      if (landForm == SUBJUMP_LANDFORM_SEA) {
        if (pp->y < 55) {
          lvy += 5;
        } else if (pp->y < 65) {
          lvy += 3;
        } else if ((pp->y < 65 && lvy < 0) || (pp->y > 90 && lvy > 0)) {
          lvy *= -0.5;
        }
        p->y = pp->y + lvy;
        if (nextLandFormDist < 60) {
          lvy += 4;
        }
      } else {
        if (pp->y > 50) {
          lvy -= 5;
        } else if ((pp->y < 40 && lvy < 0) || (pp->y > 45 && lvy > 0)) {
          lvy *= -0.5;
        }
        p->y = pp->y + lvy / 3;
      }
      p->y = clamp(p->y, 35, 95) + rnd(0, 5) * RNDPM();
    }
    if (pp->x < p->x) {
      if (pp->y < 50 || p->y < 50) {
        color = GREEN;
      } else {
        color = PURPLE;
      }
      thickness = 2;
      line(pp->x, pp->y, p->x, p->y);
    } else {
      gpIndex = ppIndex;
    }
  }
  color = BLUE;
  rect(0, 50, 100, 2);
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    if (sub.pos.y > 50) {
      sub.vel.y -= sd * 0.06;
      sub.vel.x += (1 * sd - sub.vel.x) * 0.1;
    } else {
      sub.vel.y += sd * 0.01;
      sub.vel.x += (1 * sd - sub.vel.x) * 0.1;
    }
  } else {
    if (sub.pos.y > 50) {
      sub.vel.y += sd * 0.03;
      sub.vel.x += (0.5 * sd - sub.vel.x) * 0.1;
    } else {
      sub.vel.y += sd * 0.05;
      sub.vel.x += (0.5 * sd - sub.vel.x) * 0.1;
    }
  }
  float py = sub.pos.y;
  float velMul;
  if (sub.pos.y > 50) {
    velMul = 0.95;
  } else {
    velMul = 0.99;
  }
  vectorMul(&sub.vel, velMul);
  vectorAdd(&sub.pos, sub.vel.x, sub.vel.y);
  sub.pos.x -= scr;
  color = BLUE;
  if (py > 50 && sub.pos.y < 50) {
    play(JUMP);
    particle(sub.pos.x, 50, 9, 1, -M_PI / 2, M_PI);
  }
  if (py < 50 && sub.pos.y > 50) {
    play(HIT);
    particle(sub.pos.x, 50, 9, 0.5, -M_PI / 2, M_PI);
  }
  color = BLACK;
  char sc[2];
  if (sub.pos.y < 50) {
    sc[0] = 'a';
  } else {
    sc[0] = 'a' + (ticks / 7) % 2;
  }
  sc[1] = 0;
  Collision scoll = character(sc, VEC_XY(sub.pos));
  if (scoll.isColliding.rect[PURPLE] || scoll.isColliding.rect[GREEN]) {
    play(EXPLOSION);
    gameOver();
  }
  if (sub.pos.y > 55) {
    color = BLUE;
    particle(sub.pos.x - 3, sub.pos.y + 1, 0.3, 1, M_PI, 0.1);
  }
  nextCoinDist -= scr;
  if (nextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, nc);
    float gpY = points[gpIndex].y;
    float coinYOfs;
    if (gpY < 50) {
      coinYOfs = rnd(5, 10);
    } else {
      coinYOfs = rnd(10, 20);
    }
    vectorSet(&nc->pos, points[gpIndex].x, gpY - coinYOfs);
    nc->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, SUBJUMP_MAX_COIN_COUNT);
    nextCoinDist = rnd(10, 40);
  }
  color = YELLOW;
  FOR_EACH(coins, ci) {
    ASSIGN_ARRAY_ITEM(coins, ci, Coin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    Collision cc = character("c", VEC_XY(c->pos));
    if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
      play(COIN);
      addScore(multiplier, VEC_XY(c->pos));
      multiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      if (multiplier > 1) {
        multiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameSubJump() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
