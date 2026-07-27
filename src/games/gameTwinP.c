#include "cglp.h"

static char *title = "TWIN P";
static char *description = "[Hold] Stretch\n[Release] Pin";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 16, .isDarkColor = true};

typedef struct {
  Vector pos;
  Vector vel;
  bool isCross;
  bool isIn;
  bool isAlive;
} Obj;
#define TWINP_MAX_OBJ_COUNT 32
static Obj objs[TWINP_MAX_OBJ_COUNT];
static int objIndex;

static Vector pos;
static float angle;
static float len;
static float targetLen;
static bool colorFlag;
static float objAddTicks;
static int crossAddCount;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&pos, 40, 50);
    angle = 0;
    len = 15;
    targetLen = 15;
    colorFlag = true;
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    objAddTicks = 0;
    crossAddCount = 9;
    multiplier = 1;
  }
  Vector np;
  vectorSet(&np, pos.x, pos.y);
  addWithAngle(&np, angle, len);
  color = BLACK;
  thickness = 2;
  line(pos.x, pos.y, np.x, np.y);
  if (colorFlag) {
    color = RED;
  } else {
    color = BLUE;
  }
  text("P", pos.x + 3, pos.y - 3);
  if (colorFlag) {
    color = BLUE;
  } else {
    color = RED;
  }
  text("P", np.x + 3, np.y - 3);
  bool posInRect =
      pos.x >= 0 && pos.x < 99 && pos.y >= 0 && pos.y < 99;
  bool npInRect = np.x >= 0 && np.x < 99 && np.y >= 0 && np.y < 99;
  if (!posInRect && !npInRect) {
    play(RANDOM);
    color = RED;
    text("X", pos.x, pos.y);
    text("X", np.x, np.y);
    gameOver();
  }
  if (input.isJustReleased) {
    play(SELECT);
    colorFlag = !colorFlag;
    pos = np;
    angle += M_PI;
    targetLen = 15;
  } else if (input.isPressed) {
    play(LASER);
    if (targetLen < 99) {
      targetLen += difficulty * 0.2;
    }
  }
  angle += difficulty * 0.05;
  len += (targetLen - len) * 0.2;
  objAddTicks--;
  if (objAddTicks < 0) {
    bool isCrossType = crossAddCount == 0;
    ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, o);
    vectorSet(&o->pos, 50, 50);
    addWithAngle(&o->pos, rnd(0, M_PI * 2), 80);
    float velAngle = angleTo(&o->pos, rnd(20, 80), rnd(20, 80));
    vectorSet(&o->vel, 0, 0);
    addWithAngle(&o->vel, velAngle, rnd(1, difficulty) * 0.2);
    o->isCross = isCrossType;
    o->isIn = false;
    o->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, TWINP_MAX_OBJ_COUNT);
    crossAddCount--;
    if (crossAddCount < 0) {
      crossAddCount = rndi(8, 12);
    }
    objAddTicks = rnd(60, 90) / difficulty;
  }
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, Obj, o);
    SKIP_IS_NOT_ALIVE(o);
    vectorAdd(&o->pos, o->vel.x, o->vel.y);
    bool oInRect =
        o->pos.x >= -5 && o->pos.x < 105 && o->pos.y >= -5 && o->pos.y < 105;
    if (oInRect) {
      o->isIn = true;
    } else if (o->isIn) {
      if (!o->isCross) {
        if (multiplier > 1) {
          play(HIT);
          multiplier--;
        }
      }
      o->isAlive = false;
      continue;
    }
    if (!o->isCross) {
      color = YELLOW;
      Collision oc = box(o->pos.x, o->pos.y, 4, 4);
      if (oc.isColliding.rect[BLACK]) {
        play(COIN);
        addScore(multiplier, o->pos.x, o->pos.y);
        multiplier++;
        o->isAlive = false;
        continue;
      }
    } else {
      color = PURPLE;
      thickness = 2;
      Collision c1 = bar(o->pos.x, o->pos.y, 4, M_PI_4);
      thickness = 2;
      Collision c2 = bar(o->pos.x, o->pos.y, 4, -M_PI_4);
      if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
        play(RANDOM);
        gameOver();
      }
    }
  }
}

void addGameTwinP() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
