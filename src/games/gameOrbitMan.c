#include "cglp.h"

static char *title = "ORBIT MAN";
static char *description = "[Tap] Launch";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 600, .isDarkColor = true};

typedef struct {
  Vector pos;
  float radius;
  bool isDestroyed;
  bool isAlive;
} OrbitManPlanet;
#define ORBITMAN_MAX_PLANET_COUNT 64
static OrbitManPlanet planets[ORBITMAN_MAX_PLANET_COUNT];
static int planetIndex;
static float nextPlanetDist;

typedef struct {
  int planetIndex;
  float angle;
  float av;
  Vector pos;
  Vector target;
} OrbitManMan;
static OrbitManMan man;
static float flyingTicks;
static float multiplier;

typedef struct {
  Vector pos;
  float vy;
} OrbitManStar;
#define ORBITMAN_STAR_COUNT 20
static OrbitManStar stars[ORBITMAN_STAR_COUNT];

#define ORBITMAN_MAX_PIERCED_COUNT 16

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(planets);
    planetIndex = 0;
    ASSIGN_ARRAY_ITEM(planets, planetIndex, OrbitManPlanet, p0);
    vectorSet(&p0->pos, 50, 0);
    p0->radius = 5;
    p0->isDestroyed = false;
    p0->isAlive = true;
    int planet0Index = planetIndex;
    planetIndex = wrap(planetIndex + 1, 0, ORBITMAN_MAX_PLANET_COUNT);
    nextPlanetDist = 20;
    man.planetIndex = planet0Index;
    man.angle = M_PI / 2;
    man.av = 1;
    vectorSet(&man.pos, 50, 0);
    vectorSet(&man.target, 50, 0);
    flyingTicks = 0;
    multiplier = 1;
    TIMES(ORBITMAN_STAR_COUNT, i) {
      vectorSet(&stars[i].pos, rnd(0, 99), rnd(0, 99));
      stars[i].vy = rnd(3, 6);
    }
  }
  float scr = sqrt(difficulty) * 0.05;
  flyingTicks = clamp(flyingTicks - difficulty, 1, 99);
  OrbitManPlanet *manPlanet = &planets[man.planetIndex];
  if (manPlanet->pos.y < 80) {
    scr += (80 - manPlanet->pos.y) * (0.1 / flyingTicks);
  }
  color = LIGHT_BLACK;
  TIMES(ORBITMAN_STAR_COUNT, i) {
    OrbitManStar *s = &stars[i];
    s->pos.y += scr / s->vy;
    if (s->pos.y > 99) {
      vectorSet(&s->pos, rnd(0, 99), 0);
      s->vy = rnd(3, 6);
    }
    rect(s->pos.x, s->pos.y, 1, 1);
  }
  nextPlanetDist -= scr;
  while (nextPlanetDist < 0) {
    float radius = rnd(4, 9);
    ASSIGN_ARRAY_ITEM(planets, planetIndex, OrbitManPlanet, np);
    vectorSet(&np->pos, rnd(10 + radius, 90 - radius), nextPlanetDist - 30);
    np->radius = radius;
    np->isDestroyed = false;
    np->isAlive = true;
    planetIndex = wrap(planetIndex + 1, 0, ORBITMAN_MAX_PLANET_COUNT);
    nextPlanetDist += radius * rnd(1, 2);
  }
  man.angle += difficulty * 0.03 * man.av;
  color = LIGHT_BLUE;
  thickness = 4;
  barCenterPosRatio = -manPlanet->radius * 0.015;
  bar(manPlanet->pos.x, manPlanet->pos.y, 99, man.angle);
  color = BLACK;
  int nextPlanetIndex = -1;
  float maxDist = 0;
  int piercedPlanetIndices[ORBITMAN_MAX_PIERCED_COUNT];
  int piercedCount = 0;
  piercedPlanetIndices[piercedCount] = man.planetIndex;
  piercedCount++;
  FOR_EACH(planets, i) {
    ASSIGN_ARRAY_ITEM(planets, i, OrbitManPlanet, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->isDestroyed) {
      particle(p->pos.x, p->pos.y, ceil(p->radius * 4), sqrt(p->radius) * 0.5, 0, M_PI * 2);
      p->isAlive = false;
      continue;
    }
    p->pos.y += scr;
    thickness = 3;
    Collision pc = arc(p->pos.x, p->pos.y, p->radius, 0, M_PI * 2);
    if (i != man.planetIndex && pc.isColliding.rect[BLACK]) {
      p->isAlive = false;
      continue;
    }
    if (i != man.planetIndex && p->pos.y > -p->radius - 4 && pc.isColliding.rect[LIGHT_BLUE]) {
      if (piercedCount < ORBITMAN_MAX_PIERCED_COUNT) {
        piercedPlanetIndices[piercedCount] = i;
        piercedCount++;
      }
      float d = distanceTo(&p->pos, manPlanet->pos.x, manPlanet->pos.y);
      if (d > maxDist) {
        nextPlanetIndex = i;
        maxDist = d;
      }
    }
    p->isAlive = p->pos.y <= 100 + p->radius * 2;
  }
  if (input.isJustPressed) {
    if (nextPlanetIndex == -1) {
      play(EXPLOSION);
      TIMES(99, i) {
        addWithAngle(&man.pos, man.angle, 3);
        bool posInRect = man.pos.x >= 5 && man.pos.x < 100 && man.pos.y >= 5 && man.pos.y < 100;
        if (!posInRect) {
          break;
        }
      }
      gameOver();
    } else {
      play(POWER_UP);
      if (multiplier > 1) {
        multiplier--;
      }
      if (piercedCount > 2) {
        play(HIT);
      }
      TIMES(piercedCount, i) {
        if (piercedPlanetIndices[i] != nextPlanetIndex) {
          OrbitManPlanet *pp = &planets[piercedPlanetIndices[i]];
          pp->isDestroyed = true;
          addScore(multiplier, pp->pos.x, pp->pos.y);
          multiplier++;
        }
      }
      man.planetIndex = nextPlanetIndex;
      man.angle += M_PI;
      man.av *= -1;
      flyingTicks = 20;
    }
  }
  manPlanet = &planets[man.planetIndex];
  float a = man.angle;
  vectorSet(&man.target, manPlanet->pos.x, manPlanet->pos.y);
  addWithAngle(&man.target, a, manPlanet->radius);
  Vector delta;
  vectorSet(&delta, man.target.x, man.target.y);
  vectorAdd(&delta, -man.pos.x, -man.pos.y);
  vectorMul(&delta, 0.1);
  vectorAdd(&man.pos, delta.x, delta.y);
  color = CYAN;
  thickness = 2;
  Vector p1;
  vectorSet(&p1, man.pos.x, man.pos.y);
  addWithAngle(&p1, a, 4);
  Vector p2;
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a + M_PI * 0.75, 3);
  line(p1.x, p1.y, p2.x, p2.y);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a - M_PI * 0.75, 3);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a, 4);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y);
  vectorSet(&p2, p1.x, p1.y);
  addWithAngle(&p2, a - M_PI * 0.3, 3);
  addWithAngle(&p1, a + M_PI * 0.3, 3);
  thickness = 2;
  line(p1.x, p1.y, p2.x, p2.y);
  if (manPlanet->pos.y - manPlanet->radius > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameOrbitMan() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
