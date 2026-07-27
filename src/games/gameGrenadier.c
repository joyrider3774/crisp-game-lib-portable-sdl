#include "cglp.h"

static char *title = "GRENADIER";
static char *description = "[Tap]  Climb out\n[Hold] Throw";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        " ll   ",
        "  llll",
        "ll   l",
    },
    {
        "ll    ",
        "l  ll ",
        " ll   ",
        "ll    ",
        "  llll",
        "ll    ",
    },
    {
        "  ll  ",
        "  l   ",
        " lllll",
        "l l   ",
        "  llll",
        "ll    ",
    },
    {
        "   ll ",
        "ll l  ",
        "  lll ",
        "   l l",
        "llll  ",
        "    l ",
    },
    {
        "  ll  ",
        "llll  ",
        "  lll ",
        " llll ",
        "l l ll",
        " llll ",
    },
    {
        " llll ",
        "llll  ",
        " llll ",
            "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 80, .soundSeed = 5, .isDarkColor = false};

#define GRENADIER_STATE_IN_HOLE 0
#define GRENADIER_STATE_THROWING 1
#define GRENADIER_STATE_RUNNING 2

typedef struct {
  float x;
  bool isAlive;
} Hole;
#define GRENADIER_MAX_HOLE_COUNT 32
static Hole holes[GRENADIER_MAX_HOLE_COUNT];
static int holeIndex;

typedef struct {
  float x;
  float vx;
  float fireTicks;
  float fireInterval;
  float fireSpeed;
  bool isAlive;
} Tank;
#define GRENADIER_MAX_TANK_COUNT 32
static Tank tanks[GRENADIER_MAX_TANK_COUNT];
static int tankIndex;
static float tankAddTicks;

typedef struct {
  float x;
  float vx;
  bool isAlive;
} Bullet;
#define GRENADIER_MAX_BULLET_COUNT 32
static Bullet bullets[GRENADIER_MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Grenade;
#define GRENADIER_MAX_GRENADE_COUNT 16
static Grenade grenades[GRENADIER_MAX_GRENADE_COUNT];
static int grenadeIndex;

static float px;
static int pState;
static float pAngle;
static float speedRatio;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(holes);
    holeIndex = 0;
    ASSIGN_ARRAY_ITEM(holes, holeIndex, Hole, h0);
    h0->x = 10;
    h0->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, GRENADIER_MAX_HOLE_COUNT);
    INIT_UNALIVED_ARRAY(tanks);
    tankIndex = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    INIT_UNALIVED_ARRAY(grenades);
    grenadeIndex = 0;
    px = 10;
    pState = GRENADIER_STATE_IN_HOLE;
    tankAddTicks = 0;
    speedRatio = 1;
  }
  float scr = (px - 10) * 0.05;
  color = BLACK;
  rect(0, 70, 200, 9);
  color = WHITE;
  FOR_EACH(holes, i) {
    ASSIGN_ARRAY_ITEM(holes, i, Hole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    if (box(h->x, 70, 6, 10).isColliding.character['e']) {
      h->isAlive = false;
      continue;
    }
    if (h->x <= -4) {
      h->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(tanks, i) {
    ASSIGN_ARRAY_ITEM(tanks, i, Tank, t);
    SKIP_IS_NOT_ALIVE(t);
    character("e", t->x, 67);
  }
  color = BLACK;
  FOR_EACH(grenades, i) {
    ASSIGN_ARRAY_ITEM(grenades, i, Grenade, g);
    SKIP_IS_NOT_ALIVE(g);
    vectorAdd(&g->pos, g->vel.x, g->vel.y);
    g->vel.y += 0.1 * difficulty;
    if (text("o", VEC_XY(g->pos)).isColliding.character['e']) {
      g->isAlive = false;
      continue;
    }
    if (g->pos.y > 68) {
      play(HIT);
      particle(g->pos.x, g->pos.y, 10, 1, -M_PI_2, M_PI * 0.7);
      ASSIGN_ARRAY_ITEM(holes, holeIndex, Hole, nh);
      nh->x = g->pos.x;
      nh->isAlive = true;
      holeIndex = wrap(holeIndex + 1, 0, GRENADIER_MAX_HOLE_COUNT);
      g->isAlive = false;
      continue;
    }
  }
  tankAddTicks--;
  if (tankAddTicks < 0) {
    float sd = rnd(0, sqrt(difficulty) - 1) + 1;
    float fi = 300 / (rnd(0, sqrt(difficulty)) + 1);
    float fs = rnd(0, sqrt(sqrt(difficulty)) - 1) + 1;
    ASSIGN_ARRAY_ITEM(tanks, tankIndex, Tank, nt);
    nt->x = 203;
    nt->vx = 0.08 * sd;
    nt->fireTicks = rnd(0, fi);
    nt->fireInterval = fi;
    nt->fireSpeed = 0.4 * fs;
    nt->isAlive = true;
    tankIndex = wrap(tankIndex + 1, 0, GRENADIER_MAX_TANK_COUNT);
    tankAddTicks = 200 / (rnd(0, sqrt(difficulty)) + 1);
  }
  FOR_EACH(tanks, i) {
    ASSIGN_ARRAY_ITEM(tanks, i, Tank, t);
    SKIP_IS_NOT_ALIVE(t);
    t->x -= t->vx * speedRatio + scr;
    color = TRANSPARENT;
    if (box(t->x, 67, 6, 6).isColliding.text['o']) {
      play(EXPLOSION);
      color = RED;
      particle(t->x, 67, 20, 2, -M_PI_2, M_PI * 1.2);
      addScore(t->x, t->x, 67);
      t->isAlive = false;
      continue;
    }
    t->fireTicks--;
    if (t->x > 150 && pState != GRENADIER_STATE_IN_HOLE && t->fireTicks < 0) {
      play(LASER);
      t->fireTicks = t->fireInterval;
      ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
      nb->x = t->x - 5;
      nb->vx = t->fireSpeed;
      nb->isAlive = true;
      bulletIndex = wrap(bulletIndex + 1, 0, GRENADIER_MAX_BULLET_COUNT);
    }
    if (t->x <= -4) {
      t->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(bullets, i) {
    ASSIGN_ARRAY_ITEM(bullets, i, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->x -= b->vx * speedRatio + scr;
    character("f", b->x, 65);
    if (b->x <= -4) {
      b->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(tanks, aliveTankCount);
  if (aliveTankCount == 0) {
    tankAddTicks *= 0.7;
  }
  color = TRANSPARENT;
  FOR_EACH(holes, i) {
    ASSIGN_ARRAY_ITEM(holes, i, Hole, h);
    SKIP_IS_NOT_ALIVE(h);
    if (box(h->x, 70, 7, 10).isColliding.character['e']) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  px -= scr;
  if (pState == GRENADIER_STATE_IN_HOLE) {
    speedRatio += 0.05;
    if (input.isJustPressed) {
      pState = GRENADIER_STATE_RUNNING;
      px += 6;
    } else {
      if (character("a", px, 72).isColliding.character['e']) {
        play(RANDOM);  // Equivalent to "lucky" in JS
        gameOver();
      }
      color = TRANSPARENT;
      if (box(px + 5, 72, 6, 6).isColliding.rect[WHITE]) {
        px++;
      }
    }
  } else if (pState == GRENADIER_STATE_RUNNING) {
    speedRatio += (1 - speedRatio) * 0.1;
    px += 0.8 * sqrt(difficulty);
    char rc[2];
    rc[0] = 'c' + (int)floor(ticks / 30) % 2;
    rc[1] = 0;
    Collision runColl = character(rc, px, 67);
    if (runColl.isColliding.character['e'] || runColl.isColliding.character['f']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    if (runColl.isColliding.rect[WHITE]) {
      pState = GRENADIER_STATE_IN_HOLE;
    } else if (input.isJustPressed) {
      pState = GRENADIER_STATE_THROWING;
      pAngle = 0;
    }
  } else if (pState == GRENADIER_STATE_THROWING) {
    speedRatio += (1 - speedRatio) * 0.1;
    Vector p;
    vectorSet(&p, px, 67);
    if (input.isJustReleased || pAngle < -1) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(grenades, grenadeIndex, Grenade, ng);
      ng->pos = p;
      vectorSet(&ng->vel, (4 - pAngle * 0.5) * sqrt(difficulty), 0);
      rotate(&ng->vel, pAngle);
      ng->isAlive = true;
      grenadeIndex = wrap(grenadeIndex + 1, 0, GRENADIER_MAX_GRENADE_COUNT);
      pState = GRENADIER_STATE_RUNNING;
    } else {
      Collision bcoll = character("b", VEC_XY(p));
      if (bcoll.isColliding.character['e'] || bcoll.isColliding.character['f']) {
        play(RANDOM);  // Equivalent to "lucky" in JS
        gameOver();
      }
      Vector dir;
      vectorSet(&dir, 10, 0);
      rotate(&dir, pAngle);
      thickness = 2;
      line(p.x, p.y, p.x + dir.x, p.y + dir.y);
      pAngle -= 0.02 * sqrt(difficulty);
    }
  }
}

void addGameGrenadier() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
