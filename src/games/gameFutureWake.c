#include "cglp.h"
#include "string.h"

static char *title = "FUTURE WAKE";
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
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

#define FUTUREWAKE_BALL_SIZE 3

typedef struct {
  Vector pos;
  Vector vel;
  float speed;
} BallRoot;
static BallRoot ballRoot;

typedef struct {
  Vector pos;
  Vector vel;
} BallWake;
static BallWake ballWake;

static float ballDist;
static int wakeColor;
static float wakeDist;
static int wakeCount;
static Vector ballPos;
static bool isHittingRacket;
static float multiplier;
static float rx;

static void updateBall() {
  wakeDist += FUTUREWAKE_BALL_SIZE;
  if (wakeColor == LIGHT_BLACK && wakeDist >= ballDist) {
    float r = (wakeDist - ballDist) / FUTUREWAKE_BALL_SIZE;
    vectorSet(&ballPos,
              ballWake.pos.x + ballWake.vel.x * (1 - r),
              ballWake.pos.y + ballWake.vel.y * (1 - r));
    wakeColor = LIGHT_CYAN;
  }
  if (wakeColor == LIGHT_CYAN &&
      (ballWake.pos.y < 7 || ballWake.pos.y > 93)) {
    wakeColor = LIGHT_RED;
    wakeCount = 5;
  }
  vectorAdd(&ballWake.pos, ballWake.vel.x, ballWake.vel.y);
  color = wakeColor;
  Collision c = box(ballWake.pos.x, ballWake.pos.y, FUTUREWAKE_BALL_SIZE,
                     FUTUREWAKE_BALL_SIZE);
  if (c.isColliding.rect[LIGHT_BLUE]) {
    if ((ballWake.pos.x < 50 && ballWake.vel.x < 0) ||
        (ballWake.pos.x > 50 && ballWake.vel.x > 0)) {
      ballWake.vel.x *= -1;
    }
  }
  if (c.isColliding.rect[BLUE] &&
      ((ballWake.pos.y < 50 && ballWake.vel.y < 0) ||
       (ballWake.pos.y > 50 && ballWake.vel.y > 0))) {
    float d = (ballWake.pos.x - rx) / 10;
    if (fabs(d) < 0.6) {
      ballWake.vel.y *= -1;
    } else {
      float a;
      if (d > 0) {
        a = M_PI / 4 + (1 - d);
      } else {
        a = (M_PI / 4) * 3 - (1 + d);
      }
      if (ballWake.pos.y > 50) {
        a *= -1;
      }
      vectorSet(&ballWake.vel, 0, 0);
      addWithAngle(&ballWake.vel, a, FUTUREWAKE_BALL_SIZE);
    }
    if (wakeDist < ballDist) {
      play(HIT);
      ballRoot.pos = ballWake.pos;
      ballRoot.vel = ballWake.vel;
      ballPos = ballWake.pos;
      isHittingRacket = true;
    }
    wakeColor = LIGHT_GREEN;
    wakeCount = 9;
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&ballRoot.pos, 50, 50);
    vectorSet(&ballRoot.vel, 0, 0);
    addWithAngle(&ballRoot.vel, M_PI / 4, FUTUREWAKE_BALL_SIZE);
    ballRoot.speed = 1;
    vectorSet(&ballWake.pos, 0, 0);
    vectorSet(&ballWake.vel, 0, 0);
    wakeColor = LIGHT_BLACK;
    ballDist = 0;
    wakeDist = 0;
    wakeCount = 0;
    vectorSet(&ballPos, 0, 0);
    multiplier = 1;
    isHittingRacket = false;
  }
  color = LIGHT_BLUE;
  rect(0, 0, 7, 100);
  rect(93, 0, 7, 100);
  rx = clamp(input.pos.x, 17, 83);
  color = BLUE;
  box(rx, 5, 20, 5);
  box(rx, 95, 20, 5);
  wakeColor = LIGHT_BLACK;
  wakeDist = 0;
  wakeCount = 99;
  ballDist += ballRoot.speed;
  ballWake.pos = ballRoot.pos;
  ballWake.vel = ballRoot.vel;
  isHittingRacket = false;
  while (wakeCount > 0) {
    updateBall();
    wakeCount--;
  }
  if (wakeColor == LIGHT_GREEN) {
    ballRoot.speed += difficulty;
  } else {
    ballRoot.speed = difficulty;
  }
  color = BLACK;
  box(ballPos.x, ballPos.y, FUTUREWAKE_BALL_SIZE, FUTUREWAKE_BALL_SIZE);
  if (ballPos.y < 0 || ballPos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  if (isHittingRacket) {
    ballDist = 0;
    addScore(floor(multiplier), ballPos.x,
             clamp(ballPos.y, 30, 80));
    multiplier += 10;
    if (multiplier > 100) {
      ballRoot.vel.x += rnd(0, 1) * RNDPM();
      float len = vectorLength(&ballRoot.vel);
      vectorMul(&ballRoot.vel, 1.0 / len);
      vectorMul(&ballRoot.vel, 3);
    }
  }
  multiplier = clamp(multiplier * 0.99, 1, 100.99);
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)floor(multiplier)));
  text(multText, 3, 9);
}

void addGameFutureWake() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
