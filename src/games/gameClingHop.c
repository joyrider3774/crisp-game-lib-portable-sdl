#include "cglp.h"
#include "string.h"

static char *title = "CLING HOP";
static char *description = "[Tap] Flap";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
  float vx;
} Player;
static Player player;

typedef struct {
  int id;
  Vector pos;
  float w;
  float vx;
  float phase;
  bool isAlive;
} Platform;
// Lifetime (~383/spd frames) and spawn interval (~90/spd frames, floor'd)
// both scale with spd=sqrt(difficulty), so concurrent count stays near a
// constant ~4.3 - sized with generous headroom.
#define CLINGHOP_MAX_PLATFORM_COUNT 16
static Platform platforms[CLINGHOP_MAX_PLATFORM_COUNT];
static int platformIndex;

typedef struct {
  Vector pos;
  float vx;
  float vy;
  float spin;
  float squashX;
  float squashY;
  Vector trail[4];
  int trailCount;
  bool isAlive;
} Obstacle;
// Obstacles bounce forever until popped by the player (not on a timer), and
// spawn interval shrinks to 180/sqrt(difficulty) ticks - a normal session can
// reach the old 48-slot cap, at which point the ring buffer's "count < cap"
// spawn guard can still overwrite a still-alive slot out of FIFO order;
// raised well above the realistic accumulation for headroom.
#define CLINGHOP_MAX_OBSTACLE_COUNT 128
static Obstacle obstacles[CLINGHOP_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float nextObstacleTicks;
static int clingTargetId;
static int lastPlatformId;
static Vector trail[5];
static int trailCount;
static bool wasClinging;

static void pushObstacleTrail(Obstacle *obs, float x, float y) {
  int i;
  for (i = 3; i > 0; i--) {
    obs->trail[i] = obs->trail[i - 1];
  }
  vectorSet(&obs->trail[0], x, y);
  if (obs->trailCount < 4) {
    obs->trailCount++;
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    player.vy = 0;
    player.vx = 0;
    INIT_UNALIVED_ARRAY(platforms);
    platformIndex = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    clingTargetId = -1;
    lastPlatformId = -1;
    trailCount = 0;
    wasClinging = false;
  }

  float spd = sqrt(difficulty);

  int platformSpawnInterval = (int)floor(90 / spd);
  if (platformSpawnInterval < 1) {
    platformSpawnInterval = 1;
  }
  if (ticks % platformSpawnInterval == 0) {
    float w = rnd(15, 30) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(platforms, platformIndex, Platform, np);
    np->id = ticks;
    vectorSet(&np->pos, rnd(20, 80), -5);
    np->w = w;
    np->vx = rnd(-0.3, 0.3) * spd;
    np->phase = rnd(0, M_PI * 2);
    np->isAlive = true;
    platformIndex = wrap(platformIndex + 1, 0, CLINGHOP_MAX_PLATFORM_COUNT);
  }

  nextObstacleTicks--;
  COUNT_IS_ALIVE(obstacles, obstacleAliveCount);
  if ((nextObstacleTicks < 0 || obstacleAliveCount == 0) &&
      obstacleAliveCount < CLINGHOP_MAX_OBSTACLE_COUNT) {
    bool side = player.pos.x > 50;
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    float sideSign;
    float startX;
    if (side) {
      startX = -5;
      sideSign = 1;
    } else {
      startX = 105;
      sideSign = -1;
    }
    vectorSet(&no->pos, startX, rnd(20, 60));
    no->vx = sideSign * rnd(0.5, 1.2) * spd;
    no->vy = rnd(-1, 1) * spd;
    no->spin = 0;
    no->squashX = 1;
    no->squashY = 1;
    no->trailCount = 0;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, CLINGHOP_MAX_OBSTACLE_COUNT);
    nextObstacleTicks = 180 / spd;
  }

  float oldX = player.pos.x;

  if (clingTargetId < 0) {
    player.vy += 0.12;
    if (input.isJustPressed) {
      player.vy = -2.2;
      play(JUMP);
      color = YELLOW;
      particle(player.pos.x, player.pos.y, 8, 1.5, M_PI / 2, M_PI / 3);
    }
    player.pos.y += player.vy;
    player.pos.x = clamp(player.pos.x, 5, 95);
  } else {
    Platform *plat = NULL;
    FOR_EACH(platforms, platIdx) {
      ASSIGN_ARRAY_ITEM(platforms, platIdx, Platform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->id == clingTargetId) {
        plat = p;
      }
    }
    if (plat != NULL) {
      player.pos.x = clamp(plat->pos.x + sin(ticks * 0.05 + plat->phase) * 20, 5, 95);
      player.pos.y = plat->pos.y + 7;
      if (!wasClinging) {
        color = CYAN;
        particle(player.pos.x, player.pos.y, 12, 2, -M_PI / 2, M_PI / 2);
      }
      player.vy = 0;
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      if (input.isJustPressed) {
        clingTargetId = -1;
        player.vy = -2.5;
        play(JUMP);
        color = YELLOW;
        particle(player.pos.x, player.pos.y, 8, 1.5, M_PI / 2, M_PI / 3);
      }
    } else {
      clingTargetId = -1;
    }
  }

  player.vx = player.pos.x - oldX;
  wasClinging = clingTargetId >= 0;

  int i2;
  for (i2 = 4; i2 > 0; i2--) {
    trail[i2] = trail[i2 - 1];
  }
  vectorSet(&trail[0], player.pos.x, player.pos.y);
  if (trailCount < 5) {
    trailCount++;
  }

  if (player.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }

  color = GREEN;
  FOR_EACH(platforms, pi2) {
    ASSIGN_ARRAY_ITEM(platforms, pi2, Platform, plat2);
    SKIP_IS_NOT_ALIVE(plat2);
    plat2->pos.y += 0.3 * spd;
    float drawX = plat2->pos.x + sin(ticks * 0.05 + plat2->phase) * 20;
    drawX = clamp(drawX, plat2->w / 2 + 2, 98 - plat2->w / 2);
    thickness = 4;
    bar(drawX, plat2->pos.y, plat2->w, 0);

    if (clingTargetId < 0 && player.vy > 0) {
      float dx = fabs(player.pos.x - drawX);
      float dy = player.pos.y - plat2->pos.y;
      if (dx < plat2->w / 2 + 3 && dy > -3 && dy < 8) {
        clingTargetId = plat2->id;
        if (plat2->id != lastPlatformId) {
          COUNT_IS_ALIVE(obstacles, obstacleCountAtLanding);
          addScore(obstacleCountAtLanding, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
          lastPlatformId = plat2->id;
          play(COIN);
        }
      }
    }

    if (plat2->pos.y >= 110) {
      plat2->isAlive = false;
      continue;
    }
  }

  color = LIGHT_YELLOW;
  int ti;
  for (ti = trailCount - 1; ti >= 1; ti--) {
    float alpha = (float)(trailCount - ti) / trailCount;
    float size = 3 * alpha;
    box(trail[ti].x, trail[ti].y, size, size);
  }

  float scaleX = 1;
  float scaleY = 1;
  float tilt = 0;
  if (clingTargetId >= 0) {
    float breath = sin(ticks * 0.1) * 0.08;
    scaleX = 1.4 + breath;
    scaleY = 0.75 - breath;
    tilt = 0;
  } else {
    float spinRate;
    if (player.vy < 0) {
      spinRate = 1;
    } else {
      spinRate = 0.5;
    }
    tilt = ticks * 15 * spinRate;
    if (fabs(player.vy) > 1.5) {
      scaleX = 1;
      scaleY = 1;
    } else {
      scaleX = 1 + fabs(player.vx) * 0.1;
      scaleY = 1 - fabs(player.vy) * 0.05;
    }
  }

  if (clingTargetId >= 0) {
    color = CYAN;
  } else {
    color = YELLOW;
  }
  float pw = 5 * scaleX;
  float ph = 7 * scaleY;
  float px = player.pos.x;
  float py = player.pos.y;

  thickness = ph;
  bar(px, py, pw, tilt);

  if (clingTargetId >= 0) {
    float eyeOffsetX = (1.8 * scaleX) / 1.4;
    float eyeOffsetY = -0.3;
    float lookX = clamp(player.vx * 2, -1, 1);

    color = WHITE;
    box(px - eyeOffsetX, py + eyeOffsetY, 2, 1.5);
    box(px + eyeOffsetX, py + eyeOffsetY, 2, 1.5);

    color = BLACK;
    box(px - eyeOffsetX + lookX * 0.5, py + eyeOffsetY, 1, 1);
    box(px + eyeOffsetX + lookX * 0.5, py + eyeOffsetY, 1, 1);
  }

  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, obs);
    SKIP_IS_NOT_ALIVE(obs);
    obs->pos.x += obs->vx;
    obs->pos.y += obs->vy;

    pushObstacleTrail(obs, obs->pos.x, obs->pos.y);

    obs->spin += (fabs(obs->vx) + fabs(obs->vy)) * 0.1;

    obs->squashX += (1 - obs->squashX) * 0.2;
    obs->squashY += (1 - obs->squashY) * 0.2;

    color = RED;
    if (obs->pos.x < 3 || obs->pos.x > 97) {
      obs->vx *= -1;
      obs->pos.x = clamp(obs->pos.x, 3, 97);
      obs->squashX = 0.5;
      obs->squashY = 1.4;
      particle(obs->pos.x, obs->pos.y, 5, 1, 0, M_PI * 2);
    }
    if (obs->pos.y < 3 || obs->pos.y > 97) {
      obs->vy *= -1;
      obs->pos.y = clamp(obs->pos.y, 3, 97);
      obs->squashX = 1.4;
      obs->squashY = 0.5;
      particle(obs->pos.x, obs->pos.y, 5, 1, 0, M_PI * 2);
    }

    color = LIGHT_RED;
    int oti;
    for (oti = obs->trailCount - 1; oti >= 1; oti--) {
      float osize = (3.0 * (obs->trailCount - oti)) / obs->trailCount;
      box(obs->trail[oti].x, obs->trail[oti].y, osize, osize);
    }

    color = RED;
    float ow = 5 * obs->squashX;
    float oh = 4 * obs->squashY;
    thickness = oh;
    Collision obsCollision = bar(obs->pos.x, obs->pos.y, ow, obs->spin);

    float lookX2;
    if (obs->vx > 0) {
      lookX2 = 1;
    } else {
      lookX2 = -1;
    }
    float lookY2 = clamp(obs->vy * 0.5, -1, 1);
    color = WHITE;
    box(obs->pos.x - 1.2, obs->pos.y - 0.5, 1.5, 1.5);
    box(obs->pos.x + 1.2, obs->pos.y - 0.5, 1.5, 1.5);
    color = BLACK;
    box(obs->pos.x - 1.2 + lookX2 * 0.3, obs->pos.y - 0.5 + lookY2 * 0.3, 0.8, 0.8);
    box(obs->pos.x + 1.2 + lookX2 * 0.3, obs->pos.y - 0.5 + lookY2 * 0.3, 0.8, 0.8);

    if (obsCollision.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      gameOver();
    } else if (obsCollision.isColliding.rect[YELLOW]) {
      play(POWER_UP);
      color = RED;
      particle(obs->pos.x, obs->pos.y, 20, 3, 0, M_PI * 2);
      obs->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  COUNT_IS_ALIVE(obstacles, obstacleCountForText);
  char countText[16];
  strcpy(countText, "x");
  strcat(countText, intToChar(obstacleCountForText));
  text(countText, 3, 9);
}

void addGameClingHop() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
