#include "cglp.h"

static char *title = "CYWALL";
static char *description = "[Tap] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 27, .isDarkColor = false};

typedef struct {
  Vector pos;
  float r;
  int num;
  float a;
  float v;
  float l;
  int nextIndex;
  bool isAlive;
} Circle;
#define CYWALL_MAX_CIRCLE_COUNT 64
static Circle circles[CYWALL_MAX_CIRCLE_COUNT];
static int circleIndex;
static float circleAddDist;
static int lastCircleIndex;
static int playerCircleIndex;

static void addCircle() {
  float r = rnd(20, 30);
  int newIndex = circleIndex;
  ASSIGN_ARRAY_ITEM(circles, newIndex, Circle, c);
  vectorSet(&c->pos, rnd(20, 80), -r);
  c->r = r;
  c->num = rndi(1, 4);
  c->a = rnd(0, M_PI * 2);
  c->v = rnd(0.02, 0.05) * RNDPM() * difficulty;
  c->l = rnd(10, 20);
  c->nextIndex = -1;
  c->isAlive = true;
  if (lastCircleIndex >= 0) {
    circles[lastCircleIndex].nextIndex = newIndex;
  }
  if (playerCircleIndex < 0) {
    playerCircleIndex = newIndex;
  }
  lastCircleIndex = newIndex;
  circleIndex = wrap(circleIndex + 1, 0, CYWALL_MAX_CIRCLE_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(circles);
    circleIndex = 0;
    circleAddDist = 0;
    lastCircleIndex = -1;
    playerCircleIndex = -1;
  }
  if (circleAddDist <= 0) {
    addCircle();
    circleAddDist += rnd(20, 40);
  }
  float sc = difficulty * 0.1;
  Circle *playerCircle = &circles[playerCircleIndex];
  float py = playerCircle->pos.y;
  if (py < 50) {
    sc += (50 - py) * 0.05;
  }
  circleAddDist -= sc;
  addScore(sc, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  if (playerCircle->pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(circles, i) {
    ASSIGN_ARRAY_ITEM(circles, i, Circle, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += sc;
    if (c->pos.y > 99 + c->r) {
      c->isAlive = false;
      continue;
    }
    bool isPlayerOrNext =
        (i == playerCircleIndex) ||
        (playerCircle->nextIndex >= 0 && i == playerCircle->nextIndex);
    if (isPlayerOrNext) {
      color = CYAN;
    } else {
      color = RED;
    }
    box(VEC_XY(c->pos), 3, 3);
    color = RED;
    c->a += c->v;
    TIMES(c->num, k) {
      float a = c->a + k * M_PI * 2 / c->num;
      Vector barPos;
      barPos = c->pos;
      addWithAngle(&barPos, a, c->r);
      thickness = 3;
      barCenterPosRatio = 0.5;
      bar(barPos.x, barPos.y, c->l, a + M_PI_2);
    }
  }
  color = CYAN;
  if (playerCircle->nextIndex >= 0 && input.isJustPressed) {
    Circle *nextCircle = &circles[playerCircle->nextIndex];
    thickness = 3;
    Collision c2 = line(VEC_XY(playerCircle->pos), VEC_XY(nextCircle->pos));
    if (c2.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    } else {
      play(COIN);
      Vector p;
      p = playerCircle->pos;
      Vector o;
      o.x = (nextCircle->pos.x - playerCircle->pos.x) / 9;
      o.y = (nextCircle->pos.y - playerCircle->pos.y) / 9;
      float a = vectorAngle(&o);
      TIMES(9, i) {
        particle(p.x, p.y, 4, 2, a + M_PI, 0.5);
        vectorAdd(&p, o.x, o.y);
      }
    }
    playerCircleIndex = playerCircle->nextIndex;
  } else {
    box(VEC_XY(playerCircle->pos), 5, 5);
  }
}

void addGameCywall() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
