#include "cglp.h"

static char *title = "UP DOWN PRESS";
static char *description = "[Tap]  Jump\n[Hold] Speed up";

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
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 600, .isDarkColor = false};

#define UPDOWNPRESS_MY_CAR_SIZE 5
#define UPDOWNPRESS_MY_CAR_SPEED 1

typedef struct {
  Vector from;
  Vector to;
  float angle;
  bool isAlive;
} Road;
#define UPDOWNPRESS_MAX_ROAD_COUNT 32
static Road roads[UPDOWNPRESS_MAX_ROAD_COUNT];
static int roadIndex;
static int lastRoadIndex;
static float nextRoadDist;

static int carColors[4] = {RED, YELLOW, GREEN, PURPLE};

typedef struct {
  float x;
  float vx;
  float angle;
  float size;
  int color;
  float speed;
  float currentSpeed;
  bool isAlive;
} Car;
#define UPDOWNPRESS_MAX_CAR_COUNT 64
static Car cars[UPDOWNPRESS_MAX_CAR_COUNT];
static int carIndex;
static float nextCarDist;

static Vector scr;

#define UPDOWNPRESS_STATE_GROUND 0
#define UPDOWNPRESS_STATE_JUMP 1
typedef struct {
  Vector pos;
  float vy;
  float vx;
  float angle;
  float speed;
  int state;
} MyCar;
static MyCar myCar;

static int multiplier;

static bool calcRoad(float x, float *yOut, float *angleOut) {
  bool found = false;
  FOR_EACH(roads, ri) {
    ASSIGN_ARRAY_ITEM(roads, ri, Road, r);
    SKIP_IS_NOT_ALIVE(r);
    if (r->from.x <= x && x < r->to.x) {
      *yOut = ((r->from.y - r->to.y) * (x - r->to.x)) / (r->from.x - r->to.x) + r->to.y;
      *angleOut = r->angle;
      found = true;
    }
  }
  return found;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(roads);
    roadIndex = 0;
    ASSIGN_ARRAY_ITEM(roads, roadIndex, Road, r0);
    vectorSet(&r0->from, 0, 50);
    vectorSet(&r0->to, 100, 50);
    r0->angle = 0;
    r0->isAlive = true;
    lastRoadIndex = roadIndex;
    roadIndex = wrap(roadIndex + 1, 0, UPDOWNPRESS_MAX_ROAD_COUNT);
    nextRoadDist = -250;
    INIT_UNALIVED_ARRAY(cars);
    carIndex = 0;
    nextCarDist = 0;
    vectorSet(&scr, 0, 0);
    vectorSet(&myCar.pos, 20, 50);
    myCar.vy = 0;
    myCar.vx = 0;
    myCar.angle = 0;
    myCar.speed = 1;
    myCar.state = UPDOWNPRESS_STATE_GROUND;
    multiplier = 1;
  }
  vectorSet(&scr, difficulty * 0.1, difficulty * 0.1);
  if (myCar.pos.x > 50) {
    scr.x += (myCar.pos.x - 50) * 0.1;
  }
  float ry, dummyAngle;
  bool foundRy = calcRoad(myCar.pos.x + 50, &ry, &dummyAngle);
  if (foundRy) {
    if (ry < 60) {
      scr.y += (ry - 60) * 0.1;
    } else if (ry > 90) {
      scr.y += (ry - 90) * 0.1;
    }
  }
  nextRoadDist -= scr.x;
  while (nextRoadDist < 0) {
    Road *lr = &roads[lastRoadIndex];
    Vector from = lr->to;
    Vector to = lr->to;
    float w = rnd(20, 60);
    to.x += w;
    if (lr->from.y - lr->to.y == 0) {
      to.y += rnd(0.4, 1.1) * RNDPM() * w;
    }
    ASSIGN_ARRAY_ITEM(roads, roadIndex, Road, nr);
    nr->from = from;
    nr->to = to;
    nr->angle = angleTo(&from, to.x, to.y);
    nr->isAlive = true;
    lastRoadIndex = roadIndex;
    roadIndex = wrap(roadIndex + 1, 0, UPDOWNPRESS_MAX_ROAD_COUNT);
    nextRoadDist += w;
  }
  color = LIGHT_BLACK;
  FOR_EACH(roads, ri2) {
    ASSIGN_ARRAY_ITEM(roads, ri2, Road, r2);
    SKIP_IS_NOT_ALIVE(r2);
    r2->from.x -= scr.x;
    r2->from.y -= scr.y;
    r2->to.x -= scr.x;
    r2->to.y -= scr.y;
    line(r2->from.x, r2->from.y, r2->to.x, r2->to.y);
    if (r2->to.x < -50) {
      r2->isAlive = false;
      continue;
    }
  }
  myCar.pos.x += myCar.speed * sqrt(difficulty) - scr.x + myCar.vx;
  myCar.vx *= 0.9;
  if (myCar.pos.x < 0) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  float y, a;
  bool foundY = calcRoad(myCar.pos.x, &y, &a);
  if (foundY) {
    if (myCar.state == UPDOWNPRESS_STATE_JUMP && myCar.pos.y > y) {
      myCar.state = UPDOWNPRESS_STATE_GROUND;
    }
    if (myCar.state == UPDOWNPRESS_STATE_GROUND) {
      myCar.pos.y = y;
      myCar.speed += UPDOWNPRESS_MY_CAR_SIZE * myCar.angle * 0.02;
      myCar.angle += (a - myCar.angle) * 0.025;
      if (input.isJustPressed) {
        play(JUMP);
        myCar.state = UPDOWNPRESS_STATE_JUMP;
        myCar.vy = -2;
        multiplier = 1;
      }
    }
  }
  if (myCar.state == UPDOWNPRESS_STATE_JUMP) {
    myCar.pos.y += myCar.vy * sqrt(difficulty);
    float dvy2;
    if (input.isPressed) {
      dvy2 = 0.05;
    } else {
      dvy2 = 0.2;
    }
    myCar.vy += dvy2;
    myCar.angle += (atan2(myCar.vy, myCar.speed) - myCar.angle) * 0.05;
  }
  float speedMul;
  if (input.isPressed) {
    speedMul = 2.5;
  } else {
    speedMul = 0.5;
  }
  myCar.speed += (UPDOWNPRESS_MY_CAR_SPEED * speedMul - myCar.speed) * 0.1;
  Vector p;
  vectorSet(&p, myCar.pos.x, myCar.pos.y);
  addWithAngle(&p, myCar.angle, UPDOWNPRESS_MY_CAR_SIZE * 0.6);
  float ts = UPDOWNPRESS_MY_CAR_SIZE;
  color = BLUE;
  box(p.x, p.y - ts / 2, ts, ts);
  addWithAngle(&p, myCar.angle, UPDOWNPRESS_MY_CAR_SIZE * -1.2);
  box(p.x, p.y - ts / 2, ts, ts);
  color = CYAN;
  vectorSet(&p, myCar.pos.x, myCar.pos.y);
  addWithAngle(&p, myCar.angle - M_PI_2, ts);
  thickness = ts;
  barCenterPosRatio = 0.5;
  bar(p.x, p.y, ts, myCar.angle);
  addWithAngle(&p, myCar.angle - (M_PI_4) * 3, ts * 0.5);
  thickness = ts;
  bar(p.x, p.y, ts / 2, myCar.angle);
  nextCarDist -= scr.x;
  if (nextCarDist < 0) {
    float carSpeed = rnd(0.3, 1 + sqrt(difficulty));
    float cx;
    if (carSpeed > 2.5) {
      cx = -5;
    } else {
      cx = 205;
    }
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, nc);
    nc->x = cx;
    nc->vx = 0;
    nc->size = rnd(5, 8);
    nc->speed = carSpeed;
    nc->currentSpeed = 0;
    nc->angle = 0;
    nc->color = carColors[rndi(0, 4)];
    nc->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, UPDOWNPRESS_MAX_CAR_COUNT);
    nextCarDist += rnd(100, 120) / sqrt(difficulty);
  }
  FOR_EACH(cars, ci) {
    ASSIGN_ARRAY_ITEM(cars, ci, Car, c);
    SKIP_IS_NOT_ALIVE(c);
    c->x += c->currentSpeed * sqrt(difficulty) - scr.x;
    float cy, ca;
    bool foundCy = calcRoad(c->x, &cy, &ca);
    c->currentSpeed += c->size * c->angle * 0.02;
    c->currentSpeed += (c->speed - c->currentSpeed) * 0.1;
    if (!foundCy) {
      c->isAlive = false;
      continue;
    }
    c->angle += (ca - c->angle) * 0.025;
    Vector cp;
    vectorSet(&cp, c->x, cy);
    addWithAngle(&cp, c->angle, c->size * 0.6);
    float cts = c->size;
    color = BLACK;
    box(cp.x, cp.y - cts / 2, cts, cts);
    addWithAngle(&cp, c->angle, c->size * -1.2);
    box(cp.x, cp.y - cts / 2, cts, cts);
    color = c->color;
    vectorSet(&cp, c->x, cy);
    addWithAngle(&cp, c->angle - M_PI_2, cts);
    thickness = cts;
    barCenterPosRatio = 0.5;
    Collision cl1 = bar(cp.x, cp.y, cts, c->angle);
    addWithAngle(&cp, c->angle - (M_PI_4) * 3, cts * 0.5);
    thickness = cts;
    Collision cl2 = bar(cp.x, cp.y, cts / 2, c->angle);
    bool clCyan = cl1.isColliding.rect[CYAN] || cl2.isColliding.rect[CYAN];
    bool clBlue = cl1.isColliding.rect[BLUE] || cl2.isColliding.rect[BLUE];
    bool isPressing = myCar.state == UPDOWNPRESS_STATE_JUMP && myCar.vy >= 0;
    if (isPressing && (clCyan || clBlue)) {
      play(POWER_UP);
      addScore(multiplier, cp.x, cp.y);
      if (multiplier < 16) {
        multiplier *= 2;
      }
      particle(cp.x, cp.y, 16, 1, 0, M_PI * 2);
      myCar.vy = -2;
      c->isAlive = false;
      continue;
    }
    if (!isPressing && clCyan) {
      myCar.vx = -c->size - myCar.speed * 1.5;
      play(EXPLOSION);
      color = CYAN;
      particle(myCar.pos.x, myCar.pos.y, 9, 2, 0, 1);
    }
  }
}

void addGameUpDownPress() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
