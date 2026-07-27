#include "cglp.h"

static char *title = "PULSE PIPER";
static char *description = "[Hold] Charge";

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

#define PULSE_SPEED 0.7
#define CHARGE_RATE 1
#define OBSTACLE_SPEED 0.5
#define SPAWN_INTERVAL 60
#define WIRE_Y 50

typedef struct {
  Vector pos;
  float charge;
} Pulse;
static Pulse pulse;

typedef struct {
  Vector pos;
  Vector size;
  float gap;
  bool isDot;
  bool isAlive;
} Obstacle;
#define MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleDist;
static float obstacleScore;

static void updatePulse() {
  pulse.pos.x += PULSE_SPEED * difficulty;
  if (pulse.pos.x > 100) {
    pulse.pos.x = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    pulse.charge = clamp(pulse.charge + CHARGE_RATE * difficulty, 0, 99);
  }
  if (input.isJustReleased) {
    play(LASER);
    color = YELLOW;
    particle(VEC_XY(pulse.pos), 10, 2, 0, M_PI * 2);
    color = RED;
    obstacleScore = floor(pulse.charge);
    pulse.charge = 0;
  } else {
    color = YELLOW;
  }
  float pulseGap = 4 + pulse.charge;
  box(pulse.pos.x, pulse.pos.y - pulseGap / 2, 8, 4);
  box(pulse.pos.x, pulse.pos.y + pulseGap / 2, 8, 4);
}

static void updateObstacles() {
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, Obstacle, obstacle);
    SKIP_IS_NOT_ALIVE(obstacle);
    obstacle->pos.x -= OBSTACLE_SPEED * difficulty;
    if (!obstacle->isDot) {
      color = PURPLE;
    } else {
      color = RED;
    }
    float obstacleGap = obstacle->gap + obstacle->size.y / 2;
    Collision c1 = box(obstacle->pos.x, obstacle->pos.y - obstacleGap, obstacle->size.x,
                        obstacle->size.y);
    Collision c2 = box(obstacle->pos.x, obstacle->pos.y + obstacleGap, obstacle->size.x,
                        obstacle->size.y);
    if (obstacle->isDot && (c1.isColliding.rect[RED] || c2.isColliding.rect[RED])) {
      play(POWER_UP);
      color = RED;
      particle(VEC_XY(obstacle->pos), 15, 3, 0, M_PI * 2);
      addScore(obstacleScore, VEC_XY(obstacle->pos));
      obstacle->isAlive = false;
      continue;
    }
    if (!obstacle->isDot && (c1.isColliding.rect[YELLOW] || c2.isColliding.rect[YELLOW])) {
      play(EXPLOSION);
      gameOver();
    }
    if (obstacle->pos.x + obstacle->size.x / 3 < 0) {
      obstacle->isAlive = false;
      continue;
    }
  }
}

static void spawnObstacle() {
  play(CLICK);
  bool isDot = rnd(0, 1) < 0.7;
  bool isWall = rnd(0, 1) < 0.3;
  Vector size;
  if (isDot) {
    vectorSet(&size, 4, 2);
  } else {
    float h;
    if (isWall) {
      h = 50;
    } else {
      h = 4;
    }
    vectorSet(&size, 9, h);
  }
  float gap;
  if (isDot) {
    gap = 0;
  } else {
    if (!isWall && pulse.pos.x < 40 && rnd(0, 1) < 0.65) {
      gap = 0;
    } else {
      gap = rnd(10, 20);
    }
  }
  ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
  vectorSet(&no->pos, 100 + size.x / 2, WIRE_Y);
  no->size = size;
  no->gap = gap;
  no->isDot = isDot;
  no->isAlive = true;
  obstacleIndex = wrap(obstacleIndex + 1, 0, MAX_OBSTACLE_COUNT);
}

static void update() {
  if (!ticks) {
    vectorSet(&pulse.pos, 10, WIRE_Y);
    pulse.charge = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleDist = 0;
    obstacleScore = 0;
  }
  color = CYAN;
  thickness = 4;
  line(0, WIRE_Y, 100, WIRE_Y);
  updatePulse();
  updateObstacles();
  nextObstacleDist -= difficulty;
  if (nextObstacleDist < 0) {
    spawnObstacle();
    nextObstacleDist += SPAWN_INTERVAL;
  }
}

void addGamePulsePiper() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
