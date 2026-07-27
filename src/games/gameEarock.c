#include "cglp.h"
#include "string.h"

static char *title = "EAROCK";
static char *description = "[Hold] Thrust\n[Tap]  Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "  ll  ",
    "  ll  ",
    "llllll",
    " llll ",
    "ll  ll",
    "l    l",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = true};

static Vector earockPos;
static Vector earockVel;
static float earockAngle;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} EarockStar;
#define EAROCK_MAX_STAR_COUNT 32
static EarockStar earockStars[EAROCK_MAX_STAR_COUNT];
static int earockStarIndex;
static float earockStarAppTicks;
static int earockAddingScore;

typedef struct {
  Vector pos;
} EarockBgStar;
#define EAROCK_BG_STAR_COUNT 36
static EarockBgStar earockBgStars[EAROCK_BG_STAR_COUNT];

static float earockTurnTo(float a, float px, float py, float v) {
  float at = angleTo(&earockPos, px, py);
  float o = wrap(a - at, -M_PI, M_PI);
  if (fabs(o) < v) {
    return at;
  } else if (o > 0) {
    return a - v;
  } else {
    return a + v;
  }
}

static void earockReflect(float n) {
  float r = wrap(earockAngle + M_PI - n, -M_PI, M_PI);
  if (fabs(r) < M_PI / 2) {
    earockAngle = earockAngle + M_PI - r * 2;
  }
  float s = vectorLength(&earockVel);
  vectorSet(&earockVel, 0, 0);
  addWithAngle(&earockVel, earockAngle, s / 2);
  play(SELECT);
  earockAddingScore = 1;
}

static void update() {
  if (!ticks) {
    vectorSet(&earockPos, 50, 20);
    vectorSet(&earockVel, 0.1, 0);
    earockAngle = 0;
    INIT_UNALIVED_ARRAY(earockStars);
    earockStarIndex = 0;
    earockStarAppTicks = 0;
    earockAddingScore = 1;
    TIMES(EAROCK_BG_STAR_COUNT, i) {
      vectorSet(&earockBgStars[i].pos, rnd(0, 99), rnd(0, 99));
    }
  }
  Vector centerVec;
  vectorSet(&centerVec, 50, 50);
  float cr = 15;
  float df = sqrt(difficulty);
  color = LIGHT_BLACK;
  TIMES(EAROCK_BG_STAR_COUNT, i) { box(VEC_XY(earockBgStars[i].pos), 1, 1); }
  color = BLUE;
  thickness = 10;
  arc(centerVec.x, centerVec.y, cr - 5, 0, M_PI * 2);
  color = GREEN;
  thickness = cr * 0.5;
  arc(centerVec.x + 5, centerVec.y - 3, cr * 0.2, 0, M_PI * 2);
  thickness = cr * 0.5;
  arc(centerVec.x - 7, centerVec.y + 4, cr * 0.3, 0, M_PI * 2);
  color = RED;
  if (input.isJustPressed) {
    play(HIT);
    vectorMul(&earockVel, 0.5);
    addWithAngle(&earockVel, earockAngle, df * 0.1);
    particle(earockPos.x, earockPos.y, 9, 2, earockAngle + M_PI, 0.2);
  }
  if (input.isPressed) {
    addWithAngle(&earockVel, earockAngle, df * 0.01);
    particle(earockPos.x, earockPos.y, 1, 1, earockAngle + M_PI, 0.2);
  } else {
    earockAngle = earockTurnTo(earockAngle, centerVec.x, centerVec.y, 0.01);
  }
  if (input.isJustReleased) {
    earockAngle = earockTurnTo(earockAngle, centerVec.x, centerVec.y, 0.2);
    float relAngle = angleTo(&centerVec, earockPos.x, earockPos.y);
    particle(earockPos.x, earockPos.y, 5, 1, relAngle, 0.2);
  }
  vectorMul(&earockVel, 0.98);
  vectorAdd(&earockPos, earockVel.x * df, earockVel.y * df);
  if (earockPos.x < 0) {
    earockReflect(0);
  }
  if (earockPos.x > 99) {
    earockReflect(M_PI);
  }
  if (earockPos.y < 0) {
    earockReflect(M_PI / 2);
  }
  if (earockPos.y > 99) {
    earockReflect(-M_PI / 2);
  }
  if (distanceTo(&earockPos, centerVec.x, centerVec.y) < cr * 1.1) {
    earockReflect(angleTo(&centerVec, earockPos.x, earockPos.y));
  }
  color = RED;
  thickness = 3;
  barCenterPosRatio = 1.4;
  bar(earockPos.x, earockPos.y, 3, earockAngle - 0.2);
  thickness = 3;
  barCenterPosRatio = 1.4;
  bar(earockPos.x, earockPos.y, 3, earockAngle + 0.2);
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(earockPos.x, earockPos.y, 5, earockAngle);
  earockStarAppTicks--;
  if (earockStarAppTicks < 0) {
    ASSIGN_ARRAY_ITEM(earockStars, earockStarIndex, EarockStar, ns);
    vectorSet(&ns->pos, centerVec.x, centerVec.y);
    addWithAngle(&ns->pos, rnd(0, M_PI * 2), 70);
    float toCenterAngle = angleTo(&ns->pos, centerVec.x, centerVec.y);
    vectorSet(&ns->vel, 0, 0);
    addWithAngle(&ns->vel, toCenterAngle + rnd(0, 1) * RNDPM(),
                 0.1 + rnd(0, difficulty - 1) * 0.1);
    ns->isAlive = true;
    earockStarIndex = wrap(earockStarIndex + 1, 0, EAROCK_MAX_STAR_COUNT);
    earockStarAppTicks += 300 / difficulty;
  }
  color = YELLOW;
  FOR_EACH(earockStars, i) {
    ASSIGN_ARRAY_ITEM(earockStars, i, EarockStar, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    vectorMul(&s->vel, 1 - 0.01 / df);
    float toCenterAngle2 = angleTo(&s->pos, centerVec.x, centerVec.y);
    addWithAngle(&s->vel, toCenterAngle2, 0.0002 * df);
    Collision sc = character("a", VEC_XY(s->pos));
    if (sc.isColliding.rect[BLACK]) {
      play(COIN);
      addScore(earockAddingScore, s->pos.x, s->pos.y);
      earockAddingScore++;
      s->isAlive = false;
      continue;
    }
    bool inRectSmall = s->pos.x >= -3 && s->pos.x < 100 && s->pos.y >= -3 && s->pos.y < 100;
    if (!inRectSmall) {
      vectorAdd(&s->pos, s->vel.x * 9, s->vel.y * 9);
    }
    bool inRectBig = s->pos.x >= -50 && s->pos.x < 100 && s->pos.y >= -50 && s->pos.y < 100;
    if (!inRectBig) {
      s->isAlive = false;
      continue;
    }
    if (distanceTo(&s->pos, centerVec.x, centerVec.y) < cr) {
      play(EXPLOSION);
      color = RED;
      text("X", VEC_XY(s->pos));
      color = YELLOW;
      gameOver();
    }
  }
  COUNT_IS_ALIVE(earockStars, aliveStarCount);
  if (aliveStarCount == 0) {
    earockStarAppTicks = 0;
  }
  color = BLACK;
  char scoreText[16];
  strcpy(scoreText, "+");
  strcat(scoreText, intToChar(earockAddingScore));
  text(scoreText, 3, 95);
}

void addGameEarock() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
