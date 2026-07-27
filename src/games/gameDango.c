#include "cglp.h"
#include "string.h"

static char *title = "DANGO";
static char *description = "[Tap] Stretch";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = false};

#define DANGO_WIDTH 12

typedef struct {
  float x;
  float angle;
  float angleVel;
  float radius;
  float height;
  char exp[8];
  bool isSticked;
  bool isAlive;
} Dango;
#define DANGO_MAX_DANGO_COUNT 16
static Dango dangos[DANGO_MAX_DANGO_COUNT];
static int dangoIndex;

static float nextDangoX;
static float stickTicks;
static int stickLeft;
static int stickAdd;
static char expStr[64];
static float expAdd;
static float offsetX;

// Vircon32 port note: replaces the JS version's
// `Function("return " + expStr + ";")()` dynamic eval - not possible in
// static C. expStr is always a flat chain of single digits separated by
// '+'/'*' (built one 2-char token at a time by this same file, see the
// dango-creation code below), so a plain two-pass evaluate (collapse '*'
// pairs left-to-right, then sum the remaining '+'-separated terms)
// reproduces the exact same operator precedence JS would have applied.
static float evalExpr(char *expr) {
  int len = strlen(expr);
  if (len == 0) {
    return 0;
  }
  float terms[32];
  int termCount = 0;
  float current = expr[0] - '0';
  int i = 1;
  while (i < len) {
    int op = expr[i];
    float digit = expr[i + 1] - '0';
    if (op == '*') {
      current *= digit;
    } else {
      terms[termCount] = current;
      termCount++;
      current = digit;
    }
    i += 2;
  }
  terms[termCount] = current;
  termCount++;
  float sum = 0;
  TIMES(termCount, k) { sum += terms[k]; }
  return sum;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(dangos);
    dangoIndex = 0;
    nextDangoX = 0;
    stickTicks = 0;
    stickLeft = 9;
    expStr[0] = 0;
    expAdd = 0;
    offsetX = 0;
  }
  float sd = sqrt(difficulty);
  if (stickTicks == 0 && input.isJustPressed) {
    play(LASER);
    stickTicks = 1;
    expStr[0] = 0;
    stickAdd = 0;
    stickLeft--;
  }
  if (stickTicks > 0) {
    stickTicks += sd;
    if (stickTicks < 10) {
      color = BLUE;
    } else {
      color = LIGHT_BLUE;
    }
    float x;
    if (stickTicks < 10) {
      x = 80 - stickTicks * 10;
    } else {
      x = 80 - (10 - stickTicks) * 10;
    }
    rect(x, 75, 120, 2);
    if (stickTicks > 19) {
      stickTicks = 0;
      if (expStr[0] == '+') {
        int len = strlen(expStr);
        TIMES(len, k) { expStr[k] = expStr[k + 1]; }
      } else if (expStr[0] == '*') {
        int len = strlen(expStr);
        for (int k = len; k >= 0; k--) {
          expStr[k + 1] = expStr[k];
        }
        expStr[0] = '0';
      }
      if (strlen(expStr) == 0) {
        expAdd = 0;
      } else {
        expAdd = evalExpr(expStr);
      }
      addScore(expAdd, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      int sl = stickLeft;
      stickLeft = (int)clamp(stickLeft + floor(expAdd / 100), -1, 9);
      stickAdd = stickLeft - sl;
      if (stickAdd > 0) {
        play(POWER_UP);
      } else if (expAdd > 0) {
        play(COIN);
      }
    }
    offsetX *= 0.1;
  } else if (stickLeft >= 0) {
    color = CYAN;
    rect(90, 75, 120, 2);
  } else {
    gameOver();
    return;
  }
  if (stickLeft > 0) {
    color = BLUE;
    TIMES(stickLeft, i) { rect(92, 70 - i * 3, 8, 1); }
  }
  if (stickAdd > 0) {
    char sa[16];
    strcpy(sa, "+");
    strcat(sa, intToChar(stickAdd));
    text(sa, 90, 30);
  }
  offsetX += 0.002 * sqrt(difficulty);
  float targetX = 80;
  color = BLACK;
  FOR_EACH(dangos, i) {
    ASSIGN_ARRAY_ITEM(dangos, i, Dango, d);
    SKIP_IS_NOT_ALIVE(d);
    if (d->isSticked) {
      if (stickTicks > 10) {
        d->x += 20 * sqrt(difficulty);
      }
      if (d->x > 99) {
        nextDangoX -= DANGO_WIDTH;
        char tmpExp[64];
        strcpy(tmpExp, d->exp);
        strcat(tmpExp, expStr);
        strcpy(expStr, tmpExp);
        d->isAlive = false;
        targetX -= DANGO_WIDTH;
        continue;
      }
    } else {
      d->x += (targetX - d->x) * 0.1;
      d->angle += d->angleVel;
    }
    float x = d->x + offsetX;
    float y = 50 + sin(d->angle) * d->radius;
    color = BLACK;
    Collision c = box(x, y, 13, d->height);
    if (c.isColliding.rect[BLUE]) {
      play(HIT);
      d->isSticked = true;
    } else if (c.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    color = WHITE;
    box(x, y, 11, d->height - 2);
    color = BLACK;
    text(d->exp, x - 3.5, y);
    targetX -= DANGO_WIDTH;
  }
  while (targetX > DANGO_WIDTH / 2.0) {
    ASSIGN_ARRAY_ITEM(dangos, dangoIndex, Dango, nd);
    nd->x = -DANGO_WIDTH / 2.0;
    nd->angle = rnd(0, M_PI * 2);
    nd->angleVel = rnd(0.02, 0.05) * RNDPM() * sd;
    nd->radius = 30;
    nd->height = rnd(15, 30);
    int opChar;
    if (rnd(0, 1) < 0.5) {
      opChar = '+';
    } else {
      opChar = '*';
    }
    int digit = rndi(2, 10);
    nd->exp[0] = opChar;
    nd->exp[1] = '0' + digit;
    nd->exp[2] = 0;
    nd->isSticked = false;
    nd->isAlive = true;
    dangoIndex = wrap(dangoIndex + 1, 0, DANGO_MAX_DANGO_COUNT);
    targetX -= DANGO_WIDTH;
  }
  color = BLACK;
  text(expStr, 3, 96);
  if (expAdd > 0) {
    char ea[16];
    strcpy(ea, "+");
    strcat(ea, intToChar((int)expAdd));
    text(ea, 102 - strlen(ea) * 6, 90);
  }
}

void addGameDango() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
