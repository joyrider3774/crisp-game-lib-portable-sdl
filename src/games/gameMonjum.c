#include "cglp.h"

static char *title = "MONJUM";
static char *description = "[Hold] Back\n[Release] Jump";

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
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

#define MONJUM_ITEM_HOLE 1
#define MONJUM_ITEM_MON 2
#define MONJUM_ITEM_COIN 4

typedef struct {
  float x;
  float w;
  bool isAlive;
} MonjumFloor;
#define MONJUM_MAX_FLOOR_COUNT 16
static MonjumFloor floors[MONJUM_MAX_FLOOR_COUNT];
static int floorIndex;
static float flt;

typedef struct {
  Vector pos;
  Vector vel;
  bool jump;
  bool isAlive;
} MonjumMon;
#define MONJUM_MAX_MON_COUNT 16
static MonjumMon mons[MONJUM_MAX_MON_COUNT];
static int monIndex;
static float mnt;

typedef struct {
  Vector pos;
  bool isAlive;
} MonjumCoin;
#define MONJUM_MAX_COIN_COUNT 16
static MonjumCoin coins[MONJUM_MAX_COIN_COUNT];
static int coinIndex;
static float cnt;

static Vector p;
static Vector v;
static bool jump;
static bool fall;
static int item;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(floors);
    floors[0].x = 0;
    floors[0].w = 99;
    floors[0].isAlive = true;
    floorIndex = 1;
    INIT_UNALIVED_ARRAY(mons);
    monIndex = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    flt = 0;
    mnt = 0;
    cnt = 0;
    vectorSet(&p, 50, 87);
    vectorSet(&v, 0, 0);
    jump = false;
    fall = false;
  }
  color = BLACK;
  char pc[2] = {'a' + ((int)(ticks / 30) % 2), 0};
  character(pc, p.x, p.y);
  float fvy = difficulty * 0.5;
  flt -= fvy;
  if (flt < 0) {
    float w = rndi(50, 90);
    ASSIGN_ARRAY_ITEM(floors, floorIndex, MonjumFloor, nf);
    nf->x = 110;
    nf->w = w;
    nf->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MONJUM_MAX_FLOOR_COUNT);
    flt += 10 + w;
  }
  color = BLACK;
  FOR_EACH(floors, i) {
    ASSIGN_ARRAY_ITEM(floors, i, MonjumFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->x -= fvy;
    rect(f->x, 90, f->w, 9);
    if (f->x + f->w <= 0) {
      f->isAlive = false;
    }
  }
  mnt -= difficulty;
  if (mnt < 0) {
    ASSIGN_ARRAY_ITEM(mons, monIndex, MonjumMon, nm);
    vectorSet(&nm->pos, 99, 87);
    vectorSet(&nm->vel, -difficulty * 0.75, 0);
    nm->jump = false;
    nm->isAlive = true;
    monIndex = wrap(monIndex + 1, 0, MONJUM_MAX_MON_COUNT);
    mnt += rnd(200, 300);
  }
  FOR_EACH(mons, i) {
    ASSIGN_ARRAY_ITEM(mons, i, MonjumMon, m);
    SKIP_IS_NOT_ALIVE(m);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);
    if (m->jump) {
      m->vel.y += difficulty * 0.09;
      if (m->pos.y > 87) {
        m->pos.y = 87;
        m->vel.y = 0;
        m->jump = false;
      }
    } else {
      color = TRANSPARENT;
      if (!box(m->pos.x - 4, m->pos.y + 6, 6, 6).isColliding.rect[BLACK]) {
        m->jump = true;
        m->vel.y = -1.4 * sqrt(difficulty);
      }
    }
    color = PURPLE;
    characterOptions.isMirrorX = true;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    char mc[2] = {'c' + ((int)(ticks / 30) % 2), 0};
    Collision mcCol = character(mc, m->pos.x, m->pos.y);
    if (mcCol.isColliding.character['a'] || mcCol.isColliding.character['b']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.x <= -3) {
      m->isAlive = false;
    }
  }
  characterOptions.isMirrorX = false;
  cnt -= difficulty;
  if (cnt < 0) {
    ASSIGN_ARRAY_ITEM(coins, coinIndex, MonjumCoin, nc);
    vectorSet(&nc->pos, 99, 60);
    nc->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, MONJUM_MAX_COIN_COUNT);
    cnt += rnd(50, 100);
  }
  color = YELLOW;
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, MonjumCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= difficulty;
    Collision cCol = text("$", c->pos.x, c->pos.y);
    if (cCol.isColliding.character['a'] || cCol.isColliding.character['b']) {
      play(LASER);
      item |= MONJUM_ITEM_COIN;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x <= -3) {
      c->isAlive = false;
    }
  }
  color = TRANSPARENT;
  Collision pCol = box(p.x, p.y, 6, 50);
  if (pCol.isColliding.character['c'] || pCol.isColliding.character['d']) {
    item |= MONJUM_ITEM_MON;
  }
  Collision holeCheck = box(p.x, 93, 6, 6);
  bool hasHole = !holeCheck.isColliding.rect[BLACK];
  if (hasHole) {
    item |= MONJUM_ITEM_HOLE;
  }
  if (fall) {
    v.y += 0.14 * difficulty;
    if (p.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
  } else if (jump) {
    v.y += 0.12 * difficulty;
    if (p.y > 87) {
      p.y = 87;
      v.y = 0;
      if (hasHole) {
        fall = true;
      } else {
        jump = false;
        int bc = (item & 1) + ((item >> 1) & 1) + ((item >> 2) & 1);
        int scoreTable[4] = {0, 1, 5, 20};
        int sc = scoreTable[bc];
        if (sc > 0) {
          play(COIN);
          addScore(sc, p.x, p.y);
        }
      }
    }
  } else {
    if (hasHole) {
      fall = true;
    }
    if (input.isPressed) {
      v.x = -0.5 * difficulty;
    } else {
      v.x = 0;
      p.x += (50 - p.x) * 0.02;
    }
    if (input.isJustReleased && ticks > 5) {
      play(JUMP);
      v.y = -2.5 * sqrt(difficulty);
      jump = true;
      item = 0;
    }
  }
  vectorAdd(&p, v.x, v.y);
  p.x = clamp(p.x, 0, 50);
}

void addGameMonjum() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
