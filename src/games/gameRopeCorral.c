#include "cglp.h"

static char *title = "ROPE CORRAL";
static char *description = "[Hold] Anchor & swing\n[Release] Roll on";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 45, .isDarkColor = false};

#define ROPECORRAL_ROPE 22

typedef struct {
  Vector pos;
  float vx;
  float vy;
} Player;
static Player player;

typedef struct {
  Vector pos;
} Buddy;
static Buddy buddy;

typedef struct {
  Vector pos;
  float ang;
  bool isAlive;
} Enemy;
#define ROPECORRAL_MAX_ENEMY_COUNT 32
static Enemy enemies[ROPECORRAL_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  float warm;
  bool isAlive;
} Spike;
#define ROPECORRAL_MAX_SPIKE_COUNT 16
static Spike spikes[ROPECORRAL_MAX_SPIKE_COUNT];
static int spikeIndex;

static float swingA;
static float swept;
static float nextCritT;
static float hitstop;
static float shakeMag;
static float shakeT;

// A single-pixel "eye" toward a facing angle - bodies here are only 5px,
// too small for a readable whites+pupil pair, so one offset dot stands in.
static void eye(float x, float y, float ang, float dist) {
  color = BLACK;
  box(x + cos(ang) * dist, y + sin(ang) * dist, 1, 1);
}

static void spawnCritter() {
  ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
  vectorSet(&ne->pos, rnd(15, 85), rnd(15, 85));
  ne->ang = rnd(0, M_PI * 2);
  ne->isAlive = true;
  enemyIndex = wrap(enemyIndex + 1, 0, ROPECORRAL_MAX_ENEMY_COUNT);
}

static void update() {
  // Never reads a Collision result - corralling/hits are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 30, 50);
    player.vx = 0.75;
    player.vy = 0.32;
    vectorSet(&buddy.pos, 20, 50);
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    TIMES(5, ci) {
      Vector p;
      vectorSet(&p, rnd(15, 85), rnd(15, 85));
      if (distanceTo(&p, 30, 50) < 18) {
        p.x = wrap(p.x + 40, 15, 85);
      }
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne0);
      ne0->pos = p;
      ne0->ang = rnd(0, M_PI * 2);
      ne0->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, ROPECORRAL_MAX_ENEMY_COUNT);
    }
    TIMES(3, si) {
      Vector p2;
      vectorSet(&p2, 50, 50);
      int k;
      for (k = 0; k < 8; k++) {
        vectorSet(&p2, rnd(10, 90), rnd(10, 90));
        bool ok = distanceTo(&p2, 30, 50) >= 20;
        FOR_EACH(spikes, sj) {
          ASSIGN_ARRAY_ITEM(spikes, sj, Spike, sChk0);
          SKIP_IS_NOT_ALIVE(sChk0);
          if (distanceTo(&sChk0->pos, VEC_XY(p2)) < 24) {
            ok = false;
          }
        }
        if (ok) {
          break;
        }
      }
      ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, ns0);
      ns0->pos = p2;
      ns0->warm = 0;
      ns0->isAlive = true;
      spikeIndex = wrap(spikeIndex + 1, 0, ROPECORRAL_MAX_SPIKE_COUNT);
    }
    swingA = 0;
    swept = 0;
    nextCritT = 200;
    hitstop = 0;
    shakeMag = 0;
    shakeT = 0;
  }

  // difficulty = pace: everything moves faster
  float pace = 1 + (difficulty - 1) * 0.35;
  bool frozen = hitstop > 0;
  if (frozen) {
    hitstop--;
  }

  if (input.isPressed) {
    // buddy anchors; you swing around it
    if (input.isJustPressed) {
      swingA = angleTo(&buddy.pos, VEC_XY(player.pos));
      swept = 0;
      play(SELECT);
    }
    float w = 0.075 * pace;
    if (!frozen) {
      swingA += w;
      swept += w;
    }
    vectorSet(&player.pos, buddy.pos.x + cos(swingA) * ROPECORRAL_ROPE,
              buddy.pos.y + sin(swingA) * ROPECORRAL_ROPE);
    player.vx = -sin(swingA) * (ROPECORRAL_ROPE * w * 0.8);
    player.vy = cos(swingA) * (ROPECORRAL_ROPE * w * 0.8);
    // full revolution -> corral everything inside the circle (critters AND spikes)
    if (swept >= M_PI * 2) {
      swept = 0;
      int nCrit = 0;
      FOR_EACH(enemies, ei) {
        ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
        SKIP_IS_NOT_ALIVE(e);
        if (distanceTo(&e->pos, VEC_XY(buddy.pos)) < ROPECORRAL_ROPE - 2) {
          nCrit++;
        }
      }
      int nSpike = 0;
      FOR_EACH(spikes, si2) {
        ASSIGN_ARRAY_ITEM(spikes, si2, Spike, s2);
        SKIP_IS_NOT_ALIVE(s2);
        if (distanceTo(&s2->pos, VEC_XY(buddy.pos)) < ROPECORRAL_ROPE - 2) {
          nSpike++;
        }
      }
      int n = nCrit + nSpike;
      if (n > 0) {
        // particle color/count escalate with corral size - the jackpot gets louder
        int chainColor;
        if (n >= 6) {
          // Vircon32 port note: no "magenta" in this engine's 15-color
          // palette (see cglp.h) - the biggest corral tier borrows
          // light_purple instead.
          chainColor = LIGHT_PURPLE;
        } else if (n >= 3) {
          chainColor = LIGHT_RED;
        } else {
          chainColor = LIGHT_YELLOW;
        }
        color = chainColor;
        FOR_EACH(enemies, ei2) {
          ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e2);
          SKIP_IS_NOT_ALIVE(e2);
          if (distanceTo(&e2->pos, VEC_XY(buddy.pos)) < ROPECORRAL_ROPE - 2) {
            particle(VEC_XY(e2->pos), 10 + n * 2, 2 + n * 0.25, 0, M_PI * 2);
            e2->isAlive = false;
          }
        }
        FOR_EACH(spikes, si3) {
          ASSIGN_ARRAY_ITEM(spikes, si3, Spike, s3);
          SKIP_IS_NOT_ALIVE(s3);
          if (distanceTo(&s3->pos, VEC_XY(buddy.pos)) < ROPECORRAL_ROPE - 2) {
            particle(VEC_XY(s3->pos), 10 + n * 2, 2 + n * 0.25, 0, M_PI * 2);
            s3->isAlive = false;
          }
        }
        particle(VEC_XY(buddy.pos), 16 + n * 3, 2.5 + n * 0.3, 0, M_PI * 2);
        addScore(n * n * 4, VEC_XY(buddy.pos));
        play(POWER_UP);
        hitstop = clamp(2 + n, 2, 8);
        shakeMag = clamp(1.5 + n, 1.5, 7);
        shakeT = 8;
        // everything removed (critters AND spikes) is immediately replaced
        // by a critter - clearing a spike this way nets +1 critter on
        // screen, not just a like-for-like swap
        TIMES(n, spawnIdx) {
          spawnCritter();
        }
      } else {
        play(CLICK);
      }
    }
  } else {
    // roll free; buddy trails behind on the rope
    if (!frozen) {
      player.pos.x += player.vx * pace;
      player.pos.y += player.vy * pace;
      if (player.pos.x < 6 || player.pos.x > 94) {
        player.vx = -player.vx;
      }
      if (player.pos.y < 6 || player.pos.y > 94) {
        player.vy = -player.vy;
      }
      player.pos.x = clamp(player.pos.x, 6, 94);
      player.pos.y = clamp(player.pos.y, 6, 94);
      float d = distanceTo(&buddy.pos, VEC_XY(player.pos));
      if (d > ROPECORRAL_ROPE) {
        float a = angleTo(&buddy.pos, VEC_XY(player.pos));
        vectorSet(&buddy.pos, player.pos.x - cos(a) * ROPECORRAL_ROPE,
                  player.pos.y - sin(a) * ROPECORRAL_ROPE);
      }
    }
  }

  if (!frozen) {
    nextCritT--;
    COUNT_IS_ALIVE(enemies, enemyCountNow);
    if (nextCritT < 0 && enemyCountNow < 8) {
      spawnCritter();
      nextCritT = rnd(140, 220) / sqrt(difficulty);
    }
  }

  // spikes slowly accrue (telegraphed), spaced away from other spikes
  if (ticks % 700 == 699) {
    COUNT_IS_ALIVE(spikes, spikeCountNow);
    if (spikeCountNow < 6) {
      Vector p3;
      vectorSet(&p3, 50, 50);
      int k2;
      for (k2 = 0; k2 < 8; k2++) {
        vectorSet(&p3, rnd(10, 90), rnd(10, 90));
        bool ok2 = distanceTo(&p3, VEC_XY(player.pos)) >= 20;
        FOR_EACH(spikes, sk) {
          ASSIGN_ARRAY_ITEM(spikes, sk, Spike, sChk);
          SKIP_IS_NOT_ALIVE(sChk);
          if (distanceTo(&sChk->pos, VEC_XY(p3)) < 24) {
            ok2 = false;
          }
        }
        if (ok2) {
          break;
        }
      }
      ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, ns);
      ns->pos = p3;
      ns->warm = 60;
      ns->isAlive = true;
      spikeIndex = wrap(spikeIndex + 1, 0, ROPECORRAL_MAX_SPIKE_COUNT);
    }
  }

  // critters wander; direct touch COLLECTS one (+1)
  int collected = 0;
  FOR_EACH(enemies, eci) {
    ASSIGN_ARRAY_ITEM(enemies, eci, Enemy, ec);
    SKIP_IS_NOT_ALIVE(ec);
    ec->ang += rnd(-0.2, 0.2);
    addWithAngle(&ec->pos, ec->ang, 0.28 * pace);
    ec->pos.x = clamp(ec->pos.x, 8, 92);
    ec->pos.y = clamp(ec->pos.y, 8, 92);
    if (ec->pos.x <= 8 || ec->pos.x >= 92) {
      ec->ang = M_PI - ec->ang;
    }
    if (ec->pos.y <= 8 || ec->pos.y >= 92) {
      ec->ang = -ec->ang;
    }
    if (distanceTo(&ec->pos, VEC_XY(player.pos)) < 4.5) {
      addScore(1, VEC_XY(ec->pos));
      play(COIN);
      // Upstream left this particle's color incidental; explicitly yellow here (matches critter's own color).
      color = YELLOW;
      particle(VEC_XY(ec->pos), 8, 1.5, 0, M_PI * 2);
      collected++;
      ec->isAlive = false;
      continue;
    }
  }
  // each collected critter is immediately replaced, so the trickle never runs dry
  TIMES(collected, spawnIdx2) {
    spawnCritter();
  }

  // decaying screen shake on a corral capture - decorative rope/arc only,
  // never the box() hitboxes
  float shakeX = 0;
  float shakeY = 0;
  if (shakeT > 0) {
    shakeT--;
    float m = shakeMag * (shakeT / 8);
    shakeX = rnd(-m, m);
    shakeY = rnd(-m, m);
  }

  // draw: swing sweep indicator
  if (input.isPressed) {
    color = LIGHT_YELLOW;
    thickness = 1;
    arc(buddy.pos.x + shakeX, buddy.pos.y + shakeY, ROPECORRAL_ROPE,
        swingA - swept, swingA);
  }
  color = LIGHT_BLACK;
  thickness = 1;
  line(player.pos.x + shakeX, player.pos.y + shakeY, buddy.pos.x + shakeX,
       buddy.pos.y + shakeY);

  // afterimage trail, algebraic from current velocity (straight-line roll
  // only, no history array; curved swing motion is excluded since a
  // straight offset would misread the arc)
  if (!input.isPressed) {
    float spd = sqrt(player.vx * player.vx + player.vy * player.vy) * pace;
    if (spd > 1.5) {
      color = LIGHT_CYAN;
      box(clamp(player.pos.x - player.vx * pace * 2.5, 6, 94),
          clamp(player.pos.y - player.vy * pace * 2.5, 6, 94), 4, 4);
      color = CYAN;
      box(clamp(player.pos.x - player.vx * pace * 5, 6, 94),
          clamp(player.pos.y - player.vy * pace * 5, 6, 94), 3, 3);
    }
  }

  // spikes kill YOUR ball
  FOR_EACH(spikes, sdi) {
    ASSIGN_ARRAY_ITEM(spikes, sdi, Spike, sd);
    SKIP_IS_NOT_ALIVE(sd);
    if (sd->warm > 0) {
      sd->warm--;
      if (((int)sd->warm) % 8 < 4) {
        color = LIGHT_RED;
        box(VEC_XY(sd->pos), 2, 2);
      }
      continue;
    }
    color = RED;
    box(VEC_XY(sd->pos), 3, 3);
    if (distanceTo(&sd->pos, VEC_XY(player.pos)) < 4) {
      play(EXPLOSION);
      color = RED;
      particle(VEC_XY(player.pos), 25, 3, 0, M_PI * 2);
      gameOver();
    }
  }

  // critters (collectible - yellow), eye toward their own wander direction
  FOR_EACH(enemies, edi) {
    ASSIGN_ARRAY_ITEM(enemies, edi, Enemy, ed);
    SKIP_IS_NOT_ALIVE(ed);
    color = YELLOW;
    box(VEC_XY(ed->pos), 5, 4);
    eye(ed->pos.x, ed->pos.y, ed->ang, 1.2);
  }

  color = LIGHT_BLUE;
  box(VEC_XY(buddy.pos), 5, 5);
  eye(buddy.pos.x, buddy.pos.y, angleTo(&buddy.pos, VEC_XY(player.pos)), 1.4);
  color = CYAN;
  box(VEC_XY(player.pos), 5, 5);
  eye(player.pos.x, player.pos.y, atan2(player.vy, player.vx), 1.4);
}

void addGameRopeCorral() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
