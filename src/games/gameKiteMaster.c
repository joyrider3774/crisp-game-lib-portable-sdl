#include "cglp.h"

static char *title = "KITE MASTER";
static char *description = "[Hold]\n Pull string\nCatch clouds with kite";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l   ",
        "  l  l",
        "lllll ",
        "  l   ",
        "l  l  ",
    },
    {
        " ll   ",
        " l    ",
        "  l ll",
        "lllll ",
        " l    ",
        "l ll  ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
} KitemasterPlayer;
static KitemasterPlayer player;

typedef struct {
  Vector pos;
  Vector vel;
} KitemasterKite;
static KitemasterKite kite;

typedef struct {
  Vector pos;
  Vector vel;
} KitemasterTailSegment;
#define KITEMASTER_TAIL_COUNT 5
static KitemasterTailSegment tail[KITEMASTER_TAIL_COUNT];

typedef struct {
  Vector pos;
  Vector vel;
  bool exists;
} KitemasterCloud;
static KitemasterCloud cloud;

static float stringLength;
static float maxStringLength;
static float wind;
static float windTicks;

// Vircon32 port note: vector.h has no normalize() - divide by length here,
// guarding the (rare, but possible when kite and player exactly coincide)
// zero-length case since dividing by zero hard-traps this CPU.
static void normalize(Vector *v) {
  float len = vectorLength(v);
  if (len > 0) {
    vectorMul(v, 1.0 / len);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 20, 87);
    vectorSet(&kite.pos, 35, 70);
    vectorSet(&kite.vel, 0, 0);
    stringLength = 20;
    maxStringLength = 120;
    wind = 0.8;
    windTicks = 0;
    TIMES(KITEMASTER_TAIL_COUNT, i) {
      vectorSet(&tail[i].pos, 35, 70 + (i + 1) * 3);
      vectorSet(&tail[i].vel, 0, 0);
    }
    cloud.exists = false;
  }

  wind = 1 + 0.5 * sin(windTicks);
  windTicks += 0.01 * difficulty;

  Vector forces;
  vectorSet(&forces, 0, 0);

  Vector stringDirection;
  vectorSet(&stringDirection, kite.pos.x - player.pos.x, kite.pos.y - player.pos.y);
  normalize(&stringDirection);
  vectorAdd(&forces, stringDirection.x * (wind * 0.81), stringDirection.y * (wind * 0.81));
  vectorAdd(&forces, wind * 0.19, 0);

  if (input.isPressed) {
    stringLength = fmax(5, stringLength - 0.2 * difficulty);
    vectorAdd(&forces, 0, -1);
  }
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isJustReleased) {
    play(HIT);
  }

  float distanceToPlayer = distanceTo(&kite.pos, VEC_XY(player.pos));
  if (distanceToPlayer > stringLength) {
    stringLength = fmin(maxStringLength, stringLength + wind * 0.1 * difficulty);
  }

  vectorAdd(&forces, 0, 0.3);

  if (distanceToPlayer > stringLength) {
    Vector tensionDirection;
    vectorSet(&tensionDirection, player.pos.x - kite.pos.x, player.pos.y - kite.pos.y);
    normalize(&tensionDirection);
    float tensionForce = (distanceToPlayer - stringLength) * 0.1;
    vectorAdd(&forces, tensionDirection.x * tensionForce, tensionDirection.y * tensionForce);
  }

  vectorMul(&forces, 0.02);
  vectorAdd(&kite.vel, forces.x, forces.y);
  vectorMul(&kite.vel, 0.98);

  vectorAdd(&kite.pos, kite.vel.x * difficulty, kite.vel.y * difficulty);

  kite.pos.x = clamp(kite.pos.x, 5, 95);
  kite.pos.y = clamp(kite.pos.y, 5, 95);

  if (!cloud.exists) {
    vectorSet(&cloud.pos, rnd(25, 75), -5);
    vectorSet(&cloud.vel, rnd(-0.1, 0.1), 0.15);
    cloud.exists = true;
  }

  if (cloud.exists) {
    vectorAdd(&cloud.pos, cloud.vel.x * difficulty, cloud.vel.y * difficulty);
    if ((cloud.pos.x <= 20 && cloud.vel.x < 0) || (cloud.pos.x >= 80 && cloud.vel.x > 0)) {
      cloud.vel.x *= -1;
    }
    if (cloud.pos.y > 90) {
      play(EXPLOSION);
      gameOver();
    }
  }

  float stringAngle = atan2(kite.pos.y - player.pos.y, kite.pos.x - player.pos.x);
  float windInfluence = wind * 0.3;
  float kiteAngle = stringAngle + windInfluence + M_PI_2;

  TIMES(KITEMASTER_TAIL_COUNT, ti) {
    KitemasterTailSegment *segment = &tail[ti];
    vectorAdd(&segment->vel, 0, 0.1);
    float windForce = wind * 0.05 * (ti + 1);
    vectorAdd(&segment->vel, windForce + rnd(0, 0.25) * RNDPM(), rnd(0, 0.25) * RNDPM());
    vectorMul(&segment->vel, 0.99);

    Vector anchor;
    if (ti == 0) {
      vectorSet(&anchor, kite.pos.x, kite.pos.y);
      addWithAngle(&anchor, kiteAngle, 6);
    } else {
      vectorSet(&anchor, tail[ti - 1].pos.x, tail[ti - 1].pos.y);
    }

    float tailDistance = distanceTo(&segment->pos, VEC_XY(anchor));
    float maxDistance = 4;
    if (tailDistance > maxDistance) {
      float dirX = (anchor.x - segment->pos.x) / tailDistance;
      float dirY = (anchor.y - segment->pos.y) / tailDistance;
      segment->pos.x = anchor.x - dirX * maxDistance;
      segment->pos.y = anchor.y - dirY * maxDistance;
    }

    vectorMul(&segment->vel, 0.85);
    vectorAdd(&segment->pos, segment->vel.x, segment->vel.y);
    segment->pos.x = clamp(segment->pos.x, 2, 98);
    segment->pos.y = clamp(segment->pos.y, 2, 90);
  }

  color = GREEN;
  rect(0, 90, 100, 10);

  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  char playerChar[2];
  if (input.isPressed) {
    playerChar[0] = 'b';
  } else {
    playerChar[0] = 'a';
  }
  playerChar[1] = 0;
  character(playerChar, VEC_XY(player.pos));

  color = LIGHT_BLACK;
  thickness = 1;
  float lineEndX = kite.pos.x - stringDirection.x * 6;
  float lineEndY = kite.pos.y - stringDirection.y * 6;
  line(player.pos.x + 3, player.pos.y, lineEndX, lineEndY);

  color = BLUE;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(kite.pos), 12, kiteAngle);

  color = LIGHT_BLUE;
  TIMES(KITEMASTER_TAIL_COUNT, si) {
    float size = fmax(2, 4 - si * 0.5);
    box(tail[si].pos.x, tail[si].pos.y, size, size);
  }

  if (cloud.exists) {
    color = CYAN;
    thickness = 3;
    Collision c1 = arc(cloud.pos.x - 3, cloud.pos.y, 3, 0, M_PI * 2);
    Collision c2 = arc(cloud.pos.x + 3, cloud.pos.y, 3, 0, M_PI * 2);

    if (c1.isColliding.rect[BLUE] || c1.isColliding.character['a'] || c1.isColliding.character['b'] ||
        c2.isColliding.rect[BLUE] || c2.isColliding.character['a'] || c2.isColliding.character['b']) {
      float cloudScoreValue = 100 - cloud.pos.y;
      cloud.pos.x = clamp(cloud.pos.x, 10, 90);
      cloud.pos.y = clamp(cloud.pos.y, 10, 90);
      addScore(cloudScoreValue, VEC_XY(cloud.pos));
      play(COIN);
      cloud.exists = false;
    }
  }

  color = CYAN;
  rect(5, 5, wind * 30, 3);

  if (kite.pos.y > 85) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameKiteMaster() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
