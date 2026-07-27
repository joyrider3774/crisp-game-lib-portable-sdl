#include "cglp.h"
#include "string.h"

static char *title = "ROCKET TRAIL";
static char *description = "[Hold] Thrust & Destroy";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
  float thrustPower;
  float horizontalDir;
  float angle;
} Rocket;
static Rocket rocket;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
} Star;
#define ROCKETTRAIL_STAR_COUNT 50
static Star stars[ROCKETTRAIL_STAR_COUNT];

typedef struct {
  Vector pos;
  Vector vel;
  int age;
  bool isAlive;
} Flame;
#define ROCKETTRAIL_MAX_FLAME_COUNT 64
static Flame flames[ROCKETTRAIL_MAX_FLAME_COUNT];
static int flameIndex;

typedef struct {
  Vector pos;
  Vector vel;
  int life;
  bool isAlive;
} Fuel;
#define ROCKETTRAIL_MAX_FUEL_COUNT 64
static Fuel fuels[ROCKETTRAIL_MAX_FUEL_COUNT];
static int fuelIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Asteroid;
// Sized well beyond the ~15-20 typical count since spawn interval can go negative at high difficulty.
#define ROCKETTRAIL_MAX_ASTEROID_COUNT 256
static Asteroid asteroids[ROCKETTRAIL_MAX_ASTEROID_COUNT];
static int asteroidIndex;

static float asteroidSpawnTimer;
static float asteroidSpawnInterval;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&rocket.pos, 50, 65);
    vectorSet(&rocket.vel, 0, 0);
    rocket.thrustPower = 0.3;
    rocket.horizontalDir = 1;
    rocket.angle = M_PI_2;
    INIT_UNALIVED_ARRAY(asteroids);
    asteroidIndex = 0;
    INIT_UNALIVED_ARRAY(fuels);
    fuelIndex = 0;
    INIT_UNALIVED_ARRAY(flames);
    flameIndex = 0;
    asteroidSpawnTimer = 0;
    asteroidSpawnInterval = 60;
    multiplier = 1;
    TIMES(ROCKETTRAIL_STAR_COUNT, sti) {
      Star *s = &stars[sti];
      vectorSet(&s->pos, rnd(0, 100), rnd(0, 100));
      vectorSet(&s->vel, 0, rnd(0.5, 2));
      s->size = rnd(0.5, 2);
    }
  }

  rocket.vel.x *= 0.95;
  rocket.vel.y += 0.1;
  rocket.vel.y *= 0.98;

  if (rocket.pos.x <= 45) {
    rocket.horizontalDir = 1;
  } else if (rocket.pos.x >= 55) {
    rocket.horizontalDir = -1;
  }
  rocket.angle += (M_PI_2 + rocket.horizontalDir - rocket.angle) * 0.05;

  if (input.isPressed) {
    addWithAngle(&rocket.vel, rocket.angle, -rocket.thrustPower);
    if (rnd(0, 1) < 0.3) {
      ASSIGN_ARRAY_ITEM(fuels, fuelIndex, Fuel, nf);
      vectorSet(&nf->pos, VEC_XY(rocket.pos));
      vectorSet(&nf->vel, 0, 0);
      addWithAngle(&nf->vel, rocket.angle + rnd(0, 0.5) * RNDPM(), rocket.thrustPower * 6);
      nf->life = 20;
      nf->isAlive = true;
      fuelIndex = wrap(fuelIndex + 1, 0, ROCKETTRAIL_MAX_FUEL_COUNT);
    }
  }
  if (input.isJustPressed) {
    play(EXPLOSION);
  }

  vectorAdd(&rocket.pos, VEC_XY(rocket.vel));

  color = BLACK;
  TIMES(ROCKETTRAIL_STAR_COUNT, sti2) {
    Star *s = &stars[sti2];
    vectorAdd(&s->pos, VEC_XY(s->vel));
    box(VEC_XY(s->pos), s->size, s->size);
    if (s->pos.y > 100) {
      s->pos.y = -5;
      s->pos.x = rnd(0, 100);
      s->vel.y = rnd(0.5, 2);
      s->size = rnd(0.5, 2);
    }
  }

  ASSIGN_ARRAY_ITEM(flames, flameIndex, Flame, newFlame);
  vectorSet(&newFlame->pos, VEC_XY(rocket.pos));
  vectorSet(&newFlame->vel, 0, 0);
  addWithAngle(&newFlame->vel, rocket.angle, rocket.thrustPower * 7);
  newFlame->age = 0;
  newFlame->isAlive = true;
  flameIndex = wrap(flameIndex + 1, 0, ROCKETTRAIL_MAX_FLAME_COUNT);

  rocket.pos.x = clamp(rocket.pos.x, 1, 99);
  if (rocket.pos.y < 0 || rocket.pos.y > 105) {
    play(EXPLOSION);
    gameOver();
  }

  color = RED;
  thickness = 3;
  barCenterPosRatio = 1;
  bar(VEC_XY(rocket.pos), 5, rocket.angle);
  barCenterPosRatio = 0.5;

  color = YELLOW;
  FOR_EACH(flames, fi) {
    ASSIGN_ARRAY_ITEM(flames, fi, Flame, f);
    SKIP_IS_NOT_ALIVE(f);
    f->age++;
    vectorAdd(&f->pos, VEC_XY(f->vel));
    if (f->age < 60) {
      float sz = 4 - ((float)f->age / 60) * 3;
      box(VEC_XY(f->pos), sz, sz);
    }
    if (f->age >= 25) {
      f->isAlive = false;
      continue;
    }
  }

  FOR_EACH(fuels, fuelI) {
    ASSIGN_ARRAY_ITEM(fuels, fuelI, Fuel, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, VEC_XY(p->vel));
    p->life--;
    if (p->life > 0) {
      box(VEC_XY(p->pos), 2, 2);
    }
    if (p->life <= 0) {
      p->isAlive = false;
      continue;
    }
  }

  asteroidSpawnTimer++;
  COUNT_IS_ALIVE(asteroids, aliveAsteroidCount);
  if (aliveAsteroidCount == 0 || asteroidSpawnTimer >= asteroidSpawnInterval) {
    Vector apos;
    vectorSet(&apos, rnd(5, 95), -5);
    Vector avel;
    vectorSet(&avel, 0, 0);
    addWithAngle(&avel, angleTo(&apos, rnd(20, 80), 99), rnd(0.3, 0.5));
    ASSIGN_ARRAY_ITEM(asteroids, asteroidIndex, Asteroid, na);
    na->pos = apos;
    na->vel = avel;
    na->isAlive = true;
    asteroidIndex = wrap(asteroidIndex + 1, 0, ROCKETTRAIL_MAX_ASTEROID_COUNT);
    asteroidSpawnTimer = 0;
    asteroidSpawnInterval = 80 - rnd(0, 30) * RNDPM() * difficulty;
  }

  color = BLACK;
  FOR_EACH(asteroids, ai) {
    ASSIGN_ARRAY_ITEM(asteroids, ai, Asteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    vectorAdd(&a->pos, VEC_XY(a->vel));
    Collision c = box(VEC_XY(a->pos), 3, 3);
    particle(VEC_XY(a->pos), 1, vectorLength(&a->vel), vectorAngle(&a->vel) + M_PI, 1);

    if (c.isColliding.rect[YELLOW]) {
      addScore(multiplier, VEC_XY(a->pos));
      multiplier = (int)clamp(multiplier + 1, 1, 16);
      particle(VEC_XY(a->pos), 7, 2, 0, M_PI * 2);
      play(EXPLOSION);
      a->isAlive = false;
      continue;
    }

    if (c.isColliding.rect[RED]) {
      rocket.vel.y += 2;
      multiplier = 1;
      color = RED;
      particle(VEC_XY(a->pos), 25, 3, -M_PI_2, 1);
      color = BLACK;
      play(EXPLOSION);
      a->isAlive = false;
      continue;
    }

    if (a->pos.y > 105) {
      multiplier = (int)clamp(multiplier - 1, 1, 16);
      a->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameRocketTrail() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
