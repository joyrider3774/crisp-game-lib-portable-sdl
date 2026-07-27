#include "cglp.h"

static char *title = "PERISCOPE PING";
static char *description = "[Hold] Ping\n[Tap]  Fire & Turn";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

#define PLAY_AREA_RADIUS 50
#define ROTATION_SPEED 0.04
#define OBJECT_MOVE_SPEED 0.1
#define SHOT_SPEED 1
#define TYPE_CIRCLE 0
#define TYPE_TRIANGLE 1

typedef struct {
  float angle;
  float pingRadius;
  float angleVel;
} Periscope;
static Periscope periscope;
static Vector towerPos;

typedef struct {
  Vector pos;
  bool isHidden;
  int type;
  float angle;
  bool isAlive;
} Object;
// Concurrent objects ~= life/spawn-interval = (450/sqrt(difficulty)) /
// (199/difficulty) = ~2.26*sqrt(difficulty) - unbounded growth; already
// exceeds 16 by ~difficulty 50 (under an hour of play).
#define MAX_OBJECT_COUNT 256
static Object objects[MAX_OBJECT_COUNT];
static int objectIndex;

typedef struct {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
} Shot;
#define MAX_SHOT_COUNT 16
static Shot shots[MAX_SHOT_COUNT];
static int shotIndex;

// Port note: the JS version never resets this in its "if (!ticks)" block
// either (only initialized once at module scope) - kept as a persistent
// global that's simply never reset on replay, matching upstream behavior
// exactly rather than "fixing" what looks like a minor oversight in the
// original.
static float spawnTimer;

static void fireShot(float x, float y, float angle) {
  ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
  vectorSet(&ns->pos, x, y);
  ns->angle = angle;
  ns->speed = SHOT_SPEED;
  ns->isAlive = true;
  shotIndex = wrap(shotIndex + 1, 0, MAX_SHOT_COUNT);
}

static void spawnObject() {
  float angle = rnd(0, M_PI * 2);
  Vector pos;
  pos = towerPos;
  vectorAdd(&pos, cos(angle) * PLAY_AREA_RADIUS, sin(angle) * PLAY_AREA_RADIUS);
  ASSIGN_ARRAY_ITEM(objects, objectIndex, Object, no);
  no->pos = pos;
  no->isHidden = true;
  if (rnd(0, 1) < 0.5) {
    no->type = TYPE_CIRCLE;
  } else {
    no->type = TYPE_TRIANGLE;
  }
  no->angle = angle + M_PI + rnd(0, 0.5) * RNDPM();
  no->isAlive = true;
  objectIndex = wrap(objectIndex + 1, 0, MAX_OBJECT_COUNT);
}

static void update() {
  if (!ticks) {
    periscope.angle = 0;
    periscope.pingRadius = 0;
    periscope.angleVel = 1;
    vectorSet(&towerPos, 50, 50);
    INIT_UNALIVED_ARRAY(objects);
    objectIndex = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
  }
  color = LIGHT_BLACK;
  thickness = 3;
  arc(VEC_XY(towerPos), PLAY_AREA_RADIUS, 0, M_PI * 2);
  periscope.angle += ROTATION_SPEED * periscope.angleVel * sqrt(difficulty);
  Vector periscopePos;
  periscopePos = towerPos;
  vectorAdd(&periscopePos, cos(periscope.angle) * 10, sin(periscope.angle) * 10);
  color = BLUE;
  box(VEC_XY(periscopePos), 3, 3);
  if (input.isJustPressed) {
    play(LASER);
    periscope.pingRadius = 0;
    periscope.angleVel *= -1;
    fireShot(periscopePos.x, periscopePos.y, periscope.angle);
  }
  if (input.isPressed) {
    periscope.pingRadius += 1;
    color = LIGHT_BLUE;
    thickness = 5;
    arc(VEC_XY(periscopePos), periscope.pingRadius, 0, M_PI * 2);
  }
  color = BLACK;
  box(VEC_XY(towerPos), 10, 10);
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, shot);
    SKIP_IS_NOT_ALIVE(shot);
    addWithAngle(&shot->pos, shot->angle, shot->speed * sqrt(difficulty));
    color = CYAN;
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(VEC_XY(shot->pos), 3, shot->angle);
    if (distanceTo(&shot->pos, VEC_XY(towerPos)) > PLAY_AREA_RADIUS) {
      shot->isAlive = false;
      continue;
    }
  }
  FOR_EACH(objects, i) {
    ASSIGN_ARRAY_ITEM(objects, i, Object, obj);
    SKIP_IS_NOT_ALIVE(obj);
    addWithAngle(&obj->pos, obj->angle, OBJECT_MOVE_SPEED * sqrt(difficulty));
    if (distanceTo(&obj->pos, VEC_XY(towerPos)) < 5) {
      obj->isAlive = false;
      continue;
    }
    color = TRANSPARENT;
    Collision scratch;
    if (obj->type == TYPE_CIRCLE) {
      thickness = 3;
      scratch = arc(VEC_XY(obj->pos), 2, 0, M_PI * 2);
    } else {
      thickness = 4;
      barCenterPosRatio = 0.5;
      scratch = bar(VEC_XY(obj->pos), 4, (ticks * 10 % 360) * M_PI / 180);
    }
    if (scratch.isColliding.rect[CYAN]) {
      color = RED;
      play(HIT);
      particle(VEC_XY(obj->pos), 20, 1, 0, M_PI * 2);
      addScore(floor(distanceTo(&obj->pos, VEC_XY(towerPos))), VEC_XY(obj->pos));
      obj->isAlive = false;
      continue;
    } else if (scratch.isColliding.rect[LIGHT_BLUE] || scratch.isColliding.rect[BLUE] ||
               scratch.isColliding.rect[BLACK]) {
      color = RED;
      if (obj->type == TYPE_CIRCLE) {
        arc(VEC_XY(obj->pos), 2, 0, M_PI * 2);
      } else {
        bar(VEC_XY(obj->pos), 4, (ticks * 10 % 360) * M_PI / 180);
      }
      if (obj->isHidden) {
        play(COIN);
        obj->isHidden = false;
      }
      if (scratch.isColliding.rect[BLUE] || scratch.isColliding.rect[BLACK]) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      obj->isHidden = true;
    }
  }
  spawnTimer += difficulty;
  COUNT_IS_ALIVE(objects, aliveObjectCount);
  if (aliveObjectCount == 0 || spawnTimer > 199) {
    spawnTimer = 0;
    spawnObject();
  }
}

void addGamePeriscopePing() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
