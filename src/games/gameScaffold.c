#include "cglp.h"

static char *title = "SCAFFOLD";
static char *description = "[Tap]\n Change angle\n[Hold]\n Scaffold ";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "    ll",
        "   lll",
        "  ll  ",
        " ll   ",
        "ll    ",
        "l     ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "llllll",
        "llllll",
    },
    {
        "l     ",
        "ll    ",
        " ll   ",
        "  ll  ",
        "   lll",
        "    ll",
    },
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
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 100, .isDarkColor = false};

#define SCAFFOLD_OBJ_TYPE_GOLD 0
#define SCAFFOLD_OBJ_TYPE_SPIKE 1

typedef struct {
  Vector pos;
  int type;
  bool isAlive;
} Floor;
#define SCAFFOLD_MAX_FLOOR_COUNT 64
static Floor floors[SCAFFOLD_MAX_FLOOR_COUNT];
static int floorIndex;

static Vector nextFloorPos;
static int nextFloorType;
static int tv;
static int pressedCount;
static Vector wallPos;

typedef struct {
  Vector pos;
  float vy;
  float d;
  float distance;
  int type;
  bool isAlive;
} Obj;
#define SCAFFOLD_MAX_OBJ_COUNT 64
static Obj objs[SCAFFOLD_MAX_OBJ_COUNT];
static int objIndex;
static float nextObjDist;

static Vector player;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
    TIMES(9, fi) {
      ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, f0);
      vectorSet(&f0->pos, fi * 6 + 3, 52);
      f0->type = 1;
      f0->isAlive = true;
      floorIndex = wrap(floorIndex + 1, 0, SCAFFOLD_MAX_FLOOR_COUNT);
    }
    vectorSet(&nextFloorPos, 9 * 6 + 3, 52);
    nextFloorType = 0;
    tv = 1;
    pressedCount = 0;
    vectorSet(&wallPos, -9, 0);
    INIT_UNALIVED_ARRAY(objs);
    objIndex = 0;
    nextObjDist = 0;
    vectorSet(&player, 5, 50);
    multiplier = 1;
  }
  float scr = difficulty * 0.05;
  if (nextFloorPos.x > 40) {
    scr += (nextFloorPos.x - 40) * 0.1;
  }
  wallPos.x -= scr;
  color = RED;
  if (wallPos.x < -6) {
    float wy = 47;
    TIMES(9, wi) {
      float y = 47 + rndi(-9, 9) * 4;
      if (fabs(y - nextFloorPos.y) < 25) {
        wy = y;
        break;
      }
    }
    vectorSet(&wallPos, 6 - fmod(nextFloorPos.x, 6) + 120, wy);
    ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, ngo);
    vectorSet(&ngo->pos, wallPos.x + 4, wallPos.y + 4);
    ngo->vy = 0.1;
    ngo->d = 2;
    ngo->distance = 4;
    ngo->type = SCAFFOLD_OBJ_TYPE_GOLD;
    ngo->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, SCAFFOLD_MAX_OBJ_COUNT);
    color = PURPLE;
    rect(100, 0, 40, 100);
    color = RED;
    nextObjDist += 30;
  }
  rect(wallPos.x + 3, 0, 2, wallPos.y - 12);
  rect(wallPos.x + 3, wallPos.y + 18, 2, 100 - 18 - wallPos.y);
  rect(0, -7, 100, 9);
  rect(0, 98, 100, 9);
  if (input.isJustReleased) {
    play(LASER);
    if ((nextFloorType == 0 && tv == -1) ||
        (nextFloorType == 2 && tv == 1)) {
      tv *= -1;
    }
    nextFloorType += tv;
  }
  if (input.isPressed) {
    pressedCount++;
    if (pressedCount > 15 / sqrt(difficulty)) {
      play(SELECT);
      ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, nf);
      float fy;
      if (nextFloorType == 2) {
        fy = nextFloorPos.y + 4;
      } else {
        fy = nextFloorPos.y;
      }
      vectorSet(&nf->pos, nextFloorPos.x, fy);
      nf->type = nextFloorType;
      nf->isAlive = true;
      floorIndex = wrap(floorIndex + 1, 0, SCAFFOLD_MAX_FLOOR_COUNT);
      vectorAdd(&nextFloorPos, 6, nextFloorType * 4 - 4);
      pressedCount = 0;
    }
  } else {
    pressedCount = 0;
  }
  color = BLACK;
  FOR_EACH(floors, fi2) {
    ASSIGN_ARRAY_ITEM(floors, fi2, Floor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= scr;
    char fc[2];
    fc[0] = 'a' + f->type;
    fc[1] = 0;
    character(fc, VEC_XY(f->pos));
    if (f->pos.x < -3) {
      f->isAlive = false;
      continue;
    }
  }
  color = CYAN;
  nextFloorPos.x -= scr;
  char nfc[2];
  nfc[0] = 'a' + nextFloorType;
  nfc[1] = 0;
  float nfy;
  if (nextFloorType == 2) {
    nfy = nextFloorPos.y + 4;
  } else {
    nfy = nextFloorPos.y;
  }
  character(nfc, nextFloorPos.x, nfy);
  float vx = 0;
  if (player.x < 20) {
    vx += (20 - player.x) * 0.2;
  }
  player.x += vx - scr;
  char pc[2];
  pc[0] = 'd' + (ticks / 15) % 2;
  pc[1] = 0;
  Collision pcoll = character(pc, VEC_XY(player));
  if (pcoll.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  color = TRANSPARENT;
  int type = -1;
  TIMES(9, ti) {
    Collision cc = box(player.x + 4, player.y, 1, 6);
    if (cc.isColliding.character['a']) {
      type = 0;
      player.y--;
    } else if (cc.isColliding.character['b']) {
      type = 1;
      player.y--;
    } else if (cc.isColliding.character['c']) {
      type = 2;
      player.y--;
    } else {
      if (type != -1) {
        break;
      }
      player.y++;
    }
  }
  if (type == 0) {
    player.y += 4;
  }
  nextObjDist -= scr;
  if (nextObjDist < 0) {
    int otype;
    if (rnd(0, 1) < 0.5) {
      otype = SCAFFOLD_OBJ_TYPE_GOLD;
    } else {
      otype = SCAFFOLD_OBJ_TYPE_SPIKE;
    }
    float odiv;
    if (otype == SCAFFOLD_OBJ_TYPE_GOLD) {
      odiv = 4;
    } else {
      odiv = 1.5;
    }
    float odistance = rnd(20, 60) / odiv;
    ASSIGN_ARRAY_ITEM(objs, objIndex, Obj, no);
    vectorSet(&no->pos, 120, clamp(nextFloorPos.y + rnd(0, 20) * RNDPM(), 10, 90));
    no->d = odistance / 2;
    no->distance = odistance;
    no->vy = rnd(1, sqrt(difficulty)) * RNDPM() * 0.3;
    no->type = otype;
    no->isAlive = true;
    objIndex = wrap(objIndex + 1, 0, SCAFFOLD_MAX_OBJ_COUNT);
    nextObjDist = rnd(15, 25);
  }
  FOR_EACH(objs, oi2) {
    ASSIGN_ARRAY_ITEM(objs, oi2, Obj, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.x -= scr;
    o->pos.y += o->vy;
    o->d -= fabs(o->vy);
    if (o->d < 0) {
      o->d = o->distance;
      o->vy *= -1;
    }
    if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
      color = YELLOW;
    } else {
      color = RED;
    }
    Collision oc;
    if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
      oc = text("$", VEC_XY(o->pos));
    } else {
      oc = text("x", VEC_XY(o->pos));
    }
    if (o->distance > 4 && oc.isColliding.rect[PURPLE]) {
      o->isAlive = false;
      continue;
    }
    if (oc.isColliding.character['d'] || oc.isColliding.character['e']) {
      if (o->type == SCAFFOLD_OBJ_TYPE_GOLD) {
        play(COIN);
        addScore(multiplier, VEC_XY(o->pos));
        multiplier++;
        o->isAlive = false;
        continue;
      } else {
        play(EXPLOSION);
        gameOver();
      }
    }
    if (o->pos.x < -3) {
      if (o->type == SCAFFOLD_OBJ_TYPE_GOLD && multiplier > 1) {
        play(HIT);
        multiplier--;
      }
      o->isAlive = false;
      continue;
    }
  }
}

void addGameScaffold() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
