#include "cglp.h"

static char *title = "PILLARS 3D";
static char *description = "[Slide] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 20, .isDarkColor = false};

typedef struct {
  float x;
  float z;
  Vector size;
  int color;
  bool isAlive;
} Pillar;
#define MAX_PILLAR_COUNT 32
static Pillar pillars[MAX_PILLAR_COUNT];
static int pillarIndex;
static int nextPillarTicks;
static int nextYellowPillar;
static Vector pos;
static float vy;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(pillars);
    pillarIndex = 0;
    ASSIGN_ARRAY_ITEM(pillars, pillarIndex, Pillar, p0);
    p0->x = 0;
    p0->z = 20;
    vectorSet(&p0->size, 100, 100);
    p0->color = YELLOW;
    p0->isAlive = true;
    pillarIndex = wrap(pillarIndex + 1, 0, MAX_PILLAR_COUNT);
    nextPillarTicks = 9;
    nextYellowPillar = 9;
    vectorSet(&pos, 50, 10);
    vy = 0;
    multiplier = 1;
  }
  nextPillarTicks--;
  if (nextPillarTicks < 0) {
    nextYellowPillar--;
    ASSIGN_ARRAY_ITEM(pillars, pillarIndex, Pillar, np);
    np->x = rnd(60, 160) * RNDPM();
    np->z = 20;
    vectorSet(&np->size, rnd(50, 100), rnd(70, 180));
    if (nextYellowPillar < 0) {
      np->color = YELLOW;
    } else {
      np->color = BLACK;
    }
    np->isAlive = true;
    pillarIndex = wrap(pillarIndex + 1, 0, MAX_PILLAR_COUNT);
    nextPillarTicks = 20 / difficulty;
    if (nextYellowPillar < 0) {
      nextYellowPillar = 9;
    }
  }
  color = LIGHT_BLACK;
  rect(0, 60, 100, 1);
  color = BLACK;
  pos.x = clamp(input.pos.x, 6, 93);
  pos.y += vy;
  vy += 0.1 * difficulty;
  text("TT", pos.x - 3, pos.y);
  if (pos.y > 95) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(pillars, i) {
    ASSIGN_ARRAY_ITEM(pillars, i, Pillar, p);
    SKIP_IS_NOT_ALIVE(p);
    color = p->color;
    Collision pc = box(p->x / p->z + 50, p->size.y / 3 / p->z + 60, p->size.x / p->z,
                        p->size.y / p->z);
    if (pc.isColliding.text['T']) {
      float ty = p->size.y / 3 / p->z + 60 - p->size.y / p->z / 2;
      if (vy > 0) {
        play(LASER);
        vy = -2.5 * sqrt(difficulty);
        if (pos.y > ty) {
          pos.y = ty;
        }
      }
      addScore(multiplier, p->x / p->z + 50, ty);
      if (p->color == YELLOW) {
        play(SELECT);
        multiplier++;
      }
      p->isAlive = false;
      continue;
    }
    p->z -= difficulty * 0.2;
    p->isAlive = p->z > 1;
  }
}

void addGamePillars3d() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
