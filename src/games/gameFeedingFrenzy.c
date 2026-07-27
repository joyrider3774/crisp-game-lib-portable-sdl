#include "cglp.h"

static char *title = "FEEDING FRENZY";
static char *description = "[Hold]\n Dart and Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " lll  ",
    "  lll ",
    "l ll l",
    "llllll",
    "l lll ",
    "  ll  ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

#define FEEDINGFRENZY_SHARK_SPEED 0.7
#define FEEDINGFRENZY_SHARK_DART_SPEED 3.9
#define FEEDINGFRENZY_SEABED_RADIUS 45

typedef struct {
  Vector pos;
  float angle;
  bool isDarting;
} FeedingfrenzyShark;
static FeedingfrenzyShark feedingfrenzyShark;

typedef struct {
  Vector pos;
  float angle;
  float speed;
  int color;
  bool isAlive;
} FeedingfrenzyFish;
// Spawn interval is 10*sqrt(aliveCount)/difficulty/difficulty ticks - fish never
// expire except by being caught, so a long session (difficulty grows forever)
// or a deliberate "let them pile up for a big combo" chain can outrun 64 fast.
#define FEEDINGFRENZY_MAX_FISH_COUNT 512
static FeedingfrenzyFish feedingfrenzyFish[FEEDINGFRENZY_MAX_FISH_COUNT];
static int feedingfrenzyFishIndex;
static float feedingfrenzyFishSpawnTicks;
static int feedingfrenzyMultiplier;

static int feedingfrenzyFishColors[4] = {CYAN, BLUE, LIGHT_BLUE, PURPLE};

static void update() {
  if (!ticks) {
    vectorSet(&feedingfrenzyShark.pos, 50 + FEEDINGFRENZY_SEABED_RADIUS, 50);
    feedingfrenzyShark.angle = 0;
    feedingfrenzyShark.isDarting = false;
    INIT_UNALIVED_ARRAY(feedingfrenzyFish);
    feedingfrenzyFishIndex = 0;
    feedingfrenzyFishSpawnTicks = 0;
    feedingfrenzyMultiplier = 1;
  }
  color = LIGHT_BLACK;
  thickness = 3;
  arc(50, 50, FEEDINGFRENZY_SEABED_RADIUS, 0, M_PI * 2);
  color = BLACK;
  if (feedingfrenzyShark.isDarting) {
    if (input.isPressed) {
      feedingfrenzyShark.angle -= 0.1 * difficulty;
    }
    addWithAngle(&feedingfrenzyShark.pos, feedingfrenzyShark.angle,
                 FEEDINGFRENZY_SHARK_DART_SPEED * difficulty);
    if (distanceTo(&feedingfrenzyShark.pos, 50, 50) > FEEDINGFRENZY_SEABED_RADIUS) {
      feedingfrenzyShark.isDarting = false;
      float ca = angleTo(&feedingfrenzyShark.pos, 50, 50);
      vectorSet(&feedingfrenzyShark.pos, 50, 50);
      addWithAngle(&feedingfrenzyShark.pos, ca + M_PI, FEEDINGFRENZY_SEABED_RADIUS);
    }
  } else {
    feedingfrenzyShark.angle = angleTo(&feedingfrenzyShark.pos, 50, 50) + M_PI_2;
    addWithAngle(&feedingfrenzyShark.pos, feedingfrenzyShark.angle,
                 FEEDINGFRENZY_SHARK_SPEED * difficulty);
    if (input.isJustPressed) {
      play(LASER);
      feedingfrenzyShark.isDarting = true;
      feedingfrenzyShark.angle -= 0.5;
      feedingfrenzyMultiplier = 1;
    }
  }
  float triangleHeight = 5;
  float triangleWidth = 3;
  Vector triangleTip;
  triangleTip = feedingfrenzyShark.pos;
  addWithAngle(&triangleTip, feedingfrenzyShark.angle, triangleHeight);
  Vector triangleLeft;
  triangleLeft = feedingfrenzyShark.pos;
  addWithAngle(&triangleLeft, feedingfrenzyShark.angle - 2.8, triangleWidth);
  Vector triangleRight;
  triangleRight = feedingfrenzyShark.pos;
  addWithAngle(&triangleRight, feedingfrenzyShark.angle + 2.8, triangleWidth);
  thickness = 3;
  line(triangleTip.x, triangleTip.y, triangleLeft.x, triangleLeft.y);
  line(triangleLeft.x, triangleLeft.y, triangleRight.x, triangleRight.y);
  line(triangleRight.x, triangleRight.y, triangleTip.x, triangleTip.y);
  feedingfrenzyFishSpawnTicks--;
  if (feedingfrenzyFishSpawnTicks < 0) {
    float angle = rnd(0, M_PI * 2);
    float radius = rnd(0, FEEDINGFRENZY_SEABED_RADIUS * 0.8);
    Vector pos;
    vectorSet(&pos, 50, 50);
    addWithAngle(&pos, angle, radius);
    ASSIGN_ARRAY_ITEM(feedingfrenzyFish, feedingfrenzyFishIndex, FeedingfrenzyFish, nf);
    nf->pos = pos;
    nf->angle = wrap(angleTo(&pos, 50, 50) + rnd(0, 1) * RNDPM(), 0, M_PI * 2);
    nf->speed = rnd(0.03, 0.05) * difficulty;
    nf->color = feedingfrenzyFishColors[(int)floor(rnd(0, 4))];
    nf->isAlive = true;
    feedingfrenzyFishIndex = wrap(feedingfrenzyFishIndex + 1, 0, FEEDINGFRENZY_MAX_FISH_COUNT);
    COUNT_IS_ALIVE(feedingfrenzyFish, aliveFishCount);
    feedingfrenzyFishSpawnTicks = 10 * sqrt(aliveFishCount) / difficulty / difficulty;
  }
  FOR_EACH(feedingfrenzyFish, i) {
    ASSIGN_ARRAY_ITEM(feedingfrenzyFish, i, FeedingfrenzyFish, f);
    SKIP_IS_NOT_ALIVE(f);
    float distMul;
    if (distanceTo(&f->pos, 50, 50) > FEEDINGFRENZY_SEABED_RADIUS * 1.1) {
      distMul = 4;
    } else {
      distMul = 1;
    }
    float dartMul;
    if (feedingfrenzyShark.isDarting) {
      dartMul = 2.5;
    } else {
      dartMul = 1;
    }
    addWithAngle(&f->pos, f->angle, f->speed * dartMul * distMul);
    color = f->color;
    if (f->angle > M_PI_2 && f->angle < M_PI_2 * 3) {
      characterOptions.isMirrorX = true;
    } else {
      characterOptions.isMirrorX = false;
    }
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    Collision c = character("a", VEC_XY(f->pos));
    if (c.isColliding.rect[BLACK]) {
      play(COIN);
      addScore(feedingfrenzyMultiplier, f->pos.x, f->pos.y);
      particle(f->pos.x, f->pos.y, 9, 1, 0, M_PI * 2);
      feedingfrenzyMultiplier++;
      f->isAlive = false;
      continue;
    }
    if (!(f->pos.x >= 0 && f->pos.x <= 100 && f->pos.y >= 0 && f->pos.y <= 100)) {
      play(EXPLOSION);
      color = RED;
      text("X", VEC_XY(f->pos));
      gameOver();
    }
  }
}

void addGameFeedingFrenzy() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
