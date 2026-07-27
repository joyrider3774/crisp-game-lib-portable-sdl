#include "cglp.h"
#include <string.h>

static char *title = "SNAKE 1";
static char *description = "[Tap] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = true};

typedef struct {
  Vector pos;
  float angle;
  float rotation;
} Head;
static Head head;
static int headMoveTicks;
static bool isHeadGettingDollar;
static bool isHeadTurning;
#define MAX_BODY_COUNT 200
static Vector bodies[MAX_BODY_COUNT];
static int bodyCount;
// Each dollar eaten removes 1 but spawns up to 2 replacements (net +1/eat) and dollars never expire
// on their own - a direct (non-wrapping) array write, so this must outsize any realistic eat count.
#define MAX_DOLLAR_COUNT 256
static Vector dollars[MAX_DOLLAR_COUNT];
static int dollarCount;
static int angleOfs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
static char *headChars = ">v<^";
static char *wallChars = "################";
static char *edgeWallChars = "#              #";

static void shiftBodiesDown() {
  memcpy(&bodies[0], &bodies[1], (bodyCount - 1) * sizeof(bodies[0]));
  bodyCount--;
}

static void shiftDollarsDown(int removedIndex) {
  memcpy(&dollars[removedIndex], &dollars[removedIndex + 1],
         (dollarCount - 1 - removedIndex) * sizeof(dollars[0]));
  dollarCount--;
}

static void update() {
  if (!ticks) {
    color = GREEN;
    vectorSet(&head.pos, 8, 8);
    head.angle = 0;
    head.rotation = 1;
    headMoveTicks = 0;
    isHeadGettingDollar = false;
    isHeadTurning = false;
    bodyCount = 0;
    TIMES(4, i) {
      vectorSet(&bodies[bodyCount], 4 + i, 8);
      bodyCount++;
    }
    dollarCount = 0;
    vectorSet(&dollars[dollarCount], 12, 8);
    dollarCount++;
  }
  text(wallChars, 3, 9);
  for (int y = 1; y <= 13; y++) {
    text(edgeWallChars, 3, 9 + y * 6);
  }
  text(wallChars, 3, 9 + 14 * 6);
  if (!isHeadTurning && input.isJustPressed) {
    play(SELECT);
    isHeadTurning = true;
  }
  headMoveTicks--;
  if (headMoveTicks < 0) {
    play(LASER);
    if (!isHeadGettingDollar) {
      shiftBodiesDown();
    } else {
      isHeadGettingDollar = false;
    }
    bodies[bodyCount] = head.pos;
    bodyCount++;
    if (isHeadTurning) {
      head.angle = wrap(head.angle + head.rotation, 0, 4);
      isHeadTurning = false;
    }
    int angleIndex = (int)head.angle;
    vectorAdd(&head.pos, angleOfs[angleIndex][0], angleOfs[angleIndex][1]);
    headMoveTicks = 20 / difficulty;
  }
  for (int i = 0; i < bodyCount; i++) {
    text("o", bodies[i].x * 6 + 3, bodies[i].y * 6 + 3);
  }
  char hc[2];
  hc[0] = headChars[(int)wrap(head.angle + head.rotation, 0, 4)];
  hc[1] = 0;
  Collision scratch = text(hc, head.pos.x * 6 + 3, head.pos.y * 6 + 3);
  if (scratch.isColliding.text['o'] || scratch.isColliding.text['#']) {
    play(EXPLOSION);
    color = WHITE;
    rect(head.pos.x * 6, head.pos.y * 6, 6, 6);
    color = GREEN;
    text("X", head.pos.x * 6 + 3, head.pos.y * 6 + 3);
    gameOver();
  }
  bool ig = false;
  int di = 0;
  while (di < dollarCount) {
    Collision dc = text("$", dollars[di].x * 6 + 3, dollars[di].y * 6 + 3);
    if (dc.isColliding.text['v'] || dc.isColliding.text['>'] || dc.isColliding.text['<'] ||
        dc.isColliding.text['^']) {
      ig = true;
      shiftDollarsDown(di);
    } else {
      di++;
    }
  }
  if (ig) {
    play(COIN);
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    isHeadGettingDollar = true;
    head.rotation *= -1;
    color = TRANSPARENT;
    TIMES(2, i) {
      TIMES(99, j) {
        int x = rndi(2, 14);
        int y = rndi(3, 14);
        Collision pc = text("$", x * 6 + 3, y * 6 + 3);
        if (pc.isColliding.text['v'] || pc.isColliding.text['>'] || pc.isColliding.text['<'] ||
            pc.isColliding.text['^'] || pc.isColliding.text['o']) {
        } else {
          vectorSet(&dollars[dollarCount], x, y);
          dollarCount++;
          break;
        }
      }
    }
    color = GREEN;
  }
}

void addGameSnake1() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
