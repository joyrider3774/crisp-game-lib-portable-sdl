#include "cglp.h"

static char *title = "CUE CRUMB";
static char *description = "[Tap] Thrust forward";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 56, .isDarkColor = false};

typedef struct {
  Vector pos;
  float face;
  float vx;
  float vy;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float size;
  float vx;
  float vy;
  int hot;
  int chainDepth;
  bool splitQ;
  float splitFast;
  int splitChainDepth;
  bool isAlive;
} Rock;
// Uncollected crumbs never expire upstream either, so this is sized generously to match.
#define CUECRUMB_MAX_ROCK_COUNT 128
static Rock rocks[CUECRUMB_MAX_ROCK_COUNT];
static int rockIndex;
// Running count of alive rocks with size > 4.5 - maintained incrementally
// instead of re-filtering the whole array every pairwise check (which the
// original JS does), since that would be an O(n^3) scan per frame here.
static int nonCrumbCount;

static float nextRockT;
static float hitstop;
static float shakeMag;
static float shakeT;

static void addRock(float x, float y, float size, float vx, float vy, int hot, int chainDepth) {
  ASSIGN_ARRAY_ITEM(rocks, rockIndex, Rock, r);
  r->pos.x = x;
  r->pos.y = y;
  r->size = size;
  r->vx = vx;
  r->vy = vy;
  r->hot = hot;
  r->chainDepth = chainDepth;
  r->splitQ = false;
  r->splitFast = 0;
  r->splitChainDepth = 0;
  r->isAlive = true;
  rockIndex = wrap(rockIndex + 1, 0, CUECRUMB_MAX_ROCK_COUNT);
  if (size > 4.5) {
    nonCrumbCount++;
  }
}

static void killRock(Rock *r) {
  if (r->size > 4.5) {
    nonCrumbCount--;
  }
  r->isAlive = false;
}

static void update() {
  // Never reads a Collision result - rock/crumb hits are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    player.face = rnd(0, M_PI * 2);
    player.vx = 0;
    player.vy = 0;
    INIT_UNALIVED_ARRAY(rocks);
    rockIndex = 0;
    nonCrumbCount = 0;
    TIMES(4, initRi) {
      Vector p;
      vectorSet(&p, rnd(12, 88), rnd(12, 88));
      if (distanceTo(&p, 50, 50) < 20) {
        p.x = wrap(p.x + 35, 12, 88);
      }
      addRock(p.x, p.y, 9, 0, 0, 0, 0);
    }
    nextRockT = 300;
    hitstop = 0;
    shakeMag = 0;
    shakeT = 0;
  }

  bool frozen = hitstop > 0;
  if (frozen) {
    hitstop--;
  }

  if (!frozen) {
    player.face += 0.05 + difficulty * 0.008;
  }

  if (input.isJustPressed) {
    player.vx += cos(player.face) * 1.7;
    player.vy += sin(player.face) * 1.7;
    play(LASER);
  }

  if (!frozen) {
    player.vx *= 0.965;
    player.vy *= 0.965;
    player.pos.x += player.vx;
    player.pos.y += player.vy;
    if (player.pos.x < 5 || player.pos.x > 95) {
      player.vx = -player.vx;
    }
    if (player.pos.y < 5 || player.pos.y > 95) {
      player.vy = -player.vy;
    }
    player.pos.x = clamp(player.pos.x, 5, 95);
    player.pos.y = clamp(player.pos.y, 5, 95);
  }

  if (!frozen) {
    nextRockT--;
    if (nextRockT < 0 && nonCrumbCount < 10) {
      int side = rndi(0, 4);
      float sx;
      float sy;
      if (side == 1) {
        sx = 97;
      } else if (side == 3) {
        sx = 3;
      } else {
        sx = rnd(0, 100);
      }
      if (side == 0) {
        sy = 3;
      } else if (side == 2) {
        sy = 97;
      } else {
        sy = rnd(0, 100);
      }
      addRock(sx, sy, 9, rnd(-0.2, 0.2), rnd(-0.2, 0.2), 0, 0);
      nextRockT = rnd(200, 320) / sqrt(difficulty);
    }
  }

  float spd = sqrt(player.vx * player.vx + player.vy * player.vy);

  if (!frozen) {
    int ri;
    for (ri = 0; ri < CUECRUMB_MAX_ROCK_COUNT; ri++) {
      Rock *a = &rocks[ri];
      if (!a->isAlive) continue;
      int rj;
      for (rj = ri + 1; rj < CUECRUMB_MAX_ROCK_COUNT; rj++) {
        Rock *b = &rocks[rj];
        if (!b->isAlive) continue;
        float d = distanceTo(&a->pos, b->pos.x, b->pos.y);
        float minD = (a->size + b->size) / 2;
        if (d < minD && d > 0.01) {
          float ang = angleTo(&a->pos, b->pos.x, b->pos.y);
          float aFast = sqrt(a->vx * a->vx + a->vy * a->vy);
          float bFast = sqrt(b->vx * b->vx + b->vy * b->vy);
          Rock *frag = NULL;
          if (a->hot > 0 && aFast > 0.7) {
            frag = a;
          } else if (b->hot > 0 && bFast > 0.7) {
            frag = b;
          }
          Rock *target;
          if (frag == a) {
            target = b;
          } else {
            target = a;
          }
          if (frag != NULL && target->size > 4.5 && !target->splitQ && nonCrumbCount < 10) {
            target->splitQ = true;
            float thisFast;
            if (frag == a) {
              thisFast = aFast;
            } else {
              thisFast = bFast;
            }
            target->splitFast = thisFast;
            int fragCD = frag->chainDepth;
            if (fragCD == 0) {
              fragCD = 1;
            }
            target->splitChainDepth = fragCD + 1;
          } else {
            a->vx -= cos(ang) * 0.3;
            a->vy -= sin(ang) * 0.3;
            b->vx += cos(ang) * 0.3;
            b->vy += sin(ang) * 0.3;
          }
        }
      }
    }
  }

  FOR_EACH(rocks, rsi) {
    ASSIGN_ARRAY_ITEM(rocks, rsi, Rock, r);
    SKIP_IS_NOT_ALIVE(r);
    if (!r->splitQ) continue;
    int chainDepth = r->splitChainDepth;
    if (chainDepth == 0) {
      chainDepth = 2;
    }
    int tier;
    if (r->splitFast > 1.6) {
      tier = 2;
    } else if (r->splitFast > 1.0) {
      tier = 1;
    } else {
      tier = 0;
    }
    int burstColor;
    if (tier == 2) {
      burstColor = LIGHT_RED;
    } else if (tier == 1) {
      burstColor = RED;
    } else {
      burstColor = LIGHT_BLACK;
    }
    play(HIT);
    color = burstColor;
    particle(r->pos.x, r->pos.y, 12 + tier * 6, 2 + tier * 0.5, 0, M_PI * 2);
    addScore(chainDepth, r->pos.x, r->pos.y - 6);
    hitstop = fmax(hitstop, clamp(2 + tier, 2, 5));
    shakeMag = fmax(shakeMag, clamp(1 + tier, 1, 4));
    shakeT = 7;
    float a0 = rnd(0, M_PI * 2);
    TIMES(2, fragI) {
      float sa;
      if (fragI == 0) {
        sa = a0 - 0.6;
      } else {
        sa = a0 + 0.6;
      }
      addRock(r->pos.x + cos(sa) * 4, r->pos.y + sin(sa) * 4, r->size * 0.62,
              cos(sa) * 1.3, sin(sa) * 1.3, 55, chainDepth);
    }
    killRock(r);
  }

  FOR_EACH(rocks, ri2) {
    ASSIGN_ARRAY_ITEM(rocks, ri2, Rock, r);
    SKIP_IS_NOT_ALIVE(r);
    if (!frozen) {
      if (r->hot > 0) {
        r->hot--;
      }
      r->vx *= 0.985;
      r->vy *= 0.985;
      r->pos.x += r->vx;
      r->pos.y += r->vy;
      if (r->pos.x < 4 || r->pos.x > 96) {
        r->vx = -r->vx;
      }
      if (r->pos.y < 4 || r->pos.y > 96) {
        r->vy = -r->vy;
      }
      r->pos.x = clamp(r->pos.x, 4, 96);
      r->pos.y = clamp(r->pos.y, 4, 96);
    }
    float rSpd = sqrt(r->vx * r->vx + r->vy * r->vy);

    float d2 = distanceTo(&r->pos, player.pos.x, player.pos.y);
    if (d2 < r->size / 2 + 3) {
      if (r->size <= 4.5 && r->hot <= 0) {
        addScore(2 + floor(difficulty), r->pos.x, r->pos.y);
        play(COIN);
        color = YELLOW;
        particle(r->pos.x, r->pos.y, 8, 1.5, 0, M_PI * 2);
        killRock(r);
        continue;
      }
      if (r->hot > 0 && rSpd > 0.4) {
        play(EXPLOSION);
        color = LIGHT_RED;
        particle(player.pos.x, player.pos.y, 30, 3, 0, M_PI * 2);
        gameOver();
      } else if (spd > 0.9 && r->size > 4.5) {
        int tier2;
        if (spd > 2.2) {
          tier2 = 2;
        } else if (spd > 1.4) {
          tier2 = 1;
        } else {
          tier2 = 0;
        }
        int burstColor2;
        if (tier2 == 2) {
          burstColor2 = LIGHT_RED;
        } else if (tier2 == 1) {
          burstColor2 = RED;
        } else {
          burstColor2 = YELLOW;
        }
        play(HIT);
        color = burstColor2;
        particle(r->pos.x, r->pos.y, 15 + tier2 * 6, 2 + tier2, 0, M_PI * 2);
        addScore(1, r->pos.x, r->pos.y - 6);
        hitstop = fmax(hitstop, clamp(2 + tier2 * 2, 2, 6));
        shakeMag = fmax(shakeMag, clamp(1 + tier2 * 1.5, 1, 5));
        shakeT = 8;
        float a2 = angleTo(&player.pos, r->pos.x, r->pos.y);
        TIMES(2, fragI2) {
          float sa2;
          if (fragI2 == 0) {
            sa2 = a2 - 0.6;
          } else {
            sa2 = a2 + 0.6;
          }
          addRock(r->pos.x + cos(sa2) * 4, r->pos.y + sin(sa2) * 4, r->size * 0.62,
                  cos(sa2) * (1.2 + spd * 0.4), sin(sa2) * (1.2 + spd * 0.4), 55, 1);
        }
        player.vx *= 0.3;
        player.vy *= 0.3;
        killRock(r);
        continue;
      } else {
        float a3 = angleTo(&player.pos, r->pos.x, r->pos.y);
        r->vx += cos(a3) * 0.5;
        r->vy += sin(a3) * 0.5;
      }
    }

    if (r->hot > 0) {
      color = RED;
    } else if (r->size <= 4.5) {
      color = YELLOW;
    } else {
      color = LIGHT_BLACK;
    }
    box(VEC_XY(r->pos), r->size, r->size);
  }

  float shakeX = 0;
  float shakeY = 0;
  if (shakeT > 0) {
    shakeT--;
    float m = shakeMag * (shakeT / 8);
    shakeX = rnd(-m, m);
    shakeY = rnd(-m, m);
  }

  if (spd > 1.2) {
    color = LIGHT_CYAN;
    box(player.pos.x - player.vx * 1.6, player.pos.y - player.vy * 1.6, 4, 4);
    color = CYAN;
    box(player.pos.x - player.vx * 3.2, player.pos.y - player.vy * 3.2, 3, 3);
  }

  color = LIGHT_BLUE;
  thickness = 2;
  bar(player.pos.x + cos(player.face) * 8 + shakeX,
      player.pos.y + sin(player.face) * 8 + shakeY, 4, player.face);
  color = CYAN;
  box(VEC_XY(player.pos), 5, 5);
}

void addGameCueCrumb() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
