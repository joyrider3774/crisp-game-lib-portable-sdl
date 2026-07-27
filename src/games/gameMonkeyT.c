#include "cglp.h"
#include "string.h"

static char *title = "MONKEY T";
static char *description = "[Hold]\n  Compress\n[Release]\n  Launch";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "l  ll ",
        "l l  l",
        "l  ll ",
        " lll l",
        "lll  l",
        "      ",
    },
    {
        "  lll ",
        "   l  ",
        "  l l ",
        "  l l ",
        " l l l",
        "l l l ",
    },
    {
        "llll  ",
        " llll ",
        "  llll",
        " lll  ",
        "ll  l ",
        "      ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define MONKEYT_MAX_COLLECTIBLE_COUNT 8

typedef struct {
  Vector pos;
  bool isAlive;
} MonkeytCollectible;
static MonkeytCollectible collectibles[MONKEYT_MAX_COLLECTIBLE_COUNT];

typedef struct {
  Vector pos;
  float speed;
} MonkeytHazard;
static MonkeytHazard hazards[3];

static Vector monkeyPosition;
static Vector monkeyVelocity;
static int monkeyCollectingCount;
static float trampolineCompression;
static float trampolineLaunchPower;
static int multiplier;

static void addCollectables() {
  TIMES(5, i) {
    vectorSet(&collectibles[i].pos, rnd(30, 70), rnd(20, 70));
    collectibles[i].isAlive = true;
  }
  for (int i = 5; i < MONKEYT_MAX_COLLECTIBLE_COUNT; i++) {
    collectibles[i].isAlive = false;
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&monkeyPosition, 50, 90);
    vectorSet(&monkeyVelocity, 0.3, 0);
    monkeyCollectingCount = 0;
    trampolineCompression = 0;
    trampolineLaunchPower = 0;
    addCollectables();
    TIMES(3, i) {
      vectorSet(&hazards[i].pos, rnd(10, 90), rnd(10, 70));
      hazards[i].speed = 0.5 + i * 0.1;
    }
    multiplier = 1;
  }
  if (input.isJustPressed) {
    trampolineLaunchPower = 0;
    trampolineCompression = 0;
  }
  if (input.isPressed) {
    float c2 = trampolineCompression + 1;
    if (c2 > 30) {
      c2 = 30;
    }
    trampolineCompression = c2;
  }
  if (input.isJustReleased) {
    trampolineLaunchPower = trampolineCompression;
    trampolineCompression = 0;
    monkeyCollectingCount = 0;
  }
  float trampolineHeight =
      90 + trampolineCompression * 0.25 + trampolineLaunchPower * 0.25;
  color = BLACK;
  thickness = 2;
  line(25, trampolineHeight, 75, trampolineHeight);
  monkeyVelocity.y += 0.1;
  vectorAdd(&monkeyPosition, monkeyVelocity.x * difficulty, monkeyVelocity.y);
  if ((monkeyPosition.x < 25 && monkeyVelocity.x < 0) ||
      (monkeyPosition.x > 75 && monkeyVelocity.x > 0)) {
    monkeyVelocity.x = -monkeyVelocity.x;
  }
  color = RED;
  if (monkeyVelocity.x > 0) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  Collision scratch = character("a", monkeyPosition.x, monkeyPosition.y);
  if (scratch.isColliding.rect[BLACK] && monkeyVelocity.y > 0) {
    monkeyPosition.y = trampolineHeight - 5;
    monkeyVelocity.y = -monkeyVelocity.y * 0.2 - trampolineLaunchPower * 0.13;
    if (trampolineLaunchPower > 1) {
      play(JUMP);
      trampolineLaunchPower = 0;
    }
  }
  if (monkeyPosition.y > 99) {
    monkeyPosition.y = 80;
  }
  color = YELLOW;
  FOR_EACH(collectibles, i) {
    ASSIGN_ARRAY_ITEM(collectibles, i, MonkeytCollectible, c);
    SKIP_IS_NOT_ALIVE(c);
    Collision cCol = character("b", c->pos.x, c->pos.y);
    if (cCol.isColliding.character['a']) {
      play(COIN);
      multiplier += monkeyCollectingCount;
      monkeyCollectingCount++;
      addScore(multiplier, monkeyPosition.x, monkeyPosition.y);
      particle(c->pos.x, c->pos.y, 9, 1, 0, M_PI * 2);
      c->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(collectibles, aliveCollectibleCount);
  if (aliveCollectibleCount == 0) {
    addCollectables();
  }
  color = BLACK;
  TIMES(3, i) {
    MonkeytHazard *h = &hazards[i];
    float mul;
    if ((h->pos.x < 10 && h->speed > 0) || (h->pos.x > 90 && h->speed < 0)) {
      mul = 0.5;
    } else {
      mul = 1;
    }
    h->pos.x += h->speed * mul;
    if ((h->pos.x < -10 && h->speed < 0) || (h->pos.x > 110 && h->speed > 0)) {
      h->pos.y = rnd(10, 70);
      h->speed = -h->speed;
    }
    if (h->speed > 0) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    Collision hCol = character("c", h->pos.x, h->pos.y);
    if (hCol.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
  }
  characterOptions.isMirrorX = false;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 10);
}

void addGameMonkeyT() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
