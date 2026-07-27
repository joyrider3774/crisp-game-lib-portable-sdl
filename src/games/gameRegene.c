#include "cglp.h"
#include "string.h"

static char *title = "REGENE";
static char *description = "[Slide]\n Erase wall";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 16, .isDarkColor = false};

typedef struct {
  Vector pos;
  bool isBall;
  bool isAlive;
} Wall;
#define MAX_WALL_COUNT 128
static Wall walls[MAX_WALL_COUNT];
static int wallIndex;

#define MAX_REMOVED_COUNT 32
static Vector removedWalls[MAX_REMOVED_COUNT];
static int removedWallCount;
static int removeWallCount;
static float nextWallsDist;

typedef struct {
  Vector pos;
  Vector vel;
  int hitCount;
  bool isAlive;
} Ball;
#define MAX_BALL_COUNT 16
static Ball balls[MAX_BALL_COUNT];
static int ballIndex;

static float randomBallAngle() {
  float a = rnd(0, M_PI * 0.1) * RNDPM();
  if (rnd(0, 1) < 0.5) {
    a += M_PI * 0.7;
  } else {
    a -= M_PI * 0.7;
  }
  return a;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    removedWallCount = 0;
    removeWallCount = 9;
    nextWallsDist = 0;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, b0);
    vectorSet(&b0->pos, 20, 50);
    vectorSet(&b0->vel, 1, 0);
    rotate(&b0->vel, randomBallAngle());
    b0->hitCount = 0;
    b0->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, MAX_BALL_COUNT);
  }
  color = LIGHT_BLUE;
  rect(0, 0, 100, 10);
  rect(0, 90, 100, 10);
  rect(0, 10, 10, 80);
  COUNT_IS_ALIVE(balls, aliveBallCount);
  float scr = difficulty * 0.05 * sqrt(aliveBallCount);
  float ipx = clamp(input.pos.x, 0, 99);
  float ipy = clamp(input.pos.y, 0, 99);
  if (ipx > 90) {
    scr += (ipx - 90) * 0.05;
  }
  nextWallsDist -= scr;
  if (nextWallsDist < 0) {
    int w = rndi(3, 7);
    int h;
    if (rnd(0, 1) < 0.4) {
      h = 0;
    } else {
      h = rndi(2, 6);
    }
    int hy = rndi(h, 8 - h);
    TIMES(w, x) {
      TIMES(8, y) {
        if (y < hy || y >= hy + h) {
          ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, nw);
          vectorSet(&nw->pos, x * 10 + 105 - nextWallsDist, y * 10 + 15);
          nw->isBall = rnd(0, 1) < 0.1 / sqrt(aliveBallCount);
          nw->isAlive = true;
          wallIndex = wrap(wallIndex + 1, 0, MAX_WALL_COUNT);
        }
      }
    }
    nextWallsDist += (w + rndi(0, 3)) * 10;
  }
  color = LIGHT_BLACK;
  box(ipx, ipy, 5, 5);
  int ri = 0;
  while (ri < removedWallCount) {
    removedWalls[ri].x -= scr;
    if (removedWalls[ri].x < 14) {
      memcpy(&removedWalls[ri], &removedWalls[ri + 1],
             (removedWallCount - 1 - ri) * sizeof(removedWalls[0]));
      removedWallCount--;
    } else {
      ri++;
    }
  }
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.x -= scr;
    if (w->isBall) {
      color = LIGHT_RED;
    } else {
      color = PURPLE;
    }
    float wsize;
    if (w->isBall) {
      wsize = 6;
    } else {
      wsize = 8;
    }
    Collision wc = box(VEC_XY(w->pos), wsize, wsize);
    if (wc.isColliding.rect[LIGHT_BLACK]) {
      if (w->isBall) {
        if (w->pos.x < 97) {
          play(POWER_UP);
          ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, nb);
          nb->pos = w->pos;
          vectorSet(&nb->vel, 1, 0);
          rotate(&nb->vel, randomBallAngle());
          nb->hitCount = 0;
          nb->isAlive = true;
          ballIndex = wrap(ballIndex + 1, 0, MAX_BALL_COUNT);
          w->isAlive = false;
          continue;
        }
      } else {
        play(LASER);
        removedWalls[removedWallCount] = w->pos;
        removedWallCount++;
        w->isAlive = false;
        continue;
      }
    }
    w->isAlive = !wc.isColliding.rect[LIGHT_BLUE];
  }
  while (removedWallCount > removeWallCount) {
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, rw);
    rw->pos = removedWalls[0];
    rw->isBall = false;
    rw->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, MAX_WALL_COUNT);
    memcpy(&removedWalls[0], &removedWalls[1], (removedWallCount - 1) * sizeof(removedWalls[0]));
    removedWallCount--;
  }
  if (removedWallCount >= removeWallCount) {
    color = LIGHT_PURPLE;
    box(VEC_XY(removedWalls[0]), 8, 8);
  }
  color = LIGHT_BLUE;
  rect(100, 10, 10, 80);
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x -= scr;
    color = TRANSPARENT;
    float spd = sqrt(difficulty);
    bool isHitX = false;
    bool isHitY = false;
    Collision cx = box(b->pos.x + b->vel.x * spd, b->pos.y, 6, 6);
    if (cx.isColliding.rect[PURPLE] || cx.isColliding.rect[LIGHT_BLUE]) {
      isHitX = true;
    }
    Collision cy = box(b->pos.x, b->pos.y + b->vel.y * spd, 6, 6);
    if (cy.isColliding.rect[PURPLE] || cy.isColliding.rect[LIGHT_BLUE]) {
      isHitY = true;
    }
    if (isHitX) {
      b->vel.x *= -1;
      b->pos.x += b->vel.x * spd * 2;
    }
    if (isHitY) {
      b->vel.y *= -1;
      b->pos.y += b->vel.y * spd * 2;
    }
    if (b->pos.x < 14) {
      b->pos.x = 14;
      if (b->vel.x < 0) {
        b->vel.x *= -1;
      }
    }
    color = RED;
    if (isHitX || isHitY) {
      if (b->hitCount == 0) {
        COUNT_IS_ALIVE(balls, currentAliveBallCount);
        addScore(currentAliveBallCount, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      }
      b->hitCount++;
      if (b->hitCount > 9) {
        particle(VEC_XY(b->pos), 16, 1, 0, M_PI * 2);
        play(HIT);
        b->isAlive = false;
        continue;
      }
    } else {
      b->hitCount = 0;
    }
    b->pos.x += b->vel.x * spd;
    b->pos.y += b->vel.y * spd;
    box(VEC_XY(b->pos), 6, 6);
  }
  COUNT_IS_ALIVE(balls, finalAliveBallCount);
  if (finalAliveBallCount == 0) {
    play(RANDOM);
    gameOver();
  }
}

void addGameRegene() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
