#include "cglp.h"

static char *title = "BASERUNNER DASH";
static char *description = "[Hold] Run forward\n[Release] Run back";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 60, .isDarkColor = false};

#define BASERUNNERDASH_BASE_RADIUS 3
#define BASERUNNERDASH_RUNNER_RADIUS 4
#define BASERUNNERDASH_BALL_RADIUS 2

static Vector basePositions[4];

typedef struct {
  Vector pos;
  int targetBase;
  int currentBase;
  float speed;
  bool isOnBase;
} Runner;
static Runner runner;

typedef struct {
  Vector pos;
  int targetBase;
  int currentBase;
  float speed;
  bool isThrown;
} Ball;
static Ball ball;

static void moveToward(Vector *p, float tx, float ty, float speed) {
  float dx = tx - p->x;
  float dy = ty - p->y;
  float len = sqrt(dx * dx + dy * dy);
  if (len > 0) {
    vectorAdd(p, dx / len * speed, dy / len * speed);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&basePositions[0], 50, 90);
    vectorSet(&basePositions[1], 90, 50);
    vectorSet(&basePositions[2], 50, 10);
    vectorSet(&basePositions[3], 10, 50);
    runner.pos = basePositions[1];
    runner.targetBase = 2;
    runner.currentBase = 1;
    runner.speed = 0.6;
    runner.isOnBase = true;
    ball.pos = basePositions[0];
    ball.targetBase = 2;
    ball.currentBase = 0;
    ball.speed = 0.3;
    ball.isThrown = true;
  }
  Vector currentBase = basePositions[runner.currentBase];
  Vector nextBase = basePositions[runner.targetBase];
  float runnerSpeed = runner.speed * difficulty;
  if (input.isJustPressed || input.isJustReleased) {
    play(CLICK);
  }
  if (input.isPressed) {
    moveToward(&runner.pos, nextBase.x, nextBase.y, runnerSpeed);
    runner.isOnBase = false;
    if (distanceTo(&runner.pos, nextBase.x, nextBase.y) < runnerSpeed) {
      play(COIN);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      runner.pos = nextBase;
      runner.currentBase = runner.targetBase;
      runner.targetBase = (runner.targetBase + 1) % 4;
      runner.isOnBase = true;
    }
  } else {
    if (distanceTo(&runner.pos, currentBase.x, currentBase.y) < runnerSpeed) {
      runner.pos = currentBase;
      if (!runner.isOnBase) {
        play(HIT);
      }
      runner.isOnBase = true;
    } else {
      moveToward(&runner.pos, currentBase.x, currentBase.y, runnerSpeed);
    }
  }
  Vector targetPos = basePositions[ball.targetBase];
  float thrownMul;
  if (ball.isThrown) {
    thrownMul = 3;
  } else {
    thrownMul = 1;
  }
  float ballSpeed = ball.speed * thrownMul * difficulty;
  moveToward(&ball.pos, targetPos.x, targetPos.y, ballSpeed);
  if (distanceTo(&ball.pos, targetPos.x, targetPos.y) < ballSpeed) {
    ball.isThrown = false;
    ball.pos = targetPos;
    ball.currentBase = ball.targetBase;
    if (ball.targetBase == runner.currentBase) {
      ball.targetBase = runner.targetBase;
    } else {
      ball.targetBase = runner.currentBase;
    }
  } else if (!ball.isThrown && rnd(0, 1) < 0.05 * difficulty) {
    play(POWER_UP);
    ball.isThrown = true;
    if (ball.currentBase != runner.currentBase &&
        ball.currentBase != runner.targetBase) {
      ball.targetBase = runner.targetBase;
    }
  }
  color = LIGHT_BLACK;
  TIMES(4, i) {
    box(basePositions[i].x, basePositions[i].y, BASERUNNERDASH_BASE_RADIUS * 2,
        BASERUNNERDASH_BASE_RADIUS * 2);
  }
  if (runner.isOnBase) {
    color = LIGHT_BLUE;
  } else {
    color = BLUE;
  }
  box(runner.pos.x, runner.pos.y, BASERUNNERDASH_RUNNER_RADIUS * 2,
      BASERUNNERDASH_RUNNER_RADIUS * 2);
  if (ball.isThrown) {
    color = LIGHT_RED;
  } else {
    color = RED;
  }
  Collision c = box(ball.pos.x, ball.pos.y, BASERUNNERDASH_BALL_RADIUS * 2,
                     BASERUNNERDASH_BALL_RADIUS * 2);
  if (c.isColliding.rect[BLUE] && !ball.isThrown) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameBaserunnerDash() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
