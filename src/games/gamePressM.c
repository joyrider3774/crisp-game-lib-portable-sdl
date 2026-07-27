#include "cglp.h"

static char *title = "PRESS M";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
            "      ",
},
    {
        "llllll",
        "ll l l",
        "llllll",
        "ll  ll",
            "      ",
        "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

#define MODE_PRESS 0
#define MODE_RETURN 1

typedef struct {
  Vector pos;
  float sy;
  float ey;
  float ney;
} Wall;
#define WALL_COUNT 20
static Wall walls[WALL_COUNT];

static int wallMode;
static float wallTicks;
static float wallModeInterval;
static int hitWallIndex;
static float px;
static int pmx;

typedef struct {
  Vector pos;
  int wallIndex;
  float wallOy;
  bool isAlive;
} Coin;
#define MAX_COIN_COUNT 20
static Coin coins[MAX_COIN_COUNT];
static int coinCount;

typedef struct {
  Vector pos;
  float speed;
  bool isAlive;
} InhalingCoin;
#define MAX_INHALING_COUNT 64
static InhalingCoin inhalingCoins[MAX_INHALING_COUNT];
static int inhalingCoinIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(WALL_COUNT, wi) {
      int xi = wi % 10;
      int yi = wi / 10;
      float ey;
      if (yi == 0) {
        ey = 40;
      } else {
        ey = 60;
      }
      vectorSet(&walls[wi].pos, xi * 10, ey);
      walls[wi].sy = 0;
      walls[wi].ey = ey;
      walls[wi].ney = 0;
    }
    wallTicks = 0;
    wallMode = MODE_PRESS;
    px = 50;
    pmx = 1;
    hitWallIndex = -1;
    INIT_UNALIVED_ARRAY(inhalingCoins);
    inhalingCoinIndex = 0;
    multiplier = 1;
  }
  if (wallMode == MODE_PRESS && wallTicks < 5) {
    color = RED;
  } else {
    color = PURPLE;
  }
  if (wallTicks == 0) {
    if (wallMode == MODE_PRESS) {
      if (multiplier > 1) {
        multiplier--;
      }
      if (hitWallIndex >= 0) {
        play(EXPLOSION);
        particle(walls[hitWallIndex].pos.x, walls[hitWallIndex].pos.y, 30, 2, 0, M_PI * 2);
      }
      int wi = 0;
      TIMES(2, yi) {
        float a = rnd(0, M_PI * 2);
        float av = rnd(0.4, 1) * RNDPM();
        float r = rnd(10, 20);
        float cy;
        if (yi == 0) {
          cy = rnd(15, 25);
        } else {
          cy = rnd(75, 85);
        }
        TIMES(10, xi) {
          Wall *w = &walls[wi];
          w->sy = clamp(sin(a) * r + cy, 2, 97);
          av += rnd(0, 0.1) * RNDPM();
          r += rnd(0, 1) * RNDPM();
          a += av;
          cy += rnd(0, 1) * RNDPM();
          wi++;
        }
      }
      float mw = 99;
      TIMES(10, xi2) {
        float wdiff = walls[xi2 + 10].sy - walls[xi2].sy;
        if (wdiff < mw) {
          mw = wdiff;
          hitWallIndex = xi2;
        }
      }
      mw /= 2;
      TIMES(WALL_COUNT, wi3) {
        if (wi3 < 10) {
          walls[wi3].ney = walls[wi3].sy + mw;
        } else {
          walls[wi3].ney = walls[wi3].sy - mw;
        }
      }
      bool hasSpace = false;
      TIMES(10, xi3) {
        Wall *w1 = &walls[xi3];
        Wall *w2 = &walls[xi3 + 10];
        if (w1->ney < 40 && w2->ney > 60) {
          hasSpace = true;
        }
      }
      if (!hasSpace) {
        TIMES(10, xi4) {
          if (xi4 != hitWallIndex) {
            Wall *w1 = &walls[xi4];
            Wall *w2 = &walls[xi4 + 10];
            if (w1->ney > 40) {
              float wamt = w1->ney - 40;
              w1->ney -= wamt;
              w1->sy -= wamt;
            }
            if (w2->ney < 60) {
              float wamt2 = 60 - w2->ney;
              w2->ney += wamt2;
              w2->sy += wamt2;
            }
          }
        }
      }
      INIT_UNALIVED_ARRAY(coins);
      coinCount = 0;
      TIMES(WALL_COUNT, wi4b) {
        if (rnd(0, 1) < 0.2) {
          Coin *c = &coins[coinCount];
          c->wallIndex = wi4b;
          if (wi4b < 10) {
            c->wallOy = 4;
          } else {
            c->wallOy = -4;
          }
          c->isAlive = true;
          coinCount++;
        }
      }
      wallMode = MODE_RETURN;
      wallModeInterval = ceil(60 / sqrt(difficulty));
      wallTicks = wallModeInterval;
    } else {
      TIMES(WALL_COUNT, wi5) { walls[wi5].ey = walls[wi5].ney; }
      wallMode = MODE_PRESS;
      wallModeInterval = ceil(20 / sqrt(difficulty));
      wallTicks = wallModeInterval;
    }
  }
  wallTicks--;
  TIMES(WALL_COUNT, wi6) {
    Wall *w = &walls[wi6];
    float factor;
    if (wallMode == MODE_PRESS) {
      if (wallTicks < 5) {
        factor = 1 - wallTicks / 5.0;
      } else {
        factor = (1 - wallTicks / wallModeInterval) * 0.2;
      }
      w->pos.y = w->sy + (w->ey - w->sy) * factor;
    } else {
      w->pos.y = w->ey + (w->sy - w->ey) * (1 - (wallTicks + 1) / wallModeInterval);
    }
    if (wi6 < 10) {
      rect(w->pos.x, 0, 9, w->pos.y);
    } else {
      rect(w->pos.x, w->pos.y, 9, 99 - w->pos.y);
    }
  }
  Vector p;
  vectorSet(&p, clamp(input.pos.x, 3, 96), 50);
  if (p.x < px - 2) {
    pmx = -1;
  } else if (p.x > px + 2) {
    pmx = 1;
  }
  px = p.x;
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (ticks % 60) / 30;
  pc[1] = 0;
  characterOptions.isMirrorX = pmx < 0;
  Collision pcoll = character(pc, VEC_XY(p));
  characterOptions.isMirrorX = false;
  if (pcoll.isColliding.rect[RED]) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  if (wallMode == MODE_PRESS) {
    color = YELLOW;
  } else {
    color = LIGHT_YELLOW;
  }
  TIMES(coinCount, ci) {
    Coin *c = &coins[ci];
    SKIP_IS_NOT_ALIVE(c);
    vectorSet(&c->pos, walls[c->wallIndex].pos.x + 5, walls[c->wallIndex].pos.y + c->wallOy);
    if (wallMode == MODE_PRESS && distanceTo(&c->pos, VEC_XY(p)) < 30) {
      ASSIGN_ARRAY_ITEM(inhalingCoins, inhalingCoinIndex, InhalingCoin, ic);
      ic->pos = c->pos;
      ic->speed = rnd(0.1, 0.3);
      ic->isAlive = true;
      inhalingCoinIndex = wrap(inhalingCoinIndex + 1, 0, MAX_INHALING_COUNT);
      c->isAlive = false;
      continue;
    }
    text("o", VEC_XY(c->pos));
  }
  color = YELLOW;
  FOR_EACH(inhalingCoins, ii) {
    ASSIGN_ARRAY_ITEM(inhalingCoins, ii, InhalingCoin, ic2);
    SKIP_IS_NOT_ALIVE(ic2);
    ic2->pos.x += (p.x - ic2->pos.x) * ic2->speed;
    ic2->pos.y += (p.y - ic2->pos.y) * ic2->speed;
    Collision cc = text("o", VEC_XY(ic2->pos));
    if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
      play(COIN);
      addScore(multiplier, VEC_XY(ic2->pos));
      multiplier++;
      ic2->isAlive = false;
      continue;
    }
  }
}

void addGamePressM() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
