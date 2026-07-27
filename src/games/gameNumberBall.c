#include "cglp.h"
#include "string.h"

static char *title = "NUMBER BALL";
static char *description = "[Hold]    Set angle\n[Release] Hit a shot";

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
    .viewSizeX = 200, .viewSizeY = 60, .soundSeed = 8, .isDarkColor = false};

#define NUMBERBALL_STATE_STAY 0
#define NUMBERBALL_STATE_FLYING 1
#define NUMBERBALL_STATE_ON_FLOOR 2
#define NUMBERBALL_STATE_REMOVING 3
#define NUMBERBALL_STATE_FALLING 4

typedef struct {
  int value;
  Vector pos;
  Vector vel;
  int state;
  bool isAlive;
} NumberBallBall;
#define NUMBERBALL_MAX_BALL_COUNT 64
static NumberBallBall balls[NUMBERBALL_MAX_BALL_COUNT];
static int ballIndex;
static bool isAddingNextBall;

typedef struct {
  bool hasValue;
  int value;
  float x;
  float width;
} NumberBallFloor;
// Floors are only ever created at the right edge and removed by a matching
// ball (or the game ends before they'd ever pile up) - generous but bounded.
#define NUMBERBALL_MAX_FLOOR_COUNT 64
static NumberBallFloor floors[NUMBERBALL_MAX_FLOOR_COUNT];
static int floorCount;

static int multiplier;
static float scrV;
static float scrVB;

static void update() {
  // Never reads a Collision result anywhere in this file - ball-vs-floor
  // matching is a direct position-range check (see "b->pos.x > f->x - 3"
  // below) and ball-vs-ball bouncing uses distanceTo() directly, so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    isAddingNextBall = true;
    floorCount = 1;
    floors[0].hasValue = false;
    floors[0].value = 0;
    floors[0].x = 0;
    floors[0].width = 200;
    multiplier = 1;
    scrV = 0;
    scrVB = 0;
  }
  float lx = floors[0].x + floors[0].width;
  float scr = sqrt(difficulty) * 0.04 + scrV;
  scrV *= 0.9;
  scrVB = clamp(scrVB - 0.01, 0, 9);
  if (lx > 100) {
    scr += (lx - 100) * 0.1;
  }
  float ballMaxValue = clamp(difficulty * 2, 1, 9);
  float rx = 0;
  int fi = 0;
  while (fi < floorCount) {
    NumberBallFloor *f = &floors[fi];
    f->x -= scr;
    if (fi == 0) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_GREEN;
    }
    rect(f->x, 50, f->width - 2, 10);
    rx = f->x + f->width;
    bool removed = false;
    if (f->hasValue) {
      color = RED;
      text(intToChar(f->value), f->x + 7, 55);
      bool isRemoving = false;
      FOR_EACH(balls, bi) {
        ASSIGN_ARRAY_ITEM(balls, bi, NumberBallBall, b);
        SKIP_IS_NOT_ALIVE(b);
        if (b->state == NUMBERBALL_STATE_ON_FLOOR && b->pos.x > f->x - 3 &&
            b->pos.x < rx + 3 && f->value == b->value) {
          isRemoving = true;
        }
      }
      if (isRemoving) {
        play(COIN);
        addScore(f->value * multiplier, f->x + 7, 50);
        if (multiplier < 16) {
          multiplier *= 2;
        }
        removed = true;
      }
    }
    if (!removed && fi < floorCount - 1) {
      NumberBallFloor *rf = &floors[fi + 1];
      if (rx < rf->x + 2) {
        f->x += (rf->x - rx) * 0.1;
      } else {
        f->x = rf->x - f->width;
      }
    }
    if (removed) {
      memcpy(&floors[fi], &floors[fi + 1], (floorCount - 1 - fi) * sizeof(floors[0]));
      floorCount--;
    } else {
      fi++;
    }
  }
  color = GREEN;
  rect(0, 50, 20, 10);
  color = WHITE;
  rect(20, 50, 13, 10);
  if (floors[0].x + floors[0].width < 30) {
    color = RED;
    text("X", 33, 55);
    play(EXPLOSION);
    gameOver();
  }
  if (rx < 200) {
    play(HIT);
    if (floorCount < NUMBERBALL_MAX_FLOOR_COUNT) {
      NumberBallFloor *newF = &floors[floorCount];
      newF->value = rndi(0, (int)floor(ballMaxValue)) + 1;
      newF->hasValue = true;
      newF->x = 200;
      newF->width = rnd(20, 50);
      floorCount++;
    }
  }
  if (ticks > 30 && isAddingNextBall) {
    int value = 1;
    TIMES(9, vi) {
      value = rndi(0, (int)floor(ballMaxValue)) + 1;
      bool isMatched = false;
      TIMES(floorCount, fj) {
        if (floors[fj].hasValue && floors[fj].value == value) {
          isMatched = true;
        }
      }
      if (isMatched) {
        break;
      }
    }
    play(LASER);
    ASSIGN_ARRAY_ITEM(balls, ballIndex, NumberBallBall, nb);
    nb->value = value;
    vectorSet(&nb->pos, 10, 47);
    vectorSet(&nb->vel, 1, 0);
    rotate(&nb->vel, -0.1);
    nb->state = NUMBERBALL_STATE_STAY;
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, NUMBERBALL_MAX_BALL_COUNT);
    isAddingNextBall = false;
  }
  FOR_EACH(balls, i) {
    ASSIGN_ARRAY_ITEM(balls, i, NumberBallBall, b);
    SKIP_IS_NOT_ALIVE(b);
    if (b->state == NUMBERBALL_STATE_REMOVING) {
      b->isAlive = false;
      continue;
    }
    if (b->state == NUMBERBALL_STATE_STAY) {
      if (input.isJustPressed) {
        play(SELECT);
      }
      if (input.isPressed) {
        rotate(&b->vel, -0.01 * difficulty);
        color = BLACK;
        Vector lp1;
        lp1 = b->vel;
        vectorMul(&lp1, 3);
        vectorAdd(&lp1, b->pos.x, b->pos.y);
        Vector lp2;
        lp2 = b->vel;
        vectorMul(&lp2, 15);
        vectorAdd(&lp2, b->pos.x, b->pos.y);
        thickness = 2;
        line(lp1.x, lp1.y, lp2.x, lp2.y);
      }
      if (input.isJustReleased || vectorAngle(&b->vel) < -M_PI * 0.47) {
        play(POWER_UP);
        if (multiplier > 1) {
          multiplier /= 2;
        }
        scrV = scrVB;
        scrVB++;
        vectorMul(&b->vel, 5);
        b->state = NUMBERBALL_STATE_FLYING;
      }
    } else if (b->state == NUMBERBALL_STATE_FLYING) {
      b->vel.y += 0.1;
      vectorMul(&b->vel, 0.99);
      vectorAdd(&b->pos, b->vel.x, b->vel.y);
      if (b->pos.y > 47) {
        b->vel.y *= -0.5;
        b->vel.x *= 0.5;
        b->pos.y = 47;
        if (fabs(b->vel.y) < 0.5) {
          b->state = NUMBERBALL_STATE_ON_FLOOR;
          isAddingNextBall = true;
        }
      }
    } else if (b->state == NUMBERBALL_STATE_ON_FLOOR) {
      b->vel.x *= 0.9;
      b->pos.x += b->vel.x;
      b->vel.y = 0;
      if (b->pos.x < 30) {
        b->state = NUMBERBALL_STATE_FALLING;
      }
    } else if (b->state == NUMBERBALL_STATE_FALLING) {
      b->vel.y += 0.1;
      b->pos.y += b->vel.y;
      if (b->pos.y > 63) {
        b->isAlive = false;
        continue;
      }
    }
    if (b->state != NUMBERBALL_STATE_STAY && b->state != NUMBERBALL_STATE_FALLING) {
      b->pos.x -= scr;
      FOR_EACH(balls, oi) {
        if (oi == i) {
          continue;
        }
        ASSIGN_ARRAY_ITEM(balls, oi, NumberBallBall, ob);
        SKIP_IS_NOT_ALIVE(ob);
        if (distanceTo(&ob->pos, b->pos.x, b->pos.y) < 6) {
          addWithAngle(&b->vel, angleTo(&ob->pos, b->pos.x, b->pos.y), vectorLength(&ob->vel) * 0.7);
          addWithAngle(&ob->vel, angleTo(&b->pos, ob->pos.x, ob->pos.y), vectorLength(&b->vel) * 0.7);
        }
      }
    }
    color = BLUE;
    text(intToChar(b->value), b->pos.x, b->pos.y);
    if (b->pos.x > 220 || b->pos.x < -20 || b->pos.y < -50) {
      if (b->state == NUMBERBALL_STATE_FLYING) {
        isAddingNextBall = true;
      }
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 10);
}

void addGameNumberBall() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
