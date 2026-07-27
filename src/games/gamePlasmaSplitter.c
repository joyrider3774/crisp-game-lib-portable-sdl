#include "cglp.h"

static char *title = "PLASMA SPLITTER";
static char *description = "[Hold] Stop";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 50, .isDarkColor = false};

#define SCROLLING_BASE_SPEED 0.2

typedef struct {
  Vector pos;
  float size;
  bool isSplit;
  float splitPos;
  float vx;
} Player;
static Player player;

typedef struct {
  float start;
  float end;
} Gap;
typedef struct {
  Vector pos;
  Gap gaps[3];
  int gapCount;
  bool isAlive;
} Wall;
#define MAX_WALL_COUNT 16
static Wall walls[MAX_WALL_COUNT];
static int wallIndex;
static float nextWallDist;
static float scrollingY;

static int gapCountTable[6] = {1, 2, 2, 3, 3, 3};

static void drawLightning(float x1, float y1, float x2, float y2, int divisionCount) {
  if (divisionCount == 0) {
    thickness = 2;
    line(x1, y1, x2, y2);
    return;
  }
  float midX = (x1 + x2) / 2 + rnd(0, 3) * RNDPM();
  float midY = (y1 + y2) / 2 + rnd(0, 3) * RNDPM();
  drawLightning(x1, y1, midX, midY, divisionCount - 1);
  drawLightning(midX, midY, x2, y2, divisionCount - 1);
}

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 96);
    player.size = 8;
    player.isSplit = false;
    player.splitPos = 0;
    player.vx = 1;
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    nextWallDist = 0;
    scrollingY = SCROLLING_BASE_SPEED;
  }
  float sd = sqrt(difficulty);
  if (input.isPressed) {
    if (input.isJustPressed) {
      play(LASER);
    }
    scrollingY += sd * 0.01;
  } else {
    if (input.isJustReleased) {
      play(HIT);
      player.splitPos = player.pos.x;
    }
    scrollingY += (sd * SCROLLING_BASE_SPEED - scrollingY) * 0.1;
    player.pos.x += player.vx * sd;
    if (player.pos.x < player.size / 2 || player.pos.x > 100 - player.size / 2) {
      player.vx *= -1;
    }
  }
  color = LIGHT_BLUE;
  rect(player.pos.x, player.pos.y - 2, player.splitPos - player.pos.x, 2);
  color = YELLOW;
  TIMES(2, k) {
    drawLightning(player.pos.x, player.pos.y, player.splitPos, player.pos.y, 2);
  }
  color = CYAN;
  box(player.splitPos, player.pos.y, player.size, player.size);
  box(player.pos.x, player.pos.y, player.size, player.size);
  nextWallDist -= scrollingY;
  if (nextWallDist < 0) {
    int gapCount = gapCountTable[rndi(0, 6)];
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, nw);
    vectorSet(&nw->pos, 0, -3);
    nw->gapCount = gapCount;
    float gx = 0;
    TIMES(gapCount, i) {
      float gw = player.size * 3;
      float lo;
      if (i > 0) {
        lo = 10;
      } else {
        lo = 0;
      }
      gx += rnd(lo, 100 - gx - gw - (gapCount - i - 1) * (gw + 10));
      nw->gaps[i].start = gx;
      nw->gaps[i].end = gx + gw;
      gx += gw;
    }
    nw->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, MAX_WALL_COUNT);
    nextWallDist = rnd(50, 80);
  }
  color = BLUE;
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y += scrollingY;
    float wx = 0;
    bool isDestroyed = false;
    TIMES(w->gapCount, k) {
      Gap *g = &w->gaps[k];
      Collision scratch = rect(wx, w->pos.y, g->start - wx, 3);
      if (scratch.isColliding.rect[CYAN]) {
        play(EXPLOSION);
        gameOver();
      } else if (scratch.isColliding.rect[LIGHT_BLUE]) {
        isDestroyed = true;
        float dw = g->start - wx;
        float dx = wx + dw / 2;
        play(POWER_UP);
        particle(dx, w->pos.y, 9, 2, 0, M_PI * 2);
        addScore(floor(scrollingY / SCROLLING_BASE_SPEED * dw), dx, w->pos.y);
      }
      wx = g->end;
    }
    Collision scratch = rect(wx, w->pos.y, 100 - wx, 3);
    if (scratch.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    if (isDestroyed) {
      w->isAlive = false;
      continue;
    }
    if (w->pos.y > 103) {
      w->isAlive = false;
      continue;
    }
  }
}

void addGamePlasmaSplitter() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
