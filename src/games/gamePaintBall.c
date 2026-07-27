#include "cglp.h"

static char *title = "PAINT BALL";
static char *description = "[Tap] Throw";

// Vircon32 port note: drawn with color=BLACK, activating this engine's
// per-pixel multi-color glyph rendering (see gameCount.c's port note and
// colorGridChars/setColorGrid() in cglp.c) - 'y' renders yellow, 'c'/'b'
// render cyan/blue directly from the template letters.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " llll ",
        "lyyyyl",
        "lyyyyl",
        "lyyyyl",
        "lyyyyl",
        " llll ",
    },
    {
        " cccc ",
        "cbbbbc",
        "cbbbbc",
        "cbbbbc",
        "cbbbbc",
        " cccc ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define PAINTBALL_GRID_COUNT 12
#define PAINTBALL_GRID_ROWS 15

static int grid[PAINTBALL_GRID_COUNT][PAINTBALL_GRID_ROWS];
static float gridY;

typedef struct {
  Vector pos;
  Vector vel;
  int color;
  float paintingCount;
  bool isAlive;
} PaintBallBall;
#define PAINTBALL_MAX_BALL_COUNT 32
static PaintBallBall balls[PAINTBALL_MAX_BALL_COUNT];
static int ballIndex;
static float nextBallTicks;

typedef struct {
  Vector pos;
  float angle;
  float va;
} PaintBallWaitingBall;
static PaintBallWaitingBall waitingBall;
static bool hasWaitingBall;

static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(PAINTBALL_GRID_COUNT, x) {
      TIMES(PAINTBALL_GRID_ROWS, y) { grid[x][y] = 0; }
    }
    gridY = 0;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    nextBallTicks = 0;
    vectorSet(&waitingBall.pos, 9, 70);
    waitingBall.angle = 0;
    waitingBall.va = -1;
    hasWaitingBall = true;
    multiplier = 1;
  }
  float scr = sqrt(difficulty) * 0.03;
  color = LIGHT_BLACK;
  box(4, 50, 4, 100);
  box(95, 50, 4, 100);
  gridY += scr;
  if (gridY > 4) {
    gridY -= 7;
    TIMES(PAINTBALL_GRID_COUNT, x) {
      for (int y = 0; y < PAINTBALL_GRID_COUNT + 2; y++) {
        grid[x][PAINTBALL_GRID_COUNT + 2 - y] = grid[x][PAINTBALL_GRID_COUNT + 1 - y];
      }
      grid[x][0] = 0;
    }
  }
  TIMES(PAINTBALL_GRID_COUNT, x) {
    TIMES(PAINTBALL_GRID_ROWS, y) {
      int g = grid[x][y];
      if (g == 1) {
        color = YELLOW;
      } else if (g == 2) {
        color = BLUE;
      } else {
        color = LIGHT_BLACK;
      }
      box(50 + (x - (PAINTBALL_GRID_COUNT - 1) / 2.0) * 7,
          50 + (y - (PAINTBALL_GRID_COUNT + 2) / 2.0) * 7 + gridY, 6, 6);
    }
  }
  color = BLACK;
  if (hasWaitingBall) {
    waitingBall.pos.y += scr;
    if (waitingBall.pos.y > 95) {
      waitingBall.pos.y = 95;
    }
    waitingBall.angle += waitingBall.va * sqrt(difficulty) * 0.02;
    if ((waitingBall.va < 0 && waitingBall.angle < -M_PI / 4) ||
        (waitingBall.va > 0 && waitingBall.angle > M_PI / 4)) {
      waitingBall.va *= -1;
    }
    float a;
    if (waitingBall.pos.x < 50) {
      a = waitingBall.angle;
    } else {
      a = M_PI - waitingBall.angle;
    }
    thickness = 2;
    barCenterPosRatio = 0;
    bar(waitingBall.pos.x, waitingBall.pos.y, 20, a);
    character("a", waitingBall.pos.x, waitingBall.pos.y);
    if (input.isJustPressed) {
      play(SELECT);
      ASSIGN_ARRAY_ITEM(balls, ballIndex, PaintBallBall, nb);
      nb->pos = waitingBall.pos;
      Vector v;
      vectorSet(&v, sqrt(difficulty) * 2, 0);
      rotate(&v, a);
      nb->vel = v;
      nb->color = 1;
      nb->paintingCount = 0;
      nb->isAlive = true;
      ballIndex = wrap(ballIndex + 1, 0, PAINTBALL_MAX_BALL_COUNT);
      hasWaitingBall = false;
      multiplier = 1;
    }
  }
  nextBallTicks--;
  if (nextBallTicks < 0) {
    Vector vel;
    vectorSet(&vel, sqrt(difficulty) * 0.1, 0);
    rotate(&vel, rnd(M_PI / 8, M_PI / 8 * 7));
    ASSIGN_ARRAY_ITEM(balls, ballIndex, PaintBallBall, nb2);
    vectorSet(&nb2->pos, rnd(20, 80), -3);
    nb2->vel = vel;
    nb2->color = 2;
    nb2->paintingCount = 0;
    nb2->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, PAINTBALL_MAX_BALL_COUNT);
    nextBallTicks = 150 / difficulty;
  }
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, PaintBallBall, b);
    SKIP_IS_NOT_ALIVE(b);
    int gx = (int)floor((b->pos.x + 3 - (50 - (PAINTBALL_GRID_COUNT - 1) / 2.0 * 7)) / 7);
    int gy = (int)floor(
        (b->pos.y + 3 - (50 - (PAINTBALL_GRID_COUNT + 2) / 2.0 * 7) - gridY) / 7);
    float sp = 1;
    if (b->color == 2 && gx >= 0 && gx < PAINTBALL_GRID_COUNT && gy >= 0 &&
        gy < PAINTBALL_GRID_COUNT + 3) {
      if (grid[gx][gy] == 1) {
        b->paintingCount++;
        sp = 0.1;
      } else {
        b->paintingCount = 999;
      }
    }
    vectorAdd(&b->pos, b->vel.x * sp, b->vel.y * sp);
    b->pos.y += scr;
    bool removed = false;
    if ((b->pos.x <= 9 && b->vel.x < 0) || (b->pos.x >= 90 && b->vel.x > 0)) {
      if (b->color == 1) {
        waitingBall.pos = b->pos;
        waitingBall.angle = 0;
        waitingBall.va = -1;
        hasWaitingBall = true;
        multiplier = 1;
        b->isAlive = false;
        removed = true;
      } else {
        b->vel.x *= -1;
      }
    }
    if (removed) {
      continue;
    }
    char bc[2];
    if (b->color == 1) {
      bc[0] = 'a';
    } else {
      bc[0] = 'b';
    }
    bc[1] = 0;
    character(bc, b->pos.x, b->pos.y);
    if (b->color == 2 && b->pos.y > 99) {
      play(EXPLOSION);
      color = RED;
      text("X", b->pos.x, 97);
      color = BLACK;
      gameOver();
    }
    if ((b->pos.y < 3 && b->vel.y < 0) || (b->pos.y > 99 && b->vel.y > 0)) {
      b->vel.y *= -1;
    }
    if (gx >= 0 && gx < PAINTBALL_GRID_COUNT && gy >= 0 && gy < PAINTBALL_GRID_COUNT + 3) {
      if (b->color == 1 || b->paintingCount > 99 / sqrt(difficulty)) {
        if (b->color == 1 && grid[gx][gy] != 1) {
          if (grid[gx][gy] == 2) {
            play(LASER);
            multiplier++;
          } else {
            play(HIT);
          }
          addScore(multiplier, b->pos.x, b->pos.y);
        }
        grid[gx][gy] = b->color;
        b->paintingCount = 0;
      }
    }
  }
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, PaintBallBall, b);
    SKIP_IS_NOT_ALIVE(b);
    color = TRANSPARENT;
    Collision bCol = character("b", b->pos.x, b->pos.y);
    if (b->color == 2 && bCol.isColliding.character['a']) {
      play(POWER_UP);
      multiplier++;
      color = CYAN;
      particle(b->pos.x, b->pos.y, 9, 1, 0, M_PI * 2);
      addScore(multiplier, b->pos.x, b->pos.y);
      b->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(balls, aliveBallCount);
  if (aliveBallCount == 0) {
    nextBallTicks = 0;
  }
}

void addGamePaintBall() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
