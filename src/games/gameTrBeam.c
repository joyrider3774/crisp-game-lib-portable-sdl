#include "cglp.h"

static char *title = "TR BEAM";
static char *description = "[Hold]\n Tractor beam";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "y cc y",
        " cccc ",
        "llllll",
        "lllll ",
        " l l  ",
            "      ",
},
    {
        "y cc y",
        " cccc ",
        "llllll",
        " lllll",
        "  l l ",
            "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = true};

typedef struct {
  Vector pos;
  float angle;
  float trLength;
} Ufo;
static Ufo ufo;

typedef struct {
  Vector pos;
  Vector vel;
  float radius;
  bool isRed;
  bool isBeamed;
  bool isAlive;
} Ball;
#define TRBEAM_MAX_BALL_COUNT 64
static Ball balls[TRBEAM_MAX_BALL_COUNT];
static int ballIndex;
static float nextBallTicks;
static int nextRedBallCount;

static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&ufo.pos, 0, 9);
    ufo.angle = -M_PI * 0.3;
    ufo.trLength = 0;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    nextBallTicks = 0;
    nextRedBallCount = 9;
    multiplier = 1;
  }
  ufo.angle += sqrt(difficulty) * 0.03;
  ufo.pos.x = sin(ufo.angle) * 40 + 50;
  if (input.isJustPressed) {
    play(SELECT);
    multiplier = 1;
  }
  if (input.isPressed) {
    play(LASER);
    ufo.trLength = clamp(ufo.trLength + difficulty * 2, 0, 82);
  } else {
    ufo.trLength *= 1 - clamp(sqrt(difficulty), 1, 3) * 0.2;
  }
  float ta = ((float)(ticks % 10) / 10) * (M_PI / 4);
  if (ufo.trLength > 1) {
    color = BLUE;
    thickness = 2;
    TIMES(4, li) {
      line(4 * cos(ta) + ufo.pos.x, ufo.pos.y + 5, 9 * cos(ta) + ufo.pos.x,
           ufo.pos.y + 5 + ufo.trLength);
      ta += M_PI / 4;
    }
  }
  color = BLACK;
  char uc[2];
  uc[0] = 'a' + (ticks / 30) % 2;
  uc[1] = 0;
  character(uc, VEC_XY(ufo.pos));
  nextBallTicks--;
  if (nextBallTicks < 0) {
    nextRedBallCount--;
    bool isRed = false;
    if (nextRedBallCount < 0) {
      isRed = true;
      nextRedBallCount = rndi(9, 12);
    }
    float radius = rnd(6, 12);
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, nb);
    vectorSet(&nb->pos, rnd(10, 90), 105 + radius);
    vectorSet(&nb->vel, 0, 0);
    nb->radius = radius;
    nb->isRed = isRed;
    nb->isBeamed = false;
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, TRBEAM_MAX_BALL_COUNT);
    nextBallTicks = rnd(20, 25) / sqrt(difficulty);
  }
  FOR_EACH(balls, bi) {
    ASSIGN_ARRAY_ITEM(balls, bi, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->pos.y > 99) {
      b->vel.y -= sqrt(difficulty);
      vectorMul(&b->vel, 0.5);
    } else if (b->pos.x < 0 || b->pos.x > 99) {
      vectorMul(&b->vel, 0.1);
    } else {
      vectorMul(&b->vel, 0.9);
      if (b->isRed) {
        b->vel.y *= 0.9;
      }
    }
    b->vel.y += sqrt(difficulty) * 0.1;
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    FOR_EACH(balls, abi) {
      if (abi == bi) {
        continue;
      }
      ASSIGN_ARRAY_ITEM(balls, abi, Ball, ab);
      SKIP_IS_NOT_ALIVE(ab);
      float d = distanceTo(&ab->pos, b->pos.x, b->pos.y) - ab->radius - b->radius;
      if (d < 0) {
        addWithAngle(&b->vel, angleTo(&ab->pos, b->pos.x, b->pos.y), -d / sqrt(b->radius));
      }
    }
    if (b->isRed) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    Collision c = arc(b->pos.x, b->pos.y, b->radius, 0, M_PI * 2);
    if (c.isColliding.character['a'] || c.isColliding.character['b']) {
      if (b->isRed) {
        play(EXPLOSION);
        gameOver();
      } else {
        play(COIN);
        addScore(multiplier, VEC_XY(b->pos));
        multiplier++;
      }
      b->isAlive = false;
      continue;
    } else if (c.isColliding.rect[BLUE]) {
      addWithAngle(&b->vel, angleTo(&b->pos, ufo.pos.x, ufo.pos.y),
                   sqrt(difficulty) / sqrt(b->radius));
      b->vel.x += (ufo.pos.x - b->pos.x) * clamp(sqrt(difficulty), 1, 5) * 0.01;
      b->isBeamed = true;
      if (b->isRed) {
        play(HIT);
      }
    } else if (b->isBeamed) {
      b->vel.y *= 0.1;
      b->vel.x *= 5;
      b->isBeamed = false;
    }
    bool inStripe = b->pos.x >= 10 && b->pos.x < 90 && b->pos.y >= 12 + b->radius &&
                    b->pos.y < 17 + b->radius;
    if (b->isRed && !b->isBeamed && (b->pos.x < b->radius || b->pos.x > 99 - b->radius)) {
      play(POWER_UP);
      b->isRed = false;
    } else if (!b->isBeamed && inStripe && b->vel.y > 0) {
      b->isRed = true;
    }
    if (b->pos.x < -20 || b->pos.x > 120) {
      b->isAlive = false;
      continue;
    }
  }
  if (ufo.trLength > 1) {
    color = CYAN;
    thickness = 3;
    TIMES(4, li2) {
      line(4 * cos(ta) + ufo.pos.x, ufo.pos.y + 5, 9 * cos(ta) + ufo.pos.x,
           ufo.pos.y + 5 + ufo.trLength);
      ta += M_PI / 4;
    }
  }
}

void addGameTrBeam() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
