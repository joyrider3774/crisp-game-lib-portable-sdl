#include "cglp.h"

static char *title = "LIGHT DARK";
static char *description = "[Tap] Jump\n[Hold] Fly";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "    ll",
        "   l  ",
        "  l   ",
        "  ll  ",
        " l  l ",
        "l    l",
    },
    {
        "  ll  ",
        "  l   ",
        "  l   ",
        "  ll  ",
        "  ll  ",
        " l  l ",
    },
    {
        "  ll  ",
        "  ll  ",
        " llll ",
        " llll ",
        "llllll",
        "llllll",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

#define LIGHTDARK_TYPE_SPIKE 0
#define LIGHTDARK_TYPE_COIN 1
#define LIGHTDARK_SIDE_LIGHT 0
#define LIGHTDARK_SIDE_DARK 1
#define LIGHTDARK_STATE_GROUND 0
#define LIGHTDARK_STATE_JUMP 1

typedef struct {
  float x;
  int type;
  int side;
  bool isAlive;
} LightdarkObj;
#define LIGHTDARK_MAX_OBJ_COUNT 64
static LightdarkObj objs[LIGHTDARK_MAX_OBJ_COUNT];
static int objIndex;

static float objDists[2];
static int objTypes[2];
static Vector pos;
static int side;
static Vector vel;
static int state;
static float multiplier;
static float scx;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    TIMES(10, i) {
      ASSIGN_ARRAY_ITEM(objs, objIndex, LightdarkObj, o);
      if (i < 5) {
        o->x = 25 + i * 9;
        o->side = LIGHTDARK_SIDE_LIGHT;
      } else {
        o->x = 99 + i * 9;
        o->side = LIGHTDARK_SIDE_DARK;
      }
      o->type = LIGHTDARK_TYPE_COIN;
      o->isAlive = true;
      objIndex = wrap(objIndex + 1, 0, LIGHTDARK_MAX_OBJ_COUNT);
    }
    objDists[0] = 200;
    objDists[1] = 300;
    objTypes[0] = LIGHTDARK_TYPE_COIN;
    objTypes[1] = LIGHTDARK_TYPE_SPIKE;
    vectorSet(&pos, 9, 0);
    side = LIGHTDARK_SIDE_LIGHT;
    vectorSet(&vel, 2, 0);
    state = LIGHTDARK_STATE_GROUND;
    multiplier = 1;
    scx = 0;
  }
  float scr = vel.x * difficulty;
  color = LIGHT_BLACK;
  rect(0, 50, 200, 50);
  if (state == LIGHTDARK_STATE_GROUND) {
    if (input.isJustPressed) {
      if (side == LIGHTDARK_SIDE_LIGHT) {
        side = LIGHTDARK_SIDE_DARK;
      } else {
        side = LIGHTDARK_SIDE_LIGHT;
      }
      if (side == LIGHTDARK_SIDE_LIGHT) {
        play(JUMP);
      } else {
        play(LASER);
      }
      vel.y = 3 * sqrt(difficulty);
      pos.y = 7;
      state = LIGHTDARK_STATE_JUMP;
      scx = 0;
    }
  } else {
    if (input.isJustPressed) {
      play(HIT);
      if (side == LIGHTDARK_SIDE_LIGHT) {
        side = LIGHTDARK_SIDE_DARK;
      } else {
        side = LIGHTDARK_SIDE_LIGHT;
      }
    }
    float dvy;
    if (input.isPressed) {
      dvy = 0.1;
    } else {
      dvy = 0.5;
    }
    vel.y -= dvy * difficulty;
    pos.y += vel.y;
    if (pos.y < 0) {
      pos.y = 0;
      state = LIGHTDARK_STATE_GROUND;
    }
  }
  float y;
  if (side == LIGHTDARK_SIDE_LIGHT) {
    y = 47 - pos.y;
  } else {
    y = 53 + pos.y;
  }
  if (side == LIGHTDARK_SIDE_LIGHT) {
    color = BLACK;
  } else {
    color = WHITE;
  }
  char ch[2];
  if (state == LIGHTDARK_STATE_JUMP) {
    ch[0] = 'b';
  } else {
    ch[0] = 'a' + (int)floor((ticks * difficulty) / 10) % 2;
  }
  ch[1] = 0;
  if (side == LIGHTDARK_SIDE_DARK) {
    characterOptions.isMirrorY = true;
  }
  character(ch, pos.x, y);
  characterOptions.isMirrorY = false;
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, LightdarkObj, o);
    SKIP_IS_NOT_ALIVE(o);
    float oy;
    if (o->side == LIGHTDARK_SIDE_LIGHT) {
      oy = 46;
    } else {
      oy = 54;
    }
    if (o->side == LIGHTDARK_SIDE_LIGHT) {
      color = BLACK;
    } else {
      color = WHITE;
    }
    Collision oc;
    if (o->side == LIGHTDARK_SIDE_DARK) {
      characterOptions.isMirrorY = true;
    }
    if (o->type == LIGHTDARK_TYPE_SPIKE) {
      oc = character("c", o->x, oy);
    } else {
      oc = text("o", o->x, oy);
    }
    characterOptions.isMirrorY = false;
    if (oc.isColliding.character['a'] || oc.isColliding.character['b']) {
      if (o->type == LIGHTDARK_TYPE_SPIKE) {
        play(EXPLOSION);
        gameOver();
      } else {
        if (o->side == LIGHTDARK_SIDE_LIGHT) {
          play(COIN);
        } else {
          play(SELECT);
        }
        addScore(multiplier, o->x + scx * 7, oy);
        multiplier++;
        scx++;
        o->isAlive = false;
        continue;
      }
    }
    o->x -= scr;
    if (o->x < -3) {
      if (o->type == LIGHTDARK_TYPE_COIN && multiplier > 1) {
        multiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
  TIMES(2, i) {
    objDists[i] -= scr;
    int sideVal;
    if (i == 0) {
      sideVal = LIGHTDARK_SIDE_LIGHT;
    } else {
      sideVal = LIGHTDARK_SIDE_DARK;
    }
    float o;
    if (objTypes[i] == LIGHTDARK_TYPE_COIN) {
      o = 9;
    } else {
      o = 6;
    }
    int c;
    if (objTypes[i] == LIGHTDARK_TYPE_COIN) {
      c = rndi(4, 8);
    } else {
      c = rndi(5, 15);
    }
    if (objDists[i] < 0) {
      TIMES(c, j) {
        ASSIGN_ARRAY_ITEM(objs, objIndex, LightdarkObj, no);
        no->x = 200 + j * o;
        no->type = objTypes[i];
        no->side = sideVal;
        no->isAlive = true;
        objIndex = wrap(objIndex + 1, 0, LIGHTDARK_MAX_OBJ_COUNT);
      }
      objDists[i] = c * o + rnd(40, 120);
      if (objTypes[i] == LIGHTDARK_TYPE_COIN) {
        objTypes[i] = LIGHTDARK_TYPE_SPIKE;
      } else {
        objTypes[i] = LIGHTDARK_TYPE_COIN;
      }
    }
  }
}

void addGameLightDark() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
