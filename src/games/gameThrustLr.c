#include "cglp.h"

static char *title = "THRUST LR";
static char *description = "[Slide] Thrust";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 80, .isDarkColor = true};

#define THRUSTLR_SHIP_Y 85

typedef struct {
  Vector pos;
  float vx;
  float size;
  bool isAlive;
} Rect;
#define THRUSTLR_MAX_RECT_COUNT 32
static Rect rects[THRUSTLR_MAX_RECT_COUNT];
static int rectIndex;
static float nextRectDist;

typedef struct {
  float x;
  float vx;
} Ship;
static Ship ship;

static float addRect(float y) {
  bool t = rnd(0, 1) < 0.3;
  float size;
  float x;
  if (t) {
    size = rnd(7, 12);
    x = rnd(10, 90);
  } else {
    size = rnd(20, 30);
    x = 50 + rnd(40, 60) * RNDPM();
  }
  ASSIGN_ARRAY_ITEM(rects, rectIndex, Rect, r);
  vectorSet(&r->pos, x, y - size);
  r->vx = 0;
  r->size = size;
  r->isAlive = true;
  rectIndex = wrap(rectIndex + 1, 0, THRUSTLR_MAX_RECT_COUNT);
  return size;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(rects);
    rectIndex = 0;
    nextRectDist = 0;
    ship.x = 50;
    ship.vx = 0;
    addRect(60);
    addRect(40);
    addRect(20);
  }
  float scr = difficulty * 0.2;
  nextRectDist -= scr;
  if (nextRectDist < 0) {
    float size = addRect(0);
    nextRectDist = rnd(10, size);
  }
  float tx = -(clamp(input.pos.x, 0, 100) - 50);
  FOR_EACH(rects, i) {
    ASSIGN_ARRAY_ITEM(rects, i, Rect, r);
    SKIP_IS_NOT_ALIVE(r);
    r->pos.y += scr;
    if (r->pos.y - r->size < THRUSTLR_SHIP_Y && r->pos.y + r->size > THRUSTLR_SHIP_Y) {
      float d = r->pos.x - ship.x;
      if (d * tx < 0) {
        r->vx -= (((tx / (fabs(d) - (r->size - fabs(r->pos.y - THRUSTLR_SHIP_Y)))) *
                   0.2) /
                  r->size) *
                 difficulty;
      }
      r->vx *= 0.98;
    }
    r->pos.x += r->vx;
    if (r->pos.x < -r->size || r->pos.x > 99 + r->size) {
      play(POWER_UP);
      float scoreX;
      if (r->pos.x < 50) {
        scoreX = 10;
      } else {
        scoreX = 90;
      }
      addScore(floor(r->size), scoreX, clamp(r->pos.y, 0, 95));
      r->isAlive = false;
      continue;
    }
    thickness = 3;
    line(r->pos.x + r->size, r->pos.y, r->pos.x, r->pos.y + r->size);
    thickness = 3;
    line(r->pos.x, r->pos.y + r->size, r->pos.x - r->size, r->pos.y);
    thickness = 3;
    line(r->pos.x - r->size, r->pos.y, r->pos.x, r->pos.y - r->size);
    thickness = 3;
    line(r->pos.x, r->pos.y - r->size, r->pos.x + r->size, r->pos.y);
    r->isAlive = r->pos.y - r->size * 2 <= 99;
  }
  color = LIGHT_BLACK;
  rect(50, 93, 1, 7);
  rect(50, 95, -tx, 3);
  color = BLACK;
  ship.vx += tx * 0.001 * sqrt(difficulty);
  ship.x = clamp(ship.x + ship.vx, 4, 96);
  if ((ship.x < 5 && ship.vx < 0) ||
      (ship.x > 95 && ship.vx > 0)) {
    ship.vx *= -0.7;
  }
  ship.vx *= 0.95;
  particle(ship.x, THRUSTLR_SHIP_Y, fabs(tx) * 0.05, -tx * 0.05, 0, 0.5);
  thickness = 2;
  Collision c1 = line(ship.x - 1, THRUSTLR_SHIP_Y - 2, ship.x - 2, THRUSTLR_SHIP_Y + 2);
  thickness = 2;
  Collision c2 = line(ship.x + 1, THRUSTLR_SHIP_Y - 2, ship.x + 2, THRUSTLR_SHIP_Y + 2);
  if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameThrustLr() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
