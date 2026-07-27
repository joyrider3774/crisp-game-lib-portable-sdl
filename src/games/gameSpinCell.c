#include "cglp.h"

static char *title = "SPIN CELL";
static char *description = "[Tap] Rotate the cell";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 25, .isDarkColor = false};

#define WALL_COUNT 12
static bool wallSpikes[WALL_COUNT];

typedef struct {
  Vector pos;
  float vx;
  float vy;
} Player;
static Player player;

static int gemCorner;
static float creepT;
static int warnSeg;
static float rotFx;
static bool dying;
static int dieT;
static int nudgeT;

#define TRAIL_CAPACITY 4
static Vector trail[TRAIL_CAPACITY];
static int trailCount;

// Returns (via out-pointer) the on-screen center point of perimeter segment
// `i` (0..11) - used both to place spike-warning particles and to draw the
// rotation pulse ring's reference geometry.
static void segCenter(int i, Vector *result) {
  int side = i / 3;
  int idx = i % 3;
  float L = 86.0 / 3;
  if (side == 0) {
    vectorSet(result, 7 + idx * L + L / 2, 5);
  } else if (side == 1) {
    vectorSet(result, 95, 7 + idx * L + L / 2);
  } else if (side == 2) {
    vectorSet(result, 7 + (2 - idx) * L + L / 2, 95);
  } else {
    vectorSet(result, 5, 7 + (2 - idx) * L + L / 2);
  }
}

static void update() {
  // Never reads a Collision result - bounces/spikes are decided by geometry math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 50, 40);
    player.vx = rnd(0.4, 0.8);
    player.vy = 0;
    TIMES(WALL_COUNT, wi0) {
      wallSpikes[wi0] = false;
    }
    wallSpikes[1] = true;
    wallSpikes[7] = true;
    gemCorner = rndi(0, 4);
    creepT = 400;
    warnSeg = -1;
    rotFx = 0;
    trailCount = 0;
    dying = false;
    dieT = 0;
    nudgeT = 0;
  }

  if (!dying) {
    if (input.isJustPressed) {
      bool nw[WALL_COUNT];
      TIMES(WALL_COUNT, wi) {
        nw[(wi + 1) % WALL_COUNT] = wallSpikes[wi];
      }
      TIMES(WALL_COUNT, wi2) {
        wallSpikes[wi2] = nw[wi2];
      }
      if (warnSeg >= 0) {
        warnSeg = (warnSeg + 1) % WALL_COUNT;
      }
      if (player.pos.y > 87) {
        player.vx -= 0.6;
        player.vx = clamp(player.vx, -1.4, 1.4);
        nudgeT = 8;
        color = LIGHT_CYAN;
        particle(player.pos.x, player.pos.y, 8, 2, 0, M_PI / 4);
        play(CLICK);
      }
      rotFx = 12;
      play(SELECT);
    }

    creepT--;
    if (creepT < 0) {
      if (warnSeg < 0) {
        int cand[WALL_COUNT];
        int candCount = 0;
        TIMES(WALL_COUNT, ci) {
          if (!wallSpikes[ci]) {
            cand[candCount] = ci;
            candCount++;
          }
        }
        if (candCount > 4) {
          warnSeg = cand[rndi(0, candCount)];
          creepT = 90;
        } else {
          creepT = 500;
        }
      } else {
        wallSpikes[warnSeg] = true;
        color = RED;
        Vector segC;
        segCenter(warnSeg, &segC);
        particle(segC.x, segC.y, 10, 1.5, 0, M_PI * 2);
        warnSeg = -1;
        int spikeCount = 0;
        TIMES(WALL_COUNT, si) {
          if (wallSpikes[si]) {
            spikeCount++;
          }
        }
        creepT = (200 * sqrt(spikeCount + 1)) / sqrt(difficulty);
        play(HIT);
      }
    }

    player.vy += 0.06;
    player.pos.x += player.vx;
    player.pos.y += player.vy;

    if (trailCount < TRAIL_CAPACITY) {
      trail[trailCount] = player.pos;
      trailCount++;
    } else {
      TIMES(TRAIL_CAPACITY - 1, si2) {
        trail[si2] = trail[si2 + 1];
      }
      trail[TRAIL_CAPACITY - 1] = player.pos;
    }

    int hitSide = -1;
    float t = 0;
    if (player.pos.y < 9) {
      hitSide = 0;
      t = (player.pos.x - 7) / 86;
      player.pos.y = 9;
      player.vy = fabs(player.vy) * 0.9;
    } else if (player.pos.x > 91) {
      hitSide = 1;
      t = (player.pos.y - 7) / 86;
      player.pos.x = 91;
      player.vx = -fabs(player.vx) * 0.95;
    } else if (player.pos.y > 91) {
      hitSide = 2;
      t = (player.pos.x - 7) / 86;
      player.pos.y = 91;
      player.vy = -(3.0 + difficulty * 0.1);
      player.vx += rnd(-0.25, 0.25);
      player.vx = clamp(player.vx, -1.2, 1.2);
      play(JUMP);
    } else if (player.pos.x < 9) {
      hitSide = 3;
      t = (player.pos.y - 7) / 86;
      player.pos.x = 9;
      player.vx = fabs(player.vx) * 0.95;
    }
    if (hitSide >= 0) {
      int idx = clamp(floor(t * 3), 0, 2);
      if (hitSide == 2 || hitSide == 3) {
        idx = 2 - idx;
      }
      if (wallSpikes[hitSide * 3 + idx]) {
        dying = true;
        dieT = 8;
        play(EXPLOSION);
        particle(player.pos.x, player.pos.y, 25, 3, 0, M_PI * 2);
      }
    }
  }

  if (rotFx > 0) {
    rotFx--;
    color = LIGHT_BLACK;
    thickness = 1;
    arc(50, 50, 44 * (1 - rotFx / 12), 0, M_PI * 2);
  }

  TIMES(WALL_COUNT, di) {
    int side = di / 3;
    int idx = di % 3;
    float L = 86.0 / 3;
    if (wallSpikes[di]) {
      color = RED;
    } else if (di == warnSeg && fmod(creepT, 10) < 5) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    if (side == 0) {
      rect(7 + idx * L, 3, L, 4);
    }
    if (side == 1) {
      rect(93, 7 + idx * L, 4, L);
    }
    if (side == 2) {
      rect(7 + (2 - idx) * L, 93, L, 4);
    }
    if (side == 3) {
      rect(3, 7 + (2 - idx) * L, 4, L);
    }
  }

  float gpx;
  float gpy;
  if (gemCorner == 0) {
    gpx = 14;
    gpy = 14;
  } else if (gemCorner == 1) {
    gpx = 86;
    gpy = 14;
  } else if (gemCorner == 2) {
    gpx = 86;
    gpy = 86;
  } else {
    gpx = 14;
    gpy = 86;
  }
  color = YELLOW;
  box(gpx, gpy, 5, 5);
  if (!dying && distanceTo(&player.pos, gpx, gpy) < 8) {
    int spikeIdx[WALL_COUNT];
    int spikeIdxCount = 0;
    TIMES(WALL_COUNT, si3) {
      if (wallSpikes[si3]) {
        spikeIdx[spikeIdxCount] = si3;
        spikeIdxCount++;
      }
    }
    addScore(pow(fmax((float)spikeIdxCount, 1), 2), gpx, gpy);
    play(COIN);
    particle(gpx, gpy, 12, 2, 0, M_PI * 2);
    if (spikeIdxCount > 0) {
      int revert = spikeIdx[rndi(0, spikeIdxCount)];
      wallSpikes[revert] = false;
      color = LIGHT_BLACK;
      Vector revertC;
      segCenter(revert, &revertC);
      particle(revertC.x, revertC.y, 10, 1.5, 0, M_PI * 2);
      play(POWER_UP);
    }
    gemCorner = (gemCorner + 1 + rndi(0, 3)) % 4;
  }

  float spd = sqrt(player.vx * player.vx + player.vy * player.vy);
  if (spd > 1.5) {
    TIMES(trailCount, tri) {
      if (tri % 2 == 0) {
        color = LIGHT_CYAN;
      } else {
        color = CYAN;
      }
      box(trail[tri].x, trail[tri].y, 3, 3);
    }
  }

  if (nudgeT > 0) {
    nudgeT--;
    if (nudgeT % 2 == 0) {
      color = LIGHT_CYAN;
    } else {
      color = CYAN;
    }
  } else {
    color = CYAN;
  }
  box(player.pos.x, player.pos.y, 5, 5);

  if (dying) {
    dieT--;
    if (dieT <= 0) {
      gameOver();
    }
  }
}

void addGameSpinCell() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
