#include "cglp.h"

static char *title = "KNIGHT VS. PAWNS";
static char *description = "[Tap] Move knight";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        " llll ",
        "lllll ",
        "  lll ",
        "      ",
        " lllll",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "  ll  ",
        "  ll  ",
        "      ",
        " llll ",
        "  ll  ",
        "llllll",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define KNIGHTVSPAWNS_BOARD_SIZE 8
#define KNIGHTVSPAWNS_SQUARE_SIZE 10

typedef struct {
  Vector pos;
} KnightvspawnsKnight;
static KnightvspawnsKnight knight;

typedef struct {
  Vector pos;
  int moveIndex;
} KnightvspawnsGhostKnight;
static KnightvspawnsGhostKnight ghostKnight;

typedef struct {
  Vector pos;
  bool isAlive;
} KnightvspawnsPawn;
#define KNIGHTVSPAWNS_MAX_PAWN_COUNT 16
static KnightvspawnsPawn pawns[KNIGHTVSPAWNS_MAX_PAWN_COUNT];
static int pawnIndex;

static float moveInterval;
static float pawnInterval;
static float spawnInterval;

static Vector validMoves[8] = {
    {1, -2}, {2, -1}, {2, 1}, {1, 2}, {-1, 2}, {-2, 1}, {-2, -1}, {-1, -2},
};

static bool isValidMove(float x, float y) {
  return x >= 0 && x < KNIGHTVSPAWNS_BOARD_SIZE && y >= 0 && y < KNIGHTVSPAWNS_BOARD_SIZE;
}

static void moveGhostKnight() {
  Vector newPos;
  do {
    newPos.x = knight.pos.x + validMoves[ghostKnight.moveIndex].x;
    newPos.y = knight.pos.y + validMoves[ghostKnight.moveIndex].y;
    ghostKnight.moveIndex = (ghostKnight.moveIndex + 1) % 8;
  } while (!isValidMove(newPos.x, newPos.y));
  ghostKnight.pos = newPos;
}

static void update() {
  // Never reads a Collision result anywhere in this file - the knight,
  // ghost knight and pawns all move on an 8x8 grid and are checked via
  // direct x/y equality (see "p->pos.x == knight.pos.x" below), so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    vectorSet(&knight.pos, 3, 7);
    vectorSet(&ghostKnight.pos, 5, 6);
    ghostKnight.moveIndex = 0;
    INIT_UNALIVED_ARRAY(pawns);
    pawnIndex = 0;
    moveInterval = 0;
    pawnInterval = 0;
    spawnInterval = 0;
  }
  TIMES(KNIGHTVSPAWNS_BOARD_SIZE, i) {
    TIMES(KNIGHTVSPAWNS_BOARD_SIZE, j) {
      if ((i + j) % 2 == 0) {
        color = LIGHT_BLACK;
      } else {
        color = WHITE;
      }
      rect(i * KNIGHTVSPAWNS_SQUARE_SIZE + 10, j * KNIGHTVSPAWNS_SQUARE_SIZE + 10,
           KNIGHTVSPAWNS_SQUARE_SIZE, KNIGHTVSPAWNS_SQUARE_SIZE);
    }
  }
  moveInterval--;
  if (moveInterval <= 0) {
    moveGhostKnight();
    moveInterval = 30 / difficulty;
  }
  color = BLUE;
  character("b", ghostKnight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
            ghostKnight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15);
  if (input.isJustPressed) {
    play(LASER);
    knight.pos = ghostKnight.pos;
    moveGhostKnight();
    addScore(KNIGHTVSPAWNS_BOARD_SIZE - knight.pos.y, knight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
             knight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15);
  }
  color = BLUE;
  character("a", knight.pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15,
            knight.pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15);
  pawnInterval--;
  if (pawnInterval <= 0) {
    FOR_EACH(pawns, i) {
      ASSIGN_ARRAY_ITEM(pawns, i, KnightvspawnsPawn, p);
      SKIP_IS_NOT_ALIVE(p);
      p->pos.y++;
    }
    pawnInterval = 60 / difficulty;
  }
  spawnInterval--;
  if (spawnInterval <= 0) {
    ASSIGN_ARRAY_ITEM(pawns, pawnIndex, KnightvspawnsPawn, np);
    vectorSet(&np->pos, floor(rnd(0, KNIGHTVSPAWNS_BOARD_SIZE)), 0);
    np->isAlive = true;
    pawnIndex = wrap(pawnIndex + 1, 0, KNIGHTVSPAWNS_MAX_PAWN_COUNT);
    spawnInterval = 120 / difficulty;
  }
  color = RED;
  FOR_EACH(pawns, i) {
    ASSIGN_ARRAY_ITEM(pawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    character("c", p->pos.x * KNIGHTVSPAWNS_SQUARE_SIZE + 15, p->pos.y * KNIGHTVSPAWNS_SQUARE_SIZE + 15);
  }
  FOR_EACH(pawns, i) {
    ASSIGN_ARRAY_ITEM(pawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->pos.x == knight.pos.x && p->pos.y == knight.pos.y) {
      play(EXPLOSION);
      gameOver();
    }
  }
  FOR_EACH(pawns, i) {
    ASSIGN_ARRAY_ITEM(pawns, i, KnightvspawnsPawn, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->pos.y >= KNIGHTVSPAWNS_BOARD_SIZE) {
      p->isAlive = false;
    }
  }
}

void addGameKnightVsPawns() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
