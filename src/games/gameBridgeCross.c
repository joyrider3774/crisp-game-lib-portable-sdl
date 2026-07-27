#include "cglp.h"
#include "string.h"

static char *title = "BRIDGE CROSS";
static char *description = "[Hold] Jump";

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
    .viewSizeX = 150, .viewSizeY = 50, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float length;
} Bridge;
#define BRIDGECROSS_MAX_BRIDGE_COUNT 16
static Bridge bridges[BRIDGECROSS_MAX_BRIDGE_COUNT];
static int bridgeHead;
static int bridgeCount;

typedef struct {
  Vector pos;
  float length;
  float vy;
  bool isJumping;
  bool isFalling;
  bool isOnBridge;
} Brigade;
static Brigade brigade;
static float nextBridgeTicks;
static int multiplier;
static float nextMultiplierDist;

static void update() {
  // Never reads a Collision result anywhere in this file - whether the
  // brigade is standing on a bridge is a direct position-overlap check
  // (see the "brigade.pos.x + brigade.length >= b->pos.x" comparison
  // below), so the engine's own O(n^2) hitbox scan is pure waste here.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&brigade.pos, 10, 37);
    brigade.length = 30;
    brigade.vy = 0;
    brigade.isJumping = false;
    brigade.isFalling = false;
    brigade.isOnBridge = false;
    bridgeHead = 0;
    bridgeCount = 0;
    nextBridgeTicks = 0;
    multiplier = 1;
    nextMultiplierDist = 0;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 150, 50);
  color = GREEN;
  rect(0, 40, 150, 10);
  if (nextBridgeTicks <= 0) {
    float length = rnd(20, 40);
    int idx = (bridgeHead + bridgeCount) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    vectorSet(&bridges[idx].pos, 153, 35);
    bridges[idx].length = length;
    bridgeCount++;
    nextBridgeTicks = rnd(50, 100);
  }
  nextBridgeTicks -= difficulty;
  TIMES(bridgeCount, k) {
    int idx = (bridgeHead + k) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    Bridge *b = &bridges[idx];
    b->pos.x -= difficulty;
    color = YELLOW;
    rect(b->pos.x, b->pos.y, b->length, 5);
    color = BLUE;
    rect(b->pos.x, b->pos.y + 5, b->length, 10);
  }
  bool isOnBridge = false;
  TIMES(bridgeCount, k) {
    int idx = (bridgeHead + k) % BRIDGECROSS_MAX_BRIDGE_COUNT;
    Bridge *b = &bridges[idx];
    if (brigade.pos.x + brigade.length >= b->pos.x &&
        brigade.pos.x <= b->pos.x + b->length) {
      isOnBridge = true;
    }
  }
  while (bridgeCount > 0) {
    Bridge *b = &bridges[bridgeHead];
    if (!(b->pos.x + b->length > 0)) {
      bridgeHead = (bridgeHead + 1) % BRIDGECROSS_MAX_BRIDGE_COUNT;
      bridgeCount--;
    } else {
      break;
    }
  }
  if (brigade.pos.y < 35 && !brigade.isFalling && !isOnBridge) {
    brigade.isFalling = true;
    brigade.isOnBridge = false;
  }
  if (brigade.isJumping || brigade.isFalling) {
    brigade.pos.y += brigade.vy;
    float vyAdd;
    if (input.isPressed) {
      vyAdd = 0.1;
    } else {
      vyAdd = 0.2;
    }
    brigade.vy += vyAdd * difficulty;
    if (isOnBridge && brigade.pos.y > 32 && brigade.vy > 0) {
      brigade.isJumping = false;
      brigade.isFalling = false;
      brigade.isOnBridge = true;
      brigade.pos.y = 32;
      brigade.vy = 0;
    }
    if (!isOnBridge && brigade.pos.y > 37 && brigade.vy > 0) {
      brigade.isJumping = false;
      brigade.isFalling = false;
      brigade.pos.y = 37;
      brigade.vy = 0;
    }
  }
  if (!brigade.isJumping && input.isJustPressed) {
    play(JUMP);
    brigade.isJumping = true;
    brigade.isOnBridge = false;
    brigade.vy = -2 * sqrt(difficulty);
    multiplier = ceil(multiplier / 2.0);
  }
  if (brigade.isOnBridge) {
    nextMultiplierDist -= difficulty;
    if (nextMultiplierDist < 0) {
      play(COIN);
      addScore(multiplier, brigade.pos.x + clamp(multiplier * 3, 0, 50), brigade.pos.y);
      nextMultiplierDist = 10;
      multiplier++;
    }
  }
  if (!brigade.isJumping && brigade.pos.y > 32 && isOnBridge) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  rect(brigade.pos.x, brigade.pos.y, brigade.length, 3);
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameBridgeCross() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
