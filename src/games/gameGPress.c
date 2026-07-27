#include "cglp.h"

static char *title = "G PRESS";
static char *description = "[Tap] Press";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define GPRESS_PRESS_HEIGHT 20

typedef struct {
  float y;
  float vy;
  float width;
  float vw;
} Press;
static Press press;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
} Bubble;
#define GPRESS_MAX_BUBBLE_COUNT 32
static Bubble bubbles[GPRESS_MAX_BUBBLE_COUNT];
static int bubbleIndex;
static float nextBubbleTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Drop;
#define GPRESS_MAX_DROP_COUNT 64
static Drop drops[GPRESS_MAX_DROP_COUNT];
static int dropIndex;

static float dropTicks;
static float waterY;
static float prevWaterY;
static float targetWaterY;

static void update() {
  if (!ticks) {
    press.y = 30;
    press.vy = 1;
    press.width = 80;
    press.vw = 0;
    INIT_UNALIVED_ARRAY(bubbles);
    bubbleIndex = 0;
    nextBubbleTicks = 0;
    INIT_UNALIVED_ARRAY(drops);
    dropIndex = 0;
    dropTicks = 0;
    waterY = 70;
    prevWaterY = 70;
    targetWaterY = 70;
  }
  if (press.vw != 0) {
    press.width += press.vw * 5 * sqrt(difficulty);
    if (press.width < 0) {
      play(EXPLOSION);
      press.width = 0;
      press.vw = 0.5;
    }
    if (press.width > 80) {
      press.width = 80;
      press.vw = 0;
    }
  } else {
    press.y += press.vy * sqrt(difficulty);
    if ((press.y < GPRESS_PRESS_HEIGHT / 2 && press.vy < 0) ||
        (press.y > waterY - GPRESS_PRESS_HEIGHT / 2 && press.vy > 0)) {
      press.vy *= -1;
    }
    if (input.isJustPressed) {
      play(SELECT);
      prevWaterY = waterY + 12;
      targetWaterY = waterY + 12;
      press.vw = -1;
    }
  }
  if (press.vw < 0 || press.width == 0) {
    color = RED;
  } else {
    color = BLACK;
  }
  rect(50 - press.width / 2, press.y - GPRESS_PRESS_HEIGHT / 2, -5,
       GPRESS_PRESS_HEIGHT);
  rect(50 + press.width / 2, press.y - GPRESS_PRESS_HEIGHT / 2, 5,
       GPRESS_PRESS_HEIGHT);
  if (press.vw < 0 || press.width == 0) {
    color = PURPLE;
    rect(1, press.y, 1, 99);
    rect(1, press.y, 44 - press.width / 2, 1);
    rect(98, press.y, 1, 99);
    rect(98, press.y, -(44 - press.width / 2), 1);
  }
  color = BLACK;
  rect(0, press.y - 1, 1, 99);
  rect(2, press.y + 1, 1, 99);
  rect(0, press.y - 1, 45 - press.width / 2, 1);
  rect(2, press.y + 1, 43 - press.width / 2, 1);
  rect(99, press.y - 1, 1, 99);
  rect(97, press.y + 1, 1, 99);
  rect(99, press.y - 1, -(45 - press.width / 2), 1);
  rect(97, press.y + 1, -(43 - press.width / 2), 1);
  nextBubbleTicks--;
  if (nextBubbleTicks < 0) {
    float size = rnd(5, 9);
    ASSIGN_ARRAY_ITEM(bubbles, bubbleIndex, Bubble, b);
    vectorSet(&b->pos, rnd(20 + size, 80 - size), waterY + size / 2);
    vectorSet(&b->vel, 0, (-rnd(1, 2) / size) * difficulty);
    b->size = size;
    b->isAlive = true;
    bubbleIndex = wrap(bubbleIndex + 1, 0, GPRESS_MAX_BUBBLE_COUNT);
    targetWaterY += size * 0.03;
    nextBubbleTicks += rnd(10, 50) / difficulty;
  }
  color = PURPLE;
  FOR_EACH(bubbles, i) {
    ASSIGN_ARRAY_ITEM(bubbles, i, Bubble, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    Collision bc = box(b->pos.x, b->pos.y, b->size, b->size);
    if (bc.isColliding.rect[RED]) {
      if (b->size > press.width) {
        play(HIT);
        int dropCount = (int)ceil(b->size);
        TIMES(dropCount, k) {
          ASSIGN_ARRAY_ITEM(drops, dropIndex, Drop, d);
          vectorSet(&d->pos, b->pos.x + rnd(0, b->size / 2) * RNDPM(),
                    b->pos.y + rnd(0, b->size / 2) * RNDPM());
          vectorSet(&d->vel, 0, 0);
          d->isAlive = true;
          dropIndex = wrap(dropIndex + 1, 0, GPRESS_MAX_DROP_COUNT);
        }
        b->isAlive = false;
        continue;
      } else {
        if (b->pos.x < 50) {
          b->pos.x = 50 - press.width / 2 + b->size / 2;
        } else {
          b->pos.x = 50 + press.width / 2 - b->size / 2;
        }
      }
    }
    b->isAlive = b->pos.y >= -b->size / 2;
  }
  color = LIGHT_PURPLE;
  FOR_EACH(drops, i) {
    ASSIGN_ARRAY_ITEM(drops, i, Drop, d);
    SKIP_IS_NOT_ALIVE(d);
    d->vel.y += difficulty * 0.1;
    vectorAdd(&d->pos, d->vel.x, d->vel.y);
    box(d->pos.x, d->pos.y, 2, 2);
    if (d->pos.y > waterY + 1) {
      dropTicks = 9 / difficulty;
      play(LASER);
      targetWaterY -= 0.35;
      d->isAlive = false;
      continue;
    }
  }
  if (dropTicks > 0) {
    dropTicks--;
    if (dropTicks <= 0) {
      if (prevWaterY > targetWaterY) {
        play(COIN);
        float s = prevWaterY - targetWaterY;
        addScore(ceil(s * sqrt(s)), 50, waterY);
      }
    }
  }
  if (targetWaterY < 50) {
    targetWaterY += (50 - targetWaterY) * 0.05;
  }
  waterY += (targetWaterY - waterY) * 0.2;
  color = PURPLE;
  rect(0, waterY, 100, 101 - waterY);
  COUNT_IS_ALIVE(drops, aliveDropCount);
  if (press.vw == 0 && aliveDropCount == 0 && waterY >= 100 &&
      targetWaterY >= 100) {
    play(RANDOM);
    gameOver();
  }
}

void addGameGPress() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
