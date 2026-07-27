#include "cglp.h"

static char *title = "M JAMMING";
static char *description = "[Hold]\n Expand range\n[Release]\n Jamming";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "bblb  ",
    "b l b ",
    "  b   ",
    "bb bb ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
  float radius;
} MjammingRobot;
static MjammingRobot robot;

typedef struct {
  Vector pos;
  float angle;
  float angleVel;
  float speed;
  float explosionRadius;
  bool isAlive;
} MjammingMissile;
// 64->1024: spawn burst is floor(rnd(6,9)*difficulty) every ~65 ticks (uncapped
// linear growth) while lifetime only shrinks like 1/sqrt(difficulty); simulation
// of the spawn/homing/bounds-exit logic shows concurrent count already exceeds
// 64 by ~difficulty 6 (under 5 min in) and reaches ~400+ by difficulty 100.
#define MJAMMING_MAX_MISSILE_COUNT 1024
static MjammingMissile missiles[MJAMMING_MAX_MISSILE_COUNT];
static int missileIndex;
static float nextMissileTicks;

typedef struct {
  Vector pos;
  float radius;
  float targetRadius;
  float vr;
  bool isAlive;
} MjammingExplosion;
#define MJAMMING_MAX_EXPLOSION_COUNT 16
static MjammingExplosion explosions[MJAMMING_MAX_EXPLOSION_COUNT];
static int explosionIndex;

typedef struct {
  Vector pos;
  float spRatio;
  int color;
} MjammingStar;
#define MJAMMING_STAR_COUNT 30
static MjammingStar stars[MJAMMING_STAR_COUNT];

static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&robot.pos, 50, 50);
    vectorSet(&robot.vel, 0, 0);
    robot.radius = 0;
    INIT_UNALIVED_ARRAY(missiles);
    missileIndex = 0;
    nextMissileTicks = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    int starColors[3];
    starColors[0] = LIGHT_CYAN;
    starColors[1] = LIGHT_PURPLE;
    starColors[2] = LIGHT_YELLOW;
    TIMES(MJAMMING_STAR_COUNT, i) {
      vectorSet(&stars[i].pos, rnd(0, 99), rnd(0, 99));
      stars[i].spRatio = rnd(0.2, 0.3);
      stars[i].color = starColors[rndi(0, 3)];
    }
    multiplier = 1;
  }
  Vector scr;
  vectorSet(&scr, 50 - robot.pos.x, 50 - robot.pos.y);
  vectorMul(&scr, 0.05 * sqrt(difficulty));
  TIMES(MJAMMING_STAR_COUNT, i) {
    MjammingStar *s = &stars[i];
    s->pos.x += scr.x * s->spRatio;
    s->pos.y += scr.y * s->spRatio;
    s->pos.x = wrap(s->pos.x, 0, 99);
    s->pos.y = wrap(s->pos.y, 0, 99);
    color = s->color;
    rect(s->pos.x, s->pos.y, 1, 1);
  }
  color = RED;
  FOR_EACH(explosions, i) {
    ASSIGN_ARRAY_ITEM(explosions, i, MjammingExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->radius += sqrt(difficulty) * e->vr * 0.5;
    if (e->radius > e->targetRadius && e->vr > 0) {
      e->vr = -1;
    }
    thickness = 5;
    arc(e->pos.x, e->pos.y, e->radius, 0, M_PI * 2);
    if (e->radius < 1) {
      e->isAlive = false;
      continue;
    }
  }
  vectorMul(&robot.vel, 0.98);
  vectorAdd(&robot.pos, robot.vel.x, robot.vel.y);
  vectorAdd(&robot.pos, scr.x, scr.y);
  color = BLACK;
  if (robot.vel.x < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  character("a", VEC_XY(robot.pos));
  characterOptions.isMirrorX = false;
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed || input.isJustReleased) {
    robot.radius *= 1 + 0.01 * sqrt(difficulty);
    vectorMul(&robot.vel, 0.9);
    color = CYAN;
  } else {
    robot.radius = clamp(robot.radius + sqrt(difficulty) * 0.5, 0, 15);
    color = LIGHT_CYAN;
  }
  thickness = 2;
  arc(robot.pos.x, robot.pos.y, robot.radius, 0, M_PI * 2);
  if (input.isJustReleased || robot.radius > 50) {
    play(LASER);
    multiplier = 1;
    int mc = 0;
    FOR_EACH(missiles, i) {
      ASSIGN_ARRAY_ITEM(missiles, i, MjammingMissile, m);
      SKIP_IS_NOT_ALIVE(m);
      if (distanceTo(&m->pos, VEC_XY(robot.pos)) < robot.radius + 1 &&
          m->explosionRadius < 0) {
        mc++;
      }
    }
    FOR_EACH(missiles, i) {
      ASSIGN_ARRAY_ITEM(missiles, i, MjammingMissile, m);
      SKIP_IS_NOT_ALIVE(m);
      if (distanceTo(&m->pos, VEC_XY(robot.pos)) < robot.radius + 1 &&
          m->explosionRadius < 0) {
        m->angle = angleTo(&robot.pos, VEC_XY(m->pos));
        m->explosionRadius = clamp(mc * sqrt(mc), 1, 25);
        m->angleVel = 0;
        m->speed /= 2;
        addScore(multiplier, VEC_XY(m->pos));
        multiplier++;
      }
    }
    robot.radius = mc;
  }
  nextMissileTicks--;
  if (nextMissileTicks < 0) {
    int c = (int)floor(rnd(6, 9) * difficulty);
    TIMES(c, k) {
      Vector pos;
      vectorSet(&pos, 50, 50);
      addWithAngle(&pos, rnd(0, M_PI * 2), 99);
      float angle = angleTo(&pos, rnd(40, 60), rnd(40, 60));
      float angleVel = rnd(1, sqrt(difficulty)) * 0.005;
      float speed = sqrt(difficulty) * 0.4;
      ASSIGN_ARRAY_ITEM(missiles, missileIndex, MjammingMissile, nm);
      nm->pos = pos;
      nm->angle = angle;
      nm->angleVel = angleVel;
      nm->speed = speed;
      nm->explosionRadius = -1;
      nm->isAlive = true;
      missileIndex = wrap(missileIndex + 1, 0, MJAMMING_MAX_MISSILE_COUNT);
    }
    nextMissileTicks = rnd(60, 70);
  }
  bool hasNearest = false;
  Vector nearestPos;
  float nmDist = 50;
  FOR_EACH(missiles, i) {
    ASSIGN_ARRAY_ITEM(missiles, i, MjammingMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    float ta = angleTo(&m->pos, VEC_XY(robot.pos));
    float oa = wrap(m->angle - ta, -M_PI, M_PI);
    if (oa > m->angleVel) {
      m->angle -= m->angleVel;
    } else if (oa < -m->angleVel) {
      m->angle += m->angleVel;
    } else {
      m->angle = ta;
    }
    addWithAngle(&m->pos, m->angle, m->speed);
    vectorAdd(&m->pos, scr.x, scr.y);
    if (m->explosionRadius < 0) {
      color = PURPLE;
    } else {
      color = CYAN;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision mc2 = bar(m->pos.x, m->pos.y, 3, m->angle);
    color = RED;
    particle(m->pos.x, m->pos.y, 1, m->speed, m->angle + M_PI, 1);
    if (mc2.isColliding.rect[RED]) {
      if (m->explosionRadius > 1) {
        play(POWER_UP);
        ASSIGN_ARRAY_ITEM(explosions, explosionIndex, MjammingExplosion, ne);
        ne->pos = m->pos;
        ne->radius = 1;
        ne->targetRadius = m->explosionRadius;
        ne->vr = 1;
        ne->isAlive = true;
        explosionIndex = wrap(explosionIndex + 1, 0, MJAMMING_MAX_EXPLOSION_COUNT);
      } else {
        play(COIN);
        particle(m->pos.x, m->pos.y, 9, 2, 0, M_PI * 2);
      }
      addScore(multiplier, VEC_XY(m->pos));
      multiplier++;
      m->isAlive = false;
      continue;
    }
    if (mc2.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    float d = distanceTo(&m->pos, VEC_XY(robot.pos));
    if (d < nmDist) {
      nmDist = d;
      hasNearest = true;
      nearestPos = m->pos;
    }
    bool inRect = m->pos.x >= -100 && m->pos.x < 100 && m->pos.y >= -100 && m->pos.y < 100;
    if (!inRect) {
      m->isAlive = false;
      continue;
    }
  }
  FOR_EACH(missiles, i) {
    ASSIGN_ARRAY_ITEM(missiles, i, MjammingMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->explosionRadius > 0) {
      color = TRANSPARENT;
      thickness = 3;
      barCenterPosRatio = 0.5;
      Collision mc3 = bar(m->pos.x, m->pos.y, 3, m->angle);
      if (mc3.isColliding.rect[PURPLE]) {
        play(POWER_UP);
        ASSIGN_ARRAY_ITEM(explosions, explosionIndex, MjammingExplosion, ne2);
        ne2->pos = m->pos;
        ne2->radius = 1;
        ne2->targetRadius = m->explosionRadius;
        ne2->vr = 1;
        ne2->isAlive = true;
        explosionIndex = wrap(explosionIndex + 1, 0, MJAMMING_MAX_EXPLOSION_COUNT);
        color = RED;
        particle(m->pos.x, m->pos.y, 9, 2, 0, M_PI * 2);
        addScore(multiplier, VEC_XY(m->pos));
        multiplier++;
        m->isAlive = false;
        continue;
      }
    }
  }
  if (hasNearest) {
    addWithAngle(&robot.vel,
                 angleTo(&nearestPos, VEC_XY(robot.pos)) + M_PI / 3,
                 sqrt(difficulty) * 0.01);
  }
}

void addGameMJamming() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
