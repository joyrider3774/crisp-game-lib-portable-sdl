#include "cglp.h"

static char *title = "KELP CLIMBER";
static char *description = "[Tap] Grab kelp";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define KELPCLIMBER_SCREEN_WIDTH 100
#define KELPCLIMBER_SCREEN_HEIGHT 100
#define KELPCLIMBER_OTTER_SIZE 5

typedef struct {
  Vector pos;
  float length;
  bool isAlive;
} KelpclimberKelp;
#define KELPCLIMBER_MAX_KELP_COUNT 16
static KelpclimberKelp kelpStrands[KELPCLIMBER_MAX_KELP_COUNT];
static int kelpIndex;
static float nextKelpStandDist;

typedef struct {
  Vector pos;
  Vector size;
  bool isAlive;
} KelpclimberObstacle;
#define KELPCLIMBER_MAX_OBSTACLE_COUNT 16
static KelpclimberObstacle obstacles[KELPCLIMBER_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleDist;

typedef struct {
  Vector pos;
  int grabbedKelpIndex;
} KelpclimberOtter;
static KelpclimberOtter otter;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(kelpStrands);
    vectorSet(&kelpStrands[0].pos, 25, 40);
    kelpStrands[0].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpStrands[0].isAlive = true;
    vectorSet(&kelpStrands[1].pos, 50, 50);
    kelpStrands[1].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpStrands[1].isAlive = true;
    vectorSet(&kelpStrands[2].pos, 75, 45);
    kelpStrands[2].length = KELPCLIMBER_SCREEN_HEIGHT;
    kelpStrands[2].isAlive = true;
    kelpIndex = 3;
    nextKelpStandDist = 0;
    vectorSet(&otter.pos, kelpStrands[1].pos.x, 90);
    otter.grabbedKelpIndex = 1;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleDist = 0;
  }
  float scrollSpeed = 0.5 * difficulty;
  int nearestKelpIndex = -1;
  float minDistance = 99;
  FOR_EACH(kelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    if (i != otter.grabbedKelpIndex && kelp->pos.y > 40) {
      float distance = fabs(kelp->pos.x - otter.pos.x);
      if (distance < minDistance) {
        minDistance = distance;
        nearestKelpIndex = i;
      }
    }
  }
  if (input.isJustPressed) {
    if (nearestKelpIndex >= 0) {
      KelpclimberKelp *nearestKelp = &kelpStrands[nearestKelpIndex];
      play(JUMP);
      addScore(ceil(fabs(nearestKelp->pos.x - otter.pos.x)), VEC_XY(otter.pos));
      kelpStrands[otter.grabbedKelpIndex].pos.y = 199;
      otter.grabbedKelpIndex = nearestKelpIndex;
      otter.pos.x = nearestKelp->pos.x;
    }
  }
  FOR_EACH(kelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    kelp->pos.y += scrollSpeed;
    if (kelp->pos.y > KELPCLIMBER_SCREEN_HEIGHT + 50) {
      kelp->isAlive = false;
    }
  }
  nextKelpStandDist -= scrollSpeed;
  if (nextKelpStandDist < 0) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(kelpStrands, kelpIndex, KelpclimberKelp, nk);
    vectorSet(&nk->pos, rnd(10, 90), -50);
    nk->length = KELPCLIMBER_SCREEN_HEIGHT;
    nk->isAlive = true;
    kelpIndex = wrap(kelpIndex + 1, 0, KELPCLIMBER_MAX_KELP_COUNT);
    nextKelpStandDist += rnd(30, 50);
  }
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, KelpclimberObstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    obstacle->pos.y += scrollSpeed;
    if (obstacle->pos.y > KELPCLIMBER_SCREEN_HEIGHT + 10) {
      obstacle->isAlive = false;
    }
  }
  nextObstacleDist -= scrollSpeed;
  if (nextObstacleDist < 0) {
    play(HIT);
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, KelpclimberObstacle, nobs);
    vectorSet(&nobs->pos, rnd(10, 90), -10);
    vectorSet(&nobs->size, rnd(5, 15), rnd(5, 15));
    nobs->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, KELPCLIMBER_MAX_OBSTACLE_COUNT);
    nextObstacleDist += rnd(40, 50);
  }
  color = GREEN;
  FOR_EACH(kelpStrands, i) {
    ASSIGN_ARRAY_ITEM(kelpStrands, i, KelpclimberKelp, kelp);
    SKIP_IS_NOT_ALIVE(kelp);
    box(kelp->pos.x, kelp->pos.y, 3, kelp->length);
  }
  color = RED;
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, KelpclimberObstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    box(obstacle->pos.x, obstacle->pos.y, obstacle->size.x, obstacle->size.y);
  }
  color = YELLOW;
  Collision scratch = box(otter.pos.x, otter.pos.y, KELPCLIMBER_OTTER_SIZE, KELPCLIMBER_OTTER_SIZE);
  if (scratch.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  if (nearestKelpIndex >= 0) {
    color = LIGHT_CYAN;
    box(kelpStrands[nearestKelpIndex].pos.x, 90, KELPCLIMBER_OTTER_SIZE - 2, KELPCLIMBER_OTTER_SIZE - 2);
  }
}

void addGameKelpClimber() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
