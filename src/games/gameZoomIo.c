#include "cglp.h"
#include "string.h"

static char *title = "ZOOM IO";
static char *description = "[Hold]\n Zoom &\n Go forward";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

#define ZOOMIO_TYPE_PLAYER 0
#define ZOOMIO_TYPE_ENEMY 1
#define ZOOMIO_TYPE_BONUS 2

typedef struct {
  Vector pos;
  float angle;
  float speed;
  int type;
  bool isAlive;
} Arrow;
// index 0 is always reserved for the player arrow and never overwritten;
// enemy/bonus arrows cycle through indices 1..MAX-1.
// Spawn interval shrinks as difficulty^-0.5 but speed grows as difficulty^1,
// so concurrent count ~sqrt(d)/(d+1.1) peaks right at game start (d~1) at
// ~60+ arrows crossing the 70-radius disk - 48 overflowed almost immediately.
#define ZOOMIO_MAX_ARROW_COUNT 128
static Arrow arrows[ZOOMIO_MAX_ARROW_COUNT];
static int nextSpawnIndex;

static float nextArrowTicks;
static int nextArrowCount;
static float nextAngles[2];
static float zoom;
static float lines[20];
static Vector scr;
static float multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(arrows);
    vectorSet(&arrows[0].pos, 50, 50);
    arrows[0].angle = -M_PI_2;
    arrows[0].speed = 0;
    arrows[0].type = ZOOMIO_TYPE_PLAYER;
    arrows[0].isAlive = true;
    nextSpawnIndex = 1;
    nextArrowTicks = 0;
    nextArrowCount = 0;
    nextAngles[0] = rnd(0, M_PI * 2);
    nextAngles[1] = rnd(0, M_PI * 2);
    zoom = 1;
    TIMES(20, i) { lines[i] = (i % 10) * 10; }
    vectorSet(&scr, 0, 0);
    multiplier = 1;
  }
  color = LIGHT_CYAN;
  for (int i = 0; i < 20; i++) {
    if (i < 10) {
      lines[i] = wrap(lines[i] + scr.x, 0, 100);
      rect((lines[i] - 50) * zoom + 50, 0, 1, 100);
    } else {
      lines[i] = wrap(lines[i] + scr.y, 0, 100);
      rect(0, (lines[i] - 50) * zoom + 50, 100, 1);
    }
  }
  nextArrowTicks--;
  if (nextArrowTicks < 0) {
    if (rnd(0, 1) < 0.1) {
      float na = rnd(0, M_PI * 2);
      if (nextArrowCount % 2 == 1 && zoom > 5) {
        na = arrows[0].angle;
      }
      nextAngles[nextArrowCount % 2] = na;
    }
    Vector pos;
    vectorSet(&pos, 50, 50);
    addWithAngle(&pos, nextAngles[nextArrowCount % 2], 70);
    Vector tp;
    vectorSet(&tp, 50, 50);
    addWithAngle(&tp, rnd(0, M_PI * 2), rnd(20, 40));
    Arrow *na2 = &arrows[nextSpawnIndex];
    na2->pos = pos;
    na2->angle = angleTo(&pos, VEC_XY(tp));
    na2->speed = rnd(1, difficulty + 0.1) * 0.1;
    if (nextArrowCount % 2 == 0) {
      na2->type = ZOOMIO_TYPE_BONUS;
    } else {
      na2->type = ZOOMIO_TYPE_ENEMY;
    }
    na2->isAlive = true;
    nextSpawnIndex = wrap(nextSpawnIndex + 1, 1, ZOOMIO_MAX_ARROW_COUNT);
    nextArrowTicks = 20 / sqrt(difficulty);
    nextArrowCount++;
  }
  if (input.isJustPressed || input.isJustReleased) {
    play(LASER);
  }
  if (input.isPressed) {
    zoom = clamp(zoom + 0.05 * sqrt(difficulty), 1, 9);
    multiplier += zoom * 0.1 * sqrt(difficulty);
  } else {
    zoom += (1 - zoom) * (0.03 * sqrt(difficulty));
  }
  if (zoom < 2) {
    multiplier += (0.5 - multiplier) * 0.02;
  }
  FOR_EACH(arrows, i) {
    ASSIGN_ARRAY_ITEM(arrows, i, Arrow, a);
    SKIP_IS_NOT_ALIVE(a);
    if (a->type == ZOOMIO_TYPE_PLAYER) {
      if (!input.isPressed) {
        a->angle += (sqrt(difficulty) * 0.1) / zoom;
      }
      a->speed += (sqrt(difficulty) * (zoom - 1) * 0.1 - a->speed) * 0.05;
      vectorSet(&scr, 0, 0);
      addWithAngle(&scr, a->angle, -a->speed);
      color = CYAN;
    } else {
      addWithAngle(&a->pos, a->angle, a->speed);
      vectorAdd(&a->pos, scr.x, scr.y);
      if (a->type == ZOOMIO_TYPE_ENEMY) {
        color = RED;
      } else {
        color = YELLOW;
      }
    }
    Vector p;
    vectorSet(&p, a->pos.x, a->pos.y);
    vectorAdd(&p, -50, -50);
    vectorMul(&p, zoom);
    vectorAdd(&p, 50, 50);
    float d = distanceTo(&a->pos, 50, 50);
    bool pInRect = p.x >= 0 && p.x < 99 && p.y >= 0 && p.y < 99;
    if (a->type == ZOOMIO_TYPE_PLAYER ||
        (sqrt(zoom) > 0.5 + d * 0.03 && pInRect)) {
      Vector bp;
      vectorSet(&bp, p.x, p.y);
      addWithAngle(&bp, a->angle, -1 * zoom);
      thickness = 3 * zoom;
      barCenterPosRatio = 0.5;
      Collision c1 = bar(p.x, p.y, 5 * zoom, a->angle);
      Vector bp1;
      vectorSet(&bp1, bp.x, bp.y);
      addWithAngle(&bp1, a->angle + M_PI_2, 2 * zoom);
      Collision c2 = box(VEC_XY(bp1), 2 * zoom, 2 * zoom);
      Vector bp2;
      vectorSet(&bp2, bp.x, bp.y);
      addWithAngle(&bp2, a->angle - M_PI_2, 2 * zoom);
      Collision c3 = box(VEC_XY(bp2), 2 * zoom, 2 * zoom);
      if (a->type != ZOOMIO_TYPE_PLAYER &&
          (c1.isColliding.rect[CYAN] || c2.isColliding.rect[CYAN] ||
           c3.isColliding.rect[CYAN])) {
        if (a->type == ZOOMIO_TYPE_BONUS) {
          play(POWER_UP);
          particle(p.x, p.y, 5 * zoom, sqrt(zoom), 0, M_PI * 2);
          addScore(ceil(multiplier), 50, 50);
          a->isAlive = false;
          continue;
        } else {
          play(EXPLOSION);
          gameOver();
        }
      }
    } else {
      thickness = 1;
      barCenterPosRatio = 0;
      Vector cpVec;
      vectorSet(&cpVec, 50, 50);
      float ang = angleTo(&cpVec, VEC_XY(p));
      bar(50, 50, 30, ang);
    }
    a->isAlive = d <= 70;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "+");
  strcat(multText, intToChar((int)ceil(multiplier)));
  text(multText, 3, 9);
}

void addGameZoomIo() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
