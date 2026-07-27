#include "cglp.h"

static char *title = "VH WALLS";
static char *description = "[Tap] Place wall";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
    {
        "llllll",
        "llllll",
        "      ",
        "      ",
        "      ",
        "      ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  Vector pos;
  int angle;
  bool isFixed;
  bool isAlive;
} Wall;
#define VHWALLS_MAX_WALL_COUNT 64
static Wall walls[VHWALLS_MAX_WALL_COUNT];
static int wallIndex;
static int nextWallAngle;

typedef struct {
  Vector pos;
  Vector vel;
} Ball;
static Ball ball;

typedef struct {
  Vector pos;
  float size;
  int initTicks;
} Target;
static Target target;

static void setTarget() {
  target.size = 20;
  target.initTicks = 1;
  Vector p;
  vectorSet(&p, target.pos.x, target.pos.y);
  TIMES(9, i) {
    target.pos.x = rnd(20, 80);
    target.pos.y = rnd(20, 80);
    if (distanceTo(&target.pos, VEC_XY(p)) > 40) {
      break;
    }
  }
}

static void addWall(float x, float y, int angle, bool isFixed) {
  ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w);
  vectorSet(&w->pos, x, y);
  w->angle = angle;
  w->isFixed = isFixed;
  w->isAlive = true;
  wallIndex = wrap(wallIndex + 1, 0, VHWALLS_MAX_WALL_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    addWall(1, 50, 0, true);
    addWall(99, 50, 0, true);
    addWall(50, 1, 1, true);
    addWall(50, 99, 1, true);
    nextWallAngle = 0;
    vectorSet(&ball.pos, 50, 50);
    vectorSet(&ball.vel, 1, 1);
    vectorSet(&target.pos, 50, 50);
    target.size = 0;
    target.initTicks = 0;
    setTarget();
  }
  ball.pos.x += ball.vel.x * sqrt(difficulty);
  ball.pos.y += ball.vel.y * sqrt(difficulty);
  ball.pos.x = clamp(ball.pos.x, 1, 99);
  ball.pos.y = clamp(ball.pos.y, 1, 99);
  color = GREEN;
  box(VEC_XY(ball.pos), 5, 5);
  if (nextWallAngle == 0) {
    color = LIGHT_BLUE;
  } else {
    color = LIGHT_CYAN;
  }
  char wc[2];
  wc[0] = 'a' + nextWallAngle;
  wc[1] = 0;
  character(wc, 45, 6);
  bool posInRect = input.pos.x >= 1 && input.pos.x < 98 && input.pos.y >= 1 &&
                    input.pos.y < 98;
  if (input.isJustPressed && posInRect) {
    play(SELECT);
    addWall(input.pos.x, input.pos.y, nextWallAngle, false);
    nextWallAngle = wrap(nextWallAngle + 1, 0, 2);
  }
  if (target.initTicks > 0) {
    color = PURPLE;
    arc(VEC_XY(target.pos), target.size, 0, M_PI * 2);
    target.initTicks++;
    target.size += (5 - target.size) * 0.2;
    if (target.initTicks > 16) {
      target.initTicks = 0;
    }
  } else {
    color = RED;
    Collision tc = arc(VEC_XY(target.pos), target.size, 0, M_PI * 2);
    if (tc.isColliding.rect[GREEN]) {
      play(COIN);
      particle(VEC_XY(target.pos), 20, 3, 0, M_PI * 2);
      COUNT_IS_ALIVE(walls, aliveWallCount);
      addScore(aliveWallCount - 4, VEC_XY(target.pos));
      setTarget();
    }
    target.size += 0.02 * difficulty;
  }
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    if (w->isFixed) {
      if (w->angle == 0) {
        color = LIGHT_BLUE;
      } else {
        color = LIGHT_CYAN;
      }
    } else {
      if (w->angle == 0) {
        color = BLUE;
      } else {
        color = CYAN;
      }
    }
    float wd;
    if (w->isFixed) {
      wd = 100;
    } else {
      wd = 20;
    }
    float bw, bh;
    if (w->angle == 0) {
      bw = 2;
      bh = wd;
    } else {
      bw = wd;
      bh = 2;
    }
    Collision wc2 = box(VEC_XY(w->pos), bw, bh);
    if (!w->isFixed && (wc2.isColliding.rect[PURPLE] || wc2.isColliding.rect[BLUE] ||
                        wc2.isColliding.rect[CYAN])) {
      particle(VEC_XY(w->pos), 16, 1, 0, M_PI * 2);
      w->isAlive = false;
      continue;
    }
    if (wc2.isColliding.rect[RED]) {
      play(EXPLOSION);
      color = PURPLE;
      text("X", VEC_XY(w->pos));
      if (w->angle == 0) {
        text("X", w->pos.x, target.pos.y);
      } else {
        text("X", target.pos.x, w->pos.y);
      }
      gameOver();
    }
    if (wc2.isColliding.rect[GREEN]) {
      play(HIT);
      if (w->angle == 0) {
        ball.vel.x *= -1;
        ball.pos.x = w->pos.x + ball.vel.x * 4;
      } else {
        ball.vel.y *= -1;
        ball.pos.y = w->pos.y + ball.vel.y * 4;
      }
    }
  }
}

void addGameVhWalls() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
