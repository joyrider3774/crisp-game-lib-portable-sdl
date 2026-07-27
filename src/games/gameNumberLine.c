#include "cglp.h"

static char *title = "NUMBER LINE";
static char *description = "[Tap] Sum";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 60, .isDarkColor = false};

static int numberColors[5] = {BLUE, CYAN, GREEN, PURPLE, RED};

typedef struct {
  float y;
  float ty;
  int index;
} NumberLineLine;
#define NUMBERLINE_LINE_COUNT 2
static NumberLineLine lines[NUMBERLINE_LINE_COUNT];

typedef struct {
  float nextTicks;
  int lineIndex;
} NumberLineNextNumber;
static NumberLineNextNumber nextNumbers[NUMBERLINE_LINE_COUNT];

typedef struct {
  float value;
  float x;
  float vx;
  int lineIndex;
  bool isSummed;
  bool isAlive;
} NumberLineNumber;
#define NUMBERLINE_MAX_NUMBER_COUNT 64
static NumberLineNumber numbers[NUMBERLINE_MAX_NUMBER_COUNT];
static int numberIndex;

static float summedTicks;

static void update() {
  // Never reads a Collision result anywhere in this file - the tappable
  // range and number-merging are direct position/fabs() distance checks
  // (see "n->x > 3 && n->x < 97" and "fabs(an->x - n->x) < 6" below), so
  // the engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is
  // pure waste here. Restored automatically when the next real game
  // starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    lines[0].y = 12;
    lines[0].ty = 12;
    lines[0].index = 0;
    lines[1].y = 94;
    lines[1].ty = 94;
    lines[1].index = 1;
    INIT_UNALIVED_ARRAY(numbers);
    numberIndex = 0;
    nextNumbers[0].nextTicks = 0;
    nextNumbers[0].lineIndex = 0;
    nextNumbers[1].nextTicks = 0;
    nextNumbers[1].lineIndex = 1;
    summedTicks = -1;
  }
  TIMES(NUMBERLINE_LINE_COUNT, i) {
    NumberLineLine *l = &lines[i];
    l->ty += (1 - l->index * 2) * difficulty * 0.01;
    l->y += (l->ty - l->y) * 0.2;
    if (l->index == 0 && l->y < 12) {
      play(COIN);
      l->ty++;
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
    if (l->index == 1 && l->y > 94) {
      l->ty--;
    }
    if (l->index == 0) {
      color = BLUE;
    } else {
      color = RED;
    }
    rect(0, l->y, 100, 1);
  }
  if (lines[0].y >= lines[1].y) {
    play(RANDOM);
    gameOver();
  }
  if (summedTicks < 0) {
    if (input.isJustPressed) {
      play(SELECT);
      FOR_EACH(numbers, i) {
        ASSIGN_ARRAY_ITEM(numbers, i, NumberLineNumber, n);
        SKIP_IS_NOT_ALIVE(n);
        if (n->x > 3 && n->x < 97) {
          n->isSummed = true;
        }
      }
      summedTicks = 0;
    }
  } else {
    summedTicks++;
  }
  TIMES(NUMBERLINE_LINE_COUNT, i) {
    nextNumbers[i].nextTicks--;
    if (nextNumbers[i].nextTicks < 0) {
      ASSIGN_ARRAY_ITEM(numbers, numberIndex, NumberLineNumber, nn);
      nn->value = rndi(1, 10);
      nn->x = -3;
      nn->vx = rnd(1, difficulty) * 0.7;
      nn->lineIndex = nextNumbers[i].lineIndex;
      nn->isSummed = false;
      nn->isAlive = true;
      numberIndex = wrap(numberIndex + 1, 0, NUMBERLINE_MAX_NUMBER_COUNT);
      nextNumbers[i].nextTicks = rnd(10, 30) / difficulty;
    }
  }
  float sums[2] = {0, 0};
  FOR_EACH(numbers, i) {
    ASSIGN_ARRAY_ITEM(numbers, i, NumberLineNumber, n);
    SKIP_IS_NOT_ALIVE(n);
    NumberLineLine *l = &lines[n->lineIndex];
    if (n->isSummed) {
      if (summedTicks < 0) {
        n->isAlive = false;
        continue;
      }
      bool merged = false;
      for (int j = 0; j < i; j++) {
        NumberLineNumber *an = &numbers[j];
        if (!an->isAlive) {
          continue;
        }
        if (an->isSummed && an->lineIndex == n->lineIndex && fabs(an->x - n->x) < 6) {
          an->value += n->value;
          merged = true;
          break;
        }
      }
      if (merged) {
        n->isAlive = false;
        continue;
      }
      n->x += (50 - n->x) * 0.1;
      float x;
      if (l->index == 0) {
        x = n->x;
      } else {
        x = 100 - n->x;
      }
      float summedTicksOfs = 0;
      if (summedTicks > 0) {
        summedTicksOfs = 10;
      }
      float y = l->y + l->index * 8 - 4 +
                (summedTicks + summedTicksOfs) * (1 - l->index * 2) * 0.2 * sqrt(difficulty);
      color = BLACK;
      float textX = x;
      if (n->value > 9) {
        textX -= 3;
      }
      text(intToChar((int)n->value), textX, y);
      sums[l->index] = n->value;
      continue;
    }
    n->x += n->vx;
    float x2;
    if (l->index == 0) {
      x2 = n->x;
    } else {
      x2 = 100 - n->x;
    }
    float y2 = l->y + l->index * 6 - 3;
    color = numberColors[(int)((n->value - 1) / 2)];
    text(intToChar((int)n->value), x2, y2);
    n->isAlive = n->x <= 103;
  }
  if (summedTicks > 60 / sqrt(difficulty)) {
    summedTicks = -1;
    float s = sums[0] - sums[1];
    if (s > 0) {
      play(POWER_UP);
    } else if (s < 0) {
      play(EXPLOSION);
    }
    addScore(s, 50, 50);
    lines[0].ty -= s * sqrt(difficulty);
    lines[1].ty += s * sqrt(difficulty);
  }
}

void addGameNumberLine() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
