#include "cglp.h"

static char *title = "TURTLE TIDE";
static char *description = "[Hold] Dive & Speed up";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

#define TURTLETIDE_TURTLE_SIZE 2
#define TURTLETIDE_WATER_COLOR LIGHT_BLUE
#define TURTLETIDE_TURTLE_COLOR GREEN
#define TURTLETIDE_OBSTACLE_COLOR_ABOVE RED
#define TURTLETIDE_OBSTACLE_COLOR_BELOW PURPLE
#define TURTLETIDE_SCROLL_SPEED 1

typedef struct {
  Vector pos;
  bool isUnderwater;
  float speed;
} Turtle;
static Turtle turtle;

typedef struct {
  float waterLevel;
  float waveAmplitude;
  float waveFrequency;
} Water;
static Water water;

typedef struct {
  Vector pos;
  bool isUnderwater;
  Vector size;
  bool isAlive;
} Obstacle;
#define TURTLETIDE_MAX_OBSTACLE_COUNT 24
static Obstacle obstacles[TURTLETIDE_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleDist;

static void initGameState() {
  vectorSet(&turtle.pos, 20, 50);
  turtle.isUnderwater = false;
  turtle.speed = 1;
  water.waterLevel = 50;
  water.waveAmplitude = 10;
  water.waveFrequency = 0.05;
  INIT_UNALIVED_ARRAY(obstacles);
  obstacleIndex = 0;
  nextObstacleDist = 60;
  ticks = 0;
  score = 0;
}

static void updateWater() {
  water.waveFrequency = clamp(
      water.waveFrequency + rnd(0, 0.0001 * sqrt(difficulty)) * RNDPM(), 0, 0.05 * sqrt(difficulty));
  water.waveAmplitude = clamp(
      water.waveAmplitude + rnd(0, 0.01 * sqrt(difficulty)) * RNDPM(), 0, 10 * sqrt(difficulty));
  float wl = 50 + sin(ticks * water.waveFrequency) * water.waveAmplitude;
  water.waterLevel += (wl - water.waterLevel) * 0.1;
  color = TURTLETIDE_WATER_COLOR;
  rect(0, water.waterLevel, 100, 100 - water.waterLevel);
}

static void updateTurtle() {
  if (input.isJustPressed) {
    play(POWER_UP);
  }
  if (input.isJustReleased) {
    play(JUMP);
  }
  if (input.isPressed) {
    turtle.isUnderwater = true;
    turtle.speed = clamp(turtle.speed + 0.01, 1, 9);
  } else {
    turtle.isUnderwater = false;
    turtle.speed += (1 - turtle.speed) * 0.1;
  }
  float targetY;
  if (turtle.isUnderwater) {
    targetY = water.waterLevel + 15;
  } else {
    targetY = water.waterLevel - 5;
  }
  turtle.pos.y += (targetY - turtle.pos.y) * 0.1 * difficulty;
  color = TURTLETIDE_TURTLE_COLOR;
  Collision turtleCollision = box(turtle.pos.x, turtle.pos.y, TURTLETIDE_TURTLE_SIZE * turtle.speed,
                                   TURTLETIDE_TURTLE_SIZE);
  if (turtleCollision.isColliding.rect[TURTLETIDE_WATER_COLOR] && !turtle.isUnderwater) {
    turtle.pos.y = water.waterLevel - TURTLETIDE_TURTLE_SIZE / 2;
  }
}

static void updateObstacles() {
  float ss = TURTLETIDE_SCROLL_SPEED * difficulty * turtle.speed;
  addScore(ss * 0.1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x -= ss;
    if (o->pos.x < -10) {
      o->isAlive = false;
      continue;
    }
  }
  nextObstacleDist -= ss;
  if (nextObstacleDist < 0) {
    play(CLICK);
    bool isUnderwater = rnd(0, 1) < 0.5;
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    float oy;
    if (isUnderwater) {
      oy = rnd(water.waterLevel + 5, 95);
    } else {
      oy = rnd(5, water.waterLevel - 5);
    }
    vectorSet(&no->pos, 110, oy);
    no->isUnderwater = isUnderwater;
    vectorSet(&no->size, rnd(5, 10), rnd(5, 10));
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, TURTLETIDE_MAX_OBSTACLE_COUNT);
    nextObstacleDist = rnd(99, 120);
  }
  FOR_EACH(obstacles, oi2) {
    ASSIGN_ARRAY_ITEM(obstacles, oi2, Obstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    if (o2->isUnderwater) {
      color = TURTLETIDE_OBSTACLE_COLOR_BELOW;
    } else {
      color = TURTLETIDE_OBSTACLE_COLOR_ABOVE;
    }
    Collision c = box(o2->pos.x, o2->pos.y, o2->size.x, o2->size.y);
    if (c.isColliding.rect[TURTLETIDE_TURTLE_COLOR]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

static void update() {
  if (!ticks) {
    initGameState();
  }
  updateWater();
  updateTurtle();
  updateObstacles();
}

void addGameTurtleTide() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
