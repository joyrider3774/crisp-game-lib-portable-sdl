#include "cglp.h"

static char *title = "FROM TWO SIDES";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "rrrrrr",
        "rRRRRr",
        " rRRr ",
            "      ",
        "      ",
        "      ",
},
    {
        "rRRr  ",
        " rr   ",
        " rr   ",
            "      ",
        "      ",
        "      ",
},
    {
        " G    ",
        "GgG   ",
        " G    ",
            "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
  int wy;
  bool isAlive;
} Arrow;
#define FROMTWOSIDES_MAX_ARROW_COUNT 64
static Arrow arrows[FROMTWOSIDES_MAX_ARROW_COUNT];
static int arrowIndex;
static int nextArrowTicks[2];

typedef struct {
  float x;
  float vx;
} Safe;
#define FROMTWOSIDES_SAFE_COUNT 2
static Safe safes[FROMTWOSIDES_SAFE_COUNT];

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(arrows);
    arrowIndex = 0;
    nextArrowTicks[0] = 0;
    nextArrowTicks[1] = 60;
    safes[0].x = 50;
    safes[0].vx = -1;
    safes[1].x = 50;
    safes[1].vx = 1;
  }
  TIMES(FROMTWOSIDES_SAFE_COUNT, i) {
    ASSIGN_ARRAY_ITEM(safes, i, Safe, s);
    s->x += s->vx;
    if ((s->x < 9 && s->vx < 0) || (s->x > 90 && s->vx > 0)) {
      s->vx *= -1;
    }
    s->vx += rnd(0, 0.5) * RNDPM();
    s->vx *= 0.98;
  }
  TIMES(2, i) {
    nextArrowTicks[i]--;
    if (nextArrowTicks[i] < 0) {
      play(EXPLOSION);
      float w = rnd(10, 40) / sqrt(difficulty) + 10;
      TIMES(17, xi) {
        float x = xi * 6 + 2;
        bool isSafe = false;
        TIMES(FROMTWOSIDES_SAFE_COUNT, si) {
          ASSIGN_ARRAY_ITEM(safes, si, Safe, s);
          if (fabs(x - s->x) < w / 2) {
            isSafe = true;
          }
        }
        if (!isSafe) {
          float ay;
          int wy;
          if (i == 0) {
            ay = -3;
            wy = 1;
          } else {
            ay = 103;
            wy = -1;
          }
          ASSIGN_ARRAY_ITEM(arrows, arrowIndex, Arrow, a);
          vectorSet(&a->pos, x, ay);
          a->vy = 0;
          a->wy = wy;
          a->isAlive = true;
          arrowIndex = wrap(arrowIndex + 1, 0, FROMTWOSIDES_MAX_ARROW_COUNT);
          addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        }
      }
      nextArrowTicks[i] = rnd(60, 90) / sqrt(difficulty);
    }
  }
  float s = difficulty * 2;
  FOR_EACH(arrows, i) {
    ASSIGN_ARRAY_ITEM(arrows, i, Arrow, a);
    SKIP_IS_NOT_ALIVE(a);
    a->vy += (a->wy * s - a->vy) * 0.05;
    a->pos.y += a->vy;
    if (a->wy > 0) {
      character("a", a->pos.x, a->pos.y - 1);
      character("b", a->pos.x, a->pos.y + 2);
    } else {
      characterOptions.isMirrorY = true;
      character("a", a->pos.x, a->pos.y + 1);
      character("b", a->pos.x, a->pos.y - 2);
      characterOptions.isMirrorY = false;
    }
    color = RED;
    particle(a->pos.x, a->pos.y, 0.4, -fabs(a->vy), (M_PI / 2) * a->wy, 0.3);
    color = BLACK;
    a->isAlive = !(a->pos.y < -3 || a->pos.y > 103);
  }
  float x = clamp(input.pos.x, 1, 98);
  Collision cc = character("c", x, 50);
  if (cc.isColliding.character['a'] || cc.isColliding.character['b']) {
    play(POWER_UP);
    gameOver();
  }
}

void addGameFromTwoSides() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
