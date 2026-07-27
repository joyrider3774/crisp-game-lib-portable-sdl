#include "cglp.h"

static char *title = "SUNFLOWER SWAY";
static char *description = "[Hold] Sunny";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " yyyy ",
        "ylllly",
        "ylllly",
        "ylllly",
        "ylllly",
        " yyyy ",
    },
    {
        "  l   ",
        " lll  ",
        "l lll ",
        "l lll ",
        " lll  ",
            "      ",
},
    {
        "l ll l",
        " l  l ",
        "l ll l",
        "llllll",
        " llll ",
        "l ll l",
    },
    {
        "   ll ",
        "  lll ",
        " llll ",
        "lllll ",
        "  ll  ",
        " ll   ",
    },
    {
        " llll ",
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  Vector pos;
  float height;
  float angle;
  float swayVelocity;
} Sunflower;
static Sunflower sunflower;
// The JS version stores these as sunflower.leafPositions (a fixed 3-value
// array); pulled out to its own global since it never changes and doesn't
// need to live inside the struct.
static float leafPositions[3];

typedef struct {
  Vector pos;
  float speed;
  bool isAlive;
} Dewdrop;
#define SUNFLOWERSWAY_MAX_DEWDROP_COUNT 32
static Dewdrop dewdrops[SUNFLOWERSWAY_MAX_DEWDROP_COUNT];
static int dewdropIndex;

typedef struct {
  Vector pos;
  float speed;
  bool isAlive;
} Insect;
#define SUNFLOWERSWAY_MAX_INSECT_COUNT 32
static Insect insects[SUNFLOWERSWAY_MAX_INSECT_COUNT];
static int insectIndex;

static float sunX;
static float nextDewdropTicks;
static float nextInsectTicks;

static void drawSun(float x) {
  color = YELLOW;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("e", x, 10);
  TIMES(7, i) {
    float a = ticks * 0.05 + i * M_PI * 2 / 7;
    float l = fabs(sin(i + ticks * 0.05) * 5) + 10;
    Vector p1;
    vectorSet(&p1, x, 10);
    addWithAngle(&p1, a, 7);
    Vector p2;
    vectorSet(&p2, x, 10);
    addWithAngle(&p2, a, l);
    line(VEC_XY(p1), VEC_XY(p2));
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&sunflower.pos, 30, 95);
    sunflower.height = 40;
    sunflower.angle = 0;
    sunflower.swayVelocity = 0.001;
    leafPositions[0] = 0.2;
    leafPositions[1] = 0.4;
    leafPositions[2] = 0.6;
    INIT_UNALIVED_ARRAY(dewdrops);
    dewdropIndex = 0;
    INIT_UNALIVED_ARRAY(insects);
    insectIndex = 0;
    sunX = 120;
    nextDewdropTicks = 30;
    nextInsectTicks = 60;
  }
  color = YELLOW;
  rect(0, 90, 100, 10);
  if (input.isPressed) {
    sunflower.swayVelocity += 0.005;
    sunX += (80 - sunX) * 0.1;
  } else {
    sunX += (120 - sunX) * 0.1;
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  sunflower.swayVelocity *= 0.97;
  sunflower.angle += sunflower.swayVelocity;
  sunflower.swayVelocity -= sunflower.angle * 0.01;
  if (sunX < 110) {
    drawSun(sunX);
  }
  sunflower.height -= 0.045 * difficulty;
  if (sunflower.height > 70) {
    sunflower.height = 70;
  }
  if (sunflower.height < 9) {
    play(EXPLOSION);
    gameOver();
  }
  color = GREEN;
  Vector stemTop;
  vectorSet(&stemTop, sunflower.pos.x, sunflower.pos.y);
  addWithAngle(&stemTop, sunflower.angle - M_PI_2, sunflower.height);
  line(VEC_XY(sunflower.pos), VEC_XY(stemTop));
  TIMES(3, i) {
    Vector leafPoint;
    vectorSet(&leafPoint, sunflower.pos.x, sunflower.pos.y);
    addWithAngle(&leafPoint, sunflower.angle - M_PI_2, sunflower.height * leafPositions[i]);
    int ox;
    if (i % 2 == 0) {
      ox = 1;
    } else {
      ox = -1;
    }
    characterOptions.isMirrorX = ox < 0;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    character("d", leafPoint.x + ox * 3, leafPoint.y);
  }
  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  // Vircon32 port note: the JS version draws the sunflower head at 2x scale
  // via a char `scale` option this port doesn't support; drawn at normal
  // size instead (visual only).
  character("a", VEC_XY(stemTop));

  nextDewdropTicks--;
  if (nextDewdropTicks <= 0) {
    ASSIGN_ARRAY_ITEM(dewdrops, dewdropIndex, Dewdrop, nd);
    vectorSet(&nd->pos, rnd(0, 99), -3);
    nd->speed = rnd(0.5, 1) * difficulty;
    nd->isAlive = true;
    dewdropIndex = wrap(dewdropIndex + 1, 0, SUNFLOWERSWAY_MAX_DEWDROP_COUNT);
    nextDewdropTicks = rnd(30, 40) / difficulty;
  }
  FOR_EACH(dewdrops, di) {
    ASSIGN_ARRAY_ITEM(dewdrops, di, Dewdrop, d);
    SKIP_IS_NOT_ALIVE(d);
    d->pos.y += d->speed;
    color = LIGHT_BLUE;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    Collision dc = character("b", VEC_XY(d->pos));
    if (dc.isColliding.character['a']) {
      play(COIN);
      addScore(sunflower.height, VEC_XY(stemTop));
      sunflower.height += 7;
      d->isAlive = false;
      continue;
    }
    if (d->pos.y > 90) {
      d->isAlive = false;
      continue;
    }
  }

  nextInsectTicks--;
  if (nextInsectTicks <= 0) {
    ASSIGN_ARRAY_ITEM(insects, insectIndex, Insect, ni);
    if (rnd(0, 1) < 0.25) {
      vectorSet(&ni->pos, rnd(0, 20), -3);
    } else {
      vectorSet(&ni->pos, rnd(40, 99), -3);
    }
    ni->speed = rnd(0.3, 0.8) * difficulty;
    ni->isAlive = true;
    insectIndex = wrap(insectIndex + 1, 0, SUNFLOWERSWAY_MAX_INSECT_COUNT);
    nextInsectTicks = rnd(60, 70) / difficulty;
  }
  FOR_EACH(insects, ii) {
    ASSIGN_ARRAY_ITEM(insects, ii, Insect, ins);
    SKIP_IS_NOT_ALIVE(ins);
    ins->pos.y += ins->speed;
    color = RED;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    Collision ic = character("c", VEC_XY(ins->pos));
    if (ic.isColliding.character['a']) {
      play(HIT);
      sunflower.height -= 15;
      ins->isAlive = false;
      continue;
    }
    if (ins->pos.y > 90) {
      ins->isAlive = false;
      continue;
    }
  }
}

void addGameSunflowerSway() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
