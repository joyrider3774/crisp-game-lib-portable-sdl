#include "cglp.h"

static char *title = "MAKE MAZE";
static char *description = "[Tap][Slide]\n Add/Remove wall";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llll l",
        "llll l",
        "      ",
        "ll lll",
        "ll lll",
        "      ",
    },
    {
        "  ll  ",
        "  lll ",
        "llllll",
        "llllll",
        "  lll ",
        "  ll  ",
    },
    {
        "  ll  ",
        " l ll ",
        "l llll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 30, .isDarkColor = false};

#define MAKEMAZE_WALL_SIZE_X 16
#define MAKEMAZE_WALL_SIZE_Y 18

static bool walls[MAKEMAZE_WALL_SIZE_X][MAKEMAZE_WALL_SIZE_Y];
static Vector wallOfs;
static bool wallFills[MAKEMAZE_WALL_SIZE_X][MAKEMAZE_WALL_SIZE_Y];
static Vector pwp;
static Vector angleOfs[8];

typedef struct {
  Vector pos;
  int angle;
  int angleVel;
  Vector scPos;
  float moveInterval;
  float ticks;
  bool isAngry;
  bool isAlive;
} MakemazeEnemy;
#define MAKEMAZE_MAX_ENEMY_COUNT 32
static MakemazeEnemy enemies[MAKEMAZE_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  bool isAlive;
} MakemazeGold;
#define MAKEMAZE_MAX_GOLD_COUNT 64
static MakemazeGold golds[MAKEMAZE_MAX_GOLD_COUNT];
static int goldIndex;
static float goldMinY;
static float missScr;
static int multiplier;

static bool getWall(Vector *cp, int ta) {
  int wrappedTa = (int)wrap(ta, 0, 8);
  float px = cp->x + angleOfs[wrappedTa].x;
  float py = cp->y + angleOfs[wrappedTa].y;
  if (py < 0 || py >= MAKEMAZE_WALL_SIZE_Y) {
    return false;
  }
  return walls[(int)px][(int)py];
}

static void removeGold(Vector *p) {
  color = YELLOW;
  FOR_EACH(golds, i) {
    ASSIGN_ARRAY_ITEM(golds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->pos.x == p->x && g->pos.y == p->y) {
      play(COIN);
      float gx = wallOfs.x + g->pos.x * 6;
      float gy = wallOfs.y + g->pos.y * 6;
      addScore(multiplier, gx, gy);
      particle(gx, gy, 16, 1, 0, M_PI * 2);
      multiplier++;
      g->isAlive = false;
    }
  }
}

static bool checkGold(Vector *p) {
  bool exists = false;
  FOR_EACH(golds, i) {
    ASSIGN_ARRAY_ITEM(golds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    if (g->pos.x == p->x && g->pos.y == p->y) {
      exists = true;
    }
  }
  return exists;
}

static bool checkDownExit(Vector *p) {
  for (int x = 1; x < MAKEMAZE_WALL_SIZE_X - 1; x++) {
    for (int y = 0; y < MAKEMAZE_WALL_SIZE_Y; y++) {
      wallFills[x][y] = false;
    }
  }
  wallFills[(int)p->x][(int)p->y] = true;
  TIMES(9, iter) {
    for (int x = 1; x < MAKEMAZE_WALL_SIZE_X - 1; x++) {
      for (int y = 1; y < MAKEMAZE_WALL_SIZE_Y; y++) {
        if (!walls[x][y] && !wallFills[x][y] &&
            (wallFills[x - 1][y] || wallFills[x][y - 1])) {
          if (y == MAKEMAZE_WALL_SIZE_Y - 1) {
            return true;
          }
          wallFills[x][y] = true;
        }
      }
    }
    for (int x = MAKEMAZE_WALL_SIZE_X - 2; x > 0; x--) {
      for (int y = MAKEMAZE_WALL_SIZE_Y - 2; y >= 0; y--) {
        if (!walls[x][y] && !wallFills[x][y] &&
            (wallFills[x + 1][y] || wallFills[x][y + 1])) {
          wallFills[x][y] = true;
        }
      }
    }
  }
  return false;
}

static void update() {
  // Never reads a Collision result anywhere in this file (including the
  // helpers above) - every wall/gold/enemy check is direct grid indexing
  // via walls[][]/golds, so the engine's own O(n^2) hitbox scan (see
  // checkHitBox() in cglp.c) is pure waste here. Restored automatically
  // when the next real game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&angleOfs[0], 1, 0);
    vectorSet(&angleOfs[1], 1, 1);
    vectorSet(&angleOfs[2], 0, 1);
    vectorSet(&angleOfs[3], -1, 1);
    vectorSet(&angleOfs[4], -1, 0);
    vectorSet(&angleOfs[5], -1, -1);
    vectorSet(&angleOfs[6], 0, -1);
    vectorSet(&angleOfs[7], 1, -1);
    TIMES(MAKEMAZE_WALL_SIZE_X, x) {
      TIMES(MAKEMAZE_WALL_SIZE_Y, y) {
        walls[x][y] = x == 0 || x == 15 || (y == 10 && (x == 1 || x == 14)) ||
                      (y == 12 && x > 1 && x < 14) || (y == 14 && (x < 7 || x > 8));
      }
    }
    vectorSet(&wallOfs, 5, -2);
    TIMES(MAKEMAZE_WALL_SIZE_X, x) {
      TIMES(MAKEMAZE_WALL_SIZE_Y, y) { wallFills[x][y] = false; }
    }
    vectorSet(&pwp, 0, 0);
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(golds);
    goldIndex = 0;
    goldMinY = 99;
    missScr = 0;
    multiplier = 1;
  }
  float scr = 0.01 * sqrt(difficulty) + missScr;
  missScr *= 0.9;
  float gy = goldMinY * 6 + wallOfs.y;
  if (gy > 50) {
    scr += (gy - 50) * 0.02 * sqrt(difficulty);
  }
  wallOfs.y -= scr;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, MakemazeEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->scPos.y -= scr;
  }
  if (wallOfs.y < -2) {
    for (int y = 1; y < MAKEMAZE_WALL_SIZE_Y; y++) {
      for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
        walls[x][y - 1] = walls[x][y];
      }
    }
    for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
      walls[x][MAKEMAZE_WALL_SIZE_Y - 1] = x == 0 || x == 15;
    }
    ASSIGN_ARRAY_ITEM(golds, goldIndex, MakemazeGold, ng);
    vectorSet(&ng->pos, rndi(1, MAKEMAZE_WALL_SIZE_X - 1), MAKEMAZE_WALL_SIZE_Y - 1);
    ng->isAlive = true;
    goldIndex = wrap(goldIndex + 1, 0, MAKEMAZE_MAX_GOLD_COUNT);
    wallOfs.y += 6;
    FOR_EACH(enemies, i) {
      ASSIGN_ARRAY_ITEM(enemies, i, MakemazeEnemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->pos.y--;
    }
    FOR_EACH(golds, i) {
      ASSIGN_ARRAY_ITEM(golds, i, MakemazeGold, g);
      SKIP_IS_NOT_ALIVE(g);
      g->pos.y--;
    }
    pwp.y--;
  }
  if (input.isPressed) {
    Vector wp;
    vectorSet(&wp, floor((input.pos.x - (wallOfs.x - 3)) / 6),
              floor((input.pos.y - (wallOfs.y - 3)) / 6));
    if (wp.x > 0 && wp.x < MAKEMAZE_WALL_SIZE_X - 1 && wp.y >= 0 && wp.y < MAKEMAZE_WALL_SIZE_Y &&
        !(wp.x == pwp.x && wp.y == pwp.y) && !checkGold(&wp)) {
      play(SELECT);
      walls[(int)wp.x][(int)wp.y] = !walls[(int)wp.x][(int)wp.y];
      pwp = wp;
    }
  } else {
    vectorSet(&pwp, 0, 0);
  }
  color = LIGHT_PURPLE;
  for (int y = 0; y < MAKEMAZE_WALL_SIZE_Y; y++) {
    for (int x = 0; x < MAKEMAZE_WALL_SIZE_X; x++) {
      if (walls[x][y]) {
        character("a", wallOfs.x + x * 6, wallOfs.y + y * 6);
      }
    }
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    int x = 1;
    TIMES(99, i) {
      x = rndi(1, MAKEMAZE_WALL_SIZE_X - 1);
      if (!walls[x][0]) {
        break;
      }
    }
    Vector pos;
    vectorSet(&pos, x, 0);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, MakemazeEnemy, ne);
    ne->pos = pos;
    ne->angle = 2;
    ne->angleVel = rndi(0, 2) * 2 - 1;
    vectorSet(&ne->scPos, wallOfs.x + pos.x * 6, wallOfs.y + pos.y * 6);
    ne->moveInterval = ceil(60 / sqrt(difficulty));
    ne->ticks = 0;
    ne->isAngry = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MAKEMAZE_MAX_ENEMY_COUNT);
    nextEnemyTicks = 150 / sqrt(difficulty);
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, MakemazeEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->ticks--;
    if (e->ticks < 0) {
      Vector pp;
      vectorSet(&pp, e->pos.x, e->pos.y);
      removeGold(&e->pos);
      bool isMoving = false;
      if (!checkDownExit(&e->pos)) {
        e->isAngry = true;
        e->angle = 2;
        e->pos.y++;
        removeGold(&e->pos);
        if (walls[(int)e->pos.x][(int)e->pos.y]) {
          play(POWER_UP);
          walls[(int)e->pos.x][(int)e->pos.y] = false;
        }
      } else {
        e->isAngry = false;
        TIMES(99, k) {
          if (getWall(&e->pos, e->angle) ||
              getWall(&e->pos, e->angle + e->angleVel * 2) ||
              getWall(&e->pos, e->angle + e->angleVel * 3) ||
              getWall(&e->pos, e->angle + e->angleVel * 4) || e->pos.y < 0 ||
              e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
            break;
          }
          vectorAdd(&e->pos, angleOfs[e->angle].x, angleOfs[e->angle].y);
          removeGold(&e->pos);
          isMoving = true;
        }
        if (e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
          isMoving = true;
        }
        if (!isMoving) {
          TIMES(99, k) {
            int a = (int)wrap(e->angle + e->angleVel * 2, 0, 8);
            TIMES(4, j) {
              if (!getWall(&e->pos, a)) {
                if (k > 0 && a != e->angle) {
                  break;
                }
                vectorAdd(&e->pos, angleOfs[a].x, angleOfs[a].y);
                removeGold(&e->pos);
                e->angle = a;
                break;
              }
              a = (int)wrap(a - e->angleVel * 2, 0, 8);
            }
            if (a != e->angle || e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
              break;
            }
          }
        }
      }
      if (distanceTo(&pp, VEC_XY(e->pos)) > 1) {
        play(HIT);
      }
      e->ticks = e->moveInterval;
    }
    Vector target;
    vectorSet(&target, wallOfs.x + e->pos.x * 6, wallOfs.y + e->pos.y * 6);
    e->scPos.x += (target.x - e->scPos.x) * 0.1;
    e->scPos.y += (target.y - e->scPos.y) * 0.1;
    if (e->isAngry) {
      color = RED;
    } else if (e->angleVel < 0) {
      color = BLUE;
    } else {
      color = PURPLE;
    }
    characterOptions.rotation = (int)wrap(e->angle / 2, 0, 4);
    character("b", VEC_XY(e->scPos));
    characterOptions.rotation = 0;
    if (e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
      play(EXPLOSION);
      missScr++;
      if (multiplier > 1) {
        multiplier--;
      }
      addScore(-multiplier, e->scPos.x, 99);
      particle(e->scPos.x, 110, 19, 2, -M_PI / 2, -M_PI / 8);
    }
    if (e->pos.y < 0 || e->pos.y >= MAKEMAZE_WALL_SIZE_Y) {
      e->isAlive = false;
      continue;
    }
  }
  goldMinY = 99;
  FOR_EACH(golds, i) {
    ASSIGN_ARRAY_ITEM(golds, i, MakemazeGold, g);
    SKIP_IS_NOT_ALIVE(g);
    walls[(int)g->pos.x][(int)g->pos.y] = false;
    float x = wallOfs.x + g->pos.x * 6;
    float y = wallOfs.y + g->pos.y * 6;
    if (y < 30) {
      color = RED;
    } else {
      color = YELLOW;
    }
    character("c", x, y);
    if (g->pos.y < goldMinY) {
      goldMinY = g->pos.y;
    }
    if (y < 0) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      text("X", x, 3);
      gameOver();
    }
  }
}

void addGameMakeMaze() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
