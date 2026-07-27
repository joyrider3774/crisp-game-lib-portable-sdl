#include "cglp.h"

static char *title = "RECOIL RADAR";
static char *description = "[Hold] Charge Scan\n[Release] Fire + Recoil";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2026071102, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  bool isAlive;
} Hazard;
// Concurrent hazards work out to roughly 3-8 at any realistic difficulty
// (spawn interval and drift speed both scale with difficulty in a way that
// keeps the ratio bounded) - sized with generous headroom.
#define MAX_HAZARD_COUNT 48
static Hazard hazards[MAX_HAZARD_COUNT];
static int hazardIndex;

static float scanRadius;
static int scanChargeTicks;
static float nextHazardTicks;
static int shotTicks;
static Vector shotFrom;
static Vector shotTo;
static int playerFlashTicks;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    vectorSet(&player.vel, 0, 0);
    INIT_UNALIVED_ARRAY(hazards);
    hazardIndex = 0;
    scanRadius = 8;
    scanChargeTicks = 0;
    nextHazardTicks = 45;
    shotTicks = 0;
    vectorSet(&shotFrom, 50, 50);
    vectorSet(&shotTo, 50, 50);
    playerFlashTicks = 0;
  }

  if (input.isPressed) {
    scanChargeTicks++;
    scanRadius = fmin(scanRadius + 0.55, 46);
  } else if (!input.isJustReleased) {
    scanChargeTicks = 0;
    scanRadius = fmax(scanRadius - 1.5, 8);
  }
  bool isScanCharged = scanChargeTicks >= 10;
  if (scanChargeTicks == 10) {
    color = LIGHT_CYAN;
    particle(VEC_XY(player.pos), 6, 0.8, 0, M_PI * 2);
    play(SELECT);
  }

  if (input.isJustReleased) {
    int targetIndex = -1;
    float targetDistance = scanRadius;
    if (isScanCharged) {
      FOR_EACH(hazards, i) {
        ASSIGN_ARRAY_ITEM(hazards, i, Hazard, h);
        SKIP_IS_NOT_ALIVE(h);
        float d = distanceTo(&player.pos, VEC_XY(h->pos));
        if (d <= targetDistance) {
          targetDistance = d;
          targetIndex = i;
        }
      }
    }
    if (targetIndex >= 0) {
      ASSIGN_ARRAY_ITEM(hazards, targetIndex, Hazard, target);
      vectorSet(&shotFrom, VEC_XY(player.pos));
      vectorSet(&shotTo, VEC_XY(target->pos));
      shotTicks = 5;
      float recoilAngle = angleTo(&target->pos, VEC_XY(player.pos));
      float kick = 1.2 + (46 - targetDistance) * 0.025;
      addWithAngle(&player.vel, recoilAngle, kick);
      addScore(round(55 - targetDistance), VEC_XY(target->pos));
      color = YELLOW;
      particle(VEC_XY(target->pos), 12, 2.2, recoilAngle, M_PI * 2);
      color = CYAN;
      particle(VEC_XY(player.pos), 4, 1.4, recoilAngle + M_PI, 0.5);
      target->isAlive = false;
      play(LASER);
    } else {
      play(CLICK);
    }
    scanChargeTicks = 0;
    scanRadius = 8;
  }

  vectorAdd(&player.pos, VEC_XY(player.vel));
  vectorMul(&player.vel, 0.985);
  bool didBounce = false;
  if (player.pos.x < 8) {
    player.pos.x = 8;
    player.vel.x = fabs(player.vel.x) * 0.8;
    didBounce = true;
  } else if (player.pos.x > 92) {
    player.pos.x = 92;
    player.vel.x = -fabs(player.vel.x) * 0.8;
    didBounce = true;
  }
  if (player.pos.y < 9) {
    player.pos.y = 9;
    player.vel.y = fabs(player.vel.y) * 0.8;
    didBounce = true;
  } else if (player.pos.y > 92) {
    player.pos.y = 92;
    player.vel.y = -fabs(player.vel.y) * 0.8;
    didBounce = true;
  }
  if (didBounce) {
    playerFlashTicks = 2;
    color = CYAN;
    particle(VEC_XY(player.pos), 5, 1.2, vectorAngle(&player.vel) + M_PI, M_PI / 2);
    play(HIT);
  }

  nextHazardTicks--;
  if (nextHazardTicks < 0) {
    int edge = rndi(0, 4);
    Vector p;
    if (edge == 0) {
      vectorSet(&p, rnd(10, 90), -5);
    } else if (edge == 1) {
      vectorSet(&p, 105, rnd(10, 90));
    } else if (edge == 2) {
      vectorSet(&p, rnd(10, 90), 105);
    } else {
      vectorSet(&p, -5, rnd(10, 90));
    }
    float a = angleTo(&p, VEC_XY(player.pos)) + rnd(-0.45, 0.45);
    ASSIGN_ARRAY_ITEM(hazards, hazardIndex, Hazard, nh);
    nh->pos = p;
    vectorSet(&nh->vel, cos(a), sin(a));
    nh->size = rnd(5, 8);
    nh->isAlive = true;
    hazardIndex = wrap(hazardIndex + 1, 0, MAX_HAZARD_COUNT);
    nextHazardTicks += fmax(24, 72 / sqrt(difficulty));
  }

  if (input.isPressed) {
    if (isScanCharged) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_BLACK;
    }
    thickness = 1.5;
    arc(VEC_XY(player.pos), scanRadius, 0, M_PI * 2);
  }

  if (shotTicks > 0) {
    color = YELLOW;
    thickness = 2;
    line(VEC_XY(shotFrom), VEC_XY(shotTo));
    shotTicks--;
  }

  color = CYAN;
  box(VEC_XY(player.pos), 6, 6);
  if (playerFlashTicks > 0) {
    color = LIGHT_CYAN;
    box(VEC_XY(player.pos), 4, 4);
    playerFlashTicks--;
  }
  color = BLUE;
  thickness = 2;
  bar(VEC_XY(player.pos), 9, vectorAngle(&player.vel));

  float rockSpeed = fmin(1.8, 0.45 + 0.18 * sqrt(difficulty));
  FOR_EACH(hazards, hi) {
    ASSIGN_ARRAY_ITEM(hazards, hi, Hazard, h2);
    SKIP_IS_NOT_ALIVE(h2);
    vectorAdd(&h2->pos, h2->vel.x * rockSpeed, h2->vel.y * rockSpeed);
    bool isRevealed = input.isPressed && isScanCharged &&
        distanceTo(&player.pos, VEC_XY(h2->pos)) <= scanRadius;
    if (isRevealed) {
      color = RED;
    } else {
      color = LIGHT_BLACK;
    }
    Collision hit = box(VEC_XY(h2->pos), h2->size, h2->size);
    if (hit.isColliding.rect[CYAN] || hit.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
    if (h2->pos.x < -12 || h2->pos.x > 112 || h2->pos.y < -12 || h2->pos.y > 112) {
      h2->isAlive = false;
      continue;
    }
  }
}

void addGameRecoilRadar() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
