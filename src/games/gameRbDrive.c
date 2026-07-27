#include "cglp.h"

static char *title = "RB DRIVE";
static char *description = "[Tap]\n Change Lane";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "rrrrr ",
        " rrrrr",
        " rrrrr",
        "rrrrr ",
            "      ",
        "      ",
},
    {
        "llll  ",
        "  llll",
        "  llll",
        "llll  ",
            "      ",
        "      ",
},
    {
        " llll ",
        "lwwwwl",
        " llll ",
            "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 10, .isDarkColor = false};

typedef struct {
  int angle;
  int prevAngle;
  float phase;
  int color;
  bool isAlive;
} Car;
#define MAX_CAR_COUNT 32
static Car cars[MAX_CAR_COUNT];
static int carIndex;

static float nextCarTicks;
static int nextColor;
static int nextColorCount;
static float laneValues[4];
static int laneAngle;
// Fixed direction table (right/down/left/up), one per lane. JS keeps this as
// a module-level constant array outside update(); set once at tick 0 here
// instead since Vircon32 globals can't take a full array initializer list
// tied to another include-order-sensitive global.
static Vector angleOffsets[4];

static void drawLanes() {
  color = LIGHT_RED;
  rect(41, 0, 19, laneValues[3] * 42);
  rect(41, 100, 19, -laneValues[1] * 41);
  color = LIGHT_BLACK;
  rect(100, 41, -laneValues[0] * 41, 19);
  rect(0, 41, laneValues[2] * 42, 19);
  color = BLACK;
  rect(40, 0, 1, 40);
  rect(40, 60, 1, 40);
  rect(60, 0, 1, 40);
  rect(60, 60, 1, 40);
  rect(0, 40, 41, 1);
  rect(60, 40, 40, 1);
  rect(0, 60, 40, 1);
  rect(60, 60, 40, 1);
}

static void update() {
  // Never reads a Collision result - car/lane logic is decided by angle/index math.
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(cars);
    carIndex = 0;
    TIMES(4, i) {
      laneValues[i] = 0.7;
    }
    nextCarTicks = 0;
    nextColorCount = 0;
    nextColor = rndi(0, 2);
    laneAngle = 3;
    vectorSet(&angleOffsets[0], 1, 0);
    vectorSet(&angleOffsets[1], 0, 1);
    vectorSet(&angleOffsets[2], -1, 0);
    vectorSet(&angleOffsets[3], 0, -1);
  }
  float sd = sqrt(difficulty);
  drawLanes();
  if (input.isJustPressed) {
    // Vircon32 port note: upstream passes a per-call sound seed
    // ({seed:3}/{seed:1}/{seed:2}) to vary these effects' tone from the
    // game's other sounds; play() here has no per-call seed parameter, so
    // every call just uses the shared sound RNG state (cosmetic only).
    play(SELECT);
    laneAngle = wrap(laneAngle + 1, 0, 4);
  }
  Vector *ao = &angleOffsets[laneAngle];
  int lpo;
  if (laneAngle == 1 || laneAngle == 2) {
    lpo = 51;
  } else {
    lpo = 50;
  }
  Vector lp;
  vectorSet(&lp, ao->x, ao->y);
  vectorMul(&lp, 10 + fmod((float)ticks / 5, 7));
  vectorAdd(&lp, lpo, lpo);
  color = BLACK;
  TIMES(6, i) {
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = laneAngle;
    character("c", VEC_XY(lp));
    vectorAdd(&lp, ao->x * 7, ao->y * 7);
  }

  nextCarTicks -= sd;
  if (nextCarTicks < 0) {
    play(CLICK);
    nextColorCount--;
    if (nextColorCount < 0) {
      nextColor = wrap(nextColor + 1, 0, 2);
      nextColorCount = rndi(1, 4);
    }
    ASSIGN_ARRAY_ITEM(cars, carIndex, Car, nc);
    nc->angle = rndi(0, 4);
    nc->prevAngle = 0;
    nc->phase = 0;
    nc->color = nextColor;
    nc->isAlive = true;
    carIndex = wrap(carIndex + 1, 0, MAX_CAR_COUNT);
    nextCarTicks += rnd(80, 99);
  }

  FOR_EACH(cars, ci) {
    ASSIGN_ARRAY_ITEM(cars, ci, Car, c);
    SKIP_IS_NOT_ALIVE(c);
    Vector *cao = &angleOffsets[c->angle];
    Vector *caoo = &angleOffsets[(int)wrap(c->angle + 1, 0, 4)];
    Vector cp;
    if (c->phase < 1) {
      vectorSet(&cp, cao->x, cao->y);
      vectorMul(&cp, (1 - c->phase) * 40 + 10);
      vectorAdd(&cp, 50 + caoo->x * 5, 50 + caoo->y * 5);
    } else if (c->phase < 2) {
      Vector *pao = &angleOffsets[c->prevAngle];
      Vector *paoo = &angleOffsets[(int)wrap(c->prevAngle + 1, 0, 4)];
      vectorSet(&cp, pao->x, pao->y);
      vectorMul(&cp, 10);
      vectorAdd(&cp, paoo->x * 5, paoo->y * 5);
      Vector cpo;
      vectorSet(&cpo, cao->x, cao->y);
      vectorMul(&cpo, 10);
      vectorAdd(&cpo, -(caoo->x * 5), -(caoo->y * 5));
      vectorAdd(&cpo, -cp.x, -cp.y);
      vectorMul(&cpo, c->phase - 1);
      vectorAdd(&cp, cpo.x, cpo.y);
      vectorAdd(&cp, 50, 50);
    } else {
      vectorSet(&cp, cao->x, cao->y);
      vectorMul(&cp, (c->phase - 2) * 40 + 10);
      vectorAdd(&cp, 50 - caoo->x * 5, 50 - caoo->y * 5);
    }
    char carChar[2];
    carChar[0] = 'a' + c->color;
    carChar[1] = 0;
    int rot = c->angle;
    if (c->phase < 1) {
      rot += 2;
    }
    if (rot >= 4) {
      rot -= 4;
    }
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = rot;
    character(carChar, VEC_XY(cp));
    float pp = c->phase;
    float phaseAdd;
    if (c->phase > 1 && c->phase < 2) {
      phaseAdd = 0.05;
    } else {
      phaseAdd = 0.01;
    }
    c->phase += sd * phaseAdd;
    if (pp < 1 && c->phase >= 1) {
      // Vircon32 port note: dropped {seed:1} per-call sound seed - see the
      // note above input.isJustPressed.
      play(HIT);
      c->prevAngle = c->angle;
      if (c->angle != laneAngle) {
        c->angle = laneAngle;
      } else {
        c->angle = wrap(c->angle + 2, 0, 4);
      }
    }
    if (c->phase > 3) {
      if (c->angle % 2 == c->color) {
        play(EXPLOSION);
        laneValues[c->angle] *= 0.66;
      } else {
        // Vircon32 port note: dropped {seed:2} per-call sound seed - see
        // the note above input.isJustPressed.
        play(POWER_UP);
        laneValues[c->angle] += 0.3;
        if (laneValues[c->angle] > 1) {
          laneValues[c->angle] = 1;
        }
        addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      }
      c->isAlive = false;
      continue;
    }
  }

  TIMES(4, i2) {
    laneValues[i2] -= sd * 0.0006;
    if (laneValues[i2] < 0.01) {
      play(RANDOM);
      gameOver();
    }
  }
}

void addGameRbDrive() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
