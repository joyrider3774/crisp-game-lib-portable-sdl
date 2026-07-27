#include "cglp.h"

static char *title = "BARGE HOP";
static char *description = "[Tap]  Jump\n[Hold] Reduce gravity";

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
    .viewSizeX = 150, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define BARGEHOP_WORLD_W 150
#define BARGEHOP_WORLD_H 100
#define BARGEHOP_WATER_Y 78
#define BARGEHOP_WATER_TOP_Y 71
#define BARGEHOP_DOCK_W 14
#define BARGEHOP_DOCK_H 12
#define BARGEHOP_BARGE_H 8
#define BARGEHOP_BARGE_CLEARANCE 5
#define BARGEHOP_PLAYER_W 6
#define BARGEHOP_PLAYER_H 8
#define BARGEHOP_GRAVITY 0.15
#define BARGEHOP_JUMP_VY -2.1
#define BARGEHOP_JUMP_VX 0.7
#define BARGEHOP_TIME_LIMIT 1000
#define BARGEHOP_MAX_TIME_BONUS 120
#define BARGEHOP_JUMP_DUR 16
#define BARGEHOP_LAND_DUR 12
#define BARGEHOP_JUMP_STRETCH_SX 0.8
#define BARGEHOP_JUMP_STRETCH_SY 1.35
#define BARGEHOP_LAND_SQUASH_SX 1.35
#define BARGEHOP_LAND_SQUASH_SY 0.8
#define BARGEHOP_BARGE_COUNT 3

typedef struct {
  float x;
  float y;
  float w;
  float h;
} Dock;
static Dock leftDock;
static Dock rightDock;

typedef struct {
  float x;
  float y;
  float w;
  float h;
  float vx;
  float amp;
  float freq;
  float phase;
} Barge;
static Barge barges[BARGEHOP_BARGE_COUNT];

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  bool onGround;
  int onBargeIndex;
} Player;
static Player player;

static int roundLevel;
static float jumpAnimT;
static float landAnimT;
static float remain;

static float lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

static void initRound() {
  float dockRaise = 3;
  leftDock.x = BARGEHOP_DOCK_W / 2 + 1;
  leftDock.y = BARGEHOP_WATER_Y - BARGEHOP_DOCK_H / 2 - dockRaise;
  leftDock.w = BARGEHOP_DOCK_W;
  leftDock.h = BARGEHOP_DOCK_H;
  rightDock.x = BARGEHOP_WORLD_W - (BARGEHOP_DOCK_W / 2 + 1);
  rightDock.y = BARGEHOP_WATER_Y - BARGEHOP_DOCK_H / 2 - dockRaise;
  rightDock.w = BARGEHOP_DOCK_W;
  rightDock.h = BARGEHOP_DOCK_H;

  float roundDifficulty = 1 + sqrt(roundLevel * 0.3);
  float speedScale = 0.5 * roundDifficulty;
  float speedBase = 0.35 * speedScale;
  float baseY = BARGEHOP_WATER_Y - BARGEHOP_BARGE_H / 2;

  TIMES(BARGEHOP_BARGE_COUNT, bgi) {
    float w = clamp(rndi(20, 30), 12, 28);
    float dir;
    if (rnd(0, 1) < 0.5) {
      dir = -1;
    } else {
      dir = 1;
    }
    float sp = speedBase * rnd(0.9, 1.5) * dir;
    float leftEdge2 = leftDock.x + leftDock.w / 2 + w / 2 + 2;
    float rightEdge2 = rightDock.x - rightDock.w / 2 - w / 2 - 2;
    float x;
    int tries = 0;
    bool overlap;
    do {
      x = rnd(leftEdge2, rightEdge2);
      tries++;
      if (tries > 50) break;
      overlap = false;
      int checkI;
      for (checkI = 0; checkI < bgi; checkI++) {
        if (fabs(barges[checkI].x - x) < (barges[checkI].w + w) * 0.55) {
          overlap = true;
        }
      }
    } while (overlap);

    Barge *bg = &barges[bgi];
    bg->x = x;
    bg->y = baseY;
    bg->w = w;
    bg->h = BARGEHOP_BARGE_H;
    bg->vx = sp;
    bg->amp = rnd(1, 2);
    bg->freq = rnd(0.02, 0.05) * roundDifficulty;
    bg->phase = rnd(0, M_PI * 2);
  }

  player.x = leftDock.x;
  player.y = leftDock.y - leftDock.h / 2 - BARGEHOP_PLAYER_H / 2;
  player.vx = 0;
  player.vy = 0;
  player.onGround = true;
  player.onBargeIndex = -1;
  remain = clamp(remain + BARGEHOP_TIME_LIMIT / 2, 0, BARGEHOP_TIME_LIMIT);
}

static void nextRound() {
  roundLevel++;
  initRound();
}

static void update() {
  if (!ticks) {
    roundLevel = 0;
    remain = BARGEHOP_TIME_LIMIT;
    initRound();
  }

  float roundDifficulty = 1 + roundLevel / 5.0;
  remain -= sqrt(roundDifficulty);
  float remainRatio = remain / BARGEHOP_TIME_LIMIT;
  if (remain <= 0) {
    gameOver();
  }

  color = LIGHT_BLUE;
  box(BARGEHOP_WORLD_W / 2, BARGEHOP_WATER_Y + 11, BARGEHOP_WORLD_W, 44);

  float barH = 4;
  float barPad = 3;
  float barWMax = BARGEHOP_WORLD_W - barPad * 2;
  float barX = barPad + barWMax * 0.5;
  float barY = BARGEHOP_WORLD_H - barPad - barH * 0.5;
  color = LIGHT_BLACK;
  box(barX, barY, barWMax, barH);
  float barW = barWMax * remainRatio;
  float barCX = barPad + barW * 0.5;
  int blinkPeriod = 8;
  bool isWarning = remainRatio <= 0.25;
  int fillColor;
  if (isWarning && (ticks / blinkPeriod) % 2 == 0) {
    fillColor = RED;
  } else {
    fillColor = YELLOW;
  }
  color = fillColor;
  if (barW > 0) {
    box(barCX, barY, barW, barH - 1);
  }

  color = BLACK;
  box(leftDock.x, leftDock.y, leftDock.w, leftDock.h);
  box(rightDock.x, rightDock.y, rightDock.w, rightDock.h);

  color = GREEN;
  TIMES(BARGEHOP_BARGE_COUNT, bgi2) {
    Barge *b = &barges[bgi2];
    b->x += b->vx;
    b->y = BARGEHOP_WATER_Y - BARGEHOP_BARGE_H / 2 - b->amp - BARGEHOP_BARGE_CLEARANCE +
           sin(ticks * b->freq + b->phase) * b->amp;
    float leftEdge = leftDock.x + leftDock.w / 2;
    float rightEdge = rightDock.x - rightDock.w / 2;
    if (b->x - b->w / 2 <= leftEdge) {
      b->x = leftEdge + b->w / 2;
      b->vx = fabs(b->vx);
    }
    if (b->x + b->w / 2 >= rightEdge) {
      b->x = rightEdge - b->w / 2;
      b->vx = -fabs(b->vx);
    }
  }

  int bi;
  for (bi = 0; bi < BARGEHOP_BARGE_COUNT; bi++) {
    int bj;
    for (bj = bi + 1; bj < BARGEHOP_BARGE_COUNT; bj++) {
      Barge *a = &barges[bi];
      Barge *b = &barges[bj];
      if (fabs(a->y - b->y) < (a->h + b->h) * 0.5) {
        float dx = b->x - a->x;
        float overlap2 = a->w * 0.5 + b->w * 0.5 - fabs(dx);
        if (overlap2 > 0) {
          float push = overlap2 / 2 + 0.01;
          if (dx > 0) {
            a->x -= push;
            b->x += push;
          } else {
            a->x += push;
            b->x -= push;
          }
          int sA;
          if (a->vx >= 0) {
            sA = 1;
          } else {
            sA = -1;
          }
          int sB;
          if (b->vx >= 0) {
            sB = 1;
          } else {
            sB = -1;
          }
          if (sA == sB) {
            bool movingRight = sA > 0;
            Barge *trailing;
            if (movingRight) {
              if (a->x < b->x) {
                trailing = a;
              } else {
                trailing = b;
              }
            } else {
              if (a->x > b->x) {
                trailing = a;
              } else {
                trailing = b;
              }
            }
            trailing->vx = -trailing->vx;
          } else {
            a->vx = -a->vx;
            b->vx = -b->vx;
          }
          play(HIT);
        }
      }
    }
  }

  float bargeTops[BARGEHOP_BARGE_COUNT];
  TIMES(BARGEHOP_BARGE_COUNT, bgi3) {
    Barge *b = &barges[bgi3];
    box(b->x, b->y, b->w, b->h);
    bargeTops[bgi3] = b->y - b->h / 2;
  }

  if (jumpAnimT > 0) {
    jumpAnimT--;
  }
  if (landAnimT > 0) {
    landAnimT--;
  }
  float wj = jumpAnimT / BARGEHOP_JUMP_DUR;
  float wl = landAnimT / BARGEHOP_LAND_DUR;
  float scaleX = lerp(1, BARGEHOP_JUMP_STRETCH_SX, wj) * lerp(1, BARGEHOP_LAND_SQUASH_SX, wl);
  float scaleY = lerp(1, BARGEHOP_JUMP_STRETCH_SY, wj) * lerp(1, BARGEHOP_LAND_SQUASH_SY, wl);
  float drawW = BARGEHOP_PLAYER_W * scaleX;
  float drawH = BARGEHOP_PLAYER_H * scaleY;

  if (player.onGround) {
    player.vx = 0;
    player.vy = 0;
    if (player.onBargeIndex >= 0) {
      int bIdx = player.onBargeIndex;
      player.x += barges[bIdx].vx;
      player.y = bargeTops[bIdx] - BARGEHOP_PLAYER_H / 2;
    }
  } else {
    float gMul;
    if (input.isPressed) {
      gMul = 0.5;
    } else {
      gMul = 1.5;
    }
    player.vy += BARGEHOP_GRAVITY * gMul * roundDifficulty;
    player.x += player.vx;
    player.y += player.vy;
  }

  if (input.isJustPressed) {
    if (player.onGround) {
      player.onGround = false;
      player.onBargeIndex = -1;
      player.vx = BARGEHOP_JUMP_VX * sqrt(roundDifficulty);
      player.vy = BARGEHOP_JUMP_VY * sqrt(roundDifficulty);
      play(JUMP);
      jumpAnimT = BARGEHOP_JUMP_DUR;
    }
  }

  color = YELLOW;
  float footBottom = player.y + drawH * 0.5;
  float footBoxY = footBottom - 1;
  box(player.x, player.y, drawW, drawH);
  box(player.x, footBoxY, BARGEHOP_PLAYER_W * 0.6, 5);
  color = TRANSPARENT;
  Collision pc = box(player.x, footBoxY - 48, drawW, 100);

  if (!player.onGround) {
    bool inRightDockX = player.x >= rightDock.x - rightDock.w * 0.6 &&
                         player.x <= rightDock.x + rightDock.w * 0.6;
    bool landedOnRightDock = pc.isColliding.rect[BLACK] && player.vy >= 0 && inRightDockX;
    bool landedOnBarge = false;
    int landedIndex = -1;
    if (pc.isColliding.rect[GREEN] && player.vy >= 0) {
      int bi2;
      for (bi2 = BARGEHOP_BARGE_COUNT - 1; bi2 >= 0; bi2--) {
        Barge *b = &barges[bi2];
        if (fabs(player.x - b->x) <= BARGEHOP_PLAYER_W * 0.6 + b->w * 0.6) {
          landedOnBarge = true;
          landedIndex = bi2;
          break;
        }
      }
    }

    if (landedOnRightDock) {
      int bonus = (int)floor(BARGEHOP_MAX_TIME_BONUS * remainRatio);
      addScore(bonus, player.x, player.y - 6);
      play(COIN);
      nextRound();
      return;
    } else if (landedOnBarge) {
      float top = bargeTops[landedIndex];
      player.onGround = true;
      player.onBargeIndex = landedIndex;
      player.y = top - BARGEHOP_PLAYER_H / 2;
      player.vx = 0;
      player.vy = 0;
      addScore(1, player.x, player.y - 6);
      play(HIT);
      landAnimT = BARGEHOP_LAND_DUR;
    }
  }

  if (!player.onGround && player.vy >= 0 && footBottom >= BARGEHOP_WATER_TOP_Y) {
    color = CYAN;
    particle(player.x, BARGEHOP_WATER_TOP_Y, 20, 2.0, -M_PI_2, M_PI / 1.3);
    color = LIGHT_BLUE;
    particle(player.x, BARGEHOP_WATER_TOP_Y, 14, 1.2, -M_PI_2, M_PI);
    color = LIGHT_CYAN;
    particle(player.x, BARGEHOP_WATER_TOP_Y - 1, 8, 1.4, 0, M_PI / 6);
    particle(player.x, BARGEHOP_WATER_TOP_Y - 1, 8, 1.4, M_PI, M_PI / 6);
    play(EXPLOSION);
    player.onGround = true;
    player.onBargeIndex = -1;
    player.x = leftDock.x;
    player.y = leftDock.y - leftDock.h / 2 - BARGEHOP_PLAYER_H / 2;
    player.vx = 0;
    player.vy = 0;
    landAnimT = BARGEHOP_LAND_DUR;
  }
}

void addGameBargeHop() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
