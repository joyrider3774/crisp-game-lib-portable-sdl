#include "cglp.h"

static char *title = "BLOCK STACKER";
static char *description = "[Tap] Release block";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector size;
  bool isPlaced;
  Vector velocity;
  bool isAlive;
} Block;

static float pendulumAngle;
static float pendulumSpeed;
static Block currentBlock;
#define BLOCKSTACKER_MAX_PLACED_COUNT 64
static Block placedBlocks[BLOCKSTACKER_MAX_PLACED_COUNT];
static int placedBlockIndex;
static float scrX;

static void makeNewBlock(Block *out, float difficultyValue) {
  scrX = rnd(0, difficultyValue - 1) * RNDPM();
  vectorSet(&out->pos, 0, 20);
  rotate(&out->pos, sin(pendulumAngle));
  vectorAdd(&out->pos, 50, 0);
  vectorSet(&out->size, rnd(10, 30), 10);
  out->isPlaced = false;
  vectorSet(&out->velocity, 0, 0);
  out->isAlive = true;
}

static void update() {
  if (!ticks) {
    pendulumAngle = 0;
    pendulumSpeed = 0.05;
    makeNewBlock(&currentBlock, difficulty);
    INIT_UNALIVED_ARRAY(placedBlocks);
    vectorSet(&placedBlocks[0].pos, 50, 99);
    vectorSet(&placedBlocks[0].size, 99, 10);
    placedBlocks[0].isAlive = true;
    placedBlockIndex = 1;
    scrX = 0;
  }
  pendulumAngle += pendulumSpeed * difficulty;
  color = BLACK;
  Vector pendulumEnd;
  vectorSet(&pendulumEnd, 0, 20);
  rotate(&pendulumEnd, sin(pendulumAngle));
  vectorAdd(&pendulumEnd, 50, 0);
  thickness = 3;
  line(50, 0, pendulumEnd.x, pendulumEnd.y);
  if (!currentBlock.isPlaced) {
    currentBlock.pos = pendulumEnd;
    vectorAdd(&currentBlock.pos, 0, 5);
    if (input.isJustPressed) {
      play(SELECT);
      currentBlock.isPlaced = true;
      vectorSet(&currentBlock.velocity, -cos(pendulumAngle), 0);
    }
  } else {
    Vector move;
    move.x = currentBlock.velocity.x * difficulty;
    move.y = currentBlock.velocity.y * difficulty;
    vectorAdd(&currentBlock.pos, move.x, move.y);
    currentBlock.velocity.y += 0.1;
    if (currentBlock.pos.y > 105) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = LIGHT_BLUE;
  box(currentBlock.pos.x, currentBlock.pos.y, currentBlock.size.x, currentBlock.size.y);
  float minY = 99;
  color = BLUE;
  bool isStacked = false;
  FOR_EACH(placedBlocks, i) {
    ASSIGN_ARRAY_ITEM(placedBlocks, i, Block, block);
    SKIP_IS_NOT_ALIVE(block);
    block->pos.x = wrap(block->pos.x + scrX, 0, 100);
    Collision s1 = box(block->pos.x - 100, block->pos.y, block->size.x, block->size.y);
    Collision s2 = box(block->pos.x, block->pos.y, block->size.x, block->size.y);
    Collision s3 = box(block->pos.x + 100, block->pos.y, block->size.x, block->size.y);
    bool c1 = s1.isColliding.rect[LIGHT_BLUE];
    bool c2 = s2.isColliding.rect[LIGHT_BLUE];
    bool c3 = s3.isColliding.rect[LIGHT_BLUE];
    if (!isStacked && (c1 || c2 || c3)) {
      currentBlock.pos.y = block->pos.y - currentBlock.size.y;
      play(HIT);
      ASSIGN_ARRAY_ITEM(placedBlocks, placedBlockIndex, Block, nb);
      nb->pos = currentBlock.pos;
      nb->size = currentBlock.size;
      nb->isAlive = true;
      placedBlockIndex = wrap(placedBlockIndex + 1, 0, BLOCKSTACKER_MAX_PLACED_COUNT);
      makeNewBlock(&currentBlock, difficulty);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      isStacked = true;
    }
    if (block->pos.y < minY) {
      minY = block->pos.y;
    }
  }
  float vy = difficulty * 0.01;
  if (minY < 70) {
    vy += (70 - minY) * 0.1;
  }
  FOR_EACH(placedBlocks, i) {
    ASSIGN_ARRAY_ITEM(placedBlocks, i, Block, block);
    SKIP_IS_NOT_ALIVE(block);
    block->pos.y += vy;
    if (block->pos.y > 106) {
      block->isAlive = false;
    }
  }
  COUNT_IS_ALIVE(placedBlocks, aliveCount);
  if (aliveCount == 0) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameBlockStacker() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
