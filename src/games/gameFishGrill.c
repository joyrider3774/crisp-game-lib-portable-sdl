#include "cglp.h"

static char *title = "FISH GRILL";
static char *description = "[Hold] Burn up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "  l   ",
    "l lll ",
    "llll l",
    "llllll",
    "l lll ",
    "  l   ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define FISHGRILL_GRAVITY 0.1
#define FISHGRILL_UPDRAFT_FORCE -1
#define FISHGRILL_INITIAL_ENERGY 100
#define FISHGRILL_MIN_FISH_SPACING 50
#define FISHGRILL_MAX_FISH_SPACING 80
#define FISHGRILL_MIN_FISH_WIDTH 10
#define FISHGRILL_MAX_FISH_WIDTH 20

typedef struct {
  Vector pos;
  Vector velocity;
  float energy;
  float baseEnergy;
} FishgrillEmber;
static FishgrillEmber fishgrillEmber;

typedef struct {
  Vector pos;
  float width;
  bool isBurned;
} FishgrillFish;
#define FISHGRILL_MAX_FISH_COUNT 16
static FishgrillFish fishgrillFishes[FISHGRILL_MAX_FISH_COUNT];
static int fishgrillFishHead;
static int fishgrillFishCount;

typedef struct {
  Vector pos;
  float timeLeft;
  bool isAlive;
} FishgrillUpdraft;
#define FISHGRILL_MAX_UPDRAFT_COUNT 16
static FishgrillUpdraft fishgrillUpdrafts[FISHGRILL_MAX_UPDRAFT_COUNT];
static int fishgrillUpdraftIndex;

static Vector fishgrillScrollSpeed;

static void update() {
  if (!ticks) {
    vectorSet(&fishgrillEmber.pos, 20, 50);
    vectorSet(&fishgrillEmber.velocity, 0, 0);
    fishgrillEmber.energy = FISHGRILL_INITIAL_ENERGY;
    fishgrillEmber.baseEnergy = FISHGRILL_INITIAL_ENERGY;
    fishgrillFishHead = 0;
    fishgrillFishCount = 1;
    vectorSet(&fishgrillFishes[0].pos, 100, 50);
    fishgrillFishes[0].width = 15;
    fishgrillFishes[0].isBurned = false;
    INIT_UNALIVED_ARRAY(fishgrillUpdrafts);
    fishgrillUpdraftIndex = 0;
    vectorSet(&fishgrillScrollSpeed, -1, 0);
  }
  fishgrillEmber.velocity.y += FISHGRILL_GRAVITY;
  vectorAdd(&fishgrillEmber.pos, fishgrillEmber.velocity.x, fishgrillEmber.velocity.y);
  fishgrillEmber.baseEnergy += 0.1;
  fishgrillEmber.energy += (fishgrillEmber.baseEnergy - fishgrillEmber.energy) * 0.01;
  float emberRadius = 1 + fishgrillEmber.energy / FISHGRILL_INITIAL_ENERGY * 2;
  if (input.isPressed) {
    fishgrillEmber.velocity.y = FISHGRILL_UPDRAFT_FORCE;
    ASSIGN_ARRAY_ITEM(fishgrillUpdrafts, fishgrillUpdraftIndex, FishgrillUpdraft, nu);
    vectorSet(&nu->pos, fishgrillEmber.pos.x, fishgrillEmber.pos.y + emberRadius);
    nu->timeLeft = 10;
    nu->isAlive = true;
    fishgrillUpdraftIndex = wrap(fishgrillUpdraftIndex + 1, 0, FISHGRILL_MAX_UPDRAFT_COUNT);
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  color = PURPLE;
  thickness = 3;
  arc(fishgrillEmber.pos.x, fishgrillEmber.pos.y, emberRadius, 0, M_PI * 2);
  FOR_EACH(fishgrillUpdrafts, i) {
    ASSIGN_ARRAY_ITEM(fishgrillUpdrafts, i, FishgrillUpdraft, updraft);
    SKIP_IS_NOT_ALIVE(updraft);
    updraft->timeLeft--;
    color = RED;
    thickness = 3;
    TIMES(3, k) {
      float x = updraft->pos.x + rnd(-2, 2);
      float y = updraft->pos.y + rnd(0, 5);
      float size = rnd(1, 3);
      line(x - size / 2, y + size / 2, x, y - size / 2);
      line(x, y - size / 2, x + size / 2, y + size / 2);
      line(x + size / 2, y + size / 2, x - size / 2, y + size / 2);
    }
    if (updraft->timeLeft <= 0) {
      updraft->isAlive = false;
    }
  }
  Collision scratch;
  TIMES(fishgrillFishCount, k) {
    int idx = (fishgrillFishHead + k) % FISHGRILL_MAX_FISH_COUNT;
    FishgrillFish *fish = &fishgrillFishes[idx];
    vectorAdd(&fish->pos, fishgrillScrollSpeed.x, fishgrillScrollSpeed.y);
    if (fish->isBurned) {
      color = BLACK;
    } else {
      color = CYAN;
    }
    // Vircon32 port note: the JS version draws this fish at scale.x =
    // fish.width/6 (1.6x-3.3x wider than the plain 6x6 character), which
    // this port's character() has no equivalent for - drawn as a plain
    // box sized to fish.width instead, to keep the burn-detection hitbox
    // width gameplay-correct (visual-only tradeoff, loses the fish shape).
    scratch = box(fish->pos.x, fish->pos.y, fish->width, 6);
    if (scratch.isColliding.rect[RED]) {
      fishgrillEmber.energy += 5;
      if (!fish->isBurned) {
        play(POWER_UP);
        addScore(floor(fishgrillEmber.energy / 10), fish->pos.x, fish->pos.y);
        fish->isBurned = true;
      } else {
        addScore(floor(fishgrillEmber.energy / 100), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      }
    }
    if (scratch.isColliding.rect[PURPLE]) {
      play(EXPLOSION);
      gameOver();
    }
  }
  while (fishgrillFishCount > 0) {
    FishgrillFish *fish = &fishgrillFishes[fishgrillFishHead];
    if (fish->pos.x + fish->width < 0) {
      fishgrillFishHead = (fishgrillFishHead + 1) % FISHGRILL_MAX_FISH_COUNT;
      fishgrillFishCount--;
    } else {
      break;
    }
  }
  int lastIdx = (fishgrillFishHead + fishgrillFishCount - 1) % FISHGRILL_MAX_FISH_COUNT;
  if (fishgrillFishes[lastIdx].pos.x < 100) {
    play(CLICK);
    float newX = fishgrillFishes[lastIdx].pos.x +
                 rnd(FISHGRILL_MIN_FISH_SPACING, FISHGRILL_MAX_FISH_SPACING);
    float newY = rnd(10, 90);
    float newWidth = rnd(FISHGRILL_MIN_FISH_WIDTH, FISHGRILL_MAX_FISH_WIDTH);
    int newIdx = (fishgrillFishHead + fishgrillFishCount) % FISHGRILL_MAX_FISH_COUNT;
    vectorSet(&fishgrillFishes[newIdx].pos, newX, newY);
    fishgrillFishes[newIdx].width = newWidth;
    fishgrillFishes[newIdx].isBurned = false;
    fishgrillFishCount++;
  }
  if (fishgrillEmber.pos.y < -emberRadius || fishgrillEmber.pos.y > 100 + emberRadius) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameFishGrill() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
