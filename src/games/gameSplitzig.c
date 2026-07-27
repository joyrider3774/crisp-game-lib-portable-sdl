#include "cglp.h"

static char *title = "SPLITZIG";
static char *description = "[Tap] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector pos;
  float dir;
  float speed;
  float squash;
  float tilt;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} Bullet;
// Fire interval floor(7/sqrt(difficulty)) plateaus at 1 tick once difficulty
// is high enough; bullet lifetime is a fixed ~28 ticks (vy is a constant -3,
// not difficulty-scaled), so worst case concurrent count plateaus near 28.
#define MAX_BULLET_COUNT 64
static Bullet bullets[MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  float vx;
  float vy;
  int size;
  float squash;
  float rot;
  bool isAlive;
} Block;
// Sized with large headroom above the worst-case split-chain burst (2 children + 4 grandchildren).
#define MAX_BLOCK_COUNT 64
static Block blocks[MAX_BLOCK_COUNT];
static int blockIndex;

typedef struct {
  Vector pos;
  float life;
  bool isAlive;
} Trail;
// Up to ~28 bullets each spawning a trail every 2 ticks with a ~7 tick
// lifetime -> worst case around 90-100 concurrent; sized with headroom.
#define MAX_TRAIL_COUNT 200
static Trail trails[MAX_TRAIL_COUNT];
static int trailIndex;

static float stackHeight;
static float spawnTimer;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 80);
    player.dir = 1;
    player.speed = 1.5;
    player.squash = 1;
    player.tilt = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    INIT_UNALIVED_ARRAY(blocks);
    blockIndex = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    stackHeight = 0;
    spawnTimer = 0;
  }

  if (input.isJustPressed) {
    player.dir *= -1;
    player.squash = 0.5;
    play(SELECT);
    color = CYAN;
    particle(player.pos.x, player.pos.y, 4, 1.5, M_PI / 2, M_PI);
  }

  player.squash += (1 - player.squash) * 0.15;
  player.tilt += (player.dir * 0.3 - player.tilt) * 0.2;

  player.pos.x += player.dir * player.speed * sqrt(difficulty);
  player.pos.x = wrap(player.pos.x, 0, 100);

  stackHeight *= 0.998;
  player.pos.y = 90 - stackHeight;
  if (player.pos.y < 0) {
    play(EXPLOSION);
    gameOver();
  }

  int fireInterval = (int)floor(7 / sqrt(difficulty));
  if (fireInterval < 1) {
    fireInterval = 1;
  }
  if (ticks % fireInterval == 0) {
    ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
    vectorSet(&nb->pos, player.pos.x, player.pos.y - 5);
    nb->vy = -3;
    nb->isAlive = true;
    bulletIndex = wrap(bulletIndex + 1, 0, MAX_BULLET_COUNT);
    play(LASER);
  }

  spawnTimer++;
  float spawnRate = rnd(88, 111) / sqrt(difficulty);
  if (spawnTimer >= spawnRate) {
    spawnTimer = 0;
    ASSIGN_ARRAY_ITEM(blocks, blockIndex, Block, newBlock);
    vectorSet(&newBlock->pos, rnd(15, 85), -10);
    newBlock->vx = rnd(-0.3, 0.3) * sqrt(difficulty);
    newBlock->vy = rnd(0.2, 0.8) * sqrt(difficulty);
    newBlock->size = 2;
    newBlock->squash = 1;
    newBlock->rot = 0;
    newBlock->isAlive = true;
    blockIndex = wrap(blockIndex + 1, 0, MAX_BLOCK_COUNT);
  }

  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life -= 0.15;
    color = LIGHT_YELLOW;
    box(t->pos.x, t->pos.y, 2 * t->life, 3 * t->life);
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  color = GREEN;
  if (stackHeight > 0) {
    rect(0, 100 - stackHeight, 100, stackHeight);
  }

  color = CYAN;
  float pw = 6 * player.squash;
  float ph = 6 / player.squash;
  thickness = pw;
  barCenterPosRatio = 0.5;
  bar(player.pos.x, player.pos.y, ph, player.tilt);
  float eyeOffX = player.dir * 1.5;
  color = WHITE;
  box(player.pos.x - 1.5, player.pos.y - 1, 2, 2);
  box(player.pos.x + 1.5, player.pos.y - 1, 2, 2);
  color = BLACK;
  box(player.pos.x - 1.5 + eyeOffX * 0.3, player.pos.y - 1, 1, 1);
  box(player.pos.x + 1.5 + eyeOffX * 0.3, player.pos.y - 1, 1, 1);

  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    if (ticks % 2 == 0) {
      ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
      vectorSet(&nt->pos, b->pos.x, b->pos.y);
      nt->life = 1;
      nt->isAlive = true;
      trailIndex = wrap(trailIndex + 1, 0, MAX_TRAIL_COUNT);
    }
    b->pos.y += b->vy;
    color = YELLOW;
    box(b->pos.x, b->pos.y, 2, 4);
    if (b->pos.y < -5) {
      b->isAlive = false;
      continue;
    }
  }

  FOR_EACH(blocks, blki) {
    ASSIGN_ARRAY_ITEM(blocks, blki, Block, blk);
    SKIP_IS_NOT_ALIVE(blk);
    blk->pos.x += blk->vx;
    blk->pos.y += blk->vy;

    if ((blk->pos.x < 5 && blk->vx < 0) || (blk->pos.x > 95 && blk->vx > 0)) {
      blk->vx *= -1;
      blk->squash = 0.6;
      color = WHITE;
      float bounceAngle;
      if (blk->vx > 0) {
        bounceAngle = 0;
      } else {
        bounceAngle = M_PI;
      }
      particle(blk->pos.x, blk->pos.y, 3, 1, bounceAngle, M_PI / 2);
    }

    blk->squash += (1 - blk->squash) * 0.1;
    blk->rot += blk->vx * 0.05;

    int blockSize;
    int blockColor;
    int points;
    if (blk->size == 2) {
      blockSize = 12;
      blockColor = RED;
      points = 1;
    } else if (blk->size == 1) {
      blockSize = 8;
      blockColor = PURPLE;
      points = 2;
    } else {
      blockSize = 5;
      blockColor = BLUE;
      points = 3;
    }

    color = blockColor;
    float bw = blockSize * blk->squash;
    float bh = blockSize / blk->squash;
    thickness = bw;
    barCenterPosRatio = 0.5;
    Collision col = bar(blk->pos.x, blk->pos.y, bh, blk->rot);

    float dx = player.pos.x - blk->pos.x;
    float dy = player.pos.y - blk->pos.y;
    float eyeDir = atan2(dy, dx);
    float es = blockSize * 0.15;
    color = WHITE;
    box(blk->pos.x - es, blk->pos.y, es * 1.5, es * 1.5);
    box(blk->pos.x + es, blk->pos.y, es * 1.5, es * 1.5);
    color = BLACK;
    box(blk->pos.x - es + cos(eyeDir) * es * 0.4, blk->pos.y + sin(eyeDir) * es * 0.4, es * 0.8,
        es * 0.8);
    box(blk->pos.x + es + cos(eyeDir) * es * 0.4, blk->pos.y + sin(eyeDir) * es * 0.4, es * 0.8,
        es * 0.8);

    if (col.isColliding.rect[YELLOW]) {
      play(HIT);
      addScore(points, blk->pos.x, blk->pos.y);
      color = blockColor;
      particle(blk->pos.x, blk->pos.y, 8, 2, 0, M_PI * 2);

      if (blk->size > 0) {
        int newSize = blk->size - 1;
        ASSIGN_ARRAY_ITEM(blocks, blockIndex, Block, child1);
        vectorSet(&child1->pos, blk->pos.x - 5, blk->pos.y);
        child1->vx = -0.8 - rnd(0, 0.3);
        child1->vy = blk->vy * 0.8;
        child1->size = newSize;
        child1->squash = 0.6;
        child1->rot = blk->rot;
        child1->isAlive = true;
        blockIndex = wrap(blockIndex + 1, 0, MAX_BLOCK_COUNT);

        ASSIGN_ARRAY_ITEM(blocks, blockIndex, Block, child2);
        vectorSet(&child2->pos, blk->pos.x + 5, blk->pos.y);
        child2->vx = 0.8 + rnd(0, 0.3);
        child2->vy = blk->vy * 0.8;
        child2->size = newSize;
        child2->squash = 0.6;
        child2->rot = blk->rot;
        child2->isAlive = true;
        blockIndex = wrap(blockIndex + 1, 0, MAX_BLOCK_COUNT);
      } else {
        play(POWER_UP);
      }
      blk->isAlive = false;
      continue;
    }

    if (col.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
      blk->isAlive = false;
      continue;
    }

    float floorY = 100 - stackHeight;
    if (blk->pos.y > floorY - blockSize / 2.0) {
      stackHeight += blockSize / 2.0;
      play(HIT);
      color = blockColor;
      particle(blk->pos.x, blk->pos.y, 6, 1, -M_PI / 2, M_PI);
      blk->isAlive = false;
      continue;
    }
  }

  FOR_EACH(bullets, bi2) {
    ASSIGN_ARRAY_ITEM(bullets, bi2, Bullet, b2);
    SKIP_IS_NOT_ALIVE(b2);
    color = TRANSPARENT;
    Collision bcol = box(b2->pos.x, b2->pos.y, 2, 4);
    if (bcol.isColliding.rect[RED] || bcol.isColliding.rect[PURPLE] ||
        bcol.isColliding.rect[BLUE]) {
      b2->isAlive = false;
      continue;
    }
  }
}

void addGameSplitzig() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
