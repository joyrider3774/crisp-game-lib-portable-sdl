#include "cglp.h"

static char *title = "TURBO TUNNEL";
static char *description = "[Tap] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define TURBOTUNNEL_TUNNEL_RADIUS 30
#define TURBOTUNNEL_TUNNEL_WIDTH 40

typedef struct {
  float angle;
  Vector pos;
  float radius;
  float vr;
} Car;
static Car car;

typedef struct {
  float angle;
  float va;
  float radius;
  float destroyedTicks;
  Vector pos;
  bool isAlive;
} Obstacle;
#define TURBOTUNNEL_MAX_OBSTACLE_COUNT 64
static Obstacle obstacles[TURBOTUNNEL_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float multiplier;

// Shared helper for the repeated upstream `vec(50, 50).add(vec(radius,
// 0).rotate(angle))` idiom - a point at `radius` from the tunnel's (50, 50)
// center, at `angle`.
static void posOnRadius(float radius, float angle, Vector *out) {
  vectorSet(out, radius, 0);
  rotate(out, angle);
  vectorAdd(out, 50, 50);
}

static void update() {
  if (!ticks) {
    car.angle = M_PI_2;
    vectorSet(&car.pos, 0, 0);
    car.radius = TURBOTUNNEL_TUNNEL_RADIUS;
    car.vr = 1;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    multiplier = 1;
  }
  bool isSpawningObstacle = false;
  if (input.isJustPressed) {
    play(LASER);
    car.vr *= -1;
    isSpawningObstacle = true;
  }
  car.angle += 0.015 * difficulty / (car.radius * 0.03);
  car.radius += car.vr * 0.4;
  posOnRadius(car.radius, car.angle, &car.pos);
  float distanceFromCenter = distanceTo(&car.pos, 50, 50);
  if ((distanceFromCenter > TURBOTUNNEL_TUNNEL_RADIUS + TURBOTUNNEL_TUNNEL_WIDTH / 2 && car.vr > 0) ||
      (distanceFromCenter < TURBOTUNNEL_TUNNEL_RADIUS - TURBOTUNNEL_TUNNEL_WIDTH / 2 && car.vr < 0)) {
    car.vr *= -1;
    isSpawningObstacle = true;
  }
  color = BLUE;
  thickness = 4;
  arc(50, 50, TURBOTUNNEL_TUNNEL_RADIUS + TURBOTUNNEL_TUNNEL_WIDTH / 2, 0, M_PI * 2);
  thickness = 4;
  arc(50, 50, TURBOTUNNEL_TUNNEL_RADIUS - TURBOTUNNEL_TUNNEL_WIDTH / 2, 0, M_PI * 2);
  color = LIGHT_RED;
  thickness = 3;
  bar(car.pos.x, car.pos.y, 1, car.angle + M_PI_2);
  color = RED;
  box(car.pos.x, car.pos.y, 1, 1);

  bool hasDestroyed = false;
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->angle += o->va / (o->radius * 0.03);
    posOnRadius(o->radius, o->angle, &o->pos);
    if (o->destroyedTicks > 0) {
      hasDestroyed = true;
      o->destroyedTicks -= difficulty;
      if (o->destroyedTicks <= 0) {
        play(POWER_UP);
        addScore(multiplier, o->pos.x, o->pos.y);
        multiplier++;
        particle(o->pos.x, o->pos.y, 9, 3, 0, M_PI * 2);
        o->isAlive = false;
        continue;
      }
    }
    if (o->destroyedTicks > 0) {
      color = PURPLE;
    } else {
      color = YELLOW;
    }
    float a = o->angle + M_PI_2 + (o->destroyedTicks + 1) * 0.5;
    thickness = 3;
    Collision oc = bar(o->pos.x, o->pos.y, 1, a);
    if (o->destroyedTicks < 0 && oc.isColliding.rect[YELLOW]) {
      o->destroyedTicks = 30;
    }
    if (o->destroyedTicks < 0 && oc.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    }
  }

  color = TRANSPARENT;
  FOR_EACH(obstacles, oi2) {
    ASSIGN_ARRAY_ITEM(obstacles, oi2, Obstacle, o2);
    SKIP_IS_NOT_ALIVE(o2);
    Collision tc = box(o2->pos.x, o2->pos.y, 5, 5);
    if (o2->destroyedTicks < 0 && tc.isColliding.rect[PURPLE]) {
      play(COIN);
      o2->destroyedTicks = 30;
      hasDestroyed = true;
    }
  }
  if (!hasDestroyed) {
    multiplier = 1;
  }
  if (isSpawningObstacle) {
    float oAngle = car.angle + (M_PI * 2 - 3 / car.radius);
    float oVa = -rnd(0.01, 0.03) * difficulty;
    float oRadius = car.radius;
    Vector oPos;
    posOnRadius(oRadius, oAngle, &oPos);
    color = TRANSPARENT;
    Collision spawnCheck = box(oPos.x, oPos.y, 9, 9);
    if (!spawnCheck.isColliding.rect[YELLOW] && !spawnCheck.isColliding.rect[PURPLE]) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
      no->angle = oAngle;
      no->va = oVa;
      no->radius = oRadius;
      no->destroyedTicks = -1;
      no->pos = oPos;
      no->isAlive = true;
      obstacleIndex = wrap(obstacleIndex + 1, 0, TURBOTUNNEL_MAX_OBSTACLE_COUNT);
    }
  }
}

void addGameTurboTunnel() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
