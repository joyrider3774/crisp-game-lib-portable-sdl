#include "cglp.h"

static char *title = "UNCTRL";
static char *description = "[Tap]  Fire\n[Hold] Go up";

// The original sprites are 7px wide ("l ll ll" etc.); this engine's
// CHARACTER_WIDTH is fixed at 6, so the first/last rows are truncated by
// one column (dropping the trailing tread pixel) to fit.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "l ll l",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        "l ll l",
    },
    {
        " ll ll",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        " ll ll",
    },
    {
        "ll ll ",
        " lll  ",
        "l  lll",
        "l  lll",
        " lll  ",
        "ll ll ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 150, .viewSizeY = 100, .soundSeed = 15, .isDarkColor = true};

typedef struct {
  Vector pos;
  float bulletAngle;
  float fireTicks;
  float animTicks;
  bool isAlive;
} Tank;
#define UNCTRL_MAX_TANK_COUNT 32
static Tank tanks[UNCTRL_MAX_TANK_COUNT];
static int tankIndex;
static float nextTankTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Bullet;
#define UNCTRL_MAX_BULLET_COUNT 32
static Bullet bullets[UNCTRL_MAX_BULLET_COUNT];
static int bulletIndex;

#define UNCTRL_SHOT_READY 0
#define UNCTRL_SHOT_FIRED 1
typedef struct {
  Vector pos;
  Vector vel;
  int state;
} Shot;
static Shot shot;

static float animTicks;
static int multiplier;

#define UNCTRL_GROUND_COUNT 20
static Vector grounds[UNCTRL_GROUND_COUNT];

static void setNextShot() {
  shot.pos.x = 8;
  shot.pos.y = 50;
  vectorSet(&shot.vel, 0.5, 0.2);
  shot.state = UNCTRL_SHOT_READY;
  multiplier = 1;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(tanks);
    tankIndex = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    nextTankTicks = 0;
    vectorSet(&shot.pos, 0, 0);
    vectorSet(&shot.vel, 0, 0);
    shot.state = UNCTRL_SHOT_READY;
    animTicks = 0;
    setNextShot();
    TIMES(UNCTRL_GROUND_COUNT, i) {
      vectorSet(&grounds[i], rnd(0, 150), rnd(0, 100));
    }
  }
  color = LIGHT_PURPLE;
  TIMES(UNCTRL_GROUND_COUNT, i) {
    box(grounds[i].x, grounds[i].y, 5, 2);
    grounds[i].x = wrap(grounds[i].x - 0.1 * sqrt(difficulty), -3, 153);
  }
  if (shot.state == UNCTRL_SHOT_READY) {
    color = LIGHT_BLUE;
  } else {
    color = BLUE;
  }
  if (shot.state == UNCTRL_SHOT_READY) {
    if (input.isJustPressed) {
      play(SELECT);
      shot.state = UNCTRL_SHOT_FIRED;
    }
  } else {
    if (input.isJustPressed) {
      play(HIT);
    }
    if (input.isJustReleased) {
      play(LASER);
    }
    if (input.isPressed) {
      shot.vel.y += -0.1;
    } else {
      shot.vel.y += 0.1;
    }
    vectorAdd(&shot.pos, shot.vel.x, shot.vel.y);
    float shotAngle = vectorAngle(&shot.vel);
    particle(shot.pos.x, shot.pos.y, 1, 1, shotAngle + M_PI, 1);
    bool shotInRect = shot.pos.x >= 0 && shot.pos.x < 150 &&
                       shot.pos.y >= 0 && shot.pos.y < 100;
    if (!shotInRect) {
      setNextShot();
    }
  }
  thickness = 4;
  barCenterPosRatio = -0.5;
  bar(shot.pos.x, shot.pos.y, 6, vectorAngle(&shot.vel));
  COUNT_IS_ALIVE(tanks, aliveTankCount);
  if (aliveTankCount == 0) {
    nextTankTicks = 0;
  }
  nextTankTicks--;
  if (nextTankTicks < 0) {
    ASSIGN_ARRAY_ITEM(tanks, tankIndex, Tank, nt);
    vectorSet(&nt->pos, 153, rnd(20, 40) * RNDPM() + 50);
    nt->bulletAngle = rnd(0, M_PI * 0.3) * RNDPM() + M_PI;
    nt->fireTicks = rnd(0, 60);
    nt->animTicks = 0;
    nt->isAlive = true;
    tankIndex = wrap(tankIndex + 1, 0, UNCTRL_MAX_TANK_COUNT);
    nextTankTicks = 200 / sqrt(difficulty);
  }
  FOR_EACH(tanks, i) {
    ASSIGN_ARRAY_ITEM(tanks, i, Tank, t);
    SKIP_IS_NOT_ALIVE(t);
    float animMul;
    if (t->pos.x < 70) {
      animMul = 4;
    } else {
      animMul = 1;
    }
    t->animTicks += sqrt(difficulty) * animMul;
    float moveMul;
    if (t->pos.x < 70) {
      moveMul = 0.4;
    } else {
      moveMul = 0.1;
    }
    t->pos.x -= sqrt(difficulty) * moveMul;
    color = RED;
    char tc[2];
    tc[0] = 'a' + (int)floor(t->animTicks / 20) % 3;
    tc[1] = 0;
    characterOptions.isMirrorX = true;
    Collision tcoll = character(tc, t->pos.x, t->pos.y);
    characterOptions.isMirrorX = false;
    if (tcoll.isColliding.rect[BLUE]) {
      play(POWER_UP);
      particle(t->pos.x, t->pos.y, 19, 3, 0, M_PI * 2);
      addScore(multiplier * 10, t->pos.x, t->pos.y);
      setNextShot();
      t->isAlive = false;
      continue;
    }
    t->fireTicks -= difficulty;
    if (t->pos.x > 70) {
      if (t->fireTicks < 0) {
        ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
        vectorSet(&nb->pos, t->pos.x - 3, t->pos.y);
        vectorSet(&nb->vel, 0, 0);
        addWithAngle(&nb->vel, t->bulletAngle, 0.3 * difficulty);
        nb->isAlive = true;
        bulletIndex = wrap(bulletIndex + 1, 0, UNCTRL_MAX_BULLET_COUNT);
        t->fireTicks = 60;
        t->bulletAngle = rnd(0, M_PI * 0.3) * RNDPM() + M_PI;
      } else {
        color = LIGHT_RED;
        thickness = 2;
        barCenterPosRatio = -0.5;
        bar(t->pos.x - 3, t->pos.y, 4, t->bulletAngle);
      }
    }
    t->isAlive = t->pos.x >= -3;
  }
  color = RED;
  FOR_EACH(bullets, i) {
    ASSIGN_ARRAY_ITEM(bullets, i, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    thickness = 2;
    barCenterPosRatio = -0.5;
    Collision bc = bar(b->pos.x, b->pos.y, 4, vectorAngle(&b->vel));
    if (bc.isColliding.rect[BLUE]) {
      play(COIN);
      addScore(multiplier, b->pos.x, b->pos.y);
      particle(b->pos.x, b->pos.y, 9, 1, 0, M_PI * 2);
      if (multiplier < 64) {
        multiplier *= 2;
      }
      b->isAlive = false;
      continue;
    }
    bool bInRect = b->pos.x >= 0 && b->pos.x < 150 && b->pos.y >= 0 && b->pos.y < 100;
    b->isAlive = bInRect;
  }
  color = BLUE;
  animTicks += difficulty;
  char pc[2];
  pc[0] = 'a' + (int)floor(animTicks / 20) % 3;
  pc[1] = 0;
  Collision pcoll = character(pc, 10, 50);
  if (pcoll.isColliding.rect[RED]) {
    play(RANDOM);
    gameOver();
  }
}

void addGameUnctrl() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
