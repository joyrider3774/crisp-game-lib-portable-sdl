#include "cglp.h"

static char *title = "FLIPBOMB";
static char *description = "[Tap] Flip";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  Vector pos;
  int t;
  float a;
  bool isAlive;
} FlipbombExplosion;
// Bomb-hit rate grows with difficulty (interval shrinks as ~1/difficulty, and
// missing a bomb ends the run so nearly every bomb becomes an explosion), but
// each explosion's 30-tick lifetime is fixed - concurrent count is ~0.75 *
// difficulty, unbounded over a long session, so 16 isn't enough headroom.
#define FLIPBOMB_MAX_EXPLOSION_COUNT 256
static FlipbombExplosion flipbombExplosions[FLIPBOMB_MAX_EXPLOSION_COUNT];
static int flipbombExplosionIndex;

typedef struct {
  Vector pos;
  Vector vel;
  int s;
  bool isAlive;
} FlipbombBall;
#define FLIPBOMB_MAX_BALL_COUNT 16
static FlipbombBall flipbombBalls[FLIPBOMB_MAX_BALL_COUNT];
static int flipbombBallIndex;
static float flipbombBallAppTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} FlipbombBomb;
#define FLIPBOMB_MAX_BOMB_COUNT 32
static FlipbombBomb flipbombBombs[FLIPBOMB_MAX_BOMB_COUNT];
static int flipbombBombIndex;
static float flipbombBombAppTicks;

static int flipbombMultiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(flipbombExplosions);
    flipbombExplosionIndex = 0;
    INIT_UNALIVED_ARRAY(flipbombBalls);
    flipbombBallIndex = 0;
    INIT_UNALIVED_ARRAY(flipbombBombs);
    flipbombBombIndex = 0;
    flipbombBombAppTicks = 0;
    flipbombBallAppTicks = 0;
    flipbombMultiplier = 1;
  }
  color = BLUE;
  thickness = 3;
  line(99, 75, 75, 86);
  if (input.isJustPressed) {
    play(LASER);
    thickness = 3;
    line(70, 89, 50, 80);
    flipbombMultiplier = 1;
  } else {
    thickness = 3;
    line(70, 89, 50, 98);
  }
  color = PURPLE;
  FOR_EACH(flipbombExplosions, i) {
    ASSIGN_ARRAY_ITEM(flipbombExplosions, i, FlipbombExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    float a = e->a;
    float r;
    if (e->t < 20) {
      r = e->t * 0.5;
    } else {
      r = 20 * 0.5 - (e->t - 20);
    }
    float s;
    if (e->t == 0) {
      s = 10;
    } else {
      s = r + 3;
    }
    TIMES(5, k) {
      Vector p;
      p = e->pos;
      addWithAngle(&p, a, r);
      box(p.x, p.y, s, s);
      a += M_PI * 2 / 5;
    }
    e->t++;
    e->a += 0.2;
    if (e->t >= 30) {
      e->isAlive = false;
    }
  }
  flipbombBallAppTicks -= difficulty;
  if (flipbombBallAppTicks < 0) {
    ASSIGN_ARRAY_ITEM(flipbombBalls, flipbombBallIndex, FlipbombBall, nb);
    vectorSet(&nb->pos, 99, 70);
    vectorSet(&nb->vel, -1, 0.5);
    vectorMul(&nb->vel, difficulty);
    nb->s = 0;
    nb->isAlive = true;
    flipbombBallIndex = wrap(flipbombBallIndex + 1, 0, FLIPBOMB_MAX_BALL_COUNT);
    flipbombBallAppTicks = 60;
  }
  FOR_EACH(flipbombBalls, i) {
    ASSIGN_ARRAY_ITEM(flipbombBalls, i, FlipbombBall, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    if (b->s == 0) {
      color = BLUE;
    } else {
      color = CYAN;
    }
    if (b->s == 0) {
      if (b->pos.x < 70) {
        if (input.isJustPressed) {
          vectorSet(&b->vel, 2, -4);
          rotate(&b->vel, (b->pos.x - 70) * 0.06);
          b->s = 1;
        }
      } else if (b->pos.x < 50) {
        b->s = 1;
      }
    } else {
      particle(b->pos.x, b->pos.y, 1, vectorLength(&b->vel), vectorAngle(&b->vel) + M_PI, 0.1);
    }
    Collision scratch = box(VEC_XY(b->pos), 3, 3);
    if (scratch.isColliding.rect[PURPLE]) {
      b->isAlive = false;
      continue;
    }
    if (!(b->pos.x >= 0 && b->pos.x <= 99 && b->pos.y >= 0 && b->pos.y <= 99)) {
      b->isAlive = false;
      continue;
    }
  }
  flipbombBombAppTicks -= difficulty;
  if (flipbombBombAppTicks < 0) {
    Vector p;
    vectorSet(&p, rnd(0, 80), 0);
    Vector v;
    vectorSet(&v, rnd(20, 70), 70);
    vectorAdd(&v, -p.x, -p.y);
    vectorMul(&v, 1.0 / (500 / rnd(1, difficulty)));
    ASSIGN_ARRAY_ITEM(flipbombBombs, flipbombBombIndex, FlipbombBomb, nbm);
    nbm->pos = p;
    nbm->vel = v;
    nbm->isAlive = true;
    flipbombBombIndex = wrap(flipbombBombIndex + 1, 0, FLIPBOMB_MAX_BOMB_COUNT);
    flipbombBombAppTicks += rnd(30, 50);
  }
  FOR_EACH(flipbombBombs, i) {
    ASSIGN_ARRAY_ITEM(flipbombBombs, i, FlipbombBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    color = RED;
    Collision scratch = box(VEC_XY(b->pos), 5, 5);
    if (scratch.isColliding.rect[CYAN] || scratch.isColliding.rect[PURPLE]) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(flipbombExplosions, flipbombExplosionIndex, FlipbombExplosion, ne);
      ne->pos = b->pos;
      ne->t = 0;
      ne->a = rnd(0, M_PI) * RNDPM();
      ne->isAlive = true;
      flipbombExplosionIndex =
          wrap(flipbombExplosionIndex + 1, 0, FLIPBOMB_MAX_EXPLOSION_COUNT);
      color = PURPLE;
      particle(b->pos.x, b->pos.y, 16, 3, 0, M_PI * 2);
      addScore(flipbombMultiplier, b->pos.x, b->pos.y);
      flipbombMultiplier++;
      b->isAlive = false;
      continue;
    }
    if (b->pos.y > 99 || scratch.isColliding.rect[BLUE]) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameFlipbomb() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
