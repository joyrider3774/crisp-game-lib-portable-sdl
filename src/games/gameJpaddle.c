#include "cglp.h"

static char *title = "JPADDLE";
static char *description = "[Tap]  Jump/Turn\n[Hold] Move";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = false};

#define JPADDLE_JUMP_HEIGHT 12
#define JPADDLE_MAX_JUMPS 99
#define JPADDLE_PADDLE_SIZE 6
#define JPADDLE_BALL_SPEED 1
#define JPADDLE_SPAWN_INTERVAL 60

static Vector paddle;
static int jumpCounter;
static int jumpToggle;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} JpaddleBall;
// Balls only die by falling past the paddle (a miss); a skilled player who
// keeps bouncing every ball never removes any, while spawn interval
// (60/difficulty) shrinks as difficulty grows forever, so live count grows
// quadratically with play time (~90 within the first minute) - size for a
// long session instead of a handful.
#define JPADDLE_MAX_BALL_COUNT 1024
static JpaddleBall balls[JPADDLE_MAX_BALL_COUNT];
static int ballIndex;

static void update() {
  if (!ticks) {
    vectorSet(&paddle, 50, 90);
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    jumpCounter = 0;
    jumpToggle = 0;
  }
  if (input.isJustPressed && jumpCounter < JPADDLE_MAX_JUMPS) {
    paddle.y -= JPADDLE_JUMP_HEIGHT / (jumpCounter + 1);
    jumpCounter++;
    jumpToggle++;
    play(JUMP);
  } else {
    paddle.y += difficulty * 0.1;
  }
  if (input.isPressed) {
    if (jumpCounter > 0) {
      float dir;
      if (jumpToggle % 2 == 1) {
        dir = 1;
      } else {
        dir = -1;
      }
      paddle.x += difficulty * 0.5 * dir;
    }
  }
  paddle.x = clamp(paddle.x, 3, 97);
  paddle.y = clamp(paddle.y, 30, 110);
  color = CYAN;
  rect(paddle.x - JPADDLE_PADDLE_SIZE / 2.0, paddle.y - 2, JPADDLE_PADDLE_SIZE, 4);
  color = BLACK;
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, JpaddleBall, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x += b->vel.x * JPADDLE_BALL_SPEED * difficulty;
    b->pos.y += b->vel.y * JPADDLE_BALL_SPEED * difficulty;
    if (b->pos.x < 0 || b->pos.x > 97) {
      b->vel.x *= -1;
      play(HIT);
    }
    thickness = 3;
    Collision scratch = arc(VEC_XY(b->pos), 3, 0, M_PI * 2);
    if (scratch.isColliding.rect[CYAN] && b->vel.y > 0) {
      b->pos.y = paddle.y - 3;
      b->vel.y *= -1;
      addScore(99 - b->pos.y, VEC_XY(b->pos));
      play(POWER_UP);
      jumpCounter -= 9;
      if (jumpCounter < 0) {
        jumpCounter = 0;
      }
    }
    if (b->pos.y > 100) {
      b->isAlive = false;
      continue;
    }
  }
  if (ticks % (int)floor(JPADDLE_SPAWN_INTERVAL / difficulty) == 0) {
    ASSIGN_ARRAY_ITEM(balls, ballIndex, JpaddleBall, nb);
    vectorSet(&nb->pos, rnd(10, 90), 0);
    Vector v;
    vectorSet(&v, rnd(-1, 1), 1);
    float len = vectorLength(&v);
    if (len > 0) {
      vectorMul(&v, 1.0 / len);
    }
    nb->vel = v;
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, JPADDLE_MAX_BALL_COUNT);
    play(LASER);
  }
  if (paddle.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameJpaddle() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
