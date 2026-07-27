#include "cglp.h"
#include "string.h"

static char *title = "SQUARE BAR";
static char *description = "[Hold] Grow";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = true};

#define SQUAREBAR_SIZE 40
static int angleToXy[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

#define SQUAREBAR_TYPE_PLAYER 0
#define SQUAREBAR_TYPE_ENEMY 1

typedef struct {
  Vector pos;
  float length;
  float spRatio;
  float angle;
  int tAngle;
  int type;
} Bar;
// Exactly one player bar plus exactly one enemy bar exist at all times (a
// destroyed enemy is always immediately replaced) - sized with headroom.
#define SQUAREBAR_MAX_BAR_COUNT 32
static Bar bars[SQUAREBAR_MAX_BAR_COUNT];
static int barCount;

static void removeBar(int index) {
  memcpy(&bars[index], &bars[index + 1], (barCount - 1 - index) * sizeof(bars[0]));
  barCount--;
}

#define SQUAREBAR_ITEM_SPIKE 0
#define SQUAREBAR_ITEM_GOLD 1
typedef struct {
  Vector pos;
  int type;
  float ticks;
  float size;
  bool isAlive;
} Item;
#define SQUAREBAR_MAX_ITEM_COUNT 32
static Item items[SQUAREBAR_MAX_ITEM_COUNT];
static int itemIndex;

static int multiplier;

static void addItem(int type) {
  Vector pos;
  TIMES(99, ai) {
    float rx = rnd(50 - SQUAREBAR_SIZE / 2.0 - 1 - 20, 50 + SQUAREBAR_SIZE / 2.0);
    float ryHigh;
    if (type == SQUAREBAR_ITEM_SPIKE) {
      ryHigh = 50 - SQUAREBAR_SIZE / 2.0 - 15;
    } else {
      ryHigh = 50 - SQUAREBAR_SIZE / 2.0 - 4;
    }
    float ry = rnd(50 - SQUAREBAR_SIZE / 2.0 - 1 - 20, ryHigh);
    vectorSet(&pos, rx, ry);
    vectorAdd(&pos, -50, -50);
    rotate(&pos, rndi(0, 4) * M_PI / 2);
    vectorAdd(&pos, 50, 50);
    if (distanceTo(&pos, VEC_XY(bars[0].pos)) > 36) {
      break;
    }
  }
  ASSIGN_ARRAY_ITEM(items, itemIndex, Item, ni);
  ni->pos = pos;
  ni->type = type;
  ni->ticks = floor(rnd(777, 999) / sqrt(difficulty));
  ni->size = 0;
  ni->isAlive = true;
  itemIndex = wrap(itemIndex + 1, 0, SQUAREBAR_MAX_ITEM_COUNT);
}

static void update() {
  if (!ticks) {
    barCount = 2;
    vectorSet(&bars[0].pos, 50, 50 - SQUAREBAR_SIZE / 2.0 - 1);
    bars[0].length = 9;
    bars[0].spRatio = 0;
    bars[0].angle = 3;
    bars[0].tAngle = 0;
    bars[0].type = SQUAREBAR_TYPE_PLAYER;
    vectorSet(&bars[1].pos, 50, 50 + SQUAREBAR_SIZE / 2.0 + 1);
    bars[1].length = 12;
    bars[1].spRatio = 0;
    bars[1].angle = 1;
    bars[1].tAngle = 2;
    bars[1].type = SQUAREBAR_TYPE_ENEMY;
    INIT_UNALIVED_ARRAY(items);
    itemIndex = 0;
    TIMES(3, ii) { addItem(SQUAREBAR_ITEM_SPIKE); }
    TIMES(7, ii2) { addItem(SQUAREBAR_ITEM_GOLD); }
    multiplier = 1;
  }
  color = LIGHT_BLACK;
  box(50, 50, SQUAREBAR_SIZE, SQUAREBAR_SIZE);
  bool isChased = false;
  Vector cpTmp;
  vectorSet(&cpTmp, 50, 50);
  float a0 = angleTo(&cpTmp, VEC_XY(bars[0].pos));
  float a1 = angleTo(&cpTmp, VEC_XY(bars[1].pos));
  if (wrap(a0 - a1, -M_PI, M_PI) > 0) {
    isChased = true;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  int bi = 0;
  while (bi < barCount) {
    Bar *b = &bars[bi];
    bool removed = false;
    if (b->type == SQUAREBAR_TYPE_PLAYER) {
      if (input.isPressed) {
        b->length += (20 + sqrt(difficulty) * 9 - b->length) * 0.1;
      } else {
        b->length += (9 - b->length) * 0.2;
      }
    } else {
      b->length = 11 + sqrt(difficulty) * 4;
    }
    b->angle = wrap(b->angle + 0.03 * difficulty, 0, 4);
    if (b->angle >= b->tAngle && b->angle < b->tAngle + M_PI / 4) {
      play(HIT);
      b->angle = b->tAngle;
      int dirIdx = (int)b->angle;
      Vector np;
      vectorSet(&np, angleToXy[dirIdx][0], angleToXy[dirIdx][1]);
      vectorMul(&np, b->length * (1 - b->spRatio));
      vectorAdd(&np, b->pos.x, b->pos.y);
      if (b->angle == 0) {
        if (np.x > 50 + SQUAREBAR_SIZE / 2.0 + 1) {
          b->tAngle = 1;
          b->spRatio = 1 - (np.x - (50 + SQUAREBAR_SIZE / 2.0 + 1)) / b->length;
          vectorSet(&b->pos, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          b->angle = 2;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 1) {
        if (np.y > 50 + SQUAREBAR_SIZE / 2.0 + 1) {
          b->tAngle = 2;
          b->spRatio = 1 - (np.y - (50 + SQUAREBAR_SIZE / 2.0 + 1)) / b->length;
          vectorSet(&b->pos, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50 + SQUAREBAR_SIZE / 2.0 + 1);
        } else {
          b->angle = 3;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 2) {
        if (np.x < 50 - SQUAREBAR_SIZE / 2.0 - 1) {
          b->tAngle = 3;
          b->spRatio = 1 - (50 - SQUAREBAR_SIZE / 2.0 - 1 - np.x) / b->length;
          vectorSet(&b->pos, 50 - SQUAREBAR_SIZE / 2.0 - 1, 50 + SQUAREBAR_SIZE / 2.0 + 1);
        } else {
          b->angle = 0;
          b->spRatio = 0;
          b->pos = np;
        }
      } else if (b->angle == 3) {
        if (np.y < 50 - SQUAREBAR_SIZE / 2.0 - 1) {
          b->tAngle = 0;
          b->spRatio = 1 - (50 - SQUAREBAR_SIZE / 2.0 - 1 - np.y) / b->length;
          vectorSet(&b->pos, 50 - SQUAREBAR_SIZE / 2.0 - 1, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          b->angle = 1;
          b->spRatio = 0;
          b->pos = np;
        }
      }
    }
    if (b->type == SQUAREBAR_TYPE_PLAYER) {
      color = GREEN;
    } else if (isChased) {
      color = RED;
    } else {
      color = YELLOW;
    }
    thickness = 3;
    barCenterPosRatio = b->spRatio;
    Collision bc = bar(b->pos.x, b->pos.y, b->length, (b->angle * M_PI) / 2);
    if (b->type == SQUAREBAR_TYPE_ENEMY && bc.isColliding.rect[GREEN]) {
      if (isChased) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(POWER_UP);
        addScore(10 * multiplier, VEC_XY(b->pos));
        multiplier++;
        int ta = (int)wrap(bars[0].tAngle + 2, 0, 4);
        Vector chosenPs;
        if (ta == 0) {
          vectorSet(&chosenPs, 50, 50 - SQUAREBAR_SIZE / 2.0 - 1);
        } else if (ta == 1) {
          vectorSet(&chosenPs, 50 + SQUAREBAR_SIZE / 2.0 + 1, 50);
        } else if (ta == 2) {
          vectorSet(&chosenPs, 50, 50 + SQUAREBAR_SIZE / 2.0 - 1);
        } else {
          vectorSet(&chosenPs, 50 - SQUAREBAR_SIZE / 2.0 + 1, 50);
        }
        Bar *newBar = &bars[barCount];
        newBar->pos = chosenPs;
        newBar->length = 12;
        newBar->spRatio = 0;
        newBar->angle = wrap(ta - 1, 0, 4);
        newBar->tAngle = ta;
        newBar->type = SQUAREBAR_TYPE_ENEMY;
        barCount++;
        removed = true;
      }
    }
    if (removed) {
      removeBar(bi);
    } else {
      bi++;
    }
  }
  FOR_EACH(items, ii3) {
    ASSIGN_ARRAY_ITEM(items, ii3, Item, it);
    SKIP_IS_NOT_ALIVE(it);
    if (it->ticks < 60) {
      if ((int)it->ticks % 15 == 0) {
        it->size--;
      }
    } else if (it->size < 4 && (int)it->ticks % 15 == 0) {
      it->size++;
    }
    if (it->type == SQUAREBAR_ITEM_SPIKE) {
      color = RED;
    } else {
      color = YELLOW;
    }
    Collision ic = box(VEC_XY(it->pos), it->size, it->size);
    if (ic.isColliding.rect[GREEN]) {
      if (it->type == SQUAREBAR_ITEM_SPIKE) {
        if (it->size == 4) {
          play(EXPLOSION);
          gameOver();
        }
      } else {
        play(COIN);
        addScore(multiplier, VEC_XY(it->pos));
        addItem(SQUAREBAR_ITEM_GOLD);
        it->isAlive = false;
        continue;
      }
    }
    it->ticks--;
    if (it->ticks < 0 || ic.isColliding.rect[RED] || ic.isColliding.rect[YELLOW]) {
      addItem(it->type);
      it->isAlive = false;
      continue;
    }
  }
}

void addGameSquareBar() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
