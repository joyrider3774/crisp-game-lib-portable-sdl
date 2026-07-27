#include "cglp.h"

static char *title = "LINE B";
static char *description = "[Tap] Grow";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 30, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
} LinebPoint;
#define LINEB_POINT_COUNT 2
static LinebPoint points[LINEB_POINT_COUNT];

typedef struct {
  Vector p1;
  Vector p2;
} LinebHistory;
#define LINEB_MAX_HISTORY_COUNT 48
static LinebHistory history[LINEB_MAX_HISTORY_COUNT];
static int historyCount;

static bool goldActive;
static Vector goldPos;
static float goldRadius;

static void update() {
  if (!ticks) {
    TIMES(LINEB_POINT_COUNT, i) {
      vectorSet(&points[i].pos, 50, 50);
      vectorSet(&points[i].vel, 1, 0);
      rotate(&points[i].vel, rnd(0, M_PI / 3) * RNDPM() + i * M_PI);
    }
    historyCount = 0;
    goldActive = false;
  }
  if (!goldActive) {
    color = WHITE;
    thickness = 20;
    line(VEC_XY(points[0].pos), VEC_XY(points[1].pos));
    Vector pos;
    color = TRANSPARENT;
    TIMES(99, i) {
      vectorSet(&pos, rnd(20, 80), rnd(20, 80));
      Collision bc = box(pos.x, pos.y, 20, 20);
      if (!bc.isColliding.rect[WHITE]) {
        break;
      }
    }
    goldPos = pos;
    goldRadius = 1;
    goldActive = true;
  }
  color = LIGHT_BLACK;
  TIMES(historyCount, i) {
    if (i % 9 == 8) {
      thickness = clamp(4 - ceil(i / 9), 1, 3);
      line(VEC_XY(history[i].p1), VEC_XY(history[i].p2));
    }
  }
  color = LIGHT_PURPLE;
  rect(0, 0, 100, 3);
  rect(0, 97, 100, 3);
  rect(0, 0, 3, 100);
  rect(97, 0, 3, 100);
  color = YELLOW;
  bool isHittingWall = false;
  thickness = 3;
  Collision arcc = arc(goldPos.x, goldPos.y, goldRadius, 0, M_PI * 2);
  if (arcc.isColliding.rect[LIGHT_PURPLE]) {
    isHittingWall = true;
  }
  goldRadius += 0.05 * difficulty;
  if (input.isJustPressed) {
    play(LASER);
    Vector cp;
    vectorSet(&cp, points[0].pos.x, points[0].pos.y);
    vectorAdd(&cp, points[1].pos.x, points[1].pos.y);
    vectorMul(&cp, 0.5);
    TIMES(LINEB_POINT_COUNT, i) {
      float ang = angleTo(&points[i].pos, VEC_XY(cp));
      addWithAngle(&points[i].vel, ang, 9);
    }
  }
  TIMES(LINEB_POINT_COUNT, i) {
    LinebPoint *p = &points[i];
    p->pos.x += p->vel.x * sqrt(difficulty);
    p->pos.y += p->vel.y * sqrt(difficulty);
    if ((p->pos.x < 4 && p->vel.x < 0) || (p->pos.x > 96 && p->vel.x > 0)) {
      play(HIT);
      p->vel.x *= -1;
      p->vel.y += (rnd(0.1, 0.2) * RNDPM()) / (fabs(p->vel.y) + 1);
    }
    if ((p->pos.y < 5 && p->vel.y < 0) || (p->pos.y > 96 && p->vel.y > 0)) {
      play(HIT);
      p->vel.y *= -1;
      p->vel.x += (rnd(0.1, 0.2) * RNDPM()) / (fabs(p->vel.x) + 1);
    }
    if (vectorLength(&p->vel) > 1) {
      vectorMul(&p->vel, 0.9);
    }
    vectorMul(&p->vel, 0.999);
  }
  int shiftCount = historyCount;
  if (shiftCount > LINEB_MAX_HISTORY_COUNT - 1) {
    shiftCount = LINEB_MAX_HISTORY_COUNT - 1;
  }
  for (int k = shiftCount - 1; k >= 0; k--) {
    history[k + 1] = history[k];
  }
  history[0].p1 = points[0].pos;
  history[0].p2 = points[1].pos;
  if (historyCount < LINEB_MAX_HISTORY_COUNT) {
    historyCount++;
  }
  color = PURPLE;
  thickness = 3;
  Collision lc = line(VEC_XY(points[0].pos), VEC_XY(points[1].pos));
  if (lc.isColliding.rect[YELLOW]) {
    play(POWER_UP);
    addScore(ceil(goldRadius * sqrt(goldRadius)), VEC_XY(goldPos));
    goldActive = false;
  } else if (isHittingWall) {
    play(EXPLOSION);
    color = RED;
    TIMES(4, i) {
      Vector p;
      vectorSet(&p, goldPos.x, goldPos.y);
      addWithAngle(&p, (i * M_PI) / 2, goldRadius);
      if (p.x < 5 || p.x > 95 || p.y < 5 || p.y > 95) {
        text("X", VEC_XY(p));
      }
    }
    gameOver();
  }
}

void addGameLineB() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
