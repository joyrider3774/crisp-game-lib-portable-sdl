#include "cglp.h"

static char *title = "HOPPIN' HAZARDS";
static char *description = "[Hold] Hop";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 80, .isDarkColor = false};

#define HOPPINHAZARDS_MIN_FROG_SIZE 10
#define HOPPINHAZARDS_MAX_FROG_SIZE 30
#define HOPPINHAZARDS_OBSTACLE_SPEED 1
#define HOPPINHAZARDS_SHAPE_BIRD 0
#define HOPPINHAZARDS_SHAPE_SNAKE 1

typedef struct {
  Vector pos;
  float size;
} Frog;
static Frog frog;

typedef struct {
  Vector pos;
  float vx;
  float vy;
  int shape;
  int color;
  bool isAlive;
} Obstacle;
#define HOPPINHAZARDS_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[HOPPINHAZARDS_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleTicks;

static void update() {
  if (!ticks) {
    vectorSet(&frog.pos, 50, 90);
    frog.size = HOPPINHAZARDS_MIN_FROG_SIZE;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
  }
  color = BLUE;
  rect(0, 95, 100, 5);
  frog.pos.x = 50;
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    float newSize = frog.size + difficulty;
    if (newSize > HOPPINHAZARDS_MAX_FROG_SIZE) {
      newSize = HOPPINHAZARDS_MAX_FROG_SIZE;
    }
    frog.size = newSize;
    float newY = frog.pos.y - difficulty;
    if (newY < 10) {
      newY = 10;
    }
    frog.pos.y = newY;
  } else {
    float newSize = frog.size - difficulty * 2;
    if (newSize < HOPPINHAZARDS_MIN_FROG_SIZE) {
      newSize = HOPPINHAZARDS_MIN_FROG_SIZE;
    }
    frog.size = newSize;
    float newY = frog.pos.y + difficulty * 2;
    if (newY > 90) {
      newY = 90;
    }
    frog.pos.y = newY;
  }
  addScore((frog.size - HOPPINHAZARDS_MIN_FROG_SIZE) / 100, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  color = GREEN;
  box(VEC_XY(frog.pos), frog.size, frog.size);
  nextObstacleTicks -= difficulty;
  if (nextObstacleTicks < 0) {
    float x;
    if (rndi(0, 2) == 0) {
      x = -10;
    } else {
      x = 110;
    }
    int shape;
    if (rndi(0, 2) == 0) {
      shape = HOPPINHAZARDS_SHAPE_BIRD;
    } else {
      shape = HOPPINHAZARDS_SHAPE_SNAKE;
    }
    float vx;
    if (x < 50) {
      vx = HOPPINHAZARDS_OBSTACLE_SPEED + rnd(0, difficulty - 1);
    } else {
      vx = -(HOPPINHAZARDS_OBSTACLE_SPEED + rnd(0, difficulty - 1));
    }
    int col;
    if (rndi(0, 2) == 0) {
      col = BLUE;
    } else {
      col = RED;
    }
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, x, rnd(10, 90));
    no->vx = vx;
    no->vy = 0;
    no->shape = shape;
    no->color = col;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, HOPPINHAZARDS_MAX_OBSTACLE_COUNT);
    if (shape == HOPPINHAZARDS_SHAPE_BIRD) {
      play(HIT);
    } else {
      play(CLICK);
    }
    nextObstacleTicks = rnd(45, 120) / sqrt(difficulty);
  }
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x += o->vx;
    o->pos.y += o->vy;
    color = o->color;
    if (o->shape == HOPPINHAZARDS_SHAPE_BIRD) {
      float angle;
      if (o->vx > 0) {
        angle = 0;
      } else {
        angle = M_PI;
      }
      thickness = 3;
      barCenterPosRatio = 0.5;
      Collision bc = bar(VEC_XY(o->pos), 10, angle);
      if (bc.isColliding.rect[GREEN]) {
        play(EXPLOSION);
        gameOver();
      }
    } else if (o->shape == HOPPINHAZARDS_SHAPE_SNAKE) {
      float angleTo2;
      if (o->vx > 0) {
        angleTo2 = M_PI_2 - M_PI;
      } else {
        angleTo2 = M_PI_2 + M_PI;
      }
      thickness = 3;
      Collision ac = arc(VEC_XY(o->pos), 5, M_PI_2, angleTo2);
      if (ac.isColliding.rect[GREEN]) {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (o->pos.x < -10 || o->pos.x > 110) {
      o->isAlive = false;
      continue;
    }
  }
}

void addGameHoppinHazards() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
