#include "cglp.h"
#include "string.h"

static char *title = "TWO LANE";
static char *description = "[Tap]\n Change Lane\n[Hold] \n Accel";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "ll  ll",
        "llrrll",
        "  rr  ",
        " rppr ",
        "llrrll",
        "ll  ll",
    },
    {
        " yyyy ",
        "yyYYyy",
        "yyYYyy",
        "yyYYyy",
        "yyYYyy",
        " yyyy ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 10, .isDarkColor = false};

#define TWOLANE_TYPE_LEFT 0
#define TWOLANE_TYPE_RIGHT 1
#define TWOLANE_TYPE_BOTH 2
#define TWOLANE_COIN_NONE 0
#define TWOLANE_COIN_LEFT 1
#define TWOLANE_COIN_RIGHT 2

typedef struct {
  float y;
  int type;
  int coin;
  int index;
} Road;
#define TWOLANE_ROAD_COUNT 12
static Road roads[TWOLANE_ROAD_COUNT];

static float speed;
static float x;
static float targetX;
static int currentType;
static float lastScore;
static int multiplier;
static int lastMultiplier;
static bool isOutOfCourse;

static void update() {
  if (!ticks) {
    TIMES(TWOLANE_ROAD_COUNT, i) {
      roads[i].y = i * 10 - 10;
      roads[i].type = TWOLANE_TYPE_BOTH;
      roads[i].coin = TWOLANE_COIN_NONE;
      roads[i].index = i;
    }
    speed = 1;
    x = 30;
    targetX = 30;
    currentType = TWOLANE_TYPE_BOTH;
    lastScore = 0;
    multiplier = 1;
    lastMultiplier = 1;
    isOutOfCourse = false;
  }
  color = LIGHT_GREEN;
  rect(0, 0, 100, 100);
  if (input.isJustPressed) {
    play(LASER);
    if (targetX == 30) {
      targetX = 70;
    } else {
      targetX = 30;
    }
  }
  if (isOutOfCourse) {
    speed *= 1 - 0.1 * sqrt(difficulty);
    if (speed < 0.1) {
      play(RANDOM);
      gameOver();
    }
  } else if (input.isPressed) {
    speed += difficulty * 0.02;
  } else {
    speed += (difficulty - speed) * 0.1;
  }
  float sl = clamp(difficulty * 3, 1, 9);
  if (speed > sl) {
    speed = sl;
  }
  x += (targetX - x) * (0.15 * sqrt(speed));
  color = BLACK;
  character("a", x, 90);
  TIMES(TWOLANE_ROAD_COUNT, i) {
    Road *r = &roads[i];
    float py = r->y;
    r->y = wrap(r->y + speed, -10, 110);
    if (r->y < py) {
      if (rnd(0, 1) < 0.1) {
        if (currentType == TWOLANE_TYPE_BOTH) {
          if (rnd(0, 1) < 0.5) {
            r->type = TWOLANE_TYPE_LEFT;
          } else {
            r->type = TWOLANE_TYPE_RIGHT;
          }
        } else {
          r->type = TWOLANE_TYPE_BOTH;
        }
        currentType = r->type;
      } else {
        r->type = currentType;
      }
      if (r->coin != TWOLANE_COIN_NONE) {
        if (multiplier > 1) {
          multiplier--;
        }
      }
      if (rnd(0, 1) < 0.2) {
        if (r->type == TWOLANE_TYPE_BOTH) {
          if (rnd(0, 1) < 0.5) {
            r->coin = TWOLANE_COIN_LEFT;
          } else {
            r->coin = TWOLANE_COIN_RIGHT;
          }
        } else if (r->type == TWOLANE_TYPE_LEFT) {
          r->coin = TWOLANE_COIN_LEFT;
        } else {
          r->coin = TWOLANE_COIN_RIGHT;
        }
      } else {
        r->coin = TWOLANE_COIN_NONE;
      }
    }
    color = LIGHT_BLACK;
    float rx;
    if (r->type == TWOLANE_TYPE_RIGHT) {
      rx = 50;
    } else {
      rx = 10;
    }
    float rw;
    if (r->type == TWOLANE_TYPE_BOTH) {
      rw = 80;
    } else {
      rw = 40;
    }
    rect(rx, r->y, rw, 10);
    color = WHITE;
    if (r->index % 2 == 0) {
      if (r->type == TWOLANE_TYPE_LEFT || r->type == TWOLANE_TYPE_BOTH) {
        rect(11, r->y, 5, 10);
      }
      if (r->type == TWOLANE_TYPE_RIGHT || r->type == TWOLANE_TYPE_BOTH) {
        rect(84, r->y, 5, 10);
      }
    } else {
      float lx;
      if (r->type == TWOLANE_TYPE_RIGHT) {
        lx = 50;
      } else {
        lx = 47;
      }
      float lw;
      if (r->type == TWOLANE_TYPE_BOTH) {
        lw = 6;
      } else {
        lw = 3;
      }
      rect(lx, r->y, lw, 10);
    }
    if (r->coin != TWOLANE_COIN_NONE) {
      color = BLACK;
      float cx;
      if (r->coin == TWOLANE_COIN_LEFT) {
        cx = 30;
      } else {
        cx = 70;
      }
      Collision cc = character("b", cx, r->y + 5);
      if (cc.isColliding.character['a']) {
        play(COIN);
        lastScore = floor(speed * speed + 1);
        lastMultiplier = multiplier;
        addScore(lastScore * multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        if (multiplier < 32) {
          multiplier++;
        }
        r->coin = TWOLANE_COIN_NONE;
      }
    }
  }
  color = BLACK;
  Collision c = character("a", x, 90);
  if (!c.isColliding.rect[LIGHT_BLACK] && !c.isColliding.rect[WHITE]) {
    play(HIT);
    isOutOfCourse = true;
    multiplier = 1;
  } else {
    isOutOfCourse = false;
  }
  if (lastScore > 0) {
    color = BLACK;
    char scoreText[16];
    strcpy(scoreText, "+");
    strcat(scoreText, intToChar((int)lastScore));
    strcat(scoreText, "x");
    strcat(scoreText, intToChar(lastMultiplier));
    text(scoreText, 3, 9);
  }
}

void addGameTwoLane() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
