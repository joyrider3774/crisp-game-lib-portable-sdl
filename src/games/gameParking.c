#include "cglp.h"

static char *title = "PARKING";
static char *description = "[Hold]\n Turn right";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

static int carColors[4] = {RED, BLUE, CYAN, PURPLE};
static char *wordChars = "PARKING";

typedef struct {
  Vector pos;
  int color;
  bool isAlive;
} Car;
#define MAX_CAR_COUNT 32
static Car cars[MAX_CAR_COUNT];
static int carIndex;

typedef struct {
  Vector pos;
  float angle;
  int color;
  bool isAlive;
} ParkedCar;
#define MAX_PARKED_CAR_COUNT 64
static ParkedCar parkedCars[MAX_PARKED_CAR_COUNT];
static int parkedCarIndex;

static float carAngle;
static float carCount;
static float nextCarDist;
static float nextParkedCarDist;
static bool goldActive;
static Vector goldPos;
static float roadY;
static int multiplier;

static Collision drawCar(float px, float py, float angle, int carColor) {
  Vector o;
  color = BLACK;
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + M_PI / 4, 3);
  box(VEC_XY(o), 3, 3);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (M_PI / 4) * 3, 3);
  box(VEC_XY(o), 3, 3);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (M_PI / 4) * 5, 3);
  box(VEC_XY(o), 3, 3);
  vectorSet(&o, px, py);
  addWithAngle(&o, angle + (M_PI / 4) * 7, 3);
  box(VEC_XY(o), 3, 3);
  color = carColor;
  thickness = 4;
  barCenterPosRatio = 0.5;
  return bar(px, py, 4, angle);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(cars);
    carIndex = 0;
    carAngle = -M_PI / 2;
    nextCarDist = 0;
    carCount = 1;
    INIT_UNALIVED_ARRAY(parkedCars);
    parkedCarIndex = 0;
    nextParkedCarDist = 0;
    goldActive = false;
    roadY = 0;
    multiplier = 1;
  }
  float carSpeed = difficulty;
  float scr = carSpeed * 1.05;
  bool foundLeadCar = false;
  float leadCarY = 0;
  FOR_EACH(cars, i) {
    ASSIGN_ARRAY_ITEM(cars, i, Car, c);
    if (c->isAlive) {
      leadCarY = c->pos.y;
      foundLeadCar = true;
      break;
    }
  }
  if (foundLeadCar) {
    if (leadCarY < 50) {
      scr += (50 - leadCarY) * 0.1;
    }
  } else {
    nextCarDist--;
  }
  roadY -= scr;
  color = LIGHT_BLACK;
  rect(0, 0, 11, 100);
  rect(89, 0, 11, 100);
  TIMES(3, x) {
    float lx = 30 + x * 20;
    TIMES(6, y) {
      float ly = y * 20 - fmod(-roadY, 20) - 10;
      box(lx, ly, 3, 10);
    }
  }
  color = LIGHT_YELLOW;
  rect(82, fmod(-roadY, 93) + 6, 7, 90);
  rect(82, fmod(-roadY, 93) - 88, 7, 90);
  color = WHITE;
  box(93, fmod(-roadY, 200) - 50, 7, 44);
  color = LIGHT_BLACK;
  float wordY = fmod(-roadY, 200) - 69;
  TIMES(7, i) {
    char wc[2];
    wc[0] = wordChars[i];
    wc[1] = 0;
    text(wc, 92, wordY);
    wordY += 6;
  }
  if (!goldActive) {
    vectorSet(&goldPos, rnd(40, 70), rnd(-50, -20));
    goldActive = true;
  }
  color = YELLOW;
  box(VEC_XY(goldPos), 8, 8);
  color = WHITE;
  text("$", VEC_XY(goldPos));
  goldPos.y += scr;
  if (goldPos.y > 103) {
    if (multiplier > 1) {
      multiplier--;
    }
    goldActive = false;
  }
  nextParkedCarDist -= scr;
  if (nextParkedCarDist < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(parkedCars, parkedCarIndex, ParkedCar, pc);
    vectorSet(&pc->pos, rnd(80, 84), -5);
    pc->angle = rnd((-M_PI / 8) * 3, -M_PI / 8);
    pc->color = carColors[rndi(0, 4)];
    pc->isAlive = true;
    parkedCarIndex = wrap(parkedCarIndex + 1, 0, MAX_PARKED_CAR_COUNT);
    if (rnd(0, 1) < 0.7) {
      nextParkedCarDist = rnd(8, 12);
    } else {
      nextParkedCarDist = rnd(100, 200);
    }
  }
  FOR_EACH(parkedCars, i) {
    ASSIGN_ARRAY_ITEM(parkedCars, i, ParkedCar, pc);
    SKIP_IS_NOT_ALIVE(pc);
    pc->pos.y += scr;
    drawCar(pc->pos.x, pc->pos.y, pc->angle, pc->color);
    pc->isAlive = pc->pos.y <= 105;
  }
  nextCarDist -= scr - carSpeed;
  COUNT_IS_ALIVE(cars, aliveCarCount);
  if (aliveCarCount < carCount && nextCarDist < 0) {
    play(HIT);
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, nc);
    vectorSet(&nc->pos, rnd(20, 60), -5);
    nc->color = carColors[rndi(0, 4)];
    nc->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, MAX_CAR_COUNT);
    nextCarDist = 15 * sqrt(difficulty);
  }
  if (input.isPressed) {
    carAngle += sqrt(difficulty) * 0.1;
  } else {
    carAngle += -sqrt(difficulty) * 0.1;
  }
  carAngle = clamp(carAngle, -M_PI / 2, 0);
  FOR_EACH(cars, i) {
    ASSIGN_ARRAY_ITEM(cars, i, Car, c);
    SKIP_IS_NOT_ALIVE(c);
    float ca;
    if (c->pos.y < 3) {
      ca = -M_PI / 2;
    } else {
      ca = carAngle;
    }
    addWithAngle(&c->pos, ca, carSpeed);
    c->pos.y += scr;
    Collision cl = drawCar(c->pos.x, c->pos.y, ca, c->color);
    if (cl.isColliding.rect[RED] || cl.isColliding.rect[BLUE] || cl.isColliding.rect[CYAN] ||
        cl.isColliding.rect[PURPLE]) {
      play(EXPLOSION);
      color = LIGHT_RED;
      text("X", VEC_XY(c->pos));
      gameOver();
    }
    if (cl.isColliding.rect[LIGHT_YELLOW]) {
      play(POWER_UP);
      addScore(multiplier * 10, VEC_XY(c->pos));
      ASSIGN_ARRAY_ITEM(parkedCars, parkedCarIndex, ParkedCar, pc2);
      pc2->pos = c->pos;
      pc2->angle = carAngle;
      pc2->color = c->color;
      pc2->isAlive = true;
      parkedCarIndex = wrap(parkedCarIndex + 1, 0, MAX_PARKED_CAR_COUNT);
      carCount += 1 / carCount;
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.rect[YELLOW]) {
      if (goldPos.y > -3) {
        play(COIN);
        addScore(multiplier, VEC_XY(goldPos));
        multiplier++;
      }
      goldActive = false;
    }
    if (c->pos.y > 103) {
      play(EXPLOSION);
      color = LIGHT_RED;
      text("X", c->pos.x, 96);
      gameOver();
    }
  }
}

void addGameParking() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
