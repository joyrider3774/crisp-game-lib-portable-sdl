#include "cglp.h"

static char *title = "GRAVELER";
static char *description = "[Hold]\n Reverse gravity";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "l     ",
    " lll  ",
    "ll  ll",
    " ll   ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 29, .isDarkColor = true};

#define GRAVELER_SX 10

typedef struct {
  Vector pos;
  float width;
} Wall;
#define GRAVELER_WALL_COUNT 11
static Wall walls[GRAVELER_WALL_COUNT];

typedef struct {
  float y;
  float vy;
  float w;
  float wy;
} NextWall;
static NextWall nextWall;
static float nextWallDist;
static int nextWallIndex;
static float grvOfs;

static float sy;
static float svy;

#define GRAVELER_OBJ_TYPE_COIN 0
#define GRAVELER_OBJ_TYPE_SPIKE 1
typedef struct {
  Vector pos;
  int type;
  bool isAlive;
} Obj;
#define GRAVELER_MAX_OBJ_COUNT 32
static Obj objs[GRAVELER_MAX_OBJ_COUNT];
static int objIndex;
static float nextObjDist;
static int coinCount;
static float coinY;
static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(GRAVELER_WALL_COUNT, i) {
      vectorSet(&walls[i].pos, i * 10, 50);
      walls[i].width = 60;
    }
    nextWall.y = 50;
    nextWall.vy = 0;
    nextWall.w = 60;
    nextWall.wy = 0;
    nextWallDist = 10;
    nextWallIndex = 0;
    grvOfs = 0;
    sy = 65;
    svy = 0;
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, no);
      vectorSet(&no->pos, i * 7 + 15, 65);
      no->type = GRAVELER_OBJ_TYPE_COIN;
      no->isAlive = true;
      objIndex = wrap(objIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
    }
    nextObjDist = 10;
    coinCount = 0;
    multiplier = 1;
  }
  // BGM tempo escalation every 600 ticks omitted: the JS sss.setTempo() /
  // stopBgm() / playBgm() dynamic-tempo API has no equivalent here - this
  // port's BGM plays at a single fixed tempo for the whole session.
  float scr = difficulty * 0.5;
  TIMES(GRAVELER_WALL_COUNT, i) {
    Wall *w = &walls[i];
    color = BLUE;
    rect(w->pos.x, 0, 11, w->pos.y - w->width / 2);
    rect(w->pos.x, w->pos.y + w->width / 2, 11, 101 - w->pos.y - w->width / 2);
    if (input.isPressed) {
      color = PURPLE;
    } else {
      color = CYAN;
    }
    for (float y = w->pos.y - w->width / 2 + grvOfs; y < w->pos.y; y += 10) {
      rect(w->pos.x, y, 10, 1);
    }
    for (float y = w->pos.y + w->width / 2 - grvOfs; y > w->pos.y; y -= 10) {
      rect(w->pos.x, y, 10, 1);
    }
    if (GRAVELER_SX >= w->pos.x && GRAVELER_SX < w->pos.x + 10) {
      float f;
      if (sy < w->pos.y) {
        f = -1;
      } else {
        f = 1;
      }
      if (input.isPressed) {
        f *= -1.5;
      }
      svy += sqrt(difficulty) * f * 0.015;
    }
    w->pos.x -= scr;
  }
  float grvDelta;
  if (input.isPressed) {
    grvDelta = 0.25;
  } else {
    grvDelta = -0.16;
  }
  grvOfs = wrap(grvOfs + difficulty * grvDelta, 0, 10);
  nextWallDist -= scr;
  if (nextWallDist <= 0) {
    Wall *w = &walls[nextWallIndex];
    nextWallIndex = (int)wrap(nextWallIndex + 1, 0, GRAVELER_WALL_COUNT);
    nextWall.vy += rnd(0, 0.2) * RNDPM();
    nextWall.y += nextWall.vy;
    nextWall.wy += rnd(0, 0.2) * RNDPM();
    nextWall.w += nextWall.wy;
    if (nextWall.y - nextWall.w / 2 < 20 && nextWall.vy < 0) {
      nextWall.vy += 1;
    }
    if (nextWall.y + nextWall.w / 2 > 80 && nextWall.vy > 0) {
      nextWall.vy -= 1;
    }
    if (nextWall.w < 32 && nextWall.wy < 0) {
      nextWall.wy += 1;
    }
    if (nextWall.w > 60 && nextWall.wy > 0) {
      nextWall.wy -= 1;
    }
    vectorSet(&w->pos, 100 + nextWallDist, nextWall.y);
    w->width = nextWall.w;
    nextWallDist += 10;
  }
  color = BLACK;
  sy += svy;
  svy *= 0.99;
  Collision sc = character("a", GRAVELER_SX, sy);
  if (sc.isColliding.rect[BLUE]) {
    play(EXPLOSION);
    gameOver();
  }
  if (input.isJustPressed) {
    play(LASER);
  } else if (input.isJustReleased) {
    play(HIT);
  }
  FOR_EACH(objs, i) {
    ASSIGN_ARRAY_ITEM(objs, i, Obj, o);
    SKIP_IS_NOT_ALIVE(o);
    if (o->type == GRAVELER_OBJ_TYPE_COIN) {
      color = YELLOW;
      Collision oc = text("o", VEC_XY(o->pos));
      if (oc.isColliding.character['a']) {
        play(POWER_UP);
        addScore(multiplier, VEC_XY(o->pos));
        multiplier++;
        o->isAlive = false;
        continue;
      }
      if (oc.isColliding.rect[BLUE]) {
        if (coinY < 50) {
          coinY += 10;
        } else {
          coinY -= 10;
        }
        o->isAlive = false;
        continue;
      }
    } else {
      color = RED;
      Collision oc2 = text("x", VEC_XY(o->pos));
      if (oc2.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    }
    o->pos.x -= scr;
    if (o->pos.x < -3) {
      if (o->type == GRAVELER_OBJ_TYPE_COIN && multiplier > 1) {
        multiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
  nextObjDist -= scr;
  if (nextObjDist < 0) {
    Wall *w =
        &walls[(int)wrap(nextWallIndex - 1, 0, GRAVELER_WALL_COUNT)];
    if (coinCount == 0) {
      ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, no);
      vectorSet(&no->pos, 103, rnd(w->pos.y - 10, w->pos.y + 10));
      no->type = GRAVELER_OBJ_TYPE_SPIKE;
      no->isAlive = true;
      objIndex = wrap(objIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
      coinCount = rndi(3, 7);
      nextObjDist = rnd(40, 60) * sqrt(difficulty);
      if (rnd(0, 1) < 0.5) {
        coinY = rnd(w->pos.y - w->width * 0.36, w->pos.y - w->width * 0.2);
      } else {
        coinY = rnd(w->pos.y + w->width * 0.36, w->pos.y + w->width * 0.2);
      }
    } else {
      ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, no2);
      vectorSet(&no2->pos, 103, coinY);
      no2->type = GRAVELER_OBJ_TYPE_COIN;
      no2->isAlive = true;
      objIndex = wrap(objIndex + 1, 0, GRAVELER_MAX_OBJ_COUNT);
      coinCount--;
      if (coinCount == 0) {
        nextObjDist = rnd(40, 60) * sqrt(difficulty);
      } else {
        nextObjDist = 7;
      }
    }
  }
}

void addGameGraveler() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
