#include "cglp.h"

static char *title = "REFBALS";
static char *description = "[Hold] Accel";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

#define WALL_COUNT 5
static Vector walls[WALL_COUNT];

// Vircon32 port note: upstream's `balls` is a plain JS array that only ever
// grows (push(), never spliced/filtered) - the game always ends the instant
// any single ball reaches the bottom (see gameOver() below), so in practice
// only a handful of balls are ever alive at once even over a long play
// session. A fixed ring buffer with generous headroom reproduces the same
// observable behavior as the upstream unbounded array.
typedef struct {
  Vector p;
  float v;
  bool isAlive;
} Ball;
#define MAX_BALL_COUNT 64
static Ball balls[MAX_BALL_COUNT];
static int ballIndex;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    TIMES(WALL_COUNT, i) {
      vectorSet(&walls[i], i * -29, -9);
    }
  }
  if (ticks % 99 == 0) {
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, nb);
    vectorSet(&nb->p, rnd(0, 50), 0);
    nb->v = 0;
    nb->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, MAX_BALL_COUNT);
  }
  color = BLUE;
  TIMES(WALL_COUNT, wi) {
    Vector *w = &walls[wi];
    if (input.isPressed) {
      w->x -= 2;
    } else {
      w->x -= 1;
    }
    box(w->x, w->y, 36, 3);
    if (w->x < -19) {
      w->x += rnd(130, 150);
      w->y = rnd(50, 90);
    }
  }
  color = PURPLE;
  FOR_EACH(balls, bi) {
    ASSIGN_ARRAY_ITEM(balls, bi, Ball, b);
    SKIP_IS_NOT_ALIVE(b);
    b->v += 0.03;
    b->p.y += b->v;
    if (b->p.y > 99) {
      play(EXPLOSION);
      gameOver();
    }
    Collision bc = box(VEC_XY(b->p), 5, 5);
    if (bc.isColliding.rect[BLUE]) {
      play(SELECT);
      score++;
      b->v *= -1;
      b->p.y += b->v * 2;
    }
  }
}

void addGameRefbals() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
