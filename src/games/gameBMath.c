#include "cglp.h"
#include "string.h"

static char *title = "B MATH";
static char *description = "[Tap] Answer";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define BMATH_OP_ADD 0
#define BMATH_OP_SUB 1
#define BMATH_OP_MUL 2
#define BMATH_OP_DIV 3
static char *opChars[4] = {"+", "-", "X", "/"};

static int values[2];
static int opType;
static int anss[5];
static int ans;
static float leftTime;
static float targetTime;
static float ansTicks;
static int ansIndex;
static int multiplier;

static void nextQuestion() {
  float cd = clamp((difficulty - 1) * 2, 0, 1);
  if (rnd(0, cd) > 0.9) {
    opType = BMATH_OP_DIV;
  } else if (rnd(0, cd) > 0.7) {
    opType = BMATH_OP_MUL;
  } else if (rnd(0, cd) > 0.5) {
    opType = BMATH_OP_SUB;
  } else {
    opType = BMATH_OP_ADD;
  }
  if (opType == BMATH_OP_DIV || opType == BMATH_OP_MUL) {
    values[1] = rndi(2, 10);
    ans = floor(rnd(10, 100) / values[1]);
    values[0] = values[1] * ans;
    if (opType == BMATH_OP_MUL) {
      int a = ans;
      ans = values[0];
      values[0] = a;
      if (rnd(0, 1) < 0.5) {
        int v0 = values[0];
        values[0] = values[1];
        values[1] = v0;
      }
    }
  } else {
    ans = rndi(10, 100);
    values[0] = rndi(1, ans);
    values[1] = ans - values[0];
    if (opType == BMATH_OP_SUB) {
      int v = values[0];
      values[0] = ans;
      ans = values[1];
      values[1] = v;
    }
  }
  int ci = rndi(0, 5);
  TIMES(5, i) {
    if (i == ci) {
      anss[i] = ans;
      continue;
    }
    TIMES(9, j) {
      float a = ans + rnd(0, 5) * RNDPM();
      if (rnd(0, 1) < 0.3) {
        a += a * (rnd(0.1, 0.2) * RNDPM());
      }
      a = clamp(round(a), 1, 99);
      anss[i] = (int)a;
      bool iv = (int)a != ans;
      TIMES(i, k) {
        if (anss[k] == (int)a) {
          iv = false;
        }
      }
      if (iv) {
        break;
      }
    }
  }
}

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped
  // answer index is computed directly from input.pos via grid math (see
  // "floor(input.pos.x / 20)" below), so the engine's own O(n^2) hitbox
  // scan is pure waste here.
  hasCollision = false;
  float sd = sqrt(difficulty);
  if (!ticks) {
    values[0] = 0;
    values[1] = 0;
    TIMES(5, i) { anss[i] = 0; }
    nextQuestion();
    leftTime = M_PI * 2;
    ansTicks = 0;
    ansIndex = 0;
    multiplier = 1;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
  float v0Ofs;
  if (values[0] > 9) {
    v0Ofs = 6;
  } else {
    v0Ofs = 0;
  }
  text(intToChar(values[0]), 50 - v0Ofs, 36);
  float v1Ofs;
  if (values[1] > 9) {
    v1Ofs = 6;
  } else {
    v1Ofs = 0;
  }
  text(intToChar(values[1]), 50 - v1Ofs, 45);
  text(opChars[opType], 35, 41);
  rect(25, 50, 40, 1);
  if (ansTicks > 0) {
    float ansOfs;
    if (ans > 9) {
      ansOfs = 6;
    } else {
      ansOfs = 0;
    }
    text(intToChar(ans), 50 - ansOfs, 55);
    leftTime += (targetTime - leftTime) * 0.1;
    ansTicks--;
    if (ansTicks <= 0) {
      nextQuestion();
    }
  } else {
    ansIndex = (int)floor(input.pos.x / 20);
    if (input.isJustPressed) {
      if (ansIndex >= 0 && ansIndex < 5) {
        ansTicks = 30 / sd;
        addScore(ceil(leftTime * 9 * multiplier), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        float dir;
        if (anss[ansIndex] == ans) {
          dir = 2;
        } else {
          dir = -1;
        }
        float tt = leftTime + M_PI_2 * dir;
        if (anss[ansIndex] == ans) {
          if (tt >= M_PI * 2) {
            multiplier++;
            play(POWER_UP);
          } else {
            play(COIN);
          }
        } else {
          if (multiplier > 1) {
            multiplier--;
            play(EXPLOSION);
          }
        }
        targetTime = clamp(tt, 0, M_PI * 2);
      }
    }
  }
  leftTime -= sd * 0.01;
  if (leftTime < 0) {
    leftTime = 0;
    float ansOfs2;
    if (ans > 9) {
      ansOfs2 = 6;
    } else {
      ansOfs2 = 0;
    }
    text(intToChar(ans), 50 - ansOfs2, 55);
    play(EXPLOSION);
    gameOver();
  }
  thickness = 3;
  arc(45, 45, 33, M_PI_2 + leftTime, M_PI_2);
  TIMES(5, i) {
    color = BLACK;
    if (ansIndex == i) {
      box(i * 20 + 10, 85, 18, 8);
      if (ansTicks > 0) {
        if (anss[i] == ans) {
          text("OK", i * 20 + 7, 92);
        } else {
          text("NG", i * 20 + 7, 92);
        }
      }
      color = WHITE;
    }
    float aOfs;
    if (anss[i] > 9) {
      aOfs = 6;
    } else {
      aOfs = 0;
    }
    text(intToChar(anss[i]), i * 20 + 13 - aOfs, 85);
  }
}

void addGameBMath() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
