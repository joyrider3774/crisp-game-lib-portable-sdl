#include "cglp.h"

static char *title = "SCRAMBIRD";
static char *description = "[Tap]\n Fly & Shoot";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " rr   ",
        " rr   ",
        "yrry  ",
        "yrry  ",
        "y  y  ",
        "y  y  ",
    },
    {
        " bbbb ",
        "bbbbbb",
        " bbbb ",
        " r  r ",
        "r    r",
            "      ",
},
    {
        "pp    ",
        " l ll ",
        " rlrrl",
        " rllll",
        "   lr ",
            "      ",
},
    {
        "      ",
        " l ll ",
        " rlrrl",
        " rllll",
        " l lr ",
        "pp    ",
    },
    {
        "l l   ",
            "      ",
        "      ",
        "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3000, .isDarkColor = false};

static int wallColors[4] = {PURPLE, BLUE, GREEN, RED};

typedef struct {
  float x;
  float height;
} Wall;
static Wall walls[11];
static float wallHeight;
static float wallHeightVel;

typedef struct {
  Vector pos;
  float launchTicks;
  bool isAlive;
} Missile;
#define SCRAMBIRD_MAX_MISSILE_COUNT 64
static Missile missiles[SCRAMBIRD_MAX_MISSILE_COUNT];
static int missileIndex;

typedef struct {
  Vector pos;
  bool isAlive;
} Tank;
#define SCRAMBIRD_MAX_TANK_COUNT 64
static Tank tanks[SCRAMBIRD_MAX_TANK_COUNT];
static int tankIndex;
static float nextTankDist;

typedef struct {
  Vector pos;
  float vy;
} Ship;
static Ship ship;

typedef struct {
  Vector pos;
  bool isAlive;
} Shot;
#define SCRAMBIRD_MAX_SHOT_COUNT 64
static Shot shots[SCRAMBIRD_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Bomb;
#define SCRAMBIRD_MAX_BOMB_COUNT 64
static Bomb bombs[SCRAMBIRD_MAX_BOMB_COUNT];
static int bombIndex;

static float fuel;
static int multiplier;

static void update() {
  if (!ticks) {
    TIMES(11, wi) {
      walls[wi].x = wi * 10;
      walls[wi].height = 10;
    }
    wallHeight = 10;
    wallHeightVel = 0;
    INIT_UNALIVED_ARRAY(missiles);
    missileIndex = 0;
    INIT_UNALIVED_ARRAY(tanks);
    tankIndex = 0;
    nextTankDist = 10;
    vectorSet(&ship.pos, 10, 50);
    ship.vy = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    INIT_UNALIVED_ARRAY(bombs);
    bombIndex = 0;
    fuel = 50;
    multiplier = 1;
  }
  float scr = difficulty * 0.3;
  int wallColor = wallColors[(ticks / 420) % 4];
  color = wallColor;
  TIMES(11, wi2) {
    Wall *w = &walls[wi2];
    w->x -= scr;
    if (w->x < -10) {
      w->x += 110;
      wallHeight += wallHeightVel;
      if ((wallHeight < 10 && wallHeightVel < 0) ||
          (wallHeight > 50 && wallHeightVel > 0)) {
        wallHeightVel *= -1;
        wallHeight += wallHeightVel;
      } else if (rnd(0, 1) < 0.2) {
        wallHeightVel = 0;
      } else if (rnd(0, 1) < 0.3) {
        if (rnd(0, 1) < 0.5) {
          wallHeightVel = -10;
        } else {
          wallHeightVel = 10;
        }
      }
      w->height = wallHeight;
      nextTankDist--;
      if (nextTankDist < 0) {
        ASSIGN_ARRAY_ITEM(tanks, tankIndex, Tank, nt);
        vectorSet(&nt->pos, w->x + 5, 90 - w->height - 3);
        nt->isAlive = true;
        tankIndex = wrap(tankIndex + 1, 0, SCRAMBIRD_MAX_TANK_COUNT);
        nextTankDist = rnd(1, 16);
      } else if (rnd(0, 1) < 0.5) {
        ASSIGN_ARRAY_ITEM(missiles, missileIndex, Missile, nm);
        vectorSet(&nm->pos, w->x + 5, 90 - w->height - 3);
        if (rnd(0, 1) < 0.5 / sqrt(difficulty)) {
          nm->launchTicks = 9999;
        } else {
          nm->launchTicks = rnd(200, 300) / difficulty;
        }
        nm->isAlive = true;
        missileIndex = wrap(missileIndex + 1, 0, SCRAMBIRD_MAX_MISSILE_COUNT);
      }
    }
    rect(w->x, 90 - w->height, 9, w->height);
    rect(w->x, 0, 9, 5);
  }
  color = BLACK;
  if (input.isJustPressed) {
    if (fuel > 0) {
      play(LASER);
    } else {
      play(HIT);
    }
    float vyDec;
    if (fuel > 0) {
      vyDec = 0.5;
    } else {
      vyDec = 0.1;
    }
    ship.vy -= difficulty * vyDec;
    ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
    ns->pos = ship.pos;
    ns->isAlive = true;
    shotIndex = wrap(shotIndex + 1, 0, SCRAMBIRD_MAX_SHOT_COUNT);
    ASSIGN_ARRAY_ITEM(bombs, bombIndex, Bomb, nb);
    nb->pos = ship.pos;
    vectorSet(&nb->vel, 2 * sqrt(difficulty), 0);
    nb->isAlive = true;
    bombIndex = wrap(bombIndex + 1, 0, SCRAMBIRD_MAX_BOMB_COUNT);
  }
  ship.vy += 0.015 * difficulty;
  ship.vy *= 0.98;
  ship.pos.y += ship.vy;
  char shipChar[2];
  if (ship.vy < 0) {
    shipChar[0] = 'c';
  } else {
    shipChar[0] = 'd';
  }
  shipChar[1] = 0;
  Collision shipColl = character(shipChar, VEC_XY(ship.pos));
  if (shipColl.isColliding.rect[wallColor]) {
    play(EXPLOSION);
    gameOver();
  }
  color = RED;
  particle(ship.pos.x - 2, ship.pos.y, 0.5, 0.5, M_PI, M_PI / 5);
  FOR_EACH(shots, si) {
    ASSIGN_ARRAY_ITEM(shots, si, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x += 2 * sqrt(difficulty);
    Collision sc = character("e", VEC_XY(s->pos));
    if (sc.isColliding.rect[wallColor]) {
      s->isAlive = false;
      continue;
    }
    if (s->pos.x > 103) {
      s->isAlive = false;
      continue;
    }
  }
  color = CYAN;
  FOR_EACH(bombs, bi) {
    ASSIGN_ARRAY_ITEM(bombs, bi, Bomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vel.y += 0.1 * difficulty;
    vectorMul(&b->vel, 0.9);
    vectorAdd(&b->pos, VEC_XY(b->vel));
    thickness = 2;
    Collision bc = bar(VEC_XY(b->pos), 2, vectorAngle(&b->vel));
    if (bc.isColliding.rect[wallColor]) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(missiles, mi) {
    ASSIGN_ARRAY_ITEM(missiles, mi, Missile, m);
    SKIP_IS_NOT_ALIVE(m);
    m->pos.x -= scr;
    m->launchTicks--;
    if (m->launchTicks < 0) {
      m->pos.y -= difficulty * 0.5;
    }
    color = BLACK;
    Collision mc = character("a", VEC_XY(m->pos));
    if (mc.isColliding.character['e'] || mc.isColliding.rect[CYAN]) {
      play(HIT);
      color = RED;
      particle(VEC_XY(m->pos), 16, 1, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(m->pos));
      multiplier++;
      m->isAlive = false;
      continue;
    }
    if (mc.isColliding.character['c'] || mc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.x < -3 || m->pos.y < -3) {
      if (multiplier > 1) {
        multiplier--;
      }
      m->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(tanks, ti2) {
    ASSIGN_ARRAY_ITEM(tanks, ti2, Tank, t);
    SKIP_IS_NOT_ALIVE(t);
    t->pos.x -= scr;
    Collision tc = character("b", VEC_XY(t->pos));
    if (tc.isColliding.character['e'] || tc.isColliding.rect[CYAN]) {
      play(POWER_UP);
      color = BLUE;
      particle(VEC_XY(t->pos), 16, 1, 0, M_PI * 2);
      fuel = clamp(fuel + 10, 0, 50);
      t->isAlive = false;
      continue;
    }
    if (tc.isColliding.character['c'] || tc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    if (t->pos.x < -3) {
      t->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(shots, si2) {
    ASSIGN_ARRAY_ITEM(shots, si2, Shot, s2);
    SKIP_IS_NOT_ALIVE(s2);
    Collision sc2 = character("e", VEC_XY(s2->pos));
    if (sc2.isColliding.character['a'] || sc2.isColliding.character['b']) {
      s2->isAlive = false;
      continue;
    }
  }
  FOR_EACH(bombs, bi2) {
    ASSIGN_ARRAY_ITEM(bombs, bi2, Bomb, b2);
    SKIP_IS_NOT_ALIVE(b2);
    thickness = 2;
    Collision bc2 = bar(VEC_XY(b2->pos), 2, vectorAngle(&b2->vel));
    if (bc2.isColliding.character['a'] || bc2.isColliding.character['b']) {
      b2->isAlive = false;
      continue;
    }
  }
  fuel = clamp(fuel - difficulty * 0.025, 0, 50);
  color = YELLOW;
  text("FUEL", 10, 93);
  rect(40, 90, fuel, 6);
  color = BLUE;
  rect(40 + fuel, 90, 50 - fuel, 6);
}

void addGameScrambird() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
