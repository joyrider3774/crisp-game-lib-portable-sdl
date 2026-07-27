#include "cglp.h"

static char *title = "BLADE DANCER";
static char *description = "[Tap] Jump & Slash";

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
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define BLADEDANCER_GROUND_Y 80
#define BLADEDANCER_JUMP_DURATION 30
#define BLADEDANCER_JUMP_HEIGHT 30
#define BLADEDANCER_SCROLL_SPEED 1
#define BLADEDANCER_SWORD_ROTATION_SPEED 0.3
#define BLADEDANCER_MAX_SWORD_LENGTH 30

typedef struct {
  Vector pos;
  bool isJumping;
  float jumpTicks;
  float swordAngle;
  float swordLength;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float size;
  bool isAlive;
} Enemy;
#define BLADEDANCER_MAX_ENEMY_COUNT 32
static Enemy enemies[BLADEDANCER_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  float x;
  float width;
} Segment;
#define BLADEDANCER_MAX_SEGMENT_COUNT 16
static Segment segments[BLADEDANCER_MAX_SEGMENT_COUNT];
static int segmentHead;
static int segmentCount;

static float enemySpawnChance;
static float gapFrequency;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 35, BLADEDANCER_GROUND_Y);
    player.isJumping = false;
    player.jumpTicks = 0;
    player.swordAngle = M_PI_2;
    player.swordLength = 10;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    segments[0].x = 0;
    segments[0].width = 200;
    segmentHead = 0;
    segmentCount = 1;
    enemySpawnChance = 0.03;
    gapFrequency = 0.3;
    multiplier = 1;
  }
  if (input.isJustPressed && !player.isJumping) {
    player.isJumping = true;
    player.jumpTicks = BLADEDANCER_JUMP_DURATION;
    play(POWER_UP);
    multiplier = 1;
  }
  if (player.isJumping) {
    if (input.isPressed) {
      player.jumpTicks -= 0.5;
    } else {
      player.jumpTicks -= 1;
    }
    float jumpProgress = player.jumpTicks / BLADEDANCER_JUMP_DURATION;
    player.pos.y =
        BLADEDANCER_GROUND_Y - sin(jumpProgress * M_PI) * BLADEDANCER_JUMP_HEIGHT;
    player.swordAngle -= BLADEDANCER_SWORD_ROTATION_SPEED;
    player.swordLength = 8 + (1 - jumpProgress) * (BLADEDANCER_MAX_SWORD_LENGTH - 8);
    if (player.jumpTicks <= 0) {
      play(CLICK);
      player.isJumping = false;
      player.pos.y = BLADEDANCER_GROUND_Y;
      player.swordAngle = M_PI_2;
      player.swordLength = 10;
    }
  }
  color = BLUE;
  box(player.pos.x, player.pos.y, 4, 8);
  color = CYAN;
  Vector swordTip;
  vectorSet(&swordTip, player.swordLength, 0);
  rotate(&swordTip, player.swordAngle);
  if (player.isJumping) {
    thickness = 5;
  } else {
    thickness = 2;
  }
  line(player.pos.x, player.pos.y - 4, player.pos.x + swordTip.x,
       player.pos.y - 4 + swordTip.y);
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (aliveEnemyCount == 0 || rnd(0, 1) < enemySpawnChance) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, 205, rnd(30, BLADEDANCER_GROUND_Y - 5));
    ne->size = rnd(3, 6);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, BLADEDANCER_MAX_ENEMY_COUNT);
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x -= BLADEDANCER_SCROLL_SPEED * sqrt(difficulty);
    color = RED;
    Collision c1 = box(e->pos.x, e->pos.y, e->size, e->size);
    bool isCollidingWithSword = c1.isColliding.rect[CYAN];
    if (isCollidingWithSword) {
      play(HIT);
      addScore(multiplier, e->pos.x, e->pos.y);
      multiplier++;
      e->isAlive = false;
      continue;
    }
    Collision c2 = box(e->pos.x, e->pos.y, e->size, e->size);
    if (c2.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    if (e->pos.x < -10) {
      e->isAlive = false;
      continue;
    }
  }
  color = GREEN;
  TIMES(segmentCount, k) {
    int idx = (segmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    Segment *segment = &segments[idx];
    segment->x -= BLADEDANCER_SCROLL_SPEED * sqrt(difficulty);
    rect(segment->x, BLADEDANCER_GROUND_Y, segment->width - 2, 20);
    if (segment->x + segment->width < 200 && k == segmentCount - 1) {
      int newIdx = (segmentHead + segmentCount) % BLADEDANCER_MAX_SEGMENT_COUNT;
      if (rnd(0, 1) < gapFrequency) {
        float gapWidth = rnd(15, 30);
        segments[newIdx].x = segment->x + segment->width + gapWidth;
        segments[newIdx].width = rnd(30, 100);
      } else {
        segments[newIdx].x = segment->x + segment->width;
        segments[newIdx].width = rnd(50, 150);
      }
      segmentCount++;
    }
  }
  while (segmentCount > 0) {
    Segment *segment = &segments[segmentHead];
    if (segment->x + segment->width < 0) {
      segmentHead = (segmentHead + 1) % BLADEDANCER_MAX_SEGMENT_COUNT;
      segmentCount--;
    } else {
      break;
    }
  }
  TIMES(segmentCount, k) {
    int idx = (segmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    Segment *segment = &segments[idx];
    if (!player.isJumping && player.pos.x > segment->x &&
        player.pos.x < segment->x + segment->width) {
      player.pos.y = BLADEDANCER_GROUND_Y;
    }
  }
  bool isOverGap = true;
  TIMES(segmentCount, k) {
    int idx = (segmentHead + k) % BLADEDANCER_MAX_SEGMENT_COUNT;
    Segment *segment = &segments[idx];
    if (player.pos.x >= segment->x && player.pos.x <= segment->x + segment->width) {
      isOverGap = false;
    }
  }
  if (isOverGap && !player.isJumping) {
    play(EXPLOSION);
    gameOver();
  }
  enemySpawnChance = clamp(0.03 + difficulty * 0.01, 0.02, 0.1);
  gapFrequency = clamp(0.3 + difficulty * 0.02, 0.3, 0.7);
}

void addGameBladeDancer() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
