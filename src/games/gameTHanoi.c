#include "cglp.h"
#include "string.h"

static char *title = "T HANOI";
static char *description = "[Tap]\n Take/Place disk\n[Hold]\n Shorten rod";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define THANOI_BAR_COUNT 3
#define THANOI_DISK_WIDTH 7
#define THANOI_DISK_HEIGHT 6
#define THANOI_MAX_DISK_COUNT 32

typedef struct {
  int disks[THANOI_MAX_DISK_COUNT];
  int diskCount;
  float height;
} Rod;
static Rod rods[THANOI_BAR_COUNT];
static float nextDiskTicks;
static int selectedRodIndex;
static int holdTicks;
static float multiplier;

static void unshift(Rod *r, int value) {
  for (int i = r->diskCount; i > 0; i--) {
    r->disks[i] = r->disks[i - 1];
  }
  r->disks[0] = value;
  r->diskCount++;
}

static void push(Rod *r, int value) {
  r->disks[r->diskCount] = value;
  r->diskCount++;
}

static int pop(Rod *r) {
  r->diskCount--;
  return r->disks[r->diskCount];
}

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped rod
  // is computed directly from input.pos via grid math (see the "i =
  // clamp(floor(input.pos.x / ...))" lines below), so the engine's own O(n^2)
  // hitbox scan is pure waste here. Restored automatically when the next
  // real game starts.
  hasCollision = false;
  if (!ticks) {
    TIMES(THANOI_BAR_COUNT, i) {
      rods[i].diskCount = 0;
      rods[i].height = 90;
    }
    nextDiskTicks = 0;
    selectedRodIndex = -1;
    holdTicks = 0;
    multiplier = 1;
  }
  if (rods[0].diskCount == 0) {
    nextDiskTicks = 0;
  }
  nextDiskTicks--;
  if (nextDiskTicks < 0) {
    play(LASER);
    unshift(&rods[0], rndi(1, 5));
    nextDiskTicks = (99 * sqrt(rods[0].diskCount)) / sqrt(difficulty);
  }
  if (input.isJustPressed) {
    int i = (int)clamp(floor(input.pos.x / (100.0 / THANOI_BAR_COUNT)), 0, THANOI_BAR_COUNT);
    if (selectedRodIndex < 0) {
      if (rods[i].diskCount > 0) {
        play(SELECT);
        selectedRodIndex = i;
      }
    } else {
      Rod *fb = &rods[selectedRodIndex];
      Rod *tb = &rods[i];
      int fp = fb->disks[fb->diskCount - 1];
      if (i == selectedRodIndex) {
        play(HIT);
      } else if (tb->diskCount == 0 || i == 0) {
        play(COIN);
        pop(fb);
        push(tb, fp);
      } else {
        int tp = tb->disks[tb->diskCount - 1];
        if (fp <= tp) {
          play(COIN);
          pop(fb);
          push(tb, fp);
        } else {
          play(HIT);
        }
      }
      selectedRodIndex = -1;
    }
  }
  if (input.isPressed) {
    holdTicks++;
    int i = (int)clamp(floor(input.pos.x / (100.0 / THANOI_BAR_COUNT)), 0, THANOI_BAR_COUNT);
    if (i > 0) {
      float h = (holdTicks * holdTicks) / 10000.0;
      rods[i].height -= h;
      if (h > 0.5) {
        play(JUMP);
      }
    }
  } else {
    holdTicks = 0;
  }
  color = LIGHT_BLACK;
  rect(0, 90, 100, 10);
  if (selectedRodIndex >= 0) {
    color = LIGHT_CYAN;
    rect((selectedRodIndex * 100.0) / THANOI_BAR_COUNT, 0, 100.0 / THANOI_BAR_COUNT, 90);
  }
  TIMES(THANOI_BAR_COUNT, i) {
    Rod *r = &rods[i];
    float x = ((i + 0.5) * 100.0) / THANOI_BAR_COUNT;
    r->height -= 0.01;
    if (r->height < r->diskCount * THANOI_DISK_HEIGHT) {
      if (i == 0) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(POWER_UP);
        int sc = r->diskCount * r->diskCount;
        addScore(sc * floor(multiplier), x, 90 - r->height);
        multiplier += sc / 100.0;
        r->diskCount = 0;
        rods[0].height = clamp(rods[0].height + sc / 4.0 / sqrt(difficulty), 0, 90);
        r->height = 90;
        if (selectedRodIndex == i) {
          selectedRodIndex = -1;
        }
        holdTicks = 0;
      }
    }
    if (i == 0) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    rect(x - 1, 90, 3, -r->height);
    float y = 90 - THANOI_DISK_HEIGHT / 2.0 + 1;
    TIMES(r->diskCount, j) {
      float w = r->disks[j] * THANOI_DISK_WIDTH;
      if (i == selectedRodIndex && j == r->diskCount - 1) {
        color = CYAN;
        box(x + 0.5, y, w, THANOI_DISK_HEIGHT - 1);
        color = RED;
        box(x + 0.5, y, w - 2, THANOI_DISK_HEIGHT - 3);
      } else {
        color = RED;
        box(x + 0.5, y, w, THANOI_DISK_HEIGHT - 1);
      }
      y -= THANOI_DISK_HEIGHT;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(multiplier)));
  text(multText, 3, 9);
}

void addGameTHanoi() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
