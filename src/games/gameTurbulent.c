#include "cglp.h"

static char *title = "TURBULENT";
static char *description = "[Tap] Jump";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 300, .isDarkColor = true};

typedef struct {
  float height;
  float angle;
  float va;
  float x;
} Wave;
static Wave waves[7];

static Vector points[25];

typedef struct {
  float x;
  float vx;
  bool isAlive;
} Mine;
#define TURBULENT_MAX_MINE_COUNT 32
static Mine mines[TURBULENT_MAX_MINE_COUNT];
static int mineIndex;
static float nextMineDist;

#define TURBULENT_STATE_FLOAT 0
#define TURBULENT_STATE_JUMP 1
typedef struct {
  Vector pos;
  Vector pp;
  Vector vel;
  float angle;
  int state;
} Ship;
static Ship ship;
static float jumpX;

static bool getPoints(float x, int *ppIndexOut, int *npIndexOut) {
  TIMES(25, i) {
    int ppIdx = (int)wrap(i - 1, 0, 25);
    int npIdx = i;
    Vector *pp = &points[ppIdx];
    Vector *np = &points[npIdx];
    if (pp->x > np->x) {
      continue;
    }
    if (pp->x <= x && x < np->x) {
      *ppIndexOut = ppIdx;
      *npIndexOut = npIdx;
      return true;
    }
  }
  return false;
}

static void update() {
  if (!ticks) {
    TIMES(7, wi) {
      waves[wi].height = rnd(10, 30);
      waves[wi].angle = (wi % 2) * M_PI + rnd(0, M_PI_4) * RNDPM();
      waves[wi].va = rnd(0.01, 0.02);
      waves[wi].x = wi * 20 - 20;
    }
    TIMES(25, pi5) { vectorSet(&points[pi5], 0, 0); }
    INIT_UNALIVED_ARRAY(mines);
    mineIndex = 0;
    nextMineDist = 0;
    vectorSet(&ship.pos, 40, 60);
    vectorSet(&ship.pp, 40, 60);
    vectorSet(&ship.vel, 0, 0);
    ship.angle = 0;
    ship.state = TURBULENT_STATE_FLOAT;
  }
  float scr = 0.1 * difficulty;
  if (ship.pos.x > 50) {
    scr += (ship.pos.x - 50) * 0.1;
  }
  TIMES(7, wi2) {
    Wave *w = &waves[wi2];
    w->x -= scr;
    if (w->x < -20) {
      w->x += 140;
      w->height = rnd(10, 30);
      w->angle = rnd(0, M_PI * 2);
      w->va = rnd(0.01, 0.02 * sqrt(difficulty));
    }
    w->angle += w->va;
    vectorSet(&points[wi2 * 4], w->x, 60 + sin(w->angle) * w->height);
  }
  color = BLUE;
  TIMES(25, pii) {
    int im = pii % 4;
    if (im != 0) {
      Vector *pp2 = &points[(pii / 4) * 4];
      Vector *np2 = &points[(pii / 4 + 1) * 4];
      float rr;
      if (im == 1) {
        rr = 0.2;
      } else if (im == 2) {
        rr = 0.5;
      } else {
        rr = 0.8;
      }
      vectorSet(&points[pii], pp2->x + 5 * im, pp2->y * (1 - rr) + np2->y * rr);
    }
    int prevIdx = (int)wrap(pii - 1, 0, 25);
    Vector *prevP = &points[prevIdx];
    Vector *curP = &points[pii];
    if (prevP->x < curP->x) {
      line(prevP->x, prevP->y, curP->x, curP->y);
    }
  }
  nextMineDist -= scr;
  if (nextMineDist < 0) {
    ASSIGN_ARRAY_ITEM(mines, mineIndex, Mine, nm);
    nm->x = 103;
    nm->vx = 0;
    nm->isAlive = true;
    mineIndex = wrap(mineIndex + 1, 0, TURBULENT_MAX_MINE_COUNT);
    nextMineDist = rnd(100, 120) / sqrt(difficulty);
  }
  color = RED;
  FOR_EACH(mines, mi) {
    ASSIGN_ARRAY_ITEM(mines, mi, Mine, m);
    SKIP_IS_NOT_ALIVE(m);
    m->x -= scr;
    int ppIdx, npIdx;
    bool found = getPoints(m->x, &ppIdx, &npIdx);
    if (!found) {
      m->isAlive = false;
      continue;
    }
    Vector *pp3 = &points[ppIdx];
    Vector *np3 = &points[npIdx];
    float oy = np3->y - pp3->y;
    m->vx += oy * 0.001;
    m->vx *= 0.9;
    m->x += m->vx * sqrt(difficulty);
    float r2 = (m->x - pp3->x) / (np3->x - pp3->x);
    text("*", m->x, pp3->y + oy * r2 - 5);
    if (m->x < -3) {
      m->isAlive = false;
      continue;
    }
  }
  float sa = 0;
  if (ship.state == TURBULENT_STATE_FLOAT) {
    int ppIdx2, npIdx2;
    bool found2 = getPoints(ship.pos.x, &ppIdx2, &npIdx2);
    if (found2) {
      Vector *pp4 = &points[ppIdx2];
      Vector *np4 = &points[npIdx2];
      float oy2 = np4->y - pp4->y;
      ship.vel.x += oy2 * 0.002;
      ship.vel.x *= 0.925;
      ship.vel.x += 0.025;
      ship.pos.x += ship.vel.x;
      float r3 = (ship.pos.x - pp4->x) / (np4->x - pp4->x);
      ship.pos.y = pp4->y + oy2 * r3;
      sa = angleTo(pp4, np4->x, np4->y);
    }
    if (input.isJustPressed) {
      play(JUMP);
      jumpX = ship.pos.x;
      ship.vel.x = (ship.pos.x - ship.pp.x) * 2;
      ship.vel.y = (ship.pos.y - ship.pp.y) * 5;
      addWithAngle(&ship.vel, sa - M_PI_2, 1);
      if (ship.vel.y > -1) {
        ship.vel.y = -1;
      }
      vectorAdd(&ship.pos, ship.vel.x, ship.vel.y);
      ship.state = TURBULENT_STATE_JUMP;
    }
  } else {
    jumpX -= scr;
    ship.vel.x += 0.005;
    float dvy;
    if (input.isPressed) {
      dvy = 0.02;
    } else {
      dvy = 0.1;
    }
    ship.vel.y += dvy;
    vectorMul(&ship.vel, 0.98);
    vectorAdd(&ship.pos, ship.vel.x, ship.vel.y);
    sa = vectorAngle(&ship.vel);
  }
  ship.pos.x -= scr;
  ship.pos.x = clamp(ship.pos.x, 5, 95);
  ship.pos.y = clamp(ship.pos.y, 5, 95);
  ship.pp = ship.pos;
  ship.angle += wrap(sa - ship.angle, -M_PI, M_PI) * 0.1;
  sa = ship.angle;
  Vector p2;
  vectorSet(&p2, ship.pos.x, ship.pos.y);
  addWithAngle(&p2, sa - M_PI * 0.5, 2);
  color = RED;
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(p2.x, p2.y, 3, sa);
  addWithAngle(&p2, sa - M_PI * 0.4, 2);
  color = BLACK;
  bar(p2.x, p2.y, 4, sa);
  addWithAngle(&p2, sa - M_PI * 0.6, 2);
  Collision c = bar(p2.x, p2.y, 1, sa);
  if (ship.state == TURBULENT_STATE_JUMP && c.isColliding.rect[BLUE]) {
    float d = ship.pos.x - jumpX;
    play(HIT);
    if (d > 0) {
      play(POWER_UP);
      addScore(ceil(sqrt(d * d)), ship.pos.x, ship.pos.y);
    }
    ship.state = TURBULENT_STATE_FLOAT;
    ship.vel.x *= 0.5;
  }
  if (c.isColliding.text['*']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTurbulent() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
