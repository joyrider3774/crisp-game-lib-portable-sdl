#include "cglp.h"
#include "string.h"

static char *title = "MARUSANSI";
static char *description = "Tap\nto start";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " rrr  ",
        "rRRRr ",
        "rRRRr ",
        "rRRRr ",
        " rrr  ",
        "      ",
    },
    {
        "  g   ",
        " gGg  ",
        " gGg  ",
        "gGGGg ",
        "ggggg ",
        "      ",
    },
    {
        "bbbbb ",
        "bBBBb ",
        "bBBBb ",
        "bBBBb ",
        "bbbbb ",
        "      ",
    },
    {
        " RRR  ",
        "R   R ",
        "R   R ",
        "R   R ",
        " RRR  ",
        "      ",
    },
    {
        "  G   ",
        " G G  ",
        " G G  ",
        "G   G ",
        "GGGGG ",
        "      ",
    },
    {
        "BBBBB ",
        "B   B ",
        "B   B ",
        "B   B ",
        "BBBBB ",
        "      ",
    },
    {
        "  l   ",
        " lll  ",
        "l l l ",
        "  l   ",
        "  l   ",
        "      ",
    },
    {
        "  l   ",
        "  l   ",
        "l l l ",
        " lll  ",
        "  l   ",
        "      ",
    },
};
static int charactersCount = 8;

// Vircon32 port note: this engine has no rewind/replay system (see the
// FOOSAN/FOOT LASER port notes elsewhere) - the JS "startTicks =
// isReplaying ? 0 : 270" always takes the non-replaying branch here.
static Options options = {
    .viewSizeX = 80, .viewSizeY = 80, .soundSeed = 9, .isDarkColor = false};

#define MARUSANSI_GRID_WIDTH 6
#define MARUSANSI_GRID_HEIGHT 12

static int grid[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT];
static int gridHeight[MARUSANSI_GRID_WIDTH];
static int hGrid[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT];
static int vGrid[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT];
static bool sGrid[MARUSANSI_GRID_WIDTH][MARUSANSI_GRID_HEIGHT];
static int blocks[2];
static int nextRow[MARUSANSI_GRID_WIDTH];
static float nextRowTicks;
static float chainingTicks;
static float fallingTicks;
static int multiplier;
static char message[32];
static float messageTicks;
static float startTicks;

static void calcPixelPosition(int x, int y, Vector *out) {
  out->x = 40 - MARUSANSI_GRID_WIDTH * 6 / 2 + x * 6 + 3;
  out->y = MARUSANSI_GRID_HEIGHT * 6 - y * 6 - 3;
}

static void calcGridHeight() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int y = 0;
    for (; y < MARUSANSI_GRID_HEIGHT; y++) {
      if (grid[x][y] == 0) {
        break;
      }
    }
    gridHeight[x] = y;
  }
}

static void checkGridSequence() {
  bool existsSequence = false;
  int count = 0;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int g = grid[x][y];
      if (g == 0) {
        continue;
      }
      if (hGrid[x][y] == 0 && x < MARUSANSI_GRID_WIDTH - 2 &&
          grid[x + 1][y] == g && grid[x + 2][y] == g) {
        existsSequence = true;
        int i = g;
        for (int gx = x; gx < MARUSANSI_GRID_WIDTH; gx++) {
          i++;
          if ((i - 1) % 3 == g - 1) {
            i++;
          }
          if (grid[gx][y] != g) {
            break;
          }
          hGrid[gx][y] = i;
          count++;
        }
      }
      if (vGrid[x][y] == 0 && y < MARUSANSI_GRID_HEIGHT - 2 &&
          grid[x][y + 1] == g && grid[x][y + 2] == g) {
        existsSequence = true;
        int vi = g;
        for (int gy = y; gy < MARUSANSI_GRID_HEIGHT; gy++) {
          vi++;
          if ((vi - 1) % 3 == g - 1) {
            vi++;
          }
          if (grid[x][gy] != g) {
            break;
          }
          vGrid[x][gy] = vi;
          count++;
        }
      }
    }
  }
  if (existsSequence) {
    int sc = count * multiplier;
    strcpy(message, intToChar(count));
    strcat(message, "x");
    strcat(message, intToChar(multiplier));
    strcat(message, "=");
    strcat(message, intToChar(sc));
    messageTicks = 60;
    addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    chainingTicks = 1;
    multiplier++;
    play(POWER_UP);
  }
}

static void changeSequence() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int vh;
      if (vGrid[x][y] > 0) {
        vh = vGrid[x][y];
      } else {
        vh = hGrid[x][y];
      }
      if (vh > 0) {
        grid[x][y] = (vh - 1) % 3 + 1;
        sGrid[x][y] = true;
        vGrid[x][y] = 0;
        hGrid[x][y] = 0;
      }
    }
  }
}

static void clearSequence() {
  play(JUMP);
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      if (sGrid[x][y]) {
        if (grid[x][y] == 1) {
          color = RED;
        } else if (grid[x][y] == 2) {
          color = GREEN;
        } else {
          color = BLUE;
        }
        Vector p;
        calcPixelPosition(x, y, &p);
        particle(p.x, p.y, 9, 1, 0, M_PI * 2);
        grid[x][y] = 0;
      }
    }
  }
  color = BLACK;
  fallingTicks = 1;
}

static bool fallBlocks() {
  bool isFalling = false;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      if (sGrid[x][y]) {
        isFalling = true;
        for (int gy = y; gy < MARUSANSI_GRID_HEIGHT - 1; gy++) {
          grid[x][gy] = grid[x][gy + 1];
          sGrid[x][gy] = sGrid[x][gy + 1];
        }
        grid[x][MARUSANSI_GRID_HEIGHT - 1] = 0;
        sGrid[x][MARUSANSI_GRID_HEIGHT - 1] = false;
      }
    }
  }
  return isFalling;
}

static void setNextBlock() {
  int pb0 = blocks[0];
  int pb1 = blocks[1];
  TIMES(2, i) { blocks[i] = rndi(1, 4); }
  if (blocks[0] == pb0 && blocks[1] == pb1) {
    blocks[0] = blocks[0] % 3 + 1;
  }
}

static void setNextRow() {
  play(LASER);
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int cs[3];
    int csCount = 0;
    TIMES(3, i) {
      int c = i + 1;
      if (grid[x][0] == c && grid[x][1] == c) {
        continue;
      }
      if (x > 1 && nextRow[x - 1] == c && nextRow[x - 2] == c) {
        continue;
      }
      cs[csCount] = c;
      csCount++;
    }
    if (csCount == 0) {
      cs[0] = rndi(1, 4);
      csCount = 1;
    }
    nextRow[x] = cs[rndi(0, csCount)];
  }
}

static void addNextRow() {
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    for (int y = MARUSANSI_GRID_HEIGHT - 1; y > 0; y--) {
      grid[x][y] = grid[x][y - 1];
    }
    grid[x][0] = nextRow[x];
  }
  setNextRow();
  calcGridHeight();
  nextRowTicks = 0;
}

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped
  // column/next-row-bar is computed directly from input.pos via grid
  // math (see "isSelected"/"isNextRowSelected" below), so the engine's
  // own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure waste
  // here. Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(MARUSANSI_GRID_WIDTH, x) {
      TIMES(MARUSANSI_GRID_HEIGHT, y) {
        grid[x][y] = 0;
        hGrid[x][y] = 0;
        vGrid[x][y] = 0;
        sGrid[x][y] = false;
      }
      gridHeight[x] = 0;
      nextRow[x] = 0;
    }
    blocks[0] = 1;
    blocks[1] = 2;
    nextRowTicks = 0;
    calcGridHeight();
    setNextRow();
    chainingTicks = 0;
    fallingTicks = 0;
    multiplier = 1;
    message[0] = 0;
    messageTicks = 0;
    startTicks = 270;
  }
  color = LIGHT_BLACK;
  rect(20, MARUSANSI_GRID_HEIGHT * 6, 40, 1);
  color = BLACK;
  int chainingIndex;
  if (chainingTicks > 0) {
    chainingIndex = (int)floor(chainingTicks / 5);
  } else {
    chainingIndex = 0;
  }
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    TIMES(MARUSANSI_GRID_HEIGHT, y) {
      int g = grid[x][y];
      if (g == 0) {
        continue;
      }
      Vector p;
      calcPixelPosition(x, y, &p);
      int vh;
      if (vGrid[x][y] > 0) {
        vh = vGrid[x][y];
      } else {
        vh = hGrid[x][y];
      }
      if (sGrid[x][y] || vh > 0) {
        color = YELLOW;
        box(p.x, p.y, 6, 6);
        color = BLACK;
      }
      char cc[2];
      cc[1] = 0;
      if (vh > 0) {
        if (vh == chainingIndex) {
          play(COIN);
        } else if (vh < chainingIndex) {
          int cg = (vh - 1) % 3 + 1;
          cc[0] = 'a' + cg - 1;
          character(cc, VEC_XY(p));
        } else {
          cc[0] = 'd' + g - 1;
          character(cc, VEC_XY(p));
        }
      } else {
        cc[0] = 'a' + g - 1;
        character(cc, VEC_XY(p));
      }
    }
  }
  if (messageTicks > 0) {
    messageTicks--;
    text(message, 3, 9);
  }
  if (fallingTicks > 0) {
    fallingTicks++;
    if ((int)fallingTicks % 5 == 0) {
      if (!fallBlocks()) {
        fallingTicks = 0;
        nextRowTicks = 0;
        calcGridHeight();
        checkGridSequence();
        setNextRow();
      }
    }
    return;
  }
  if (chainingTicks > 0) {
    chainingTicks++;
    if (chainingTicks > 50) {
      chainingTicks = 0;
      changeSequence();
      checkGridSequence();
      if (chainingTicks == 0 || multiplier >= 32) {
        chainingTicks = 0;
        clearSequence();
      }
    }
    return;
  }
  int bx = -1;
  bool hasPlace = false;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    int h = gridHeight[x];
    if (h < MARUSANSI_GRID_HEIGHT - 1) {
      hasPlace = true;
      Vector p;
      calcPixelPosition(x, h + 1, &p);
      bool isSelected = input.pos.x >= p.x - 3 && input.pos.x < p.x + 3 && input.pos.y >= p.y - 3 &&
                         input.pos.y < p.y + 9;
      TIMES(2, i) {
        if (!isSelected) {
          color = LIGHT_BLACK;
        }
        char cc[2];
        cc[0] = 'd' + blocks[i] - 1;
        cc[1] = 0;
        character(cc, p.x, p.y + i * 6);
        color = BLACK;
      }
      if (isSelected) {
        bx = x;
      }
    }
  }
  if (!hasPlace) {
    play(EXPLOSION);
    color = WHITE;
    rect(20, 0, 40, 6);
    rect(20, 30, 40, 20);
    color = BLACK;
    gameOver();
    return;
  }
  bool isNextRowSelected = input.pos.y >= MARUSANSI_GRID_HEIGHT * 6;
  color = LIGHT_PURPLE;
  nextRowTicks += sqrt(sqrt(difficulty));
  rect(20, 80, 40, -nextRowTicks / 50);
  color = BLACK;
  TIMES(MARUSANSI_GRID_WIDTH, x) {
    Vector p;
    calcPixelPosition(x, -1, &p);
    char cc[2];
    if (isNextRowSelected) {
      cc[0] = 'a' + nextRow[x] - 1;
    } else {
      cc[0] = 'd' + nextRow[x] - 1;
    }
    cc[1] = 0;
    character(cc, VEC_XY(p));
  }
  if (nextRowTicks > 400) {
    addNextRow();
    return;
  }
  if (input.isJustPressed) {
    if (isNextRowSelected) {
      addNextRow();
    } else if (bx >= 0) {
      play(SELECT);
      int y = gridHeight[bx];
      grid[bx][y + 1] = blocks[0];
      grid[bx][y] = blocks[1];
      multiplier = 1;
      setNextBlock();
      calcGridHeight();
      checkGridSequence();
    } else {
      play(CLICK);
      int tb = blocks[0];
      blocks[0] = blocks[1];
      blocks[1] = tb;
    }
  }
  startTicks--;
  if (startTicks > 0) {
    text("Tap\nto", 3, 40);
    text("rotate", 26, 30);
    text("place", 26, 66);
    text("add", 26, 76);
    text("blo\ncks", 65, 40);
  }
}

void addGameMarusansi() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
