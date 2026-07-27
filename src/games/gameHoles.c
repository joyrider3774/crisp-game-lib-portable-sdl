#include "cglp.h"

static char *title = "HOLES";
static char *description = "[Tap]\n Change holes";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define HOLES_BALL_RADIUS 1
#define HOLES_WALL_ANGLE 0.2
#define HOLES_HOLE_LENGTH 12

typedef struct {
  Vector pos;
  Vector vel;
  float angle;
  bool isAlive;
} Ball;
#define HOLES_MAX_BALL_COUNT 16
static Ball balls[HOLES_MAX_BALL_COUNT];
static int ballIndex;

// `side` is JS's `angle` field on wall objects: despite the name, it only
// ever holds 0 or 1 there - a left/right side flag, not a radian angle.
typedef struct {
  Vector pos;
  float length;
  int side;
  int index;
  bool isAlive;
} Wall;
#define HOLES_MAX_WALL_COUNT 512
static Wall walls[HOLES_MAX_WALL_COUNT];
static int wallIndex;

static Vector coinPos;
static int coinWallIndex;

static int holeIndex;
static int currentIndex;
static float scr;
static float scrBaseY;

static void reflect(Ball *b, float a, int c, bool hasColor) {
  float oa = wrap(vectorAngle(&b->vel) - a - M_PI, -M_PI, M_PI);
  if (fabs(oa) < M_PI / 2) {
    addWithAngle(&b->vel, a, vectorLength(&b->vel) * cos(oa) * 1.7);
  }
  if (hasColor) {
    color = TRANSPARENT;
    TIMES(9, i) {
      addWithAngle(&b->pos, a, 1);
      Collision rc = arc(VEC_XY(b->pos), HOLES_BALL_RADIUS, 0, M_PI * 2);
      if (!rc.isColliding.rect[c]) {
        break;
      }
    }
  }
}

static void setCoin(float y, int a) {
  float cx;
  if (a == 0) {
    cx = 10;
  } else {
    cx = 89;
  }
  vectorSet(&coinPos, cx, y);
  float wx;
  if (a == 0) {
    wx = 7;
  } else {
    wx = 93;
  }
  ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w);
  vectorSet(&w->pos, wx, y + 9);
  w->length = 9;
  w->side = a;
  w->index = -1;
  w->isAlive = true;
  coinWallIndex = wallIndex;
  wallIndex = wrap(wallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
}

static void addWalls(int wallIdx) {
  Wall *w = &walls[wallIdx];
  COUNT_IS_ALIVE(balls, aliveBallCount);
  if (aliveBallCount < 9) {
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, nb);
    vectorSet(&nb->pos, rnd(30, 70), 0);
    vectorSet(&nb->vel, 0, sqrt(difficulty));
    nb->angle = rnd(0, M_PI * 2);
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, HOLES_MAX_BALL_COUNT);
  }
  float y = w->pos.y;
  int a = w->side;
  float holeXs[5];
  int holeXsCount = 0;
  float xl = 80;
  float lp = HOLES_HOLE_LENGTH + 9;
  TIMES(5, i) {
    float hx = rnd(15, xl - lp / 2 - 5);
    bool isValid = true;
    TIMES(holeXsCount, k) {
      if (fabs(hx - holeXs[k]) < HOLES_HOLE_LENGTH + 9) {
        isValid = false;
      }
    }
    if (isValid) {
      holeXs[holeXsCount] = hx;
      holeXsCount++;
    }
  }
  TIMES(holeXsCount, i) {
    for (int j = i + 1; j < holeXsCount; j++) {
      if (holeXs[j] < holeXs[i]) {
        float t = holeXs[i];
        holeXs[i] = holeXs[j];
        holeXs[j] = t;
      }
    }
  }
  Vector p;
  if (a == 0) {
    vectorSet(&p, 7, y);
  } else {
    vectorSet(&p, 93, y);
  }
  float wa;
  if (a == 0) {
    wa = HOLES_WALL_ANGLE;
  } else {
    wa = M_PI - HOLES_WALL_ANGLE;
  }
  bool hasPhx = false;
  float phx = 0;
  TIMES(holeXsCount, k) {
    float hx = holeXs[k];
    float l;
    if (!hasPhx) {
      l = hx - HOLES_HOLE_LENGTH / 2.0;
    } else {
      l = hx - HOLES_HOLE_LENGTH / 2.0 -
          (phx + HOLES_HOLE_LENGTH / 2.0) * (1 / cos(HOLES_WALL_ANGLE));
    }
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, seg1);
    seg1->pos = p;
    seg1->length = l - 5;
    seg1->side = a;
    seg1->index = -1;
    seg1->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
    addWithAngle(&p, wa, l);
    float hl = HOLES_HOLE_LENGTH * (1 / cos(HOLES_WALL_ANGLE));
    ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, seg2);
    seg2->pos = p;
    seg2->length = hl - 5;
    seg2->side = a;
    seg2->index = holeIndex;
    seg2->isAlive = true;
    wallIndex = wrap(wallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
    if (rnd(0, 1) < 0.7) {
      if (holeIndex == 0) {
        holeIndex = 1;
      } else {
        holeIndex = 0;
      }
    }
    addWithAngle(&p, wa, hl);
    phx = hx;
    hasPhx = true;
  }
  float l2 = (xl - (phx + HOLES_HOLE_LENGTH / 2.0)) * (1 / cos(HOLES_WALL_ANGLE));
  ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, seg3);
  seg3->pos = p;
  seg3->length = l2 - 5;
  seg3->side = a;
  seg3->index = -1;
  seg3->isAlive = true;
  wallIndex = wrap(wallIndex + 1, 0, HOLES_MAX_WALL_COUNT);
  w->pos.y = -99;
  int nextSide;
  if (a == 0) {
    nextSide = 1;
  } else {
    nextSide = 0;
  }
  setCoin(y + 80 * sin(HOLES_WALL_ANGLE) + 9, nextSide);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, b0);
    vectorSet(&b0->pos, 10, 0);
    vectorSet(&b0->vel, 0, 0);
    b0->angle = 0;
    b0->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, HOLES_MAX_BALL_COUNT);
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    vectorSet(&coinPos, 0, 0);
    coinWallIndex = -1;
    setCoin(20, 0);
    holeIndex = 0;
    currentIndex = 0;
    scr = 0;
    scrBaseY = 60;
  }
  if (input.isJustPressed) {
    play(LASER);
    if (currentIndex == 0) {
      currentIndex = 1;
    } else {
      currentIndex = 0;
    }
    scrBaseY -= sqrt(difficulty) * 9;
  }
  color = LIGHT_YELLOW;
  rect(0, 0, 5, 99);
  rect(95, 0, 5, 99);
  color = YELLOW;
  coinPos.y -= scr;
  COUNT_IS_ALIVE(balls, aliveBallCountForScore);
  addScore(scr * aliveBallCountForScore, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  thickness = 7;
  line(coinPos.x, coinPos.y, coinPos.x + 1, coinPos.y);
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y -= scr;
    if (w->pos.y < 9) {
      w->pos.y--;
    }
    if (w->index < 0 || w->index == currentIndex) {
      bool isLight = w->pos.y < 9;
      if (w->side == 0) {
        if (isLight) {
          color = LIGHT_GREEN;
        } else {
          color = GREEN;
        }
      } else {
        if (isLight) {
          color = LIGHT_CYAN;
        } else {
          color = CYAN;
        }
      }
      float wallAngleVal;
      if (w->side == 0) {
        wallAngleVal = HOLES_WALL_ANGLE;
      } else {
        wallAngleVal = M_PI - HOLES_WALL_ANGLE;
      }
      thickness = 4;
      barCenterPosRatio = 0;
      bar(VEC_XY(w->pos), w->length, wallAngleVal);
    }
    if (w->pos.y < -w->length * sin(HOLES_WALL_ANGLE)) {
      w->isAlive = false;
      continue;
    }
  }
  float maxY = 0;
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vel.y += 0.2 * sqrt(difficulty);
    vectorMul(&b->vel, 1 - 0.02 * sqrt(difficulty));
    vectorAdd(&b->pos, b->vel.x * sqrt(difficulty) * 0.5, b->vel.y * sqrt(difficulty) * 0.5);
    b->pos.y -= scr;
    if (b->pos.y < scrBaseY + 20 && b->pos.y > maxY) {
      maxY = b->pos.y;
    }
    b->angle += b->vel.x * 0.03 + b->vel.y * 0.02;
    color = RED;
    Collision bc = arc(VEC_XY(b->pos), HOLES_BALL_RADIUS, b->angle, b->angle + M_PI * 2);
    if (bc.isColliding.rect[YELLOW]) {
      play(COIN);
      addScore(aliveBallCountForScore * 10, VEC_XY(b->pos));
      addWalls(coinWallIndex);
    }
    if (bc.isColliding.rect[LIGHT_YELLOW]) {
      float a;
      if (b->pos.x < 50) {
        a = 0;
      } else {
        a = M_PI;
      }
      reflect(b, a, LIGHT_YELLOW, true);
    }
    if (bc.isColliding.rect[GREEN]) {
      reflect(b, HOLES_WALL_ANGLE - M_PI_2, GREEN, true);
    }
    if (bc.isColliding.rect[CYAN]) {
      reflect(b, M_PI - HOLES_WALL_ANGLE + M_PI_2, CYAN, true);
    }
    if (b->pos.y > 99 + HOLES_BALL_RADIUS) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
    if (b->pos.y < -HOLES_BALL_RADIUS) {
      b->isAlive = false;
      continue;
    }
  }
  if (maxY > scrBaseY) {
    scr = (maxY - scrBaseY) * 0.1;
  } else {
    scr = 0;
  }
  scrBaseY += (60 - scrBaseY) * 0.01;
  COUNT_IS_ALIVE(balls, aliveBallCountAfter);
  if (aliveBallCountAfter == 0) {
    play(EXPLOSION);
    gameOver();
  }
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    FOR_EACH(balls, j) {
      ASSIGN_ARRAY_ITEM(balls, j, Ball, ab);
      SKIP_IS_NOT_ALIVE(ab);
      if (i == j || distanceTo(&ab->pos, VEC_XY(b->pos)) > HOLES_BALL_RADIUS * 2) {
        continue;
      }
      reflect(b, angleTo(&ab->pos, VEC_XY(b->pos)), 0, false);
    }
  }
}

void addGameHoles() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
