#include "cglp.h"
#include "string.h"

static char *title = "TT FENCE";
static char *description = "[Tap] Place";

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

#define TTFENCE_GRID_SIZE 15
static bool grid[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE];
static bool tmpGrid[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE];
static bool bombEdgeGrid[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE];
static bool bombAnimGrid[TTFENCE_GRID_SIZE][TTFENCE_GRID_SIZE];

#define TTFENCE_BLOCK_PATTERN_COUNT 7
static int blockPatterns[TTFENCE_BLOCK_PATTERN_COUNT][3] = {
    {0, 0, 0}, {0, 1, 0}, {0, 3, 0}, {0, 0, 1}, {0, 0, 3}, {1, 0, 0}, {3, 0, 0},
};
static int angleOfs[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};

static Vector pos;
static int angle;
static int prevAngle;
static int type;
static float bombAnimTicks;
static float damage;
static float damageTarget;
static bool isDamageShown;
static float nextRotationTicks;

static void addAngle(Vector *p, int angle) {
  p->x += angleOfs[angle][0];
  p->y += angleOfs[angle][1];
}

static bool canPlaceGrid(Vector *p) {
  bool inRect = p->x >= 0 && p->x < TTFENCE_GRID_SIZE && p->y >= 0 && p->y < TTFENCE_GRID_SIZE;
  if (!inRect) {
    return false;
  }
  return !grid[(int)p->x][(int)p->y];
}

static bool existsGrid(Vector *p) {
  bool inRect = p->x >= 0 && p->x < TTFENCE_GRID_SIZE && p->y >= 0 && p->y < TTFENCE_GRID_SIZE;
  if (!inRect) {
    return false;
  }
  return grid[(int)p->x][(int)p->y];
}

static void drawGrid(int x, int y, float size) {
  box((x - TTFENCE_GRID_SIZE / 2.0) * 6 + 53, (y - TTFENCE_GRID_SIZE / 2.0) * 6 + 56, size, size);
}

static void setBlock(float rpx, float rpy, int type, int angle) {
  int hitCount = 0;
  Vector p;
  vectorSet(&p, rpx, rpy);
  addAngle(&p, angle);
  if (!grid[(int)p.x][(int)p.y]) {
    grid[(int)p.x][(int)p.y] = true;
  } else {
    hitCount++;
  }
  TIMES(3, bi) {
    int ba = blockPatterns[type][bi];
    addAngle(&p, (int)wrap(angle + ba, 0, 4));
    if (!grid[(int)p.x][(int)p.y]) {
      grid[(int)p.x][(int)p.y] = true;
    } else {
      hitCount++;
    }
  }
  if (hitCount > 0) {
    play(HIT);
    damageTarget += hitCount * 9 * sqrt(difficulty);
  }
  pos = p;
}

static void setBlockTmp(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  addAngle(&p, angle);
  tmpGrid[(int)p.x][(int)p.y] = true;
  TIMES(3, bi) {
    int ba = blockPatterns[type][bi];
    addAngle(&p, (int)wrap(angle + ba, 0, 4));
    tmpGrid[(int)p.x][(int)p.y] = true;
  }
}

static bool checkBlock(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  addAngle(&p, angle);
  bool canPlacing = canPlaceGrid(&p);
  TIMES(3, bi) {
    int ba = blockPatterns[type][bi];
    addAngle(&p, (int)wrap(angle + ba, 0, 4));
    canPlacing = canPlacing && canPlaceGrid(&p);
  }
  return canPlacing;
}

static bool checkBlockInGrid(float rpx, float rpy, int type, int angle) {
  Vector p;
  vectorSet(&p, rpx, rpy);
  addAngle(&p, angle);
  bool canPlacing = p.x >= 0 && p.x < TTFENCE_GRID_SIZE && p.y >= 0 && p.y < TTFENCE_GRID_SIZE;
  TIMES(3, bi) {
    int ba = blockPatterns[type][bi];
    addAngle(&p, (int)wrap(angle + ba, 0, 4));
    bool inR = p.x >= 0 && p.x < TTFENCE_GRID_SIZE && p.y >= 0 && p.y < TTFENCE_GRID_SIZE;
    canPlacing = canPlacing && inR;
  }
  return canPlacing;
}

static int fillBombDown() {
  int bc = 0;
  Vector p;
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      vectorSet(&p, x, y);
      if (canPlaceGrid(&p) && bombEdgeGrid[x][y]) {
        vectorSet(&p, x, y);
        addAngle(&p, 0);
        if (canPlaceGrid(&p) && !bombEdgeGrid[(int)p.x][(int)p.y]) {
          bombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
        vectorSet(&p, x, y);
        addAngle(&p, 1);
        if (canPlaceGrid(&p) && !bombEdgeGrid[(int)p.x][(int)p.y]) {
          bombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
      }
    }
  }
  return bc;
}

static int fillBombUp() {
  int bc = 0;
  Vector p;
  for (int y = TTFENCE_GRID_SIZE - 1; y >= 0; y--) {
    for (int x = TTFENCE_GRID_SIZE - 1; x >= 0; x--) {
      vectorSet(&p, x, y);
      if (canPlaceGrid(&p) && bombEdgeGrid[x][y]) {
        vectorSet(&p, x, y);
        addAngle(&p, 2);
        if (canPlaceGrid(&p) && !bombEdgeGrid[(int)p.x][(int)p.y]) {
          bombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
        vectorSet(&p, x, y);
        addAngle(&p, 3);
        if (canPlaceGrid(&p) && !bombEdgeGrid[(int)p.x][(int)p.y]) {
          bombEdgeGrid[(int)p.x][(int)p.y] = true;
          bc++;
        }
      }
    }
  }
  return bc;
}

static void bomb() {
  int bc = 0;
  Vector bp;
  vectorSet(&bp, 0, 0);
  memset(bombAnimGrid, 0, sizeof(bombAnimGrid));
  Vector p;
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      vectorSet(&p, x, y);
      if (canPlaceGrid(&p) && !bombEdgeGrid[x][y]) {
        TIMES(4, oi) {
          int ox = angleOfs[oi][0];
          int oy = angleOfs[oi][1];
          int nx = x + ox;
          int ny = y + oy;
          if (nx < 0 || nx >= TTFENCE_GRID_SIZE || ny < 0 || ny >= TTFENCE_GRID_SIZE) {
            continue;
          }
          if (!bombAnimGrid[nx][ny]) {
            bombAnimGrid[nx][ny] = true;
            bc++;
            bp.x += nx;
            bp.y += ny;
          }
        }
      }
    }
  }
  int loopMax = (int)floor(sqrt(bc) * 0.5);
  TIMES(loopMax, li) {
    int pbc = bc;
    for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
      for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
        if (bombAnimGrid[x][y]) {
          TIMES(4, oi2) {
            int ox = angleOfs[oi2][0];
            int oy = angleOfs[oi2][1];
            vectorSet(&p, x + ox, y + oy);
            if (existsGrid(&p) && !bombAnimGrid[(int)p.x][(int)p.y]) {
              bombAnimGrid[(int)p.x][(int)p.y] = true;
              bc++;
              bp.x += p.x;
              bp.y += p.y;
            }
          }
        }
      }
    }
    if (pbc == bc) {
      break;
    }
  }
  for (int y = 0; y < TTFENCE_GRID_SIZE; y++) {
    for (int x = 0; x < TTFENCE_GRID_SIZE; x++) {
      if (bombAnimGrid[x][y]) {
        grid[x][y] = false;
      }
    }
  }
  bombAnimTicks = 30;
  if (bc > 0) {
    play(EXPLOSION);
    bp.x /= bc;
    bp.y /= bc;
    int sc = (int)ceil(bc * sqrt(bc));
    addScore(sc, (bp.x - TTFENCE_GRID_SIZE / 2.0) * 6 + 53,
             (bp.y - TTFENCE_GRID_SIZE / 2.0) * 6 + 56);
    damageTarget = clamp(damageTarget - sc * 0.1, 0, 99);
  }
}

static void checkBomb() {
  memset(bombEdgeGrid, 0, sizeof(bombEdgeGrid));
  TIMES(TTFENCE_GRID_SIZE, i) {
    bombEdgeGrid[i][0] = true;
    bombEdgeGrid[i][TTFENCE_GRID_SIZE - 1] = true;
    bombEdgeGrid[0][i] = true;
    bombEdgeGrid[TTFENCE_GRID_SIZE - 1][i] = true;
  }
  TIMES(99, i2) {
    if (fillBombDown() + fillBombUp() == 0) {
      break;
    }
  }
  bomb();
}

static void update() {
  // Never reads a Collision result anywhere in this file - every
  // placement/overlap check goes through canPlaceGrid()/existsGrid()
  // (direct bool grid indexing), so the engine's own O(n^2) hitbox scan
  // is pure waste here. Restored automatically when the next real game
  // starts.
  hasCollision = false;
  if (!ticks) {
    memset(grid, 0, sizeof(grid));
    memset(tmpGrid, 0, sizeof(tmpGrid));
    memset(bombEdgeGrid, 0, sizeof(bombEdgeGrid));
    memset(bombAnimGrid, 0, sizeof(bombAnimGrid));
    vectorSet(&pos, 0, 0);
    angle = 0;
    prevAngle = 0;
    type = rndi(0, TTFENCE_BLOCK_PATTERN_COUNT);
    bombAnimTicks = 0;
    damage = 0;
    damageTarget = 0;
    isDamageShown = true;
    nextRotationTicks = 0;
    setBlock(7, 7, rndi(0, TTFENCE_BLOCK_PATTERN_COUNT), rndi(0, 4));
  }
  bombAnimTicks--;
  TIMES(TTFENCE_GRID_SIZE, gx) {
    TIMES(TTFENCE_GRID_SIZE, gy) {
      if (grid[gx][gy] && tmpGrid[gx][gy]) {
        color = PURPLE;
        drawGrid(gx, gy, 6);
      } else if (grid[gx][gy]) {
        color = BLUE;
        drawGrid(gx, gy, 6);
      } else if (tmpGrid[gx][gy]) {
        color = LIGHT_BLUE;
        drawGrid(gx, gy, 6);
      } else if (bombAnimTicks > 0 && bombAnimGrid[gx][gy]) {
        color = RED;
        drawGrid(gx, gy, sin((bombAnimTicks / 30) * M_PI) * 6);
      }
    }
  }
  if (input.isJustPressed) {
    play(SELECT);
    setBlock(pos.x, pos.y, type, angle);
    checkBomb();
    type = rndi(0, TTFENCE_BLOCK_PATTERN_COUNT);
    nextRotationTicks = 0;
    damageTarget += sqrt(difficulty) * 2;
  }
  nextRotationTicks--;
  if (nextRotationTicks < 0) {
    bool canPlacing = false;
    TIMES(4, i3) {
      angle = (int)wrap(angle + 1, 0, 4);
      if (checkBlock(pos.x, pos.y, type, angle)) {
        canPlacing = true;
        break;
      }
    }
    memset(tmpGrid, 0, sizeof(tmpGrid));
    if (!canPlacing) {
      TIMES(4, i4) {
        angle = (int)wrap(angle + 1, 0, 4);
        if (checkBlockInGrid(pos.x, pos.y, type, angle)) {
          canPlacing = true;
          break;
        }
      }
    }
    if (!canPlacing) {
      damageTarget = 100;
    } else {
      setBlockTmp(pos.x, pos.y, type, angle);
    }
    if (angle != prevAngle) {
      play(LASER);
    }
    prevAngle = angle;
    nextRotationTicks = 30 / sqrt(sqrt(difficulty));
  }
  damageTarget += sqrt(difficulty) * 0.01;
  damage += (damageTarget - damage) * 0.1;
  color = RED;
  if (damage > 99) {
    rect(0, 98, 100, 2);
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  } else if (damage < 80 ||
             ticks % (int)ceil(109 - damage) < (109 - damage) * 0.8) {
    if (!isDamageShown) {
      play(COIN);
    }
    rect(0, 98, damage, 2);
    rect(99, 98, 1, 2);
    isDamageShown = true;
  } else {
    isDamageShown = false;
  }
}

void addGameTtFence() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
