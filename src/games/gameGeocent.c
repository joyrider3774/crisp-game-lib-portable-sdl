#include "cglp.h"
#include "string.h"

static char *title = "GEOCENT";
static char *description = "[Hold]\n Speed up & Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

typedef struct {
  Vector cPos;
  float radius;
  float angle;
  float av;
  float size;
  int color;
  Vector pos;
} Star;
#define GEOCENT_STAR_COUNT 9
static Star stars[GEOCENT_STAR_COUNT];
static float starSizes[GEOCENT_STAR_COUNT] = {3, 1, 1, 2, 1, 1, 1, 1, 1};
static int starColors[GEOCENT_STAR_COUNT] = {YELLOW, RED, RED,  CYAN, RED,
                                              RED,    RED, RED, RED};

#define GEOCENT_BG_STAR_COUNT 20
static Vector bgStars[GEOCENT_BG_STAR_COUNT];

static bool rocketDistActive;
static float rocketAngle;
static float rocketVa;
static float rocketDist;
static float rocketSpeed;

// The original JS keeps posHistory unbounded for the entire game and never
// pops it - crates only ever reference an index up to crateCount*9, and
// crateCount only grows (by at most +1) each time a full cycle completes
// without fully clearing its own crates, so this cap gives enormous
// headroom for any realistic play session while keeping the array small.
#define GEOCENT_MAX_POS_HISTORY 300
static Vector posHistory[GEOCENT_MAX_POS_HISTORY];
static int posHistoryCount;

#define GEOCENT_MAX_CRATE_COUNT 32
static int crates[GEOCENT_MAX_CRATE_COUNT];
static int crateCount;

static void update() {
  if (!ticks) {
    float av = 0.03;
    TIMES(GEOCENT_STAR_COUNT, i) {
      av *= 0.7;
      vectorSet(&stars[i].cPos, 0, 0);
      stars[i].radius = i * 4;
      stars[i].angle = rnd(0, M_PI * 2);
      stars[i].av = av;
      stars[i].size = starSizes[i];
      stars[i].color = starColors[i];
      vectorSet(&stars[i].pos, 0, 0);
    }
    rocketAngle = 0;
    rocketVa = 0;
    rocketDistActive = false;
    rocketSpeed = 0;
    posHistoryCount = 0;
    crateCount = 0;
    TIMES(GEOCENT_BG_STAR_COUNT, i) {
      vectorSet(&bgStars[i], rnd(0, 99), rnd(0, 99));
    }
  }
  color = LIGHT_BLACK;
  TIMES(GEOCENT_BG_STAR_COUNT, i) {
    box(bgStars[i].x, bgStars[i].y, 1, 1);
  }
  TIMES(GEOCENT_STAR_COUNT, i) {
    stars[i].angle += stars[i].av * difficulty;
    vectorSet(&stars[i].cPos, 50, 50);
    addWithAngle(&stars[i].cPos, stars[i].angle, stars[i].radius);
  }
  Vector ep;
  vectorSet(&ep, stars[3].cPos.x, stars[3].cPos.y);
  TIMES(GEOCENT_STAR_COUNT, i) {
    vectorSet(&stars[i].pos, stars[i].cPos.x, stars[i].cPos.y);
    vectorAdd(&stars[i].pos, -ep.x, -ep.y);
    vectorAdd(&stars[i].pos, 50, 50);
    color = stars[i].color;
    arc(stars[i].pos.x, stars[i].pos.y, stars[i].size, 0, M_PI * 2);
  }
  if (!rocketDistActive) {
    rocketAngle = ((rndi(0, 4) + 0.3 + rnd(0, 0.4)) * M_PI) / 2;
    rocketDist = fabs(sin(rocketAngle * 2)) * 25 + 36;
    rocketSpeed = 0;
    posHistoryCount = 0;
    int cn = crateCount + 1;
    if (cn > GEOCENT_MAX_CRATE_COUNT) {
      cn = GEOCENT_MAX_CRATE_COUNT;
    }
    TIMES(cn, i) { crates[i] = (i + 1) * 9; }
    crateCount = cn;
    rocketDistActive = true;
  }
  if (input.isJustPressed) {
    play(SELECT);
    rocketSpeed += difficulty;
  }
  float targetSpeed;
  if (input.isPressed) {
    targetSpeed = 5;
  } else {
    targetSpeed = 1;
  }
  rocketSpeed += (targetSpeed * difficulty * 0.1 - rocketSpeed) * 0.5;
  rocketDist -= rocketSpeed;
  float targetVa;
  if (input.isPressed) {
    targetVa = 0.03;
  } else {
    targetVa = -0.003;
  }
  rocketVa += (targetVa * difficulty - rocketVa) * 0.05;
  rocketAngle += rocketVa;
  Vector p;
  vectorSet(&p, 50, 50);
  addWithAngle(&p, rocketAngle, rocketDist);
  int shiftCount = posHistoryCount;
  if (shiftCount > GEOCENT_MAX_POS_HISTORY - 1) {
    shiftCount = GEOCENT_MAX_POS_HISTORY - 1;
  }
  for (int k = shiftCount - 1; k >= 0; k--) {
    posHistory[k + 1] = posHistory[k];
  }
  posHistory[0] = p;
  if (posHistoryCount < GEOCENT_MAX_POS_HISTORY) {
    posHistoryCount++;
  }
  color = BLACK;
  int ci = 0;
  while (ci < crateCount) {
    float cpx, cpy;
    if (crates[ci] >= posHistoryCount) {
      cpx = p.x;
      cpy = p.y;
    } else {
      cpx = posHistory[crates[ci]].x;
      cpy = posHistory[crates[ci]].y;
    }
    Collision cc = box(cpx, cpy, 2, 2);
    if (cc.isColliding.rect[RED] || cc.isColliding.rect[YELLOW]) {
      play(HIT);
      particle(cpx, cpy, 16, 1, 0, M_PI * 2);
      memcpy(&crates[ci], &crates[ci + 1],
             (crateCount - 1 - ci) * sizeof(crates[0]));
      crateCount--;
    } else {
      ci++;
    }
  }
  color = RED;
  particle(p.x, p.y, 1, rocketSpeed * 3, rocketAngle, 0.5);
  color = BLUE;
  Vector bp;
  vectorSet(&bp, p.x, p.y);
  addWithAngle(&bp, rocketAngle, 2);
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(bp.x, bp.y, 2, rocketAngle + M_PI / 2);
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0.5;
  Collision finalC = bar(p.x, p.y, 1, rocketAngle);
  if (finalC.isColliding.rect[CYAN]) {
    play(COIN);
    addScore(crateCount, VEC_XY(p));
    rocketDistActive = false;
  } else if (finalC.isColliding.rect[RED] || finalC.isColliding.rect[YELLOW]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameGeocent() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
