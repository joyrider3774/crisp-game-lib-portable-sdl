#include "cglp.h"
#include "string.h"

static char *title = "SUM TEN";
static char *description = "[Tap] Forward";

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
    .viewSizeX = 150, .viewSizeY = 100, .soundSeed = 8, .isDarkColor = false};

#define SUMTEN_GRID_COUNT 10
#define SUMTEN_GRID_SIZE 11

typedef struct {
  int v;
  bool hasValue;
  bool isFilled;
} Cell;
static Cell grid[SUMTEN_GRID_COUNT][SUMTEN_GRID_COUNT];
static Vector gridPos;

typedef struct {
  Vector pos;
  int angle;
  int ticks;
} Cursor;
static Cursor cursor;

static Vector angleOfs[2];

// Upstream can queue several completed "runs" faster than the 60-tick
// display/dismiss cycle drains them - sized generously; each run is capped
// at 10 numbers by the game's own rules (a longer run always ends the game).
#define SUMTEN_MAX_RUNS 16
#define SUMTEN_MAX_RUN_LEN 10
static int numbers[SUMTEN_MAX_RUNS][SUMTEN_MAX_RUN_LEN];
static int numbersLen[SUMTEN_MAX_RUNS];
static int numbersCount;
static float showingSumTicks;

static void shiftNumbers() {
  TIMES(numbersCount - 1, i) {
    memcpy(numbers[i], numbers[i + 1], sizeof(numbers[0]));
    numbersLen[i] = numbersLen[i + 1];
  }
  numbersCount--;
}

static void update() {
  // Never reads a Collision result anywhere in this file - every position
  // check is direct grid[x][y] indexing, so the engine's own O(n^2)
  // hitbox scan is pure waste here. Restored automatically when the next
  // real game starts.
  hasCollision = false;
  if (!ticks) {
    TIMES(SUMTEN_GRID_COUNT, gx) {
      TIMES(SUMTEN_GRID_COUNT, gy) {
        bool iw = gx > 6 || (gx > 0 && gy != 6);
        grid[gx][gy].isFilled = iw;
        if (iw) {
          grid[gx][gy].hasValue = false;
          grid[gx][gy].v = 0;
        } else {
          grid[gx][gy].hasValue = true;
          grid[gx][gy].v = rndi(1, 10);
        }
      }
    }
    int initialLineValues[6] = {6, 5, 8, 1, 7, 3};
    TIMES(6, li) {
      grid[1 + li][6].v = initialLineValues[li];
      grid[1 + li][6].hasValue = true;
    }
    vectorSet(&gridPos, 100, 0);
    vectorSet(&cursor.pos, 7, 6);
    cursor.angle = 0;
    cursor.ticks = 0;
    numbersCount = 1;
    numbersLen[0] = 0;
    showingSumTicks = 0;
    vectorSet(&angleOfs[0], -1, 0);
    vectorSet(&angleOfs[1], 0, -1);
  }
  Vector np;
  cursor.ticks++;
  if (cursor.ticks >= 30) {
    int pa = cursor.angle;
    cursor.angle = (int)wrap(cursor.angle + 1, 0, 2);
    vectorSet(&np, cursor.pos.x, cursor.pos.y);
    vectorAdd(&np, angleOfs[cursor.angle].x, angleOfs[cursor.angle].y);
    if (!grid[(int)np.x][(int)np.y].hasValue) {
      cursor.angle = pa;
    }
    cursor.ticks = 0;
  }
  vectorSet(&np, cursor.pos.x, cursor.pos.y);
  vectorAdd(&np, angleOfs[cursor.angle].x, angleOfs[cursor.angle].y);
  TIMES(SUMTEN_GRID_COUNT, gx2) {
    TIMES(SUMTEN_GRID_COUNT, gy2) {
      Cell *g = &grid[gx2][gy2];
      int c;
      if (gx2 == (int)cursor.pos.x && gy2 == (int)cursor.pos.y) {
        c = CYAN;
      } else if (gx2 == (int)np.x && gy2 == (int)np.y) {
        c = RED;
      } else {
        c = BLUE;
      }
      color = c;
      float rx = gridPos.x - gx2 * SUMTEN_GRID_SIZE;
      float ry = gridPos.y + gy2 * SUMTEN_GRID_SIZE + 1;
      rect(rx, ry, SUMTEN_GRID_SIZE - 1, SUMTEN_GRID_SIZE - 1);
      color = WHITE;
      if (!g->isFilled) {
        rect(rx + 1, ry + 1, SUMTEN_GRID_SIZE - 3, SUMTEN_GRID_SIZE - 3);
        color = c;
      }
      if (g->hasValue) {
        text(intToChar(g->v), round(gridPos.x - (gx2 - 0.5) * SUMTEN_GRID_SIZE) - 1,
             round(gridPos.y + (gy2 + 0.5) * SUMTEN_GRID_SIZE));
      }
      if (c == CYAN && (rx < 0 || ry > 91)) {
        play(EXPLOSION);
        gameOver();
      }
    }
  }
  color = WHITE;
  rect(0, 0, 100, 10);
  rect(100, 0, 50, 100);
  if (input.isJustPressed) {
    play(SELECT);
    cursor.pos = np;
    grid[(int)np.x][(int)np.y].isFilled = true;
    cursor.ticks = 0;
    int curRun = numbersCount - 1;
    int v = grid[(int)np.x][(int)np.y].v;
    numbers[curRun][numbersLen[curRun]] = v;
    numbersLen[curRun]++;
    int s = 0;
    TIMES(numbersLen[curRun], vi) { s += numbers[curRun][vi]; }
    if (s % 10 == 0) {
      play(POWER_UP);
      if (numbersCount < SUMTEN_MAX_RUNS) {
        numbersCount++;
        numbersLen[numbersCount - 1] = 0;
      }
      int sc = s / 10;
      addScore(sc * sc * 10, 133, 20 + numbersLen[curRun] * 8);
      if (showingSumTicks <= 0) {
        showingSumTicks = 60;
      }
    } else if (numbersLen[curRun] >= 10) {
      showingSumTicks = 60;
      memcpy(numbers[0], numbers[curRun], sizeof(numbers[0]));
      numbersLen[0] = numbersLen[curRun];
      numbersCount = 1;
      play(EXPLOSION);
      gameOver();
    }
    vectorSet(&np, cursor.pos.x, cursor.pos.y);
    vectorAdd(&np, angleOfs[cursor.angle].x, angleOfs[cursor.angle].y);
    if (!grid[(int)np.x][(int)np.y].hasValue) {
      cursor.angle = (int)wrap(cursor.angle + 1, 0, 2);
    }
  }
  float sum = 0;
  float y = 12;
  color = BLACK;
  TIMES(numbersLen[0], ni) {
    text(intToChar(numbers[0][ni]), 135, y);
    sum += numbers[0][ni];
    y += 8;
  }
  if (sum > 0) {
    text("+)", 110, y - 8);
    rect(115, y - 5, 30, 1);
  }
  if (showingSumTicks > 0) {
    text(intToChar((int)sum), 135 - 6, y);
    showingSumTicks--;
    if (showingSumTicks == 0) {
      shiftNumbers();
      if (numbersCount > 1) {
        showingSumTicks = 60;
      }
    }
  }
  if (cursor.pos.x < 7) {
    gridPos.x -= (7 - cursor.pos.x) * 0.5;
  }
  if (cursor.pos.y < 6) {
    gridPos.y += (6 - cursor.pos.y) * 0.5;
  }
  float sspeed = sqrt(difficulty) * 0.01;
  vectorAdd(&gridPos, -sspeed, sspeed);
  if (gridPos.x <= 90) {
    for (int gx3 = 9; gx3 >= 0; gx3--) {
      int wi = rndi(1, 15);
      TIMES(10, gy3) {
        if (gx3 > 0) {
          grid[gx3][gy3] = grid[gx3 - 1][gy3];
        } else {
          bool isFilled = (gy3 == wi) && !grid[gx3 + 1][gy3 - 1].isFilled;
          grid[gx3][gy3].isFilled = isFilled;
          if (isFilled) {
            grid[gx3][gy3].hasValue = false;
            grid[gx3][gy3].v = 0;
          } else {
            grid[gx3][gy3].hasValue = true;
            grid[gx3][gy3].v = rndi(1, 10);
          }
        }
      }
    }
    gridPos.x += 10;
    cursor.pos.x++;
  }
  if (gridPos.y >= 10) {
    for (int gy4 = 9; gy4 >= 0; gy4--) {
      int wi2 = rndi(1, 15);
      TIMES(10, gx4) {
        if (gy4 > 0) {
          grid[gx4][gy4] = grid[gx4][gy4 - 1];
        } else {
          bool isFilled2 = (gx4 == wi2) && !grid[gx4 - 1][gy4 + 1].isFilled;
          grid[gx4][gy4].isFilled = isFilled2;
          if (isFilled2) {
            grid[gx4][gy4].hasValue = false;
            grid[gx4][gy4].v = 0;
          } else {
            grid[gx4][gy4].hasValue = true;
            grid[gx4][gy4].v = rndi(1, 10);
          }
        }
      }
    }
    gridPos.y -= 10;
    cursor.pos.y++;
  }
}

void addGameSumTen() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
