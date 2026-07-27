#include "cglp.h"

static char *title = "SPEARAIN";
static char *description = "[Hold] Slow";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " l    ",
        " l    ",
        " l    ",
        " l    ",
        "lll   ",
        " l    ",
    },
    {
        "llll  ",
        "llll  ",
        " l    ",
        "lll   ",
        " lll  ",
        "l     ",
    },
    {
        "llll  ",
        "llll  ",
        "  l   ",
        " lll  ",
        "lll   ",
        "   l  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  Vector pos;
  float speed;
  bool isBonus;
  bool isAlive;
} Obj;
// While holding, appRatio grows 5%/tick (faster spawns) while speedRatio only decays 2%/tick
// (slower fall) for ~140 ticks until the <0.03 rebound kicks in - a single ordinary "hold to slow"
// can pile up ~100+ concurrent slow-falling objects before they clear.
#define MAX_OBJ_COUNT 256
static Obj objs[MAX_OBJ_COUNT];
static int objIndex;

static float appTicks;
static float appRatio;
static int bonusAppCount;
static float speedRatio;
static float xPos;
static float vx;
static float anim;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    appTicks = 0;
    appRatio = 1;
    bonusAppCount = 0;
    speedRatio = 1;
    xPos = 50;
    vx = 1;
    anim = 0;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 9, 99);
  rect(90, 0, 9, 99);
  rect(0, 90, 99, 9);
  appTicks -= difficulty;
  if (appTicks < 0) {
    play(LASER);
    bonusAppCount--;
    ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, o);
    vectorSet(&o->pos, rnd(12, 88), 2);
    float speedDivisor;
    if (bonusAppCount < 0) {
      speedDivisor = 3;
    } else {
      speedDivisor = 1;
    }
    o->speed = (1 + rnd(0, sqrt(sqrt(difficulty)))) / speedDivisor;
    o->isBonus = bonusAppCount < 0;
    o->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, MAX_OBJ_COUNT);
    if (bonusAppCount < 0) {
      bonusAppCount = 12;
    }
    appTicks += rnd(40, 60) / difficulty / appRatio;
  }
  if (input.isJustPressed) {
    speedRatio *= 0.5;
  }
  if (speedRatio < 1 && input.isPressed) {
    speedRatio *= 0.98;
    appRatio *= 1.05;
  } else {
    speedRatio += (1 - speedRatio) * 0.05;
    appRatio = 1;
  }
  if (speedRatio < 0.03 || input.isJustReleased) {
    speedRatio = 1 / sqrt(speedRatio);
  }
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, Obj, o);
    SKIP_IS_NOT_ALIVE(o);
    float s = o->speed * speedRatio * difficulty;
    if (o->isBonus) {
      color = YELLOW;
      text("$", o->pos.x, o->pos.y);
      if (s > 78) {
        s *= 0.2;
      }
    } else {
      color = RED;
      character("a", o->pos.x, o->pos.y);
      if (s > 74) {
        s *= 1.5;
      }
    }
    o->pos.y += s;
    if (o->pos.y >= 87) {
      play(HIT);
    }
    o->isAlive = o->pos.y < 87;
  }
  float sVal = (vx / sqrt(speedRatio)) * difficulty;
  xPos += sVal;
  if ((xPos < 12 && vx < 0) || (xPos > 88 && vx > 0)) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    play(SELECT);
    vx *= -1;
  }
  anim += fabs(sVal) * 0.1;
  color = BLACK;
  char pc[2];
  pc[0] = 'b' + (int)floor(fmod(anim, 2));
  pc[1] = 0;
  Collision cl = character(pc, xPos, 87);
  if (cl.isColliding.text['$']) {
    play(COIN);
    COUNT_IS_ALIVE(objs, aliveObjCount);
    addScore(aliveObjCount, xPos, 87);
    color = BLACK;
    FOR_EACH(objs, i) {
      ASSIGN_ARRAY_ITEM(objs, i, Obj, o);
      SKIP_IS_NOT_ALIVE(o);
      particle(o->pos.x, o->pos.y, 5, 1, 0, M_PI * 2);
    }
    INIT_UNALIVED_ARRAY(objs);
    appRatio = 1;
    appTicks = 60;
  } else if (cl.isColliding.character['a']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameSpearain() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
