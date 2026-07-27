#include "cglp.h"

static char *title = "FLIP O";
static char *description = "[Tap] Flip";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = true};

#define FLIPO_BALL_RADIUS 2
#define FLIPO_FLIPPER_LENGTH 12
#define FLIPO_BLOCK_SIZE_X 9
#define FLIPO_BLOCK_SIZE_Y 5
#define FLIPO_BLOCK_COUNT 8

typedef struct {
  Vector pos;
  Vector pp;
  Vector vel;
  float angle;
  int multiplier;
  bool isAlive;
} FlipoBall;
#define FLIPO_MAX_BALL_COUNT 32
static FlipoBall flipoBalls[FLIPO_MAX_BALL_COUNT];
static int flipoBallIndex;
static int flipoFlipCount;

typedef struct {
  Vector pos;
  bool hasBall;
  bool isAlive;
} FlipoBlock;
#define FLIPO_MAX_BLOCK_COUNT 128
static FlipoBlock flipoBlocks[FLIPO_MAX_BLOCK_COUNT];
static int flipoBlockIndex;
static float flipoNextBlockDist;

static void flipoReflect(FlipoBall *b, float a, int c, bool hasColor) {
  float oa = wrap(vectorAngle(&b->vel) - a - M_PI, -M_PI, M_PI);
  if (fabs(oa) < M_PI / 2) {
    addWithAngle(&b->vel, a, vectorLength(&b->vel) * cos(oa) * 1.7);
  }
  if (hasColor) {
    color = TRANSPARENT;
    TIMES(9, i) {
      addWithAngle(&b->pos, a, 1);
      Collision rc = arc(b->pos.x, b->pos.y, FLIPO_BALL_RADIUS, 0, M_PI * 2);
      if (!rc.isColliding.rect[c]) {
        break;
      }
    }
  }
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(flipoBalls);
    flipoBallIndex = 0;
    ASSIGN_ARRAY_ITEM(flipoBalls, flipoBallIndex, FlipoBall, b0);
    vectorSet(&b0->pos, 80, 10);
    vectorSet(&b0->pp, 80, 10);
    vectorSet(&b0->vel, 1, 0);
    b0->angle = rnd(0, M_PI * 2);
    b0->multiplier = 1;
    b0->isAlive = true;
    flipoBallIndex = wrap(flipoBallIndex + 1, 0, FLIPO_MAX_BALL_COUNT);
    flipoFlipCount = 0;
    INIT_UNALIVED_ARRAY(flipoBlocks);
    flipoBlockIndex = 0;
    flipoNextBlockDist = 0;
  }
  float maxBlockY = 0;
  FOR_EACH(flipoBlocks, i) {
    ASSIGN_ARRAY_ITEM(flipoBlocks, i, FlipoBlock, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->hasBall) {
      color = RED;
    } else {
      color = CYAN;
    }
    box(VEC_XY(b->pos), FLIPO_BLOCK_SIZE_X, FLIPO_BLOCK_SIZE_Y);
    if (b->pos.y > maxBlockY) {
      maxBlockY = b->pos.y;
    }
  }
  float scr;
  if (maxBlockY < 29) {
    scr = (30 - maxBlockY) * 0.1;
  } else {
    scr = sqrt(difficulty) * 0.02;
  }
  COUNT_IS_ALIVE(flipoBalls, aliveBallCountAtTap);
  if (input.isJustPressed) {
    play(LASER);
    scr += sqrt(difficulty) * 0.3 * aliveBallCountAtTap;
    flipoFlipCount = (flipoFlipCount + 1) % 2;
  }
  color = LIGHT_CYAN;
  rect(5, 0, 90, 5);
  color = LIGHT_BLUE;
  rect(0, 0, 5, 99);
  rect(95, 0, 5, 99);
  float f1a;
  if (flipoFlipCount == 0) {
    f1a = 0.5;
  } else {
    f1a = -0.5;
  }
  float f2a;
  if (flipoFlipCount == 0) {
    f2a = M_PI + 0.5;
  } else {
    f2a = M_PI - 0.5;
  }
  color = BLUE;
  thickness = 3;
  barCenterPosRatio = 0;
  Collision wc1 = bar(7, 75, 25, 0.5);
  Collision wc2 = bar(101 - 7, 75, 25, M_PI - 0.5);
  color = PURPLE;
  thickness = 3;
  barCenterPosRatio = 0;
  Collision wc3 = bar(50 - 17, 88, FLIPO_FLIPPER_LENGTH, f1a);
  Collision wc4 = bar(51 + 17, 88, FLIPO_FLIPPER_LENGTH, f2a);
  if (wc1.isColliding.rect[CYAN] || wc2.isColliding.rect[CYAN] || wc3.isColliding.rect[CYAN] ||
      wc4.isColliding.rect[CYAN] || wc1.isColliding.rect[RED] || wc2.isColliding.rect[RED] ||
      wc3.isColliding.rect[RED] || wc4.isColliding.rect[RED]) {
    color = RED;
    thickness = 3;
    barCenterPosRatio = 0;
    bar(7, 75, 25, 0.5);
    bar(101 - 7, 75, 25, M_PI - 0.5);
    play(EXPLOSION);
    gameOver();
  }
  if (input.isJustPressed) {
    thickness = 3;
    barCenterPosRatio = 0;
    if (flipoFlipCount == 0) {
      bar(51 + 17, 88, FLIPO_FLIPPER_LENGTH, M_PI);
    } else {
      bar(50 - 17, 88, FLIPO_FLIPPER_LENGTH, 0);
    }
  }
  COUNT_IS_ALIVE(flipoBalls, aliveBallCountForScore);
  FOR_EACH(flipoBalls, i) {
    ASSIGN_ARRAY_ITEM(flipoBalls, i, FlipoBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pp = b->pos;
    b->pp.y += scr;
    b->vel.y += 0.1;
    vectorMul(&b->vel, 0.99);
    vectorAdd(&b->pos, b->vel.x * sqrt(difficulty) * 0.5, b->vel.y * sqrt(difficulty) * 0.5);
    b->pos.y += scr;
    b->angle += b->vel.x * 0.03 + b->vel.y * 0.02;
    color = BLACK;
    Collision bc = arc(b->pos.x, b->pos.y, FLIPO_BALL_RADIUS, b->angle, b->angle + M_PI * 2);
    if (bc.isColliding.rect[RED] || bc.isColliding.rect[CYAN]) {
      addScore(b->multiplier * aliveBallCountForScore, b->pos.x, b->pos.y);
      b->multiplier++;
      color = TRANSPARENT;
      Collision cx = arc(b->pp.x, b->pos.y, FLIPO_BALL_RADIUS, 0, M_PI * 2);
      Collision cy = arc(b->pos.x, b->pp.y, FLIPO_BALL_RADIUS, 0, M_PI * 2);
      if (!(cx.isColliding.rect[RED] || cx.isColliding.rect[CYAN])) {
        float a;
        if (b->vel.x > 0) {
          a = -M_PI;
        } else {
          a = 0;
        }
        flipoReflect(b, a, 0, false);
      }
      if (!(cy.isColliding.rect[RED] || cy.isColliding.rect[CYAN])) {
        float a;
        if (b->vel.y > 0) {
          a = -M_PI / 2;
        } else {
          a = M_PI / 2;
        }
        flipoReflect(b, a, 0, false);
      }
    }
    if (bc.isColliding.rect[LIGHT_CYAN]) {
      play(HIT);
      flipoReflect(b, M_PI / 2, LIGHT_CYAN, true);
    }
    if (bc.isColliding.rect[LIGHT_BLUE]) {
      play(HIT);
      float a;
      if (b->pos.x < 50) {
        a = 0;
      } else {
        a = M_PI;
      }
      flipoReflect(b, a, LIGHT_BLUE, true);
    }
    if (bc.isColliding.rect[BLUE]) {
      float a;
      if (b->pos.x < 50) {
        a = 0.5 - M_PI / 2;
      } else {
        a = M_PI - 0.5 + M_PI / 2;
      }
      flipoReflect(b, a, BLUE, true);
    }
    if (bc.isColliding.rect[PURPLE]) {
      if (input.isJustPressed) {
        play(JUMP);
        Vector pp;
        vectorSet(&pp, b->pos.x, b->pos.y);
        float pf1a;
        if (flipoFlipCount == 1) {
          pf1a = 0.5;
        } else {
          pf1a = -0.5;
        }
        float pf2a;
        if (flipoFlipCount == 1) {
          pf2a = M_PI + 0.5;
        } else {
          pf2a = M_PI - 0.5;
        }
        float a1;
        if (b->pos.x < 50) {
          a1 = pf1a - M_PI / 2;
        } else {
          a1 = pf2a + M_PI / 2;
        }
        flipoReflect(b, a1, PURPLE, true);
        flipoReflect(b, -M_PI / 2, PURPLE, true);
        float a2;
        if (b->pos.x < 50) {
          a2 = f1a - M_PI / 2;
        } else {
          a2 = f2a + M_PI / 2;
        }
        flipoReflect(b, a2, PURPLE, true);
        vectorAdd(&b->vel, b->pos.x - pp.x, b->pos.y - pp.y);
        b->multiplier = 1;
      } else {
        float a3;
        if (b->pos.x < 50) {
          a3 = f1a - M_PI / 2;
        } else {
          a3 = f2a + M_PI / 2;
        }
        flipoReflect(b, a3, PURPLE, true);
      }
    }
    if (b->pos.y > 99 + FLIPO_BALL_RADIUS) {
      play(SELECT);
      b->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(flipoBalls, aliveBallCountAfterRemove);
  if (aliveBallCountAfterRemove == 0) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(flipoBalls, i) {
    ASSIGN_ARRAY_ITEM(flipoBalls, i, FlipoBall, b);
    SKIP_IS_NOT_ALIVE(b);
    FOR_EACH(flipoBalls, j) {
      ASSIGN_ARRAY_ITEM(flipoBalls, j, FlipoBall, ab);
      SKIP_IS_NOT_ALIVE(ab);
      if (i == j || distanceTo(&ab->pos, b->pos.x, b->pos.y) > FLIPO_BALL_RADIUS * 2) {
        continue;
      }
      flipoReflect(b, angleTo(&ab->pos, b->pos.x, b->pos.y), 0, false);
    }
  }
  color = TRANSPARENT;
  FOR_EACH(flipoBlocks, i) {
    ASSIGN_ARRAY_ITEM(flipoBlocks, i, FlipoBlock, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += scr;
    Collision blkc = box(VEC_XY(b->pos), FLIPO_BLOCK_SIZE_X, FLIPO_BLOCK_SIZE_Y);
    if (blkc.isColliding.rect[BLACK]) {
      if (b->hasBall) {
        play(POWER_UP);
        ASSIGN_ARRAY_ITEM(flipoBalls, flipoBallIndex, FlipoBall, nb);
        nb->pos = b->pos;
        nb->pp = b->pos;
        vectorSet(&nb->vel, 1, 0);
        rotate(&nb->vel, M_PI * 2);
        nb->angle = rnd(0, M_PI * 2);
        nb->multiplier = 1;
        nb->isAlive = true;
        flipoBallIndex = wrap(flipoBallIndex + 1, 0, FLIPO_MAX_BALL_COUNT);
      } else {
        play(COIN);
      }
      b->isAlive = false;
      continue;
    }
  }
  flipoNextBlockDist -= scr;
  while (flipoNextBlockDist < 0) {
    float x = (FLIPO_BLOCK_SIZE_X + 1) / 2.0;
    float y = -flipoNextBlockDist;
    COUNT_IS_ALIVE(flipoBalls, aliveBallCountForSpawn);
    float br = 0.1 / aliveBallCountForSpawn;
    TIMES(FLIPO_BLOCK_COUNT / 2, i) {
      if (rnd(0, 1) < 0.5) {
        ASSIGN_ARRAY_ITEM(flipoBlocks, flipoBlockIndex, FlipoBlock, b1);
        vectorSet(&b1->pos, 50 - x, y);
        b1->hasBall = rnd(0, 1) < br;
        b1->isAlive = true;
        flipoBlockIndex = wrap(flipoBlockIndex + 1, 0, FLIPO_MAX_BLOCK_COUNT);
        ASSIGN_ARRAY_ITEM(flipoBlocks, flipoBlockIndex, FlipoBlock, b2);
        vectorSet(&b2->pos, 50 + x, y);
        b2->hasBall = rnd(0, 1) < br;
        b2->isAlive = true;
        flipoBlockIndex = wrap(flipoBlockIndex + 1, 0, FLIPO_MAX_BLOCK_COUNT);
      }
      x += FLIPO_BLOCK_SIZE_X + 1;
    }
    flipoNextBlockDist += FLIPO_BLOCK_SIZE_Y + 1;
  }
}

void addGameFlipO() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
