#include "cglp.h"
#include "string.h"

static char *title = "ROTATION ROD";
static char *description = "[Tap] Turn";

// Vircon32 port note: upstream's characters array is empty (this game draws
// only bar()/box()/line()/text(), never character()) - a single blank dummy
// entry with charactersCount = 0 matches the convention already used by
// other character-less ports in this project (see gamePinClimb.c).
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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  Vector center;
  float length;
  float angle;
  float rotationSpeed;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Obstacle;
#define ROTATIONROD_MAX_OBSTACLE_COUNT 16
static Obstacle obstacles[ROTATIONROD_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Collectible;
// Spawn rate scales with difficulty while lifetime only shrinks by sqrt(difficulty), so the
// number alive at once keeps growing over a long session (~25 * sqrt(difficulty)) - sized
// generously rather than tightly to avoid the ring buffer overwriting still-alive collectibles.
#define ROTATIONROD_MAX_COLLECTIBLE_COUNT 256
static Collectible collectibles[ROTATIONROD_MAX_COLLECTIBLE_COUNT];
static int collectibleIndex;

static float nextObstacleTicks;
// Vircon32 port note: upstream's lastObstacleSpawn is the {pos, vel} object
// most recently produced by spawnAtEdge(), reused later (unchanged) to spawn
// a collectible at the exact same edge point/velocity. A plain pair of
// globals captures that with no array/reference needed.
static Vector lastSpawnPos;
static Vector lastSpawnVel;
static float nextCollectibleTicks;
static float multiplier;

static void spawnAtEdge(float speed, Vector *pos, Vector *vel) {
  int side = rndi(0, 4);
  if (side == 0) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, rnd(20, 40), -3);
    } else {
      vectorSet(pos, rnd(60, 80), -3);
    }
    vectorSet(vel, 0, speed);
  } else if (side == 1) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, 103, rnd(20, 40));
    } else {
      vectorSet(pos, 103, rnd(60, 80));
    }
    vectorSet(vel, -speed, 0);
  } else if (side == 2) {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, rnd(20, 40), 103);
    } else {
      vectorSet(pos, rnd(60, 80), 103);
    }
    vectorSet(vel, 0, -speed);
  } else {
    if (rnd(0, 1) < 0.5) {
      vectorSet(pos, -3, rnd(20, 40));
    } else {
      vectorSet(pos, -3, rnd(60, 80));
    }
    vectorSet(vel, speed, 0);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&player.center, 50, 50);
    player.length = 48;
    player.angle = 0;
    player.rotationSpeed = 0.05;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    INIT_UNALIVED_ARRAY(collectibles);
    collectibleIndex = 0;
    nextCollectibleTicks = 0;
    multiplier = 1;
  }
  float sd = sqrt(difficulty);
  if (input.isJustPressed) {
    player.rotationSpeed *= -1;
    play(SELECT);
  }
  if (!input.isPressed) {
    player.angle += player.rotationSpeed * sd;
  }
  color = BLUE;
  thickness = 2;
  bar(VEC_XY(player.center), player.length, player.angle);
  Vector ap1;
  ap1 = player.center;
  addWithAngle(&ap1, player.angle, player.length * 0.48);
  Vector ap2;
  ap2 = player.center;
  float ap2Angle;
  if (player.rotationSpeed > 0) {
    ap2Angle = player.angle + 0.2;
  } else {
    ap2Angle = player.angle - 0.2;
  }
  addWithAngle(&ap2, ap2Angle, player.length * 0.42);
  thickness = 3;
  line(VEC_XY(ap1), VEC_XY(ap2));

  nextObstacleTicks -= sd;
  if (nextObstacleTicks < 0) {
    play(LASER);
    spawnAtEdge(0.5 * sd, &lastSpawnPos, &lastSpawnVel);
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    no->pos = lastSpawnPos;
    no->vel = lastSpawnVel;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, ROTATIONROD_MAX_OBSTACLE_COUNT);
    nextObstacleTicks = 99;
    nextCollectibleTicks = 9;
  }
  color = RED;
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    vectorAdd(&o->pos, VEC_XY(o->vel));
    Collision c = box(VEC_XY(o->pos), 5, 5);
    if (c.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    o->isAlive = o->pos.x >= -5 && o->pos.x <= 110 && o->pos.y >= -5 && o->pos.y <= 110;
  }

  nextCollectibleTicks -= difficulty;
  if (nextCollectibleTicks < 0) {
    ASSIGN_ARRAY_ITEM(collectibles, collectibleIndex, Collectible, nc);
    nc->pos = lastSpawnPos;
    nc->vel = lastSpawnVel;
    nc->isAlive = true;
    collectibleIndex = wrap(collectibleIndex + 1, 0, ROTATIONROD_MAX_COLLECTIBLE_COUNT);
    nextCollectibleTicks = 9;
  }
  color = YELLOW;
  FOR_EACH(collectibles, ci) {
    ASSIGN_ARRAY_ITEM(collectibles, ci, Collectible, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, VEC_XY(c->vel));
    Collision cc = box(VEC_XY(c->pos), 3, 3);
    bool isColliding = cc.isColliding.rect[BLUE];
    if (isColliding) {
      play(COIN);
      addScore(ceil(multiplier), VEC_XY(c->pos));
      multiplier += 10;
    }
    bool inRect = c->pos.x >= -5 && c->pos.x <= 110 && c->pos.y >= -5 && c->pos.y <= 110;
    c->isAlive = !isColliding && inRect;
  }

  multiplier *= 0.99;
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)ceil(multiplier)));
  text(multText, 2, 10);
}

void addGameRotationRod() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
