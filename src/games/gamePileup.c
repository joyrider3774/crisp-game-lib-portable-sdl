#include "cglp.h"

static char *title = "PILEUP";
static char *description = "[Slide] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
} Board;
static Board board;
static bool hasBoard;

typedef struct {
  Vector pos;
  bool isFixed;
  bool isAlive;
} Box;
// Fixed boxes never expire (only removed by the rare falling-board sweep); floor
// is ~14 columns wide (99/7) x ~11 stack rows (80/7) before "by" hits 0 and ends
// the run, so ~150 concurrent fixed boxes is realistic - 32 was far too small.
#define MAX_BOX_COUNT 256
static Box boxes[MAX_BOX_COUNT];
static int boxIndex;
static float boxAddTicks;
static Vector pp;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(boxes);
    boxIndex = 0;
    boxAddTicks = 0;
    vectorSet(&pp, 0, 0);
    hasBoard = false;
  }
  if (hasBoard) {
    board.vy += difficulty * 0.1;
    board.pos.y += board.vy;
    color = RED;
    box(VEC_XY(board.pos), 20, 5);
    if (board.pos.y > 99) {
      hasBoard = false;
      play(SELECT);
      addScore(99 - pp.y, VEC_XY(pp));
    }
  } else {
    vectorSet(&board.pos, rnd(0, 99), 0);
    board.vy = 0;
    hasBoard = true;
  }
  float by = 80;
  color = BLACK;
  rect(0, 95, 99, 5);
  FOR_EACH(boxes, i) {
    ASSIGN_ARRAY_ITEM(boxes, i, Box, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->isFixed) {
      if (b->pos.y - 9 < by) {
        by = b->pos.y - 9;
      }
    } else {
      b->pos.y += difficulty;
    }
    if (b->isFixed) {
      color = BLACK;
    } else {
      color = LIGHT_BLACK;
    }
    Collision scratch = box(VEC_XY(b->pos), 7, 7);
    if (!b->isFixed) {
      if (scratch.isColliding.rect[BLACK]) {
        b->isFixed = true;
      }
    }
    if (scratch.isColliding.rect[RED]) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
  }
  color = GREEN;
  vectorSet(&pp, clamp(input.pos.x, 0, 99), by);
  Collision scratch = box(VEC_XY(pp), 7, 7);
  if (scratch.isColliding.rect[RED] || pp.y < 0) {
    play(RANDOM);
    gameOver();
  }
  boxAddTicks--;
  if (boxAddTicks < 0) {
    ASSIGN_ARRAY_ITEM(boxes, boxIndex, Box, nb);
    nb->pos = pp;
    nb->isFixed = false;
    nb->isAlive = true;
    boxIndex = wrap(boxIndex + 1, 0, MAX_BOX_COUNT);
    boxAddTicks += 10 / difficulty;
  }
}

void addGamePileup() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
