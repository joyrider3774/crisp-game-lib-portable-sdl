#include "cglp.h"
#include "string.h"

static char *title = "ZIP FALL";
static char *description = "[Tap] Fall";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "b bb b",
        " bbbb ",
        "bbbbbb",
        " b  b ",
        " b  b ",
        "      ",
    },
    {
        "r rr r",
        "rrrrrr",
        " r  r ",
        "      ",
        "      ",
        "      ",
    },
    {
        "  yy  ",
        " yyyy ",
        "y yy y",
        "y yy y",
        " yyyy ",
        "  yy  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  float y;
  bool isAlive;
} ZipLine;
// Port note: upstream's backpacker.currentZip is a live reference to one of
// the (growable) zipLines array's objects, read every frame while attached
// (it keeps scrolling with the rest of the line until either the player
// detaches by tapping again, or the line itself scrolls off the top of the
// screen). There are no live references across this port's fixed arrays that
// survive index recycling, so the backpacker instead stores which zip-line
// slot to keep reading (index + a "currently attached at all" flag). Zip
// lines are spawned roughly every 20-40 scrolled units and live for ~100
// scrolled units (much longer than the gap between spawns), so at most a
// handful are ever alive at once; ZIPFALL_MAX_ZIPLINE_COUNT is sized
// generously above that so the ring buffer never recycles a slot the
// backpacker is still attached to.
#define ZIPFALL_MAX_ZIPLINE_COUNT 32
static ZipLine zipLines[ZIPFALL_MAX_ZIPLINE_COUNT];
static int zipLineIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool hasZip;
  int zipIndex;
} Backpacker;
static Backpacker backpacker;

#define ZIPFALL_TYPE_OBSTACLE 0
#define ZIPFALL_TYPE_ITEM 1
typedef struct {
  Vector pos;
  float vx;
  int type;
  bool isAlive;
} Object;
#define ZIPFALL_MAX_OBJECT_COUNT 32
static Object objects[ZIPFALL_MAX_OBJECT_COUNT];
static int objectIndex;

static float scrollSpeed;
static float nextZipDist;
static float nextObjectTicks;
static float multiplier;

static void addZipLine(float y) {
  ASSIGN_ARRAY_ITEM(zipLines, zipLineIndex, ZipLine, z);
  z->y = y;
  z->isAlive = true;
  zipLineIndex = wrap(zipLineIndex + 1, 0, ZIPFALL_MAX_ZIPLINE_COUNT);
}

static void updateBackpacker() {
  if (input.isJustPressed) {
    play(POWER_UP);
    backpacker.pos.y += 5;
    backpacker.vel.y = 1;
    backpacker.hasZip = false;
  }
  if (!backpacker.hasZip) {
    backpacker.vel.y += 0.1;
    backpacker.pos.y += backpacker.vel.y * difficulty - scrollSpeed;
  } else {
    backpacker.pos.y = zipLines[backpacker.zipIndex].y + 3;
    backpacker.pos.x += backpacker.vel.x * difficulty;
  }
  if ((backpacker.pos.x < 20 && backpacker.vel.x < 0) ||
      (backpacker.pos.x > 80 && backpacker.vel.x > 0)) {
    backpacker.vel.x = -backpacker.vel.x;
    if (multiplier >= 2) {
      multiplier--;
    }
  }
  if (backpacker.pos.y < 3) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLUE;
  character("a", VEC_XY(backpacker.pos));
}

static void updateZipLines() {
  color = BLACK;
  FOR_EACH(zipLines, zi) {
    ASSIGN_ARRAY_ITEM(zipLines, zi, ZipLine, z);
    SKIP_IS_NOT_ALIVE(z);
    Collision scratch = box(50, z->y, 60, 2);
    if (scratch.isColliding.character['a'] && !backpacker.hasZip) {
      play(CLICK);
      backpacker.hasZip = true;
      backpacker.zipIndex = zi;
    }
    z->y -= scrollSpeed;
    z->isAlive = z->y >= 0;
  }

  nextZipDist -= scrollSpeed;
  if (nextZipDist < 0) {
    addZipLine(100);
    nextZipDist = rnd(20, 40);
  }
}

static void updateObjects() {
  nextObjectTicks -= difficulty;
  if (nextObjectTicks <= 0) {
    int type;
    if (rnd(0, 1) < 0.6) {
      type = ZIPFALL_TYPE_OBSTACLE;
    } else {
      type = ZIPFALL_TYPE_ITEM;
    }
    float vx = rnd(0.5, 1) * RNDPM();
    Vector pos;
    if (vx > 0) {
      vectorSet(&pos, -3, rnd(0, 120));
    } else {
      vectorSet(&pos, 103, rnd(0, 120));
    }
    ASSIGN_ARRAY_ITEM(objects, objectIndex, Object, no);
    no->pos = pos;
    no->vx = vx;
    no->type = type;
    no->isAlive = true;
    objectIndex = wrap(objectIndex + 1, 0, ZIPFALL_MAX_OBJECT_COUNT);
    nextObjectTicks = rnd(20, 30);
  }

  FOR_EACH(objects, oi) {
    ASSIGN_ARRAY_ITEM(objects, oi, Object, obj);
    SKIP_IS_NOT_ALIVE(obj);
    obj->pos.y -= scrollSpeed;
    obj->pos.x += obj->vx * difficulty;
    if (obj->type == ZIPFALL_TYPE_OBSTACLE) {
      color = RED;
      Collision oc = character("b", VEC_XY(obj->pos));
      if (oc.isColliding.character['a']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      color = YELLOW;
      Collision ic = character("c", VEC_XY(obj->pos));
      if (ic.isColliding.character['a']) {
        play(COIN);
        addScore(multiplier, VEC_XY(obj->pos));
        multiplier += difficulty * 3;
        obj->isAlive = false;
        continue;
      }
    }
    obj->isAlive = !(obj->pos.x < -5 || obj->pos.x > 105);
  }
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(zipLines);
    zipLineIndex = 0;
    addZipLine(20);
    addZipLine(50);
    addZipLine(75);
    vectorSet(&backpacker.pos, 20, 20);
    vectorSet(&backpacker.vel, 1, 0);
    backpacker.hasZip = true;
    backpacker.zipIndex = 0;
    INIT_UNALIVED_ARRAY(objects);
    objectIndex = 0;
    scrollSpeed = 1;
    nextZipDist = 0;
    nextObjectTicks = 0;
    multiplier = 1;
  }

  scrollSpeed = 0.05 * difficulty;
  if (backpacker.pos.y > 30) {
    scrollSpeed -= (30 - backpacker.pos.y) * 0.1;
  }
  updateBackpacker();
  updateZipLines();
  updateObjects();

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(multiplier)));
  text(multText, 2, 10);
}

void addGameZipFall() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
