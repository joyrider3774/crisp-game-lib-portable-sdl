#include "cglp.h"
#include "string.h"

static char *title = "M RIDER";
static char *description = "[Hold] Go up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l  l",
        " llll ",
        "l l   ",
        " l l  ",
        "ll  l ",
    },
    {
        "  ll  ",
        " l    ",
        "llll  ",
        " l    ",
        "  l   ",
        "   ll ",
    },
    {
        "l     ",
        "ll    ",
        "llllll",
        " ll   ",
        " l    ",
        "      ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} MriderMissile;
#define MRIDER_MAX_MISSILE_COUNT 32
static MriderMissile missiles[MRIDER_MAX_MISSILE_COUNT];
static int missileIndex;
static float nextMissileTicks;

typedef struct {
  Vector pos;
  Vector vel;
  float removeTicks;
  float baseRemoveTicks;
  bool isAlive;
} MriderPlane;
#define MRIDER_MAX_PLANE_COUNT 32
static MriderPlane planes[MRIDER_MAX_PLANE_COUNT];
static int planeIndex;
static float nextPlaneTicks;

typedef struct {
  Vector pos;
  Vector vel;
  int missileIndex;
} MriderRider;
static MriderRider rider;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(missiles);
    missileIndex = 0;
    ASSIGN_ARRAY_ITEM(missiles, missileIndex, MriderMissile, m0);
    vectorSet(&m0->pos, 30, 50);
    vectorSet(&m0->vel, 1, 0);
    m0->isAlive = true;
    missileIndex = wrap(missileIndex + 1, 0, MRIDER_MAX_MISSILE_COUNT);
    nextMissileTicks = 9;
    INIT_UNALIVED_ARRAY(planes);
    planeIndex = 0;
    nextPlaneTicks = 0;
    vectorSet(&rider.pos, 0, 0);
    vectorSet(&rider.vel, 0, 0);
    rider.missileIndex = 0;
    multiplier = 1;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (rider.missileIndex < 0) {
    float rvx;
    if (input.isPressed) {
      rvx = 0.04;
    } else {
      rvx = -0.02;
    }
    rider.vel.x += rvx * sqrt(difficulty);
    float rvy;
    if (input.isPressed) {
      rvy = 0.01;
    } else {
      rvy = 0.05;
    }
    rider.vel.y += rvy * sqrt(difficulty);
    vectorMul(&rider.vel, 0.99);
    vectorAdd(&rider.pos, rider.vel.x, rider.vel.y);
  } else {
    MriderMissile *m = &missiles[rider.missileIndex];
    float dvy;
    if (input.isPressed) {
      dvy = -1;
    } else {
      dvy = 1;
    }
    m->vel.y += dvy * 0.05 * difficulty;
    m->vel.y *= 0.99;
    m->vel.x += (sqrt(difficulty) - m->vel.x) * 0.1;
    vectorSet(&rider.pos, m->pos.x, m->pos.y - 4);
    rider.vel = m->vel;
  }
  float scr;
  if (rider.pos.x > 50) {
    scr = (rider.pos.x - 50) * 0.1 * sqrt(difficulty);
  } else {
    scr = 0;
  }
  rider.pos.x -= scr;
  color = BLACK;
  char rc[2];
  if (rider.missileIndex < 0) {
    rc[0] = 'b';
    characterOptions.rotation = (int)floor(ticks / 10) % 4;
  } else {
    rc[0] = 'a';
    characterOptions.rotation = 0;
  }
  rc[1] = 0;
  character(rc, rider.pos.x, clamp(rider.pos.y, -2, 99));
  characterOptions.rotation = 0;
  if ((rider.missileIndex >= 0 && rider.pos.y < -1) || rider.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  nextMissileTicks--;
  if (nextMissileTicks < 0) {
    Vector pos;
    vectorSet(&pos, 210, rnd(40, 70));
    ASSIGN_ARRAY_ITEM(missiles, missileIndex, MriderMissile, nm);
    nm->pos = pos;
    float a = angleTo(&pos, 0, rnd(50, 60)) + M_PI;
    vectorSet(&nm->vel, rnd(0.4, 0.5) * sqrt(difficulty), 0);
    rotate(&nm->vel, a);
    nm->isAlive = true;
    missileIndex = wrap(missileIndex + 1, 0, MRIDER_MAX_MISSILE_COUNT);
    nextMissileTicks = rnd(60, 300) / sqrt(difficulty);
  }
  FOR_EACH(missiles, i) {
    ASSIGN_ARRAY_ITEM(missiles, i, MriderMissile, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);
    m->pos.x -= scr;
    color = LIGHT_RED;
    box(m->pos.x - 9, m->pos.y, 5, 5);
    int pc;
    if (i == rider.missileIndex) {
      pc = 3;
    } else {
      pc = 2;
    }
    particle(m->pos.x - 9, m->pos.y, pc, 1, M_PI, 0.5);
    if (i == rider.missileIndex) {
      color = RED;
    } else {
      color = BLACK;
    }
    Collision mc = box(m->pos.x, m->pos.y, 18, 3);
    if (mc.isColliding.character['b']) {
      play(POWER_UP);
      multiplier = (int)ceil(multiplier * 0.5);
      rider.missileIndex = i;
    }
    bool inRect = m->pos.x >= -9 && m->pos.x < 230 && m->pos.y >= -3 && m->pos.y < 116;
    if (!inRect) {
      m->isAlive = false;
      continue;
    }
  }
  nextPlaneTicks--;
  if (nextPlaneTicks < 0) {
    Vector pos;
    vectorSet(&pos, 203, rnd(30, 90));
    ASSIGN_ARRAY_ITEM(planes, planeIndex, MriderPlane, np);
    np->pos = pos;
    float a = angleTo(&pos, 0, rnd(40, 90)) + M_PI;
    vectorSet(&np->vel, rnd(0.05, 0.3) * sqrt(difficulty), 0);
    rotate(&np->vel, a);
    np->removeTicks = 0;
    np->baseRemoveTicks = 0;
    np->isAlive = true;
    planeIndex = wrap(planeIndex + 1, 0, MRIDER_MAX_PLANE_COUNT);
    nextPlaneTicks = rnd(20, 24) / sqrt(difficulty);
  }
  FOR_EACH(planes, i) {
    ASSIGN_ARRAY_ITEM(planes, i, MriderPlane, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    p->pos.x -= scr;
    if (p->removeTicks > 0) {
      color = LIGHT_RED;
      particle(p->pos.x, p->pos.y, 1, 1, 0, M_PI * 2);
      p->removeTicks -= sqrt(difficulty);
      if (p->removeTicks <= 0) {
        play(COIN);
        particle(p->pos.x, p->pos.y, 9, 2, 0, M_PI * 2);
        addScore(multiplier, VEC_XY(p->pos));
        multiplier++;
        float rt = p->baseRemoveTicks * 0.9;
        if (rt > 2) {
          FOR_EACH(planes, j) {
            ASSIGN_ARRAY_ITEM(planes, j, MriderPlane, ap);
            SKIP_IS_NOT_ALIVE(ap);
            if (i == j) {
              continue;
            }
            if (distanceTo(&ap->pos, VEC_XY(p->pos)) < 36) {
              thickness = 3;
              line(VEC_XY(ap->pos), VEC_XY(p->pos));
              ap->removeTicks = rt;
              ap->baseRemoveTicks = rt;
            }
          }
        }
        p->isAlive = false;
        continue;
      }
    }
    color = BLACK;
    Collision pcoll = character("c", VEC_XY(p->pos));
    if (pcoll.isColliding.rect[RED]) {
      if (rider.missileIndex >= 0) {
        play(JUMP);
        particle(p->pos.x, p->pos.y, 9, 3, 0, M_PI * 2);
        rider.vel.y = -2 * sqrt(difficulty);
        missiles[rider.missileIndex].pos.x = -99;
        rider.missileIndex = -1;
      }
      p->baseRemoveTicks = 9;
      p->removeTicks = 9;
    }
    bool inRect = p->pos.x >= -3 && p->pos.x < 203 && p->pos.y >= -3 && p->pos.y < 103;
    if (!inRect) {
      p->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameMRider() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
