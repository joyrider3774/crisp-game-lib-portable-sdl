#include "cglp.h"
#include "string.h"

static char *title = "INF RANGE";
static char *description = "[Tap]  Turn 90\n[Hold] Turn slow";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = true};

typedef struct {
  Vector pos;
  float angle;
  float angleVel;
  float speed;
  bool isAlive;
} Arrow;
// Arrows only die on a mismatched-angle hit against the ship; a clean/avoided
// pass leaves them bouncing forever while spawn interval keeps shrinking with
// difficulty, so count grows roughly unbounded (~16 reached within a minute
// of ordinary play) - raise the cap well past that for long sessions.
#define INFRANGE_MAX_ARROW_COUNT 1024
static Arrow arrows[INFRANGE_MAX_ARROW_COUNT];
static int arrowIndex;
static float nextArrowTicks;

static Vector pos;
static float angle;
static float speed;
static float tapAngle;
static float tapTicks;
static Vector scr;
static float multiplier;

typedef struct {
  Vector pos;
  Vector size;
} Line;
#define INFRANGE_LINE_COUNT 10
static Line lines[INFRANGE_LINE_COUNT];

static bool checkCollision(Arrow *a) {
  if (fabs(wrap(angle + M_PI - a->angle, -M_PI, M_PI)) < 0.7) {
    a->angle += M_PI;
    addWithAngle(&a->pos, a->angle, a->speed * 2);
    angle += M_PI;
    speed = 1;
    return true;
  } else {
    return false;
  }
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(arrows);
    arrowIndex = 0;
    nextArrowTicks = 200;
    vectorSet(&pos, 50, 80);
    angle = -M_PI_2;
    tapAngle = -M_PI_2;
    speed = 1;
    tapTicks = -99;
    vectorSet(&scr, 0, 0);
    multiplier = 10.9;
    TIMES(5, i) {
      vectorSet(&lines[i].pos, 50, i * 20);
      vectorSet(&lines[i].size, 100, 1);
    }
    TIMES(5, i) {
      vectorSet(&lines[5 + i].pos, i * 20, 50);
      vectorSet(&lines[5 + i].size, 1, 100);
    }
  }
  addWithAngle(&pos, angle, speed);
  if (pos.x < 40) {
    scr.x = (40 - pos.x) * 0.2;
  } else if (pos.x > 60) {
    scr.x = (60 - pos.x) * 0.2;
  }
  if (pos.y < 40) {
    scr.y = (40 - pos.y) * 0.2;
  } else if (pos.y > 60) {
    scr.y = (60 - pos.y) * 0.2;
  }
  color = LIGHT_GREEN;
  TIMES(5, i) {
    Line *l = &lines[i];
    l->pos.y = wrap(l->pos.y + scr.y, 0, 100);
    box(VEC_XY(l->pos), l->size.x, l->size.y);
  }
  for (int i = 5; i < 10; i++) {
    Line *l = &lines[i];
    l->pos.x = wrap(l->pos.x + scr.x, 0, 100);
    box(VEC_XY(l->pos), l->size.x, l->size.y);
  }
  vectorAdd(&pos, scr.x, scr.y);
  color = BLUE;
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(pos), 5, angle);
  particle(pos.x, pos.y, 1, speed, angle + M_PI, 0.2);
  color = GREEN;
  Vector bp;
  vectorSet(&bp, pos.x, pos.y);
  addWithAngle(&bp, angle, 4);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(bp), 3, angle + M_PI_2);
  if (input.isJustPressed) {
    play(LASER);
    tapAngle = angle;
    tapTicks = ticks;
  }
  if (input.isPressed) {
    angle += 0.02 * difficulty;
    speed += (1 - speed) * 0.1;
  } else {
    speed += (difficulty - speed) * 0.05;
  }
  if (input.isJustReleased) {
    if (ticks - tapTicks < 9) {
      play(SELECT);
      angle = tapAngle + M_PI_2;
    }
  }
  FOR_EACH(arrows, i) {
    ASSIGN_ARRAY_ITEM(arrows, i, Arrow, a);
    SKIP_IS_NOT_ALIVE(a);
    a->angle += a->angleVel;
    addWithAngle(&a->pos, a->angle, a->speed);
    vectorAdd(&a->pos, scr.x, scr.y);
    a->pos.x = wrap(a->pos.x, -3, 103);
    a->pos.y = wrap(a->pos.y, -3, 103);
    color = RED;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision ac = bar(VEC_XY(a->pos), 5, a->angle);
    if (ac.isColliding.rect[BLUE] || ac.isColliding.rect[GREEN]) {
      if (!checkCollision(a)) {
        play(POWER_UP);
        particle(a->pos.x, a->pos.y, 9, (speed + a->speed) * 2, 0, M_PI * 2);
        addScore(floor(multiplier), VEC_XY(a->pos));
        multiplier += 10;
        a->isAlive = false;
        continue;
      }
    }
    if (rnd(0, 1) < 0.2) {
      particle(a->pos.x, a->pos.y, 1, a->speed, a->angle + M_PI, 0.1);
    }
    color = YELLOW;
    Vector abp;
    vectorSet(&abp, a->pos.x, a->pos.y);
    addWithAngle(&abp, a->angle, 4);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision abc = bar(VEC_XY(abp), 2, a->angle + M_PI_2);
    if (abc.isColliding.rect[BLUE]) {
      if (!checkCollision(a)) {
        play(RANDOM);
        gameOver();
      }
    }
  }
  COUNT_IS_ALIVE(arrows, aliveArrowCount);
  if (aliveArrowCount == 0) {
    nextArrowTicks = 0;
  }
  nextArrowTicks -= sqrt(difficulty);
  if (nextArrowTicks < 0) {
    Vector p;
    bool isApp = false;
    TIMES(9, k) {
      vectorSet(&p, rnd(10, 90), rnd(10, 90));
      if (fabs(wrap(pos.x - p.x, -50, 50)) + fabs(wrap(pos.y - p.y, -50, 50)) > 36) {
        isApp = true;
        break;
      }
    }
    if (isApp) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(arrows, arrowIndex, Arrow, na);
      na->pos = p;
      na->angle = rnd(0, M_PI * 2);
      na->angleVel = rnd(0, sqrt(difficulty - 1) * 0.05) * RNDPM();
      na->speed = rnd(1, difficulty) / 2;
      na->isAlive = true;
      arrowIndex = wrap(arrowIndex + 1, 0, INFRANGE_MAX_ARROW_COUNT);
      nextArrowTicks = rnd(150, 250);
    }
  }
  multiplier -= 0.02;
  if (multiplier < 10.9) {
    multiplier = 10.9;
  }
  color = BLACK;
  char scoreText[16];
  strcpy(scoreText, "+");
  strcat(scoreText, intToChar((int)floor(multiplier)));
  text(scoreText, 3, 97);
}

void addGameInfRange() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
