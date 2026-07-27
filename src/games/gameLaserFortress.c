#include "cglp.h"

static char *title = "LASER FORTRESS";
static char *description = "[Hold] Laser irradiation";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
            "      ",
},
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
            "      ",
},
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        "  l   ",
        "  l   ",
        "ll ll ",
        "  l   ",
        "  l   ",
            "      ",
},
    {
        " l    ",
        "l  l  ",
        "lllll ",
        "l  l  ",
        " l    ",
            "      ",
},
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 160, .viewSizeY = 60, .soundSeed = 1, .isDarkColor = false};

#define LASERFORTRESS_TYPE_ALLY 0
#define LASERFORTRESS_TYPE_ENEMY 1

typedef struct {
  float x;
  float vx;
  float ticks;
  int type;
  bool isAlive;
} LaserfortressObj;
#define LASERFORTRESS_MAX_OBJ_COUNT 32
static LaserfortressObj objs[LASERFORTRESS_MAX_OBJ_COUNT];
static int objIndex;
static float nextObjTicks;
static int nextObjCount;
static int nextObjType;
static int highSpeedIndex;
static float sightX;
static bool laserActive;
static float laserX;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    nextObjTicks = 0;
    nextObjCount = 5;
    highSpeedIndex = -1;
    nextObjType = LASERFORTRESS_TYPE_ENEMY;
    sightX = 40;
    laserActive = false;
    multiplier = 1;
  }
  color = BLUE;
  rect(0, 50, 200, 10);
  color = LIGHT_CYAN;
  rect(0, 7, 14, 22);
  character("f", 17, 8);
  if (input.isJustPressed) {
    play(SELECT);
    laserX = sightX;
    laserActive = true;
    multiplier = 1;
    color = BLUE;
    particle(20, 8, 9, 3, 0, M_PI / 8);
  }
  if (laserActive && input.isPressed) {
    play(LASER);
    color = BLUE;
    thickness = 2;
    line(laserX, 50, 20, 8);
    laserX += difficulty * 2;
    particle(laserX, 50, 1, 2, -M_PI_2, M_PI / 6);
    color = PURPLE;
    box(laserX - 2, 50, 5, 1);
  }
  nextObjTicks--;
  if (nextObjTicks < 0) {
    ASSIGN_ARRAY_ITEM(objs, objIndex, LaserfortressObj, no);
    no->x = 163;
    float vxMul;
    if (nextObjCount == highSpeedIndex) {
      vxMul = 2;
    } else {
      vxMul = 1;
    }
    no->vx = difficulty * vxMul;
    no->ticks = rndi(0, 99);
    no->type = nextObjType;
    no->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, LASERFORTRESS_MAX_OBJ_COUNT);
    nextObjCount--;
    if (nextObjCount < 0) {
      nextObjCount = 9 - floor(sqrt(rnd(0, 50)));
      nextObjTicks = rnd(20, 30) / difficulty;
      if (nextObjType == LASERFORTRESS_TYPE_ALLY) {
        nextObjType = LASERFORTRESS_TYPE_ENEMY;
      } else if (rnd(0, 1) < 0.3) {
        nextObjType = LASERFORTRESS_TYPE_ALLY;
      }
      if (rnd(0, 1) < 0.5) {
        highSpeedIndex = -1;
      } else {
        highSpeedIndex = rndi(0, 2);
      }
    } else {
      nextObjTicks = rnd(5, 8) / difficulty;
    }
  }
  float minX = 200;
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, LaserfortressObj, o);
    SKIP_IS_NOT_ALIVE(o);
    o->x -= o->vx;
    if (o->type == LASERFORTRESS_TYPE_ENEMY && o->x < minX) {
      minX = o->x;
    }
    o->ticks++;
    if (o->type == LASERFORTRESS_TYPE_ALLY) {
      color = BLUE;
    } else {
      color = RED;
    }
    char oc[2];
    if (o->type == LASERFORTRESS_TYPE_ALLY) {
      oc[0] = 'a' + (int)floor(o->ticks / 12) % 2;
    } else {
      oc[0] = 'c' + (int)floor(o->ticks / 12) % 2;
    }
    oc[1] = 0;
    characterOptions.isMirrorX = true;
    Collision ocoll = character(oc, o->x, 47);
    characterOptions.isMirrorX = false;
    if (ocoll.isColliding.rect[PURPLE]) {
      if (o->type == LASERFORTRESS_TYPE_ALLY) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(HIT);
        particle(o->x, 47, 16, 1, 0, M_PI * 2);
        addScore(multiplier, o->x, 47);
        multiplier++;
      }
      o->isAlive = false;
      continue;
    }
    if (o->x < 0) {
      if (o->type == LASERFORTRESS_TYPE_ENEMY) {
        play(EXPLOSION);
        color = RED;
        text("X", 3, 47);
        gameOver();
      }
      o->isAlive = false;
      continue;
    }
  }
  if (minX < 200) {
    sightX += (minX - difficulty * 3 - 5 - sightX) * 0.3;
  }
  color = BLACK;
  character("e", sightX, 47);
}

void addGameLaserFortress() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
