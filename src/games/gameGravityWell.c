#include "cglp.h"

static char *title = "GRAVITY WELL";
static char *description = "[Tap] Anti Gravity Pulse";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector velocity;
  float radius;
} Planet;
static Planet planet;
static float nextScoreAddingDist;

typedef struct {
  Vector pos;
  float radius;
  float strength;
  bool isAlive;
} BlackHole;
#define GRAVITYWELL_MAX_BLACKHOLE_COUNT 16
static BlackHole blackHoles[GRAVITYWELL_MAX_BLACKHOLE_COUNT];
static int blackHoleIndex;
static float nextBlackHoleDist;

typedef struct {
  Vector pos;
  float radius;
  float strength;
  bool isAlive;
} Pulse;
#define GRAVITYWELL_MAX_PULSE_COUNT 32
static Pulse pulses[GRAVITYWELL_MAX_PULSE_COUNT];
static int pulseIndex;

typedef struct {
  Vector pos;
  float vx;
} Star;
#define GRAVITYWELL_STAR_COUNT 20
static Star stars[GRAVITYWELL_STAR_COUNT];

static Vector scrollingSpeed;

static void update() {
  if (!ticks) {
    vectorSet(&planet.pos, 50, 50);
    vectorSet(&planet.velocity, 0, 0);
    planet.radius = 9;
    nextScoreAddingDist = 0;
    INIT_UNALIVED_ARRAY(blackHoles);
    blackHoleIndex = 0;
    nextBlackHoleDist = 0;
    INIT_UNALIVED_ARRAY(pulses);
    pulseIndex = 0;
    TIMES(GRAVITYWELL_STAR_COUNT, i) {
      vectorSet(&stars[i].pos, rnd(0, 100), rnd(0, 100));
      stars[i].vx = -rnd(0.1, 0.2);
    }
    vectorSet(&scrollingSpeed, 0, 0);
  }
  scrollingSpeed.x = -0.5 * sqrt(difficulty);
  nextScoreAddingDist += scrollingSpeed.x;
  if (nextScoreAddingDist < 0) {
    addScore(floor(planet.radius), VEC_XY(planet.pos));
    nextScoreAddingDist += 30;
  }
  color = BLACK;
  TIMES(GRAVITYWELL_STAR_COUNT, i) {
    Star *star = &stars[i];
    star->pos.x += star->vx;
    if (star->pos.x < 0) {
      star->pos.x += 100;
    }
    box(VEC_XY(star->pos), 1, 1);
  }
  FOR_EACH(blackHoles, i) {
    ASSIGN_ARRAY_ITEM(blackHoles, i, BlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    vectorAdd(&bh->pos, scrollingSpeed.x, scrollingSpeed.y);
  }
  nextBlackHoleDist += scrollingSpeed.x;
  if (nextBlackHoleDist < 0) {
    float radius = rnd(5, 9);
    ASSIGN_ARRAY_ITEM(blackHoles, blackHoleIndex, BlackHole, nbh);
    vectorSet(&nbh->pos, 100 + radius, rnd(10, 90));
    nbh->radius = radius;
    nbh->strength = 0.1;
    nbh->isAlive = true;
    blackHoleIndex = wrap(blackHoleIndex + 1, 0, GRAVITYWELL_MAX_BLACKHOLE_COUNT);
    nextBlackHoleDist += rnd(30, 40);
  }
  FOR_EACH(blackHoles, i) {
    ASSIGN_ARRAY_ITEM(blackHoles, i, BlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    if (bh->pos.x < -10) {
      bh->isAlive = false;
    }
  }
  vectorAdd(&planet.pos, planet.velocity.x, planet.velocity.y);
  float o = planet.pos.x + 10;
  planet.velocity.x += 1 / o / o;
  o = 110 - planet.pos.x;
  planet.velocity.x -= 1 / o / o;
  o = planet.pos.y + 10;
  planet.velocity.y += 1 / o / o;
  o = 105 - planet.pos.y;
  planet.velocity.y -= 1 / o / o;
  vectorMul(&planet.velocity, 0.99);
  FOR_EACH(blackHoles, i) {
    ASSIGN_ARRAY_ITEM(blackHoles, i, BlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    float dx = bh->pos.x - planet.pos.x;
    float dy = bh->pos.y - planet.pos.y;
    float distance = sqrt(dx * dx + dy * dy);
    if (distance > 0) {
      float force = bh->strength / distance;
      vectorAdd(&planet.velocity, dx / distance * force, dy / distance * force);
    }
  }
  if (input.isJustPressed && planet.radius > 2) {
    play(LASER);
    planet.radius -= 1;
    ASSIGN_ARRAY_ITEM(pulses, pulseIndex, Pulse, np);
    np->pos = planet.pos;
    np->radius = 0;
    np->strength = 0.5;
    np->isAlive = true;
    pulseIndex = wrap(pulseIndex + 1, 0, GRAVITYWELL_MAX_PULSE_COUNT);
  }
  FOR_EACH(pulses, i) {
    ASSIGN_ARRAY_ITEM(pulses, i, Pulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    pulse->pos = planet.pos;
    pulse->radius += 1;
    FOR_EACH(blackHoles, j) {
      ASSIGN_ARRAY_ITEM(blackHoles, j, BlackHole, bh);
      SKIP_IS_NOT_ALIVE(bh);
      float dx = bh->pos.x - pulse->pos.x;
      float dy = bh->pos.y - pulse->pos.y;
      float distance = sqrt(dx * dx + dy * dy);
      if (distance > 0 && distance < pulse->radius + bh->radius) {
        float force = pulse->strength / sqrt(distance);
        float ux = dx / distance;
        float uy = dy / distance;
        vectorAdd(&planet.velocity, -ux * force, -uy * force);
        vectorAdd(&bh->pos, ux * force, uy * force);
      }
    }
  }
  FOR_EACH(pulses, i) {
    ASSIGN_ARRAY_ITEM(pulses, i, Pulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    if (pulse->radius > 20) {
      pulse->isAlive = false;
    }
  }
  FOR_EACH(blackHoles, i) {
    ASSIGN_ARRAY_ITEM(blackHoles, i, BlackHole, bh);
    SKIP_IS_NOT_ALIVE(bh);
    color = WHITE;
    box(VEC_XY(bh->pos), bh->radius * 2, bh->radius * 2);
    color = PURPLE;
    thickness = 3;
    arc(VEC_XY(bh->pos), bh->radius, 0, M_PI * 2);
  }
  color = CYAN;
  FOR_EACH(pulses, i) {
    ASSIGN_ARRAY_ITEM(pulses, i, Pulse, pulse);
    SKIP_IS_NOT_ALIVE(pulse);
    thickness = 3;
    arc(VEC_XY(pulse->pos), pulse->radius, 0, M_PI * 2);
  }
  color = YELLOW;
  thickness = 3;
  Collision c = arc(VEC_XY(planet.pos), planet.radius, 0, M_PI * 2);
  if (c.isColliding.rect[PURPLE]) {
    play(HIT);
    planet.radius -= 0.2;
  } else {
    planet.radius = clamp(planet.radius + 0.05, 1, 9);
  }
  if (planet.radius < 1) {
    play(EXPLOSION);
    gameOver();
  }
  if ((planet.pos.x < 0 && planet.velocity.x < 0) ||
      (planet.pos.x > 100 && planet.velocity.x > 0)) {
    planet.velocity.x *= -1;
  }
  if ((planet.pos.y < 0 && planet.velocity.y < 0) ||
      (planet.pos.y > 100 && planet.velocity.y > 0)) {
    planet.velocity.y *= -1;
  }
}

void addGameGravityWell() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
