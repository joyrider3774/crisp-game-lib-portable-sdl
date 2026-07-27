#include "cglp.h"

static char *title = "SWINGBY";
static char *description = "[Hold]\n Turn right";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 70, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector screenPos;
  float radius;
  bool isAlive;
} Star;
#define SWINGBY_MAX_STAR_COUNT 64
static Star stars[SWINGBY_MAX_STAR_COUNT];
static int starIndex;

static Vector starAddPos;

typedef struct {
  Vector pos;
  Vector vel;
} Ship;
static Ship ship;
static float hitCount;
static Vector shipScreenPos;

typedef struct {
  Vector pos;
  float velRatio;
} BackStar;
#define SWINGBY_BACK_STAR_COUNT 30
static BackStar backStars[SWINGBY_BACK_STAR_COUNT];

static void addStar(float cx, float cy, float r, float af, float at) {
  TIMES(99, i) {
    Vector pos;
    vectorSet(&pos, cx, cy);
    addWithAngle(&pos, rnd(af, at), rnd(0, r));
    float radius = rnd(5, 15);
    bool hasSpace = true;
    FOR_EACH(stars, j) {
      ASSIGN_ARRAY_ITEM(stars, j, Star, s);
      SKIP_IS_NOT_ALIVE(s);
      if (hasSpace && distanceTo(&s->pos, VEC_XY(pos)) < s->radius + radius + 30) {
        hasSpace = false;
      }
    }
    if (hasSpace) {
      ASSIGN_ARRAY_ITEM(stars, starIndex, Star, ns);
      ns->pos = pos;
      vectorSet(&ns->screenPos, 0, 0);
      ns->radius = radius;
      ns->isAlive = true;
      starIndex = wrap(starIndex + 1, 0, SWINGBY_MAX_STAR_COUNT);
      break;
    }
  }
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(stars);
    starIndex = 0;
    ASSIGN_ARRAY_ITEM(stars, starIndex, Star, placeholder);
    vectorSet(&placeholder->pos, 0, 0);
    vectorSet(&placeholder->screenPos, 0, 0);
    placeholder->radius = 20;
    placeholder->isAlive = true;
    int placeholderIndex = starIndex;
    starIndex = wrap(starIndex + 1, 0, SWINGBY_MAX_STAR_COUNT);
    vectorSet(&starAddPos, 0, 0);
    TIMES(20, i) { addStar(starAddPos.x, starAddPos.y, 150, -M_PI, M_PI); }
    stars[placeholderIndex].isAlive = false;
    vectorSet(&ship.pos, 0, 0);
    vectorSet(&ship.vel, 0, 0);
    hitCount = 0;
    vectorSet(&shipScreenPos, 50, 50);
    TIMES(SWINGBY_BACK_STAR_COUNT, i) {
      vectorSet(&backStars[i].pos, rnd(0, 99), rnd(0, 99));
      backStars[i].velRatio = -rnd(0.05, 0.2);
    }
  }
  color = LIGHT_BLACK;
  TIMES(SWINGBY_BACK_STAR_COUNT, i) {
    BackStar *s = &backStars[i];
    vectorAdd(&s->pos, ship.vel.x * s->velRatio, ship.vel.y * s->velRatio);
    if (s->pos.x < 0 || s->pos.x > 99) {
      s->pos.y = rnd(0, 99);
    }
    if (s->pos.y < 0 || s->pos.y > 99) {
      s->pos.x = rnd(0, 99);
    }
    s->pos.x = wrap(s->pos.x, 0, 100);
    s->pos.y = wrap(s->pos.y, 0, 100);
    rect(VEC_XY(s->pos), 1, 1);
  }
  Vector shipBoxPos;
  vectorSet(&shipBoxPos, ship.pos.x, ship.pos.y);
  vectorMul(&shipBoxPos, -1);
  vectorAdd(&shipBoxPos, shipScreenPos.x, shipScreenPos.y);
  shipBoxPos.x = clamp(shipBoxPos.x, -3, 103);
  shipBoxPos.y = clamp(shipBoxPos.y, -3, 103);
  box(VEC_XY(shipBoxPos), 10, 10);
  color = BLACK;
  FOR_EACH(stars, i) {
    ASSIGN_ARRAY_ITEM(stars, i, Star, s);
    SKIP_IS_NOT_ALIVE(s);
    float d = distanceTo(&s->pos, VEC_XY(ship.pos));
    float r = s->radius;
    float ang = angleTo(&ship.pos, VEC_XY(s->pos));
    addWithAngle(&ship.vel, ang, (difficulty * r * 0.01) / clamp(d - r, 2, 99));
    if (d < 99 + r) {
      vectorSet(&s->screenPos, s->pos.x, s->pos.y);
      vectorAdd(&s->screenPos, -ship.pos.x, -ship.pos.y);
      vectorAdd(&s->screenPos, shipScreenPos.x, shipScreenPos.y);
      thickness = 5;
      arc(s->screenPos.x, s->screenPos.y, r - 2, 0, M_PI * 2);
    } else {
      vectorSet(&s->screenPos, 999, 999);
    }
    s->isAlive = d <= 150;
  }
  if (distanceTo(&starAddPos, VEC_XY(ship.pos)) > 50) {
    float a = angleTo(&starAddPos, VEC_XY(ship.pos));
    starAddPos = ship.pos;
    TIMES(5, i) {
      Vector spawnCenter;
      vectorSet(&spawnCenter, starAddPos.x, starAddPos.y);
      addWithAngle(&spawnCenter, a, 100);
      addStar(spawnCenter.x, spawnCenter.y, 70, a - M_PI / 2, a + M_PI / 2);
    }
  }
  vectorMul(&ship.vel, 1 - 0.01 / difficulty);
  vectorSet(&shipScreenPos, 50, 50);
  addWithAngle(&shipScreenPos, vectorAngle(&ship.vel) + M_PI,
               clamp(sqrt(vectorLength(&ship.vel)) * 19, 1, 30));
  if (input.isPressed) {
    play(LASER);
    float a = vectorAngle(&ship.vel) + M_PI / 2;
    addWithAngle(&ship.vel, a, difficulty * 0.05);
    particle(shipScreenPos.x, shipScreenPos.y, 1, 2, a + M_PI, M_PI / 8);
  }
  float a2 = vectorAngle(&ship.vel);
  addWithAngle(&ship.vel, a2, difficulty * 0.01);
  particle(shipScreenPos.x, shipScreenPos.y, 0.5, 1, a2 + M_PI, M_PI / 8);
  vectorAdd(&ship.pos, ship.vel.x, ship.vel.y);
  thickness = 3;
  barCenterPosRatio = 0.5;
  Collision bc = bar(shipScreenPos.x, shipScreenPos.y, 3, vectorAngle(&ship.vel));
  if (bc.isColliding.rect[BLACK]) {
    play(HIT);
    vectorMul(&ship.vel, 1 - 0.01 * difficulty);
    hitCount += 4 * difficulty;
    particle(shipScreenPos.x, shipScreenPos.y, 3, hitCount * 0.1, 0, M_PI * 2);
    if (hitCount > 99) {
      play(EXPLOSION);
      gameOver();
    }
  } else {
    if (hitCount > 0) {
      hitCount -= difficulty;
    }
  }
  score = floor(vectorLength(&ship.pos));
}

void addGameSwingby() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
