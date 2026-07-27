#include "cglp.h"

static char *title = "SLALOM";
static char *description = "[Hold] Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 14, .isDarkColor = false};

typedef struct {
  float y;
  int side;
  float length;
  bool isAlive;
} Wall;
#define MAX_WALL_COUNT 16
static Wall walls[MAX_WALL_COUNT];
static int wallIndex;

typedef struct {
  Vector p;
  Vector s;
  bool isAlive;
} Rock;
#define MAX_ROCK_COUNT 32
static Rock rocks[MAX_ROCK_COUNT];
static int rockIndex;

static int wallSide;
static float wallAppDist;
static float scrolling;
static Vector pos;
static Vector vel;
static float angle;
static float minDist;
// Upstream's targetWall is a live reference to one of the objects in the (growable) walls array,
// which keeps being the same object across frames until either a newer/closer wall takes over or it
// scrolls away. There are no live references across this port's fixed arrays, so it's tracked as an
// index + a "do we have one at all" flag instead. Each frame the candidate target is recomputed as
// the alive wall with the largest `y` that is still above the player (< pos.y + 9) - since every
// alive wall advances by the same `scrolling` amount every frame, age order and descending-y order
// are identical, so "the oldest surviving wall that hasn't yet reached the player" (upstream's
// first-match-in-array-order semantics) is exactly "the largest y under the threshold" - a selection
// that doesn't depend on the walls array's physical (ring-buffer) order at all.
static bool hasTargetWall;
static int targetWallIndex;
static float rockAppDist;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    wallSide = 1;
    wallAppDist = 0;
    scrolling = 0;
    vectorSet(&pos, 90, 90);
    angle = -M_PI / 4 * 3;
    vectorSet(&vel, 1, 0);
    rotate(&vel, angle);
    minDist = 99;
    hasTargetWall = false;
    targetWallIndex = -1;
    INIT_UNALIVED_ARRAY(rocks);
    rockIndex = 0;
    rockAppDist = 0;
  }

  rockAppDist -= scrolling;
  if (rockAppDist < 0) {
    float x;
    if (rnd(0, 1) < 0.5) {
      x = rndi(-5, 5);
    } else {
      x = rndi(95, 105);
    }
    ASSIGN_ARRAY_ITEM(rocks, rockIndex, Rock, nr);
    vectorSet(&nr->p, x, -9);
    vectorSet(&nr->s, rndi(9, 19), rndi(5, 9) * 2);
    nr->isAlive = true;
    rockIndex = wrap(rockIndex + 1, 0, MAX_ROCK_COUNT);
    rockAppDist += 10;
  }
  color = PURPLE;
  FOR_EACH(rocks, ri) {
    ASSIGN_ARRAY_ITEM(rocks, ri, Rock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->p.y += scrolling;
    box(r->p.x, r->p.y, r->s.x, r->s.y);
    r->isAlive = r->p.y < 109;
  }

  wallAppDist -= scrolling;
  if (wallAppDist < 0) {
    play(SELECT);
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, nw);
    nw->y = -5;
    nw->side = wallSide;
    nw->length = rndi(40, 60);
    nw->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, MAX_WALL_COUNT);
    wallSide *= -1;
    wallAppDist += rndi(80, 90);
  }
  color = RED;
  int candidateIndex = -1;
  float candidateY = -1000000;
  FOR_EACH(walls, wi) {
    ASSIGN_ARRAY_ITEM(walls, wi, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->y += scrolling;
    float bx;
    if (w->side == 1) {
      bx = 99 - w->length / 2;
    } else {
      bx = w->length / 2;
    }
    box(bx, w->y, w->length, 4);
    if (w->y < pos.y + 9 && w->y > candidateY) {
      candidateY = w->y;
      candidateIndex = wi;
    }
    w->isAlive = w->y < 105;
  }
  bool newHasTarget = candidateIndex != -1;
  bool targetChanged;
  if (newHasTarget != hasTargetWall) {
    targetChanged = true;
  } else if (newHasTarget && candidateIndex != targetWallIndex) {
    targetChanged = true;
  } else {
    targetChanged = false;
  }
  if (targetChanged) {
    hasTargetWall = newHasTarget;
    targetWallIndex = candidateIndex;
    float s = floor(100 / (sqrt(minDist) + 1)) - 15;
    if (s > 0) {
      play(COIN);
      addScore(s, pos.x, pos.y);
    }
    minDist = 99;
  }

  color = BLACK;
  float targetSide;
  if (hasTargetWall) {
    targetSide = walls[targetWallIndex].side;
  } else {
    targetSide = 0;
  }
  if (input.isPressed) {
    angle += targetSide * 0.07 * difficulty;
    particle(pos.x, pos.y, 1, vectorLength(&vel), angle + M_PI, 0.2);
  }
  vectorMul(&vel, 1 - 0.02 / difficulty);
  Vector accel;
  vectorSet(&accel, 0.03, 0);
  rotate(&accel, angle);
  vectorAdd(&vel, accel.x, accel.y);
  vectorAdd(&pos, vel.x, vel.y);
  scrolling = 0;
  if (pos.y < 88) {
    scrolling += (88 - pos.y) * 0.5;
  }
  pos.y += scrolling;

  thickness = 2;
  Vector corner;
  vectorSet(&corner, 2, 2);
  rotate(&corner, angle);
  bar(pos.x + corner.x, pos.y + corner.y, 1, angle);
  vectorSet(&corner, -2, 2);
  rotate(&corner, angle);
  bar(pos.x + corner.x, pos.y + corner.y, 1, angle);
  vectorSet(&corner, 2, -2);
  rotate(&corner, angle);
  bar(pos.x + corner.x, pos.y + corner.y, 1, angle);
  vectorSet(&corner, -2, -2);
  rotate(&corner, angle);
  bar(pos.x + corner.x, pos.y + corner.y, 1, angle);

  color = BLUE;
  thickness = 3;
  Collision c = bar(pos.x, pos.y, 4, angle);
  bool outOfBounds = !(pos.x >= 0 && pos.x <= 99 && pos.y >= 0 && pos.y <= 99);
  if (c.isColliding.rect[RED] || c.isColliding.rect[PURPLE] || outOfBounds) {
    play(EXPLOSION);
    gameOver();
  }

  float wy;
  float wlength;
  int wside;
  if (hasTargetWall) {
    wy = walls[targetWallIndex].y;
    wlength = walls[targetWallIndex].length;
    wside = walls[targetWallIndex].side;
  } else {
    wy = 0;
    wlength = 0;
    wside = 1;
  }
  float wpx;
  if (wside == 1) {
    wpx = 99 - wlength;
  } else {
    wpx = wlength;
  }
  Vector wp;
  vectorSet(&wp, wpx, wy);
  float d = distanceTo(&wp, pos.x, pos.y);
  if (d < minDist) {
    minDist = d;
  }
}

void addGameSlalom() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
