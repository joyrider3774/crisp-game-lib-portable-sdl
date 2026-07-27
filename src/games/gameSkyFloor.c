#include "cglp.h"

static char *title = "SKY FLOOR";
static char *description = "[Slide] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 100, .isDarkColor = false};

#define BALL_RADIUS 2

typedef struct {
  Vector pos;
  Vector size;
  bool isAlive;
} Floor;
#define MAX_FLOOR_COUNT 32
static Floor floors[MAX_FLOOR_COUNT];
static int floorIndex;
static float lastFloorX;
static float lastFloorWidth;
static float nextFloorDist;

typedef struct {
  Vector pos;
  float speed;
  float angle;
  float ticks;
  bool isAlive;
} Fan;
#define MAX_FAN_COUNT 32
static Fan fans[MAX_FAN_COUNT];
static int fanIndex;

typedef struct {
  Vector pos;
  bool isAlive;
} Coin;
#define MAX_COIN_COUNT 64
static Coin coins[MAX_COIN_COUNT];
static int coinIndex;

typedef struct {
  Vector pos;
  float width;
  float angle;
  bool isAlive;
} Line;
#define MAX_LINE_COUNT 32
static Line lines[MAX_LINE_COUNT];
static int lineIndex;
static float nextLineDist;

typedef struct {
  Vector pos;
  Vector vel;
  float radius;
  bool isOut;
} Ball;
static Ball ball;

static Vector scr;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
    ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, f0);
    vectorSet(&f0->pos, 50, 50);
    vectorSet(&f0->size, 80, 150);
    f0->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MAX_FLOOR_COUNT);
    lastFloorX = 50;
    lastFloorWidth = 80;
    nextFloorDist = 0;
    INIT_UNALIVED_ARRAY(fans);
    fanIndex = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    vectorSet(&ball.pos, 50, 90);
    vectorSet(&ball.vel, 0, 0);
    ball.radius = BALL_RADIUS;
    ball.isOut = false;
    INIT_UNALIVED_ARRAY(lines);
    lineIndex = 0;
    nextLineDist = 0;
    vectorSet(&scr, 0, 0);
    multiplier = 1;
  }
  vectorSet(&scr, 0, 0);
  if (ball.pos.x < 45) {
    scr.x += (45 - ball.pos.x) * 0.1;
  } else if (ball.pos.x > 55) {
    scr.x += (55 - ball.pos.x) * 0.1;
  }
  if (ball.pos.y < 80) {
    scr.y += (80 - ball.pos.y) * 0.1;
  }
  nextLineDist -= scr.y * 0.2;
  if (nextLineDist < 0) {
    ASSIGN_ARRAY_ITEM(lines, lineIndex, Line, nl);
    vectorSet(&nl->pos, rnd(0, 99), -20);
    nl->width = rnd(5, 20);
    if (rnd(0, 1) < 0.1) {
      nl->angle = rnd(0, M_PI * 2);
    } else {
      nl->angle = rndi(0, 2) * M_PI / 2;
    }
    nl->isAlive = true;
    lineIndex = wrap(lineIndex + 1, 0, MAX_LINE_COUNT);
    nextLineDist += rnd(5, 10);
  }
  color = LIGHT_YELLOW;
  FOR_EACH(lines, i) {
    ASSIGN_ARRAY_ITEM(lines, i, Line, l);
    SKIP_IS_NOT_ALIVE(l);
    l->pos.x += scr.x * 0.2;
    l->pos.y += scr.y * 0.2;
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(l->pos.x, l->pos.y, l->width, l->angle);
    if (l->pos.y > 110) {
      l->isAlive = false;
      continue;
    }
  }
  nextFloorDist -= scr.y;
  if (nextFloorDist < 0) {
    float lfX = lastFloorX;
    float lfW = lastFloorWidth;
    float x = 0;
    float w = 0;
    TIMES(9, i) {
      x = rnd(0, 99);
      w = rnd(30, 80);
      if (fabs(x - lfX) < (w + lfW) / 2) {
        break;
      } else {
        x = lfX;
        w = lfW;
      }
    }
    float h = rnd(30, 99);
    ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, nf);
    vectorSet(&nf->pos, x, -h / 2 - 20);
    vectorSet(&nf->size, w, h);
    nf->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MAX_FLOOR_COUNT);
    lastFloorX = x;
    lastFloorWidth = w;
    int fanCount = rndi(0, (int)ceil(h / 30));
    TIMES(fanCount, k) {
      float wy = rndi(0, 2) * 2 - 1;
      ASSIGN_ARRAY_ITEM(fans, fanIndex, Fan, nfan);
      vectorSet(&nfan->pos, x + rnd(w * 0.6, w * 0.7) * wy,
                -h / 2 - 20 + rnd(0, h * 0.4) * RNDPM());
      nfan->speed = rnd(1, 2);
      float baseAngle;
      if (wy == -1) {
        baseAngle = 0;
      } else {
        baseAngle = M_PI;
      }
      nfan->angle = baseAngle + rnd(0, M_PI / 8) * RNDPM();
      nfan->ticks = 0;
      nfan->isAlive = true;
      fanIndex = wrap(fanIndex + 1, 0, MAX_FAN_COUNT);
    }
    int cc = rndi(0, (int)ceil(h / 10));
    float cy = rnd(0, h * 0.5) * RNDPM() - (cc * 10) / 2.0;
    float cx = x + rnd(0, w * 0.2) * RNDPM();
    TIMES(cc, k) {
      if (cy > -h * 0.3 && cy < h * 0.3) {
        ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, nc);
        vectorSet(&nc->pos, cx, cy - h / 2 - 20);
        nc->isAlive = true;
        coinIndex = wrap(coinIndex + 1, 0, MAX_COIN_COUNT);
      }
      cy += 10;
    }
    nextFloorDist = rnd(h * 0.6, h * 0.8);
  }
  color = LIGHT_BLUE;
  FOR_EACH(floors, i) {
    ASSIGN_ARRAY_ITEM(floors, i, Floor, f);
    SKIP_IS_NOT_ALIVE(f);
    vectorAdd(&f->pos, scr.x, scr.y);
    box(f->pos.x, f->pos.y, f->size.x, f->size.y);
    if (f->pos.y - f->size.y / 2 > 99) {
      f->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(fans, i) {
    ASSIGN_ARRAY_ITEM(fans, i, Fan, f);
    SKIP_IS_NOT_ALIVE(f);
    f->ticks += f->speed * f->speed;
    vectorAdd(&f->pos, scr.x, scr.y);
    float oa = fabs(wrap(angleTo(&f->pos, ball.pos.x, ball.pos.y) - f->angle, -M_PI, M_PI));
    float d = distanceTo(&f->pos, ball.pos.x, ball.pos.y) + 1;
    float fr = (f->speed * clamp(M_PI / 4 - oa, 0, M_PI / 4) / d) * 3;
    addWithAngle(&ball.vel, f->angle, fr);
    Vector fanBoxPos;
    vectorSet(&fanBoxPos, f->pos.x, f->pos.y);
    addWithAngle(&fanBoxPos, f->angle + M_PI, 5);
    box(fanBoxPos.x, fanBoxPos.y, 3, 3);
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision barColl = bar(f->pos.x, f->pos.y, cos(f->ticks * 0.1) * 9, f->angle + M_PI / 2);
    if (f->pos.y < -9 &&
        (barColl.isColliding.rect[LIGHT_BLUE] || barColl.isColliding.rect[BLACK])) {
      f->isAlive = false;
      continue;
    }
    particle(f->pos.x, f->pos.y, f->speed * 0.2, f->speed * f->speed, f->angle, M_PI / 4);
    if (f->pos.y > 105) {
      f->isAlive = false;
      continue;
    }
  }
  float o = input.pos.x - ball.pos.x;
  float oSign;
  if (o < 0) {
    oSign = -1;
  } else {
    oSign = 1;
  }
  ball.vel.x += o * o * 0.0001 * oSign;
  ball.vel.y -= 0.1;
  vectorMul(&ball.vel, 0.95);
  ball.vel.y *= 1 - clamp(0.03 * sqrt(fabs(o)), 0, 0.5);
  ball.pos.x += ball.vel.x * sqrt(difficulty);
  ball.pos.y += ball.vel.y * sqrt(difficulty);
  vectorAdd(&ball.pos, scr.x, scr.y);
  ball.pos.y -= difficulty * 0.1;
  color = RED;
  thickness = 3;
  Collision arcColl = arc(ball.pos.x, ball.pos.y, ball.radius, 0, M_PI * 2);
  if (arcColl.isColliding.rect[LIGHT_BLUE]) {
    ball.isOut = false;
    ball.radius += (BALL_RADIUS - ball.radius) * 0.1;
  } else {
    play(HIT);
    if (!ball.isOut) {
      ball.vel.x /= 2;
      ball.isOut = true;
    }
    ball.radius *= 0.925;
    if (ball.radius < 0.1) {
      play(EXPLOSION);
      gameOver();
    }
  }
  color = BLUE;
  thickness = 3;
  arc(ball.pos.x, ball.pos.y, 0.1, 0, M_PI * 2);
  color = YELLOW;
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, Coin, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, scr.x, scr.y);
    Collision cc = box(c->pos.x, c->pos.y, 8, 8);
    if (cc.isColliding.rect[RED]) {
      play(COIN);
      addScore(multiplier, c->pos.x, c->pos.y);
      particle(c->pos.x, c->pos.y, 16, 1, 0, M_PI * 2);
      multiplier++;
      c->isAlive = false;
      continue;
    }
    if (c->pos.y > 105) {
      if (multiplier > 1) {
        multiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameSkyFloor() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
