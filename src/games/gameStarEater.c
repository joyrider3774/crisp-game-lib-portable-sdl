#include "cglp.h"

static char *title = "STAR EATER";
static char *description = "[Hold] Charge\n[Release] Blast";

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
  float size;
  float baseSize;
  float breathPhase;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  float rotation;
  float rotSpeed;
  bool isAlive;
} Asteroid;
// Concurrent count stays ~self-similar across difficulty (spawn interval and
// speed both scale with difficulty in offsetting ways) - measured around
// 4-6 in practice, generous headroom applied.
#define STAREATER_MAX_ASTEROID_COUNT 32
static Asteroid asteroids[STAREATER_MAX_ASTEROID_COUNT];
static int asteroidIndex;

typedef struct {
  float x;
  float y;
  float size;
  float alpha;
  bool isPlayer;
  bool isAlive;
} Trail;
#define STAREATER_MAX_TRAIL_COUNT 32
static Trail trails[STAREATER_MAX_TRAIL_COUNT];
static int trailIndex;

typedef struct {
  Vector pos;
  Vector vel;
  float life;
  int color;
  float size;
  bool isAlive;
} Dust;
// Sized for the worst case: a maxed-charge blast (radius ~63) can catch
// every concurrent asteroid (~6) at once (8 dust each), on top of the
// 16-particle blast burst and a few leftover charge particles.
#define STAREATER_MAX_DUST_COUNT 256
static Dust dust[STAREATER_MAX_DUST_COUNT];
static int dustIndex;

typedef struct {
  bool active;
  Vector pos;
  float radius;
  int timer;
} Explosion;
static Explosion explosion;

static float nextAsteroidTicks;
static float charge;
static int destroyedCount;

static void checkExplosion(float x, float y, float radius, int timer) {
  float phase = 1 - (float)timer / 12;
  float ringRadius = radius * (0.3 + phase * 0.7);

  color = LIGHT_YELLOW;
  thickness = 3 - (int)floor(phase * 2);
  arc(x, y, ringRadius, 0, M_PI * 2);

  if (timer > 6) {
    color = WHITE;
    thickness = 3;
    arc(x, y, ringRadius * 0.4, 0, M_PI * 2);
  }

  FOR_EACH(asteroids, ai) {
    ASSIGN_ARRAY_ITEM(asteroids, ai, Asteroid, ast);
    SKIP_IS_NOT_ALIVE(ast);
    float ddx = ast->pos.x - x;
    float ddy = ast->pos.y - y;
    float dist = sqrt(ddx * ddx + ddy * ddy);
    if (dist < radius + ast->size / 2) {
      int di;
      for (di = 0; di < 8; di++) {
        float angle = rnd(0, M_PI * 2);
        float spd = rnd(1, 2.5);
        ASSIGN_ARRAY_ITEM(dust, dustIndex, Dust, nd);
        vectorSet(&nd->pos, ast->pos.x, ast->pos.y);
        vectorSet(&nd->vel, cos(angle) * spd, sin(angle) * spd);
        nd->life = rnd(15, 25);
        if (rnd(0, 1) > 0.5) {
          nd->color = RED;
        } else {
          nd->color = LIGHT_RED;
        }
        nd->size = rnd(1, 3);
        nd->isAlive = true;
        dustIndex = wrap(dustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
      }
      destroyedCount++;
      addScore(destroyedCount * destroyedCount, VEC_XY(ast->pos));
      play(POWER_UP);
      ast->isAlive = false;
    }
  }
}

static void update() {
  // Never reads a Collision result - blasts are decided by distance math.
  hasCollision = false;
  if (!ticks) {
    vectorSet(&player.pos, 50, 50);
    player.size = 5;
    player.baseSize = 5;
    player.breathPhase = 0;
    INIT_UNALIVED_ARRAY(asteroids);
    asteroidIndex = 0;
    nextAsteroidTicks = 0;
    charge = 0;
    explosion.active = false;
    vectorSet(&explosion.pos, 0, 0);
    explosion.radius = 0;
    explosion.timer = 0;
    destroyedCount = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    INIT_UNALIVED_ARRAY(dust);
    dustIndex = 0;
  }

  FOR_EACH(trails, tri) {
    ASSIGN_ARRAY_ITEM(trails, tri, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->alpha -= 0.08;
    if (t->alpha <= 0) {
      t->isAlive = false;
    }
  }

  FOR_EACH(dust, dsi) {
    ASSIGN_ARRAY_ITEM(dust, dsi, Dust, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x += p->vel.x;
    p->pos.y += p->vel.y;
    vectorMul(&p->vel, 0.95);
    p->life -= 1;
    if (p->life <= 0) {
      p->isAlive = false;
    }
  }

  nextAsteroidTicks--;
  if (nextAsteroidTicks <= 0) {
    int side = rndi(0, 4);
    Vector pos;
    Vector vel;
    if (side == 0) {
      vectorSet(&pos, rnd(0, 100), 0);
      vectorSet(&vel, rnd(-0.3, 0.3), 0.5 + rnd(0, difficulty * 0.2));
    } else if (side == 1) {
      vectorSet(&pos, rnd(0, 100), 100);
      vectorSet(&vel, rnd(-0.3, 0.3), -0.5 - rnd(0, difficulty * 0.2));
    } else if (side == 2) {
      vectorSet(&pos, 0, rnd(0, 100));
      vectorSet(&vel, 0.5 + rnd(0, difficulty * 0.2), rnd(-0.3, 0.3));
    } else {
      vectorSet(&pos, 100, rnd(0, 100));
      vectorSet(&vel, -0.5 - rnd(0, difficulty * 0.2), rnd(-0.3, 0.3));
    }
    float d1 = distanceTo(&pos, VEC_XY(player.pos));
    float d2 = distanceTo(&pos, player.pos.x - 100, player.pos.y);
    if (d1 > 50 && d2 > 50) {
      vectorMul(&vel, sqrt(difficulty));
      ASSIGN_ARRAY_ITEM(asteroids, asteroidIndex, Asteroid, na);
      na->pos = pos;
      na->vel = vel;
      na->size = rnd(5, 9);
      na->rotation = rnd(0, M_PI * 2);
      na->rotSpeed = rnd(-0.1, 0.1);
      na->isAlive = true;
      asteroidIndex = wrap(asteroidIndex + 1, 0, STAREATER_MAX_ASTEROID_COUNT);
      nextAsteroidTicks = rnd(22, 55) / difficulty;
    }
  }

  float speed;
  if (input.isPressed) {
    speed = 1.8 * sqrt(difficulty);
  } else {
    speed = 1 * sqrt(difficulty);
  }
  player.pos.x += 0.2 * speed;
  if (player.pos.x > 100) {
    player.pos.x = 0;
  }
  player.pos.y += sin(ticks * 0.02 * speed) * 0.1;
  player.pos.y = clamp(player.pos.y, 5, 95);

  player.breathPhase += 0.08;
  float breathScale = 1 + sin(player.breathPhase) * 0.05;

  if (ticks % 3 == 0) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    nt->x = player.pos.x;
    nt->y = player.pos.y;
    nt->size = player.size * 0.7;
    nt->alpha = 0.5;
    nt->isPlayer = true;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, STAREATER_MAX_TRAIL_COUNT);
  }

  if (input.isPressed) {
    charge = fmin(charge + difficulty, 60);
    player.size = player.baseSize + charge * 0.15;
    breathScale = 1 + sin(ticks * 0.3) * 0.1;

    if (ticks % 4 == 0) {
      float angle = rnd(0, M_PI * 2);
      float dist = player.size + 10 + rnd(0, 5);
      ASSIGN_ARRAY_ITEM(dust, dustIndex, Dust, cd);
      vectorSet(&cd->pos, player.pos.x + cos(angle) * dist, player.pos.y + sin(angle) * dist);
      vectorSet(&cd->vel, cos(angle + M_PI) * 0.8, sin(angle + M_PI) * 0.8);
      cd->life = 15;
      cd->color = YELLOW;
      cd->size = rnd(1, 2);
      cd->isAlive = true;
      dustIndex = wrap(dustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
    }
  }

  if (input.isJustReleased && charge > 10) {
    play(EXPLOSION);
    explosion.active = true;
    vectorSet(&explosion.pos, player.pos.x, player.pos.y);
    explosion.radius = 15 + charge * 0.8;
    explosion.timer = 12;

    int bi;
    for (bi = 0; bi < 16; bi++) {
      float angle = (M_PI * 2 * bi) / 16 + rnd(-0.2, 0.2);
      float spd = rnd(1.5, 3);
      ASSIGN_ARRAY_ITEM(dust, dustIndex, Dust, bd);
      vectorSet(&bd->pos, player.pos.x, player.pos.y);
      vectorSet(&bd->vel, cos(angle) * spd, sin(angle) * spd);
      bd->life = rnd(20, 35);
      bd->color = LIGHT_YELLOW;
      bd->size = rnd(1, 3);
      bd->isAlive = true;
      dustIndex = wrap(dustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
    }

    charge = 0;
    player.size = player.baseSize;
    destroyedCount = 0;
  }

  if (!input.isPressed && charge > 0) {
    charge = fmax(0, charge - 2);
    player.size = player.baseSize + charge * 0.15;
  }

  if (explosion.active) {
    explosion.timer--;
    if (explosion.timer <= 0) {
      explosion.active = false;
    }
  }

  FOR_EACH(asteroids, ai2) {
    ASSIGN_ARRAY_ITEM(asteroids, ai2, Asteroid, ast2);
    SKIP_IS_NOT_ALIVE(ast2);
    ast2->pos.x += ast2->vel.x;
    ast2->pos.y += ast2->vel.y;
    ast2->rotation += ast2->rotSpeed;

    if (ticks % 5 == 0) {
      ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, at);
      at->x = ast2->pos.x;
      at->y = ast2->pos.y;
      at->size = ast2->size * 0.4;
      at->alpha = 0.3;
      at->isPlayer = false;
      at->isAlive = true;
      trailIndex = wrap(trailIndex + 1, 0, STAREATER_MAX_TRAIL_COUNT);
    }

    if (!(ast2->pos.x > -10 && ast2->pos.x < 110 && ast2->pos.y > -10 && ast2->pos.y < 110)) {
      ast2->isAlive = false;
    }
  }

  FOR_EACH(trails, tri2) {
    ASSIGN_ARRAY_ITEM(trails, tri2, Trail, t2);
    SKIP_IS_NOT_ALIVE(t2);
    if (t2->isPlayer) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_RED;
    }
    int a = (int)floor(t2->alpha * 3);
    if (a > 0) {
      thickness = a;
      arc(t2->x, t2->y, t2->size, 0, M_PI * 2);
    }
  }

  FOR_EACH(dust, dsi2) {
    ASSIGN_ARRAY_ITEM(dust, dsi2, Dust, p2);
    SKIP_IS_NOT_ALIVE(p2);
    color = p2->color;
    box(p2->pos.x, p2->pos.y, p2->size, p2->size);
  }

  if (explosion.active) {
    checkExplosion(explosion.pos.x, explosion.pos.y, explosion.radius, explosion.timer);
    checkExplosion(explosion.pos.x - 100, explosion.pos.y, explosion.radius, explosion.timer);
  }

  if (charge > 0) {
    color = YELLOW;
    thickness = 1;
    float pulseSize = player.size + 2 + sin(ticks * 0.4) * 1.5;
    arc(player.pos.x, player.pos.y, pulseSize, 0, M_PI * 2);
  }

  color = CYAN;
  thickness = 3;
  float drawSize = player.size * breathScale;
  arc(player.pos.x, player.pos.y, drawSize, 0, M_PI * 2);

  if (charge > 20) {
    color = LIGHT_CYAN;
    thickness = 3;
    arc(player.pos.x, player.pos.y, drawSize * 0.5, 0, M_PI * 2);
  }

  color = RED;
  FOR_EACH(asteroids, ai3) {
    ASSIGN_ARRAY_ITEM(asteroids, ai3, Asteroid, ast3);
    SKIP_IS_NOT_ALIVE(ast3);
    float len = ast3->size * 0.8;
    thickness = ast3->size * 0.6;
    bar(ast3->pos.x, ast3->pos.y, len, ast3->rotation);
    thickness = ast3->size * 0.6;
    bar(ast3->pos.x, ast3->pos.y, len, ast3->rotation + M_PI / 2);

    float dist = distanceTo(&player.pos, VEC_XY(ast3->pos));
    if (dist < player.size + ast3->size / 2) {
      int di2;
      for (di2 = 0; di2 < 12; di2++) {
        float angle = (M_PI * 2 * di2) / 12;
        ASSIGN_ARRAY_ITEM(dust, dustIndex, Dust, hd);
        vectorSet(&hd->pos, player.pos.x, player.pos.y);
        vectorSet(&hd->vel, cos(angle) * 2, sin(angle) * 2);
        hd->life = 20;
        hd->color = CYAN;
        hd->size = 2;
        hd->isAlive = true;
        dustIndex = wrap(dustIndex + 1, 0, STAREATER_MAX_DUST_COUNT);
      }
      play(HIT);
      gameOver();
    }
  }
}

void addGameStarEater() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
