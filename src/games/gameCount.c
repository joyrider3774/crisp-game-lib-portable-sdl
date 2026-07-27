#include "cglp.h"
#include "string.h"

static char *title = "COUNT";
static char *description = "[Tap]\nStop counter";

// Vircon32 port note: this engine supports multi-color character glyphs
// when color=BLACK is active at draw time - each template letter maps to
// its own fixed palette color via colorGridChars in cglp.c (setColorGrid())
// instead of the whole glyph being tinted by the caller's chosen color.
// COUNT relies on this (never explicitly setting color for these draws,
// so it stays at the frame's default BLACK) to render 3 distinct hues
// (r/R=red, g/G=green, b/B=blue) baked directly into the templates below.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  rr  ",
        " rRRr ",
        "rRRRRr",
        "rRRRRr",
        " rRRr ",
        "  rr  ",
    },
    {
        "  gg  ",
        " gGGg ",
        "gGGGGg",
        "gGGGGg",
        " gGGg ",
        "  gg  ",
    },
    {
        "  bb  ",
        " bBBb ",
        "bBBBBb",
        "bBBBBb",
        " bBBb ",
        "  bb  ",
    },
    {
        "rrrrrr",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        "rRRRRr",
        "rrrrrr",
    },
    {
        "gggggg",
        "gGGGGg",
        "gGGGGg",
        "gGGGGg",
        "gGGGGg",
        "gggggg",
    },
    {
        "bbbbbb",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        "bBBBBb",
        "bbbbbb",
    },
    {
        "  rr  ",
        "  rr  ",
        " rRRr ",
        " rRRr ",
        "rRRRRr",
        "rrrrrr",
    },
    {
        "  gg  ",
        "  gg  ",
        " gGGg ",
        " gGGg ",
        "gGGGGg",
        "gggggg",
    },
    {
        "  bb  ",
        "  bb  ",
        " bBBb ",
        " bBBb ",
        "bBBBBb",
        "bbbbbb",
    },
};
static int charactersCount = 9;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector pos;
  int type;
  int color;
  float scale;
  bool isTarget;
  bool isAlive;
} CountObj;
#define COUNT_MAX_OBJ_COUNT 128
static CountObj countObjs[COUNT_MAX_OBJ_COUNT];
static int countObjCount;

static int countCount;
static float countCountTicks;
static int targetList[9];
static int targetListCount;
static int targetCount;
static int turn;
static bool countIsPressed;
static float nextTurnTicks;

static bool checkNearest(Vector *p, float scale) {
  TIMES(countObjCount, i) {
    CountObj *o = &countObjs[i];
    if (distanceTo(&o->pos, p->x, p->y) < (o->scale + scale) * 3) {
      return true;
    }
  }
  return false;
}

static void update() {
  // Never reads a Collision result anywhere in this file - the only
  // input is a plain tap (input.isJustPressed), and matching is done by
  // comparing the type/color fields directly, so the engine's own O(n^2)
  // hitbox scan (see checkHitBox() in cglp.c) is pure waste here.
  // Restored automatically when the next real game starts, via
  // resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    countObjCount = 0;
    turn = 1;
    countIsPressed = false;
  }
  if (countObjCount == 0) {
    float scaleMax;
    if (rnd(0, 1) > wrap(sqrt(turn) * 0.1, 0, 0.5)) {
      scaleMax = 1;
    } else {
      scaleMax = 3;
    }
    bool isRandType;
    if (rnd(0, 1) > wrap(sqrt(turn) * 0.2, 0.5, 1)) {
      isRandType = false;
    } else {
      isRandType = true;
    }
    bool isRandColor;
    if (rnd(0, 1) > wrap(sqrt(turn) * 0.2, 0.5, 1)) {
      isRandColor = false;
    } else {
      isRandColor = true;
    }
    if (!isRandType && !isRandColor) {
      if (rnd(0, 1) < 0.5) {
        isRandType = true;
      } else {
        isRandColor = true;
      }
    }
    bool isTypeTarget = isRandType;
    bool isColorTarget = isRandColor;
    if (isTypeTarget && isColorTarget) {
      if (rnd(0, 1) < 0.5) {
        isTypeTarget = false;
      } else {
        isColorTarget = false;
      }
    }
    int ty = rndi(0, 3);
    int cl = rndi(0, 3);
    int genCount = 3 + (int)floor(sqrt(turn) + rndi(0, turn));
    if (genCount > COUNT_MAX_OBJ_COUNT) {
      genCount = COUNT_MAX_OBJ_COUNT;
    }
    countObjCount = 0;
    TIMES(genCount, k) {
      Vector p;
      vectorSet(&p, rnd(10, 90), rnd(30, 90));
      int type;
      if (isRandType) {
        type = rndi(0, 3);
      } else {
        type = ty;
      }
      int col;
      if (isRandColor) {
        col = rndi(0, 3);
      } else {
        col = cl;
      }
      float scale = rnd(1, scaleMax);
      if (!checkNearest(&p, scale)) {
        countObjs[countObjCount].pos = p;
        countObjs[countObjCount].type = type;
        countObjs[countObjCount].color = col;
        countObjs[countObjCount].scale = scale;
        countObjs[countObjCount].isTarget = false;
        countObjs[countObjCount].isAlive = true;
        countObjCount++;
      }
    }
    int targetIdx = rndi(0, countObjCount);
    int targetType = countObjs[targetIdx].type;
    int targetColor = countObjs[targetIdx].color;
    targetCount = 0;
    bool targetFlags[9];
    TIMES(9, k) { targetFlags[k] = false; }
    TIMES(countObjCount, i) {
      CountObj *o = &countObjs[i];
      bool matches = (!isTypeTarget || o->type == targetType) &&
                     (!isColorTarget || o->color == targetColor);
      if (matches) {
        o->isTarget = true;
        targetFlags[o->type * 3 + o->color] = true;
        targetCount++;
      }
    }
    targetListCount = 0;
    TIMES(9, k) {
      if (targetFlags[k]) {
        targetList[targetListCount] = k;
        targetListCount++;
      }
    }
    countCount = 0;
    countCountTicks = 79;
    countIsPressed = false;
  }
  color = BLACK;
  text("How many", 4, 12);
  float x = 56;
  TIMES(targetListCount, i) {
    char tc[2];
    tc[0] = 'a' + targetList[i];
    tc[1] = 0;
    character(tc, x, 12);
    x += 7;
  }
  color = BLACK;
  char qText[16];
  strcpy(qText, "? ");
  strcat(qText, intToChar(countCount));
  text(qText, x, 12);
  if (!countIsPressed) {
    TIMES(countObjCount, i) {
      CountObj *o = &countObjs[i];
      color = BLACK;
      // Vircon32 port note: the JS version draws these at up to 3x
      // scale, which this port's character() has no equivalent for -
      // drawn at normal size instead (visual only, doesn't affect the
      // counting gameplay since no collision check ever reads these).
      char oc[2];
      oc[0] = 'a' + o->type * 3 + o->color;
      oc[1] = 0;
      character(oc, VEC_XY(o->pos));
    }
    countCountTicks -= difficulty;
    if (input.isJustPressed || countCount > targetCount) {
      countIsPressed = true;
      nextTurnTicks = 0;
      if (countCount == targetCount) {
        addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        play(POWER_UP);
      } else {
        play(RANDOM);
      }
    } else if (countCountTicks < 0) {
      play(SELECT);
      countCount++;
      countCountTicks = 60;
    }
  } else {
    color = BLACK;
    if (countCount == targetCount) {
      text("OK!", 80, 20);
    } else {
      text("ERROR", 70, 20);
    }
    nextTurnTicks++;
    if (nextTurnTicks > 90) {
      if (countCount == targetCount) {
        turn++;
        countObjCount = 0;
      } else {
        gameOver();
        return;
      }
    }
    color = BLACK;
    // Vircon32 port note: the JS version draws this at 3x scale - drawn
    // at normal size instead (visual only, purely decorative here).
    text(intToChar(targetCount), 50, 50);
    TIMES(countObjCount, i) {
      CountObj *o = &countObjs[i];
      if (o->isTarget) {
        color = BLACK;
        char oc[2];
        oc[0] = 'a' + o->type * 3 + o->color;
        oc[1] = 0;
        character(oc, VEC_XY(o->pos));
      }
    }
  }
}

void addGameCount() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
