#include "cglp.h"

static char *title = "TORNADO TWISTER";
static char *description = "[Tap] Change direction\n[Hold] Shrink & slow down";

// Upstream's characters array is empty (only box() is used to draw
// everything, no character() calls) - blank dummy entry, charactersCount =
// 0, matching the convention in gamePinClimb.c etc.
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

// "shapeDark" theme -> isDarkColor = true.
static Options options = {
    .viewSizeX = 150, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = true};

#define TORNADOTWISTER_PARTICLE_COUNT 20
typedef struct {
  Vector pos;
  float size;
  float currentSize;
  float direction;
  float speed;
  Vector particles[TORNADOTWISTER_PARTICLE_COUNT];
} Tornado;
static Tornado tornado;

#define TORNADOTWISTER_TYPE_TREE 0
#define TORNADOTWISTER_TYPE_HOUSE 1
#define TORNADOTWISTER_TYPE_CAR 2

typedef struct {
  Vector pos;
  int type;
  float size;
  float angle;
  float distance;
  bool isAlive;
} Suckable;
#define TORNADOTWISTER_MAX_SUCKABLE_COUNT 16
static Suckable suckableObjects[TORNADOTWISTER_MAX_SUCKABLE_COUNT];
static int suckableIndex;

typedef struct {
  Vector pos;
  float width;
  float height;
  bool isAlive;
} Obstacle;
#define TORNADOTWISTER_MAX_OBSTACLE_COUNT 16
static Obstacle obstacles[TORNADOTWISTER_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float nextObstacleTicks;
static float scrollSpeed;
// Upstream's growthRate is assigned at init but never read anywhere else in
// update() - dead state in the original game itself, kept here only so this
// port's init block mirrors upstream's 1:1.
static float growthRate;
static float spawnTimer;

static void update() {
  if (!ticks) {
    vectorSet(&tornado.pos, 20, 75);
    tornado.size = 7;
    tornado.currentSize = 7;
    tornado.direction = 1;
    tornado.speed = 1;
    TIMES(TORNADOTWISTER_PARTICLE_COUNT, particleIdx) {
      vectorSet(&tornado.particles[particleIdx], rnd(0, 10) * RNDPM(), rnd(0, 10) * RNDPM());
    }
    INIT_UNALIVED_ARRAY(suckableObjects);
    suckableIndex = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    scrollSpeed = 1;
    growthRate = 0.01;
    spawnTimer = 0;
  }

  float sd = sqrt(difficulty);
  scrollSpeed = sd;
  if (input.isJustPressed) {
    tornado.direction *= -1;
    play(LASER);
  }
  if (input.isPressed) {
    tornado.currentSize += (3 - tornado.currentSize) * 0.1;
    tornado.speed += (0.1 - tornado.speed) * 0.1;
  } else {
    tornado.currentSize += (tornado.size - tornado.currentSize) * 0.03;
    tornado.speed += (1 - tornado.speed) * 0.1;
  }
  tornado.pos.y += tornado.direction * tornado.speed * sd;
  if (tornado.pos.y <= tornado.currentSize && tornado.direction < 0) {
    tornado.direction = 1;
  }
  if (tornado.pos.y >= 100 - tornado.currentSize && tornado.direction > 0) {
    tornado.direction = -1;
  }

  color = CYAN;
  TIMES(TORNADOTWISTER_PARTICLE_COUNT, pi2) {
    rotate(&tornado.particles[pi2], 0.15 * (1 - (float)pi2 / TORNADOTWISTER_PARTICLE_COUNT) * sd);
    Vector ppos;
    ppos = tornado.particles[pi2];
    vectorMul(&ppos, tornado.currentSize / 10);
    vectorAdd(&ppos, tornado.pos.x, tornado.pos.y);
    box(ppos.x, ppos.y, sqrt(tornado.currentSize), sqrt(tornado.currentSize));
  }

  spawnTimer -= sd;
  if (spawnTimer <= 0) {
    int type;
    float size;
    if (rnd(0, 1) < 0.5) {
      type = TORNADOTWISTER_TYPE_TREE;
      size = 3;
    } else if (rnd(0, 1) < 0.7) {
      type = TORNADOTWISTER_TYPE_HOUSE;
      size = 5;
    } else {
      type = TORNADOTWISTER_TYPE_CAR;
      size = 4;
    }
    ASSIGN_ARRAY_ITEM(suckableObjects, suckableIndex, Suckable, ns);
    vectorSet(&ns->pos, 160, rnd(10, 90));
    ns->type = type;
    ns->size = size;
    ns->angle = 0;
    ns->distance = 0;
    ns->isAlive = true;
    suckableIndex = wrap(suckableIndex + 1, 0, TORNADOTWISTER_MAX_SUCKABLE_COUNT);
    spawnTimer = rnd(30, 50);
  }

  FOR_EACH(suckableObjects, si) {
    ASSIGN_ARRAY_ITEM(suckableObjects, si, Suckable, obj);
    SKIP_IS_NOT_ALIVE(obj);
    if (obj->distance > 0) {
      obj->distance -= 1;
      obj->angle += 0.2;
      Vector offset;
      vectorSet(&offset, obj->distance, 0);
      rotate(&offset, obj->angle);
      obj->pos = tornado.pos;
      vectorAdd(&obj->pos, offset.x, offset.y);
      if (obj->distance <= 0) {
        tornado.size += obj->size * 0.05;
        addScore(floor(obj->size * tornado.currentSize), VEC_XY(tornado.pos));
        play(POWER_UP);
        obj->isAlive = false;
        continue;
      }
    } else {
      obj->pos.x -= scrollSpeed;
    }
    if (obj->type == TORNADOTWISTER_TYPE_TREE) {
      color = GREEN;
    } else if (obj->type == TORNADOTWISTER_TYPE_HOUSE) {
      color = YELLOW;
    } else {
      color = RED;
    }
    Collision oc = box(obj->pos.x, obj->pos.y, obj->size, obj->size);
    bool isCollidingTornado = oc.isColliding.rect[CYAN];
    if (isCollidingTornado && obj->distance == 0) {
      obj->distance = distanceTo(&obj->pos, tornado.pos.x, tornado.pos.y) * 1.5;
      obj->angle = angleTo(&tornado.pos, obj->pos.x, obj->pos.y);
    }
    obj->isAlive = obj->pos.x >= -obj->size;
  }

  nextObstacleTicks -= sd;
  if (nextObstacleTicks < 0) {
    float width = rnd(20, 30);
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, 150 + width / 2, rnd(10, 90));
    no->width = width;
    no->height = rnd(10, 20);
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, TORNADOTWISTER_MAX_OBSTACLE_COUNT);
    nextObstacleTicks += rnd(50, 150);
  }

  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    obs->pos.x -= scrollSpeed;
    color = BLACK;
    Collision obc = box(obs->pos.x, obs->pos.y, obs->width, obs->height);
    if (obc.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    }
    obs->isAlive = obs->pos.x > -obs->width;
  }
}

void addGameTornadoTwister() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
