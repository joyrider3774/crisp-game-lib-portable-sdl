#include "cglp.h"

static char *title = "FORFOUR";
static char *description = "[Tap] Roll";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "llllll",
    "ll  ll",
    "ll  ll",
    "llllll",
    " llll ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define FORFOUR_CIRCLE_COUNT 13
#define FORFOUR_CC 6
#define FORFOUR_CIRCLES_OFS_X 12
#define FORFOUR_CIRCLES_OFS_Y 18

static int colors[5] = {BLACK, RED, GREEN, BLUE, YELLOW};
static int lightColors[5] = {LIGHT_BLACK, LIGHT_RED, LIGHT_GREEN, LIGHT_BLUE, LIGHT_YELLOW};

static int circles[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT];
static int tmpCircles[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT];
static bool ic[FORFOUR_CIRCLE_COUNT][FORFOUR_CIRCLE_COUNT];

static Vector flying;
static int flyingCount;
static int flyingAngle;
static int flyingColor;

static int checkSize() {
  int minX = FORFOUR_CIRCLE_COUNT - 1;
  int maxX = 0;
  int minY = FORFOUR_CIRCLE_COUNT - 1;
  int maxY = 0;
  TIMES(FORFOUR_CIRCLE_COUNT, y) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      if (circles[x][y] >= 0) {
        if (minX > x) {
          minX = x;
        }
        if (maxX < x) {
          maxX = x;
        }
        if (minY > y) {
          minY = y;
        }
        if (maxY < y) {
          maxY = y;
        }
      }
    }
  }
  minX = FORFOUR_CC - minX + 1;
  maxX = maxX - FORFOUR_CC + 1;
  minY = FORFOUR_CC - minY + 1;
  maxY = maxY - FORFOUR_CC + 1;
  int m = minX;
  if (maxX > m) {
    m = maxX;
  }
  if (minY > m) {
    m = minY;
  }
  if (maxY > m) {
    m = maxY;
  }
  return m;
}

// Fills the global ic[][] grid with the connected component reachable
// from (px, py); cl > 0 restricts the match to that color index, cl < 0
// matches any non-empty cell. Returns the component's cell count.
static int checkConnection(int px, int py, int cl) {
  int c = 1;
  TIMES(FORFOUR_CIRCLE_COUNT, x) { TIMES(FORFOUR_CIRCLE_COUNT, y) { ic[x][y] = false; } }
  ic[px][py] = true;
  TIMES(9, iter) {
    int pc = c;
    for (int y = 0; y < FORFOUR_CIRCLE_COUNT - 1; y++) {
      for (int x = 0; x < FORFOUR_CIRCLE_COUNT - 1; x++) {
        if (!ic[x][y]) {
          continue;
        }
        bool matchRight;
        if (cl > 0) {
          matchRight = circles[x + 1][y] == cl;
        } else {
          matchRight = circles[x + 1][y] >= 0;
        }
        if (matchRight && !ic[x + 1][y]) {
          ic[x + 1][y] = true;
          c++;
        }
        bool matchDown;
        if (cl > 0) {
          matchDown = circles[x][y + 1] == cl;
        } else {
          matchDown = circles[x][y + 1] >= 0;
        }
        if (matchDown && !ic[x][y + 1]) {
          ic[x][y + 1] = true;
          c++;
        }
      }
    }
    for (int y = FORFOUR_CIRCLE_COUNT - 1; y > 0; y--) {
      for (int x = FORFOUR_CIRCLE_COUNT - 1; x > 0; x--) {
        if (!ic[x][y]) {
          continue;
        }
        bool matchLeft;
        if (cl > 0) {
          matchLeft = circles[x - 1][y] == cl;
        } else {
          matchLeft = circles[x - 1][y] >= 0;
        }
        if (matchLeft && !ic[x - 1][y]) {
          ic[x - 1][y] = true;
          c++;
        }
        bool matchUp;
        if (cl > 0) {
          matchUp = circles[x][y - 1] == cl;
        } else {
          matchUp = circles[x][y - 1] >= 0;
        }
        if (matchUp && !ic[x][y - 1]) {
          ic[x][y - 1] = true;
          c++;
        }
      }
    }
    if (pc == c) {
      break;
    }
  }
  return c;
}

static void update() {
  // Never reads a Collision result anywhere in this file - every hit/
  // placement check is direct circles[][] grid indexing, so the engine's
  // own O(n^2) hitbox scan is pure waste here.
  hasCollision = false;
  if (!ticks) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      TIMES(FORFOUR_CIRCLE_COUNT, y) {
        circles[x][y] = -1;
        tmpCircles[x][y] = -1;
      }
    }
    circles[FORFOUR_CC][FORFOUR_CC] = 0;
    for (int i = FORFOUR_CC + 1; i < FORFOUR_CIRCLE_COUNT - 1; i++) {
      circles[i][FORFOUR_CC] = rndi(1, 5);
    }
    vectorSet(&flying, 0, 0);
    flyingCount = -999;
  }
  if (flyingCount < -900) {
    flyingCount = (int)(60 / difficulty);
    flyingAngle = rndi(0, 4);
    flyingColor = rndi(1, 5);
    int sz = checkSize();
    if (rnd(0, 1) < sqrt(sz) * 0.1) {
      flyingColor = 0;
      sz = (int)ceil(sz * 0.7);
    }
    int minV = FORFOUR_CC - sz + 1;
    int maxV = FORFOUR_CC + sz - 1;
    if (flyingAngle == 0) {
      vectorSet(&flying, -1, rndi(minV, maxV + 1));
    } else if (flyingAngle == 1) {
      vectorSet(&flying, rndi(minV, maxV + 1), -1);
    } else if (flyingAngle == 2) {
      vectorSet(&flying, FORFOUR_CIRCLE_COUNT, rndi(minV, maxV + 1));
    } else if (flyingAngle == 3) {
      vectorSet(&flying, rndi(minV, maxV + 1), FORFOUR_CIRCLE_COUNT);
    }
    vectorSet(&flying, flying.x * 6 + FORFOUR_CIRCLES_OFS_X,
              flying.y * 6 + FORFOUR_CIRCLES_OFS_Y);
  }
  flyingCount--;
  color = lightColors[flyingColor];
  if (flyingAngle % 2 == 0) {
    rect(0, flying.y - 1, 99, 2);
  } else {
    rect(flying.x - 1, 0, 2, 99);
  }
  if (flyingCount < 0) {
    addWithAngle(&flying, (flyingAngle * M_PI) / 2, difficulty);
    Vector c;
    vectorSet(&c, round((flying.x - FORFOUR_CIRCLES_OFS_X) / 6),
              round((flying.y - FORFOUR_CIRCLES_OFS_Y) / 6));
    bool inRectC = c.x >= 0 && c.x < FORFOUR_CIRCLE_COUNT && c.y >= 0 && c.y < FORFOUR_CIRCLE_COUNT;
    if (inRectC && circles[(int)c.x][(int)c.y] >= 0) {
      if (flyingColor == 0) {
        flyingColor = circles[(int)c.x][(int)c.y];
        if (flyingColor == 0) {
          flyingColor = rndi(1, 5);
        }
      }
      TIMES(99, i) {
        addWithAngle(&c, (flyingAngle * M_PI) / 2 + M_PI, 1);
        c.x = round(c.x);
        c.y = round(c.y);
        bool cInRect =
            c.x >= 0 && c.x < FORFOUR_CIRCLE_COUNT && c.y >= 0 && c.y < FORFOUR_CIRCLE_COUNT;
        if (!cInRect) {
          play(EXPLOSION);
          color = RED;
          text("X", FORFOUR_CIRCLES_OFS_X + c.x * 6, FORFOUR_CIRCLES_OFS_Y + c.y * 6);
          gameOver();
          break;
        }
        if (circles[(int)c.x][(int)c.y] < 0) {
          play(LASER);
          circles[(int)c.x][(int)c.y] = flyingColor;
          flyingCount = -999;
          int cnt = checkConnection((int)c.x, (int)c.y, flyingColor);
          if (cnt >= 4) {
            play(COIN);
            int dcc = 0;
            TIMES(FORFOUR_CIRCLE_COUNT, y2) {
              TIMES(FORFOUR_CIRCLE_COUNT, x2) {
                if (ic[x2][y2]) {
                  color = colors[circles[x2][y2]];
                  particle(FORFOUR_CIRCLES_OFS_X + x2 * 6, FORFOUR_CIRCLES_OFS_Y + y2 * 6, 4, 1, 0,
                           M_PI * 2);
                  circles[x2][y2] = -1;
                  dcc++;
                }
              }
            }
            checkConnection(FORFOUR_CC, FORFOUR_CC, -1);
            TIMES(FORFOUR_CIRCLE_COUNT, y3) {
              TIMES(FORFOUR_CIRCLE_COUNT, x3) {
                if (!ic[x3][y3] && circles[x3][y3] > 0) {
                  color = lightColors[circles[x3][y3]];
                  particle(FORFOUR_CIRCLES_OFS_X + x3 * 6, FORFOUR_CIRCLES_OFS_Y + y3 * 6, 4, 1, 0,
                           M_PI * 2);
                  circles[x3][y3] = -1;
                  dcc++;
                }
              }
            }
            dcc -= 3;
            addScore(dcc * dcc, VEC_XY(flying));
          }
          break;
        }
      }
    }
  }
  bool flyingInRect = flying.x >= -3 && flying.x < 103 && flying.y >= -3 &&
                       flying.y < 103;
  if (!flyingInRect) {
    flyingCount = -999;
  }
  color = colors[flyingColor];
  character("a", VEC_XY(flying));
  if (input.isJustPressed) {
    play(SELECT);
    TIMES(FORFOUR_CIRCLE_COUNT, y) {
      TIMES(FORFOUR_CIRCLE_COUNT, x) { tmpCircles[x][y] = circles[x][y]; }
    }
    TIMES(FORFOUR_CIRCLE_COUNT, y) {
      TIMES(FORFOUR_CIRCLE_COUNT, x) {
        circles[FORFOUR_CIRCLE_COUNT - 1 - y][x] = tmpCircles[x][y];
      }
    }
  }
  TIMES(FORFOUR_CIRCLE_COUNT, y) {
    TIMES(FORFOUR_CIRCLE_COUNT, x) {
      int c = circles[x][y];
      if (c >= 0) {
        color = colors[c];
        character("a", FORFOUR_CIRCLES_OFS_X + x * 6, FORFOUR_CIRCLES_OFS_Y + y * 6);
      }
    }
  }
}

void addGameForfour() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
