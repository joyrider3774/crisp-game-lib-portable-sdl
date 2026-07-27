#include "cglp.h"

static char *title = "ORBIT GRAZER";
static char *description = "[Tap] Reverse orbit";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector pos;
  int life;
  bool isAlive;
} OrbitGrazerTrailPoint;
#define ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT 32
static OrbitGrazerTrailPoint playerTrail[ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT];
static int playerTrailIndex;

// Concurrent asteroid trail points ~ life(30)/spawnEvery(4) ~ 7.5 - headroom applied.
#define ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT 16
typedef struct {
  Vector pos;
  Vector vel;
  float size;
  float rotation;
  float rotSpeed;
  float breathPhase;
  OrbitGrazerTrailPoint trail[ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT];
  int trailIndex;
  bool isAlive;
} OrbitGrazerAsteroid;
// Spawn interval and per-asteroid lifetime both scale as 1/speed, so
// concurrent count stays roughly constant (~4-5) regardless of difficulty -
// generous headroom applied anyway.
#define ORBITGRAZER_MAX_ASTEROID_COUNT 32
static OrbitGrazerAsteroid asteroids[ORBITGRAZER_MAX_ASTEROID_COUNT];
static int asteroidIndex;

static float centerX;
static float centerY;
static float orbitRadius;
static float orbitAngle;
static int orbitDir;
static Vector playerPos;
static int invincibleTicks;
static float playerStretch;

static void addPlayerTrailPoint(float x, float y) {
  ASSIGN_ARRAY_ITEM(playerTrail, playerTrailIndex, OrbitGrazerTrailPoint, t);
  t->pos.x = x;
  t->pos.y = y;
  t->life = 24;
  t->isAlive = true;
  playerTrailIndex = wrap(playerTrailIndex + 1, 0, ORBITGRAZER_PLAYER_TRAIL_MAX_COUNT);
}

static void addAsteroidTrailPoint(OrbitGrazerAsteroid *a, float x, float y) {
  ASSIGN_ARRAY_ITEM(a->trail, a->trailIndex, OrbitGrazerTrailPoint, t);
  t->pos.x = x;
  t->pos.y = y;
  t->life = 30;
  t->isAlive = true;
  a->trailIndex = wrap(a->trailIndex + 1, 0, ORBITGRAZER_ASTEROID_TRAIL_MAX_COUNT);
}

static void update() {
  if (!ticks) {
    centerX = 50;
    centerY = 50;
    orbitRadius = 25;
    orbitAngle = 0;
    orbitDir = 1;
    vectorSet(&playerPos, 0, 0);
    INIT_UNALIVED_ARRAY(asteroids);
    asteroidIndex = 0;
    invincibleTicks = 0;
    playerStretch = 1;
    INIT_UNALIVED_ARRAY(playerTrail);
    playerTrailIndex = 0;
  }

  if (input.isJustPressed) {
    orbitDir *= -1;
    play(SELECT);
    playerStretch = 1.6;
    color = CYAN;
    particle(playerPos.x, playerPos.y, 8, 1.5, orbitAngle - (M_PI / 2) * orbitDir, M_PI / 4);
  }

  playerStretch += (1 - playerStretch) * 0.15;

  float orbitSpeed = 0.5 * sqrt(difficulty);
  orbitAngle += (orbitSpeed * orbitDir) / (orbitRadius + 0.1);

  playerPos.x = centerX + cos(orbitAngle) * orbitRadius;
  playerPos.y = centerY + sin(orbitAngle) * orbitRadius;

  if (ticks % 3 == 0) {
    addPlayerTrailPoint(playerPos.x, playerPos.y);
  }
  FOR_EACH(playerTrail, pti) {
    ASSIGN_ARRAY_ITEM(playerTrail, pti, OrbitGrazerTrailPoint, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
    }
  }

  float spawnRateF = floor(50 / sqrt(difficulty));
  int spawnRate = (int)spawnRateF;
  // Vircon32 port note: guard against a zero interval (modulo by zero
  // hard-traps here) - upstream JS never guards this either, but this
  // dialect can't get away with it.
  if (spawnRate < 1) {
    spawnRate = 1;
  }
  if (ticks % spawnRate == 0) {
    float spawnAngle = rnd(0, M_PI * 2);
    float dist = 60;
    float ax = centerX + cos(spawnAngle) * dist;
    float ay = centerY + sin(spawnAngle) * dist;

    float playerAngle = atan2(playerPos.y - centerY, playerPos.x - centerX);
    float targetAngle = playerAngle + orbitDir * rnd(0.2, 0.6);
    float targetX = centerX + cos(targetAngle) * orbitRadius;
    float targetY = centerY + sin(targetAngle) * orbitRadius;

    float speed = 0.5 * sqrt(difficulty);
    float dx = targetX - ax;
    float dy = targetY - ay;
    float len = sqrt(dx * dx + dy * dy);
    float distToPlayerX = ax - playerPos.x;
    float distToPlayerY = ay - playerPos.y;
    float distToPlayer = sqrt(distToPlayerX * distToPlayerX + distToPlayerY * distToPlayerY);
    if (len > 0 && distToPlayer > 50) {
      ASSIGN_ARRAY_ITEM(asteroids, asteroidIndex, OrbitGrazerAsteroid, na);
      na->pos.x = ax;
      na->pos.y = ay;
      na->vel.x = (dx / len) * speed;
      na->vel.y = (dy / len) * speed;
      na->size = rnd(6, 9);
      na->rotation = rnd(0, M_PI * 2);
      na->rotSpeed = rnd(-0.1, 0.1);
      na->breathPhase = rnd(0, M_PI * 2);
      INIT_UNALIVED_ARRAY(na->trail);
      na->trailIndex = 0;
      na->isAlive = true;
      asteroidIndex = wrap(asteroidIndex + 1, 0, ORBITGRAZER_MAX_ASTEROID_COUNT);
    }
  }
  orbitRadius = clamp(orbitRadius + 0.05, 0, 40);

  color = LIGHT_BLACK;
  thickness = 1;
  arc(centerX, centerY, orbitRadius, 0, M_PI * 2);

  float centerPulse = 1 + sin(ticks * 0.1) * 0.1;
  color = YELLOW;
  box(centerX, centerY, 4 * centerPulse, 4 * centerPulse);

  FOR_EACH(playerTrail, pti2) {
    ASSIGN_ARRAY_ITEM(playerTrail, pti2, OrbitGrazerTrailPoint, t2);
    SKIP_IS_NOT_ALIVE(t2);
    float alpha = t2->life / 24.0;
    if (alpha > 0.5) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_BLACK;
    }
    box(t2->pos.x, t2->pos.y, 8 * alpha, 8 * alpha);
  }

  invincibleTicks--;
  if (invincibleTicks > 0) {
    color = BLUE;
  } else {
    color = CYAN;
  }

  float stretchW = 6 / playerStretch;
  float stretchH = 6 * playerStretch;

  Collision playerCol = box(playerPos.x, playerPos.y, stretchW, stretchH);
  if (playerCol.isColliding.rect[YELLOW]) {
    gameOver();
  }

  FOR_EACH(asteroids, ai) {
    ASSIGN_ARRAY_ITEM(asteroids, ai, OrbitGrazerAsteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    a->pos.x += a->vel.x;
    a->pos.y += a->vel.y;
    a->rotation += a->rotSpeed;
    a->breathPhase += 0.15;

    float breathScale = 1 + sin(a->breathPhase) * 0.08;

    if (ticks % 4 == 0) {
      addAsteroidTrailPoint(a, a->pos.x, a->pos.y);
    }
    FOR_EACH(a->trail, ti) {
      ASSIGN_ARRAY_ITEM(a->trail, ti, OrbitGrazerTrailPoint, tp);
      SKIP_IS_NOT_ALIVE(tp);
      tp->life--;
      if (tp->life <= 0) {
        tp->isAlive = false;
      }
    }

    float dx2 = playerPos.x - a->pos.x;
    float dy2 = playerPos.y - a->pos.y;
    float dist2 = sqrt(dx2 * dx2 + dy2 * dy2);

    float grazeThreshold = a->size / 2 + 15;

    bool grazed = false;
    if (invincibleTicks < 0 && dist2 < grazeThreshold) {
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      play(CLICK);
      grazed = true;
      color = PURPLE;
      if (ticks % 3 == 0) {
        float oppositeDir = atan2(dy2, dx2) + M_PI;
        particle(a->pos.x, a->pos.y, 3, 1, oppositeDir, M_PI / 4);
      }
    }

    FOR_EACH(a->trail, ti2) {
      ASSIGN_ARRAY_ITEM(a->trail, ti2, OrbitGrazerTrailPoint, tp2);
      SKIP_IS_NOT_ALIVE(tp2);
      float alpha2 = tp2->life / 30.0;
      color = LIGHT_PURPLE;
      box(tp2->pos.x, tp2->pos.y, a->size * 0.7 * alpha2, a->size * 0.7 * alpha2);
    }

    if (grazed) {
      color = RED;
    } else {
      color = PURPLE;
    }
    float drawSize = a->size * breathScale;
    thickness = drawSize * 0.7;
    Collision barCol = bar(a->pos.x, a->pos.y, drawSize, a->rotation);

    if (barCol.isColliding.rect[BLUE]) {
      color = PURPLE;
      particle(a->pos.x, a->pos.y, 15, 2, 0, M_PI * 2);
      a->isAlive = false;
      continue;
    }
    if (barCol.isColliding.rect[CYAN]) {
      play(EXPLOSION);
      color = RED;
      particle(a->pos.x, a->pos.y, 20, 3, 0, M_PI * 2);
      orbitRadius = clamp(orbitRadius - 20, 0, 40);
      invincibleTicks = 60;
    }

    if (a->pos.x < -10 || a->pos.x > 110 || a->pos.y < -10 || a->pos.y > 110) {
      a->isAlive = false;
    }
  }
}

void addGameOrbitGrazer() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
