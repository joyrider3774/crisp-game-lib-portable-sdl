#include "cglp.h"
#include "string.h"

static char *title = "P FIT";
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
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 22, .isDarkColor = false};

#define PFIT_TYPE_FIX 0
#define PFIT_TYPE_MOVE 1
#define PFIT_TYPE_END 2

typedef struct {
  Vector pos;
  float width;
  float height;
  float targetY;
  int type;
} PFitWall;
#define PFIT_MAX_WALL_COUNT 40
static PFitWall walls[PFIT_MAX_WALL_COUNT];
static int wallCount;

static bool isAddingWall;
static float wallX;
static float matchTicks;
static float nextScore;
static int pairCount;
static float wallWidth;

static void update() {
  if (!ticks) {
    wallCount = 0;
    isAddingWall = true;
    wallX = 0;
    matchTicks = 0;
    nextScore = 0;
  }
  if (isAddingWall) {
    pairCount = (int)clamp(floor(6 * difficulty * rnd(0.5, 1)), 6, 20);
    wallWidth = 100.0 / (pairCount - 1);
    float h = rnd(22, 32);
    float hs[20];
    TIMES(pairCount, i) {
      h += rnd(10, 50) / pairCount;
      hs[i] = h;
    }
    TIMES(99, k) {
      int i1 = rndi(0, pairCount);
      int i2 = rndi(0, pairCount);
      float t = hs[i1];
      hs[i1] = hs[i2];
      hs[i2] = t;
    }
    wallCount = pairCount * 2;
    TIMES(wallCount, i) {
      PFitWall *w = &walls[i];
      float px = (i % pairCount) * wallWidth - wallWidth / 2;
      float py;
      float ty;
      float wh;
      int wtype;
      if (i < pairCount) {
        py = 75;
        ty = -25;
        wh = hs[i];
        wtype = PFIT_TYPE_FIX;
      } else {
        py = 225;
        ty = 125;
        wh = hs[i - pairCount] - 99;
        wtype = PFIT_TYPE_MOVE;
      }
      vectorSet(&w->pos, px, py);
      w->targetY = ty;
      w->width = wallWidth;
      w->height = wh;
      w->type = wtype;
    }
    wallX = -rnd(10, 90);
    TIMES(9, k) {
      float wrapped = fabs(wrap(wallX + input.pos.x, (-wallWidth * pairCount) / 2,
                                 (wallWidth * pairCount) / 2));
      if (wrapped > 20) {
        break;
      }
      wallX = -rnd(10, 90);
    }
    nextScore = 300;
    isAddingWall = false;
  }
  int wi2 = 0;
  while (wi2 < wallCount) {
    PFitWall *w = &walls[wi2];
    w->pos.y += (w->targetY - w->pos.y) * 0.1;
    bool removed = false;
    if (w->type == PFIT_TYPE_END) {
      float hVal;
      if (w->height > 0) {
        hVal = w->height;
      } else {
        hVal = 0;
      }
      if (w->pos.y + hVal < 1) {
        removed = true;
      }
    } else {
      float dir;
      if (w->type == PFIT_TYPE_FIX) {
        dir = 1;
      } else {
        dir = -1;
      }
      w->targetY += dir * difficulty * 0.02;
    }
    if (!removed) {
      if (w->type == PFIT_TYPE_MOVE && matchTicks < 30) {
        color = BLUE;
      } else {
        color = GREEN;
      }
      float x;
      if (w->type == PFIT_TYPE_MOVE && matchTicks <= 30) {
        x = wrap(w->pos.x + wallX + input.pos.x, -wallWidth, 100);
      } else {
        x = w->pos.x;
      }
      Collision wc = rect(x, w->pos.y, ceil(w->width), w->height);
      if (wc.isColliding.rect[GREEN] && matchTicks < 30 && w->type == PFIT_TYPE_MOVE) {
        color = RED;
        text("x", x + wallWidth / 2, wrap(w->pos.y + w->height, -wallWidth, 100));
        color = BLUE;
        play(EXPLOSION);
        gameOver();
      }
    }
    if (removed) {
      memcpy(&walls[wi2], &walls[wi2 + 1], (wallCount - 1 - wi2) * sizeof(walls[0]));
      wallCount--;
    } else {
      wi2++;
    }
  }
  if (matchTicks > 30) {
    matchTicks++;
    if (matchTicks == 60) {
      TIMES(wallCount, i) {
        PFitWall *w = &walls[i];
        if (w->type == PFIT_TYPE_FIX) {
          w->targetY = -100;
        } else {
          w->targetY = 0;
        }
        w->type = PFIT_TYPE_END;
      }
      isAddingWall = true;
      matchTicks = 0;
    }
  } else if (fabs(wrap(wallX + input.pos.x, (-wallWidth * pairCount) / 2,
                        (wallWidth * pairCount) / 2)) < 3) {
    matchTicks++;
    if (matchTicks > 30) {
      play(POWER_UP);
      TIMES(wallCount, i) {
        PFitWall *w = &walls[i];
        if (w->type == PFIT_TYPE_FIX) {
          w->targetY = 1;
        } else {
          w->targetY = 99;
        }
      }
      addScore(nextScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
  } else {
    matchTicks = 0;
  }
  color = BLACK;
  char scoreText[16];
  strcpy(scoreText, "+");
  strcat(scoreText, intToChar((int)nextScore));
  text(scoreText, 3, 10);
  if (matchTicks <= 30 && nextScore > 0) {
    nextScore--;
  }
}

void addGamePFit() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
