#include "cglp.h"
#include "string.h"

static char *title = "WAVY BIRD";
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
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

#define WAVYBIRD_GRAVITY 0.1
#define WAVYBIRD_JUMP_FORCE 1.5
#define WAVYBIRD_SHOCKWAVE_SPEED 2
#define WAVYBIRD_MAX_SHOCKWAVE_RADIUS 30
#define WAVYBIRD_PILLAR_WIDTH 8
#define WAVYBIRD_MIN_PILLAR_HEIGHT 20
#define WAVYBIRD_MAX_PILLAR_HEIGHT 60

typedef struct {
  Vector pos;
  Vector vel;
  Vector size;
  float angle;
} Bird;
static Bird bird;

typedef struct {
  Vector pos;
  Vector size;
  bool isAlive;
} Pillar;
#define WAVYBIRD_MAX_PILLAR_COUNT 64
static Pillar pillars[WAVYBIRD_MAX_PILLAR_COUNT];
static int pillarIndex;

typedef struct {
  Vector pos;
  float radius;
  float maxRadius;
  float angle;
  bool isAlive;
} Shockwave;
#define WAVYBIRD_MAX_SHOCKWAVE_COUNT 16
static Shockwave shockwaves[WAVYBIRD_MAX_SHOCKWAVE_COUNT];
static int shockwaveIndex;

static float nextPillarDist;
static float multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&bird.pos, 20, 50);
    vectorSet(&bird.vel, 0, 0);
    vectorSet(&bird.size, 5, 3);
    bird.angle = 0;
    INIT_UNALIVED_ARRAY(pillars);
    pillarIndex = 0;
    nextPillarDist = 0;
    INIT_UNALIVED_ARRAY(shockwaves);
    shockwaveIndex = 0;
    multiplier = 1;
  }

  color = PURPLE;
  FOR_EACH(shockwaves, si) {
    ASSIGN_ARRAY_ITEM(shockwaves, si, Shockwave, s);
    SKIP_IS_NOT_ALIVE(s);
    s->radius += WAVYBIRD_SHOCKWAVE_SPEED * difficulty;
    thickness = 3;
    arc(VEC_XY(s->pos), s->radius, s->angle - 0.3, s->angle + 0.3);
    if (s->radius >= s->maxRadius) {
      s->isAlive = false;
      continue;
    }
  }

  bird.pos.y += bird.vel.y * difficulty;
  bird.vel.y += WAVYBIRD_GRAVITY;
  bird.angle += (1.2 - bird.angle) * 0.02 * difficulty;
  if (input.isJustPressed) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(shockwaves, shockwaveIndex, Shockwave, ns);
    vectorSet(&ns->pos, bird.pos.x, bird.pos.y);
    ns->radius = 0;
    ns->maxRadius = WAVYBIRD_MAX_SHOCKWAVE_RADIUS;
    ns->angle = bird.angle - 0.4 * difficulty;
    ns->isAlive = true;
    shockwaveIndex = wrap(shockwaveIndex + 1, 0, WAVYBIRD_MAX_SHOCKWAVE_COUNT);
    bird.vel.y = -WAVYBIRD_JUMP_FORCE;
    bird.angle -= 0.6 * difficulty;
  }
  bird.angle = clamp(bird.angle, -1.2, 1.2);
  if (bird.pos.y < 0 || bird.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  color = RED;
  thickness = bird.size.y;
  bar(bird.pos.x, bird.pos.y, bird.size.x, bird.angle);

  color = CYAN;
  FOR_EACH(pillars, pillarIdx) {
    ASSIGN_ARRAY_ITEM(pillars, pillarIdx, Pillar, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.x -= difficulty;
    Collision scratch = box(p->pos.x, p->pos.y, p->size.x, p->size.y);
    if (scratch.isColliding.rect[PURPLE]) {
      play(POWER_UP);
      addScore(floor(multiplier), VEC_XY(p->pos));
      multiplier += 1;
      p->isAlive = false;
      continue;
    } else if (scratch.isColliding.rect[RED]) {
      play(EXPLOSION);
      gameOver();
    }
    if (p->pos.x < -WAVYBIRD_PILLAR_WIDTH) {
      p->isAlive = false;
      continue;
    }
  }

  nextPillarDist -= difficulty;
  if (nextPillarDist < 0) {
    play(LASER);
    float height = rnd(WAVYBIRD_MIN_PILLAR_HEIGHT, WAVYBIRD_MAX_PILLAR_HEIGHT);
    float y = rnd(0, 110 - height);
    int blockCount = floor(height / WAVYBIRD_PILLAR_WIDTH);
    TIMES(blockCount, bi) {
      ASSIGN_ARRAY_ITEM(pillars, pillarIndex, Pillar, np);
      vectorSet(&np->pos, 100 + WAVYBIRD_PILLAR_WIDTH, y);
      vectorSet(&np->size, WAVYBIRD_PILLAR_WIDTH, WAVYBIRD_PILLAR_WIDTH);
      np->isAlive = true;
      pillarIndex = wrap(pillarIndex + 1, 0, WAVYBIRD_MAX_PILLAR_COUNT);
      y += WAVYBIRD_PILLAR_WIDTH;
    }
    nextPillarDist += rnd(10, 40);
  }
  multiplier = clamp(multiplier - 0.03 * difficulty, 1, 99);
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(multiplier)));
  text(multText, 2, 9);
}

void addGameWavyBird() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
