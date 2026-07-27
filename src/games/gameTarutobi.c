#include "cglp.h"

static char *title = "TARUTOBI";
static char *description = "[Slide] Move";

// Vircon32 port note: upstream's raw template-literal strings each carry a
// trailing indentation-only line (whatever whitespace preceded the closing
// backtick in the source) as one extra "row" beyond the 6 real pixel rows -
// harmless upstream (that renderer just reads however many rows are given),
// but this port's character grid is a fixed CHARACTER_WIDTH(6)-row array,
// so that trailing formatting artifact is dropped here, keeping only the 6
// rows that actually draw pixels (verified row-by-row against the source
// strings, including the shorter, non-full-width rows - those are
// intentional in the original art, not a transcription mistake).
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "ll l l",
        "ll l l",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        "  ll  ",
        " llll ",
        "l    l",
        "l    l",
        " l  l ",
        "  ll  ",
    },
};
static int charactersCount = 3;

// This game reads input.pos.x directly (the original control is "slide
// anywhere on screen to steer"), so it must be registered with usesMouse =
// true for input.pos to track the cursor.
static Options options = {
    .viewSizeX = 120, .viewSizeY = 60, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  float t;
  bool isScoreAdded;
  bool isAlive;
} Obstacle;
#define TARUTOBI_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[TARUTOBI_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static Vector p;
static Vector v;
static bool isJumping;
static int addedScore;

static void update() {
  if (!ticks) {
    vectorSet(&p, 30, 30);
    vectorSet(&v, 0, 0);
    isJumping = true;
    addedScore = 1;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
  }
  if (rnd(0, 1) < 0.01 * difficulty) {
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, 123, 47);
    vectorSet(&no->vel, -rnd(0.4, 0.8 * difficulty), 0);
    no->t = 0;
    no->isScoreAdded = false;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, TARUTOBI_MAX_OBSTACLE_COUNT);
  }
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, t);
    SKIP_IS_NOT_ALIVE(t);
    vectorAdd(&t->pos, t->vel.x, t->vel.y);
    // Vircon32 port note: upstream computes this rotation as a raw,
    // unbounded floor(t.t / 10) - the original (canvas-based) renderer
    // treats rotation as a continuous angle, where any integer multiple of
    // a quarter-turn wraps around to the same visual result regardless of
    // sign or magnitude. This engine's characterOptions.rotation only
    // special-cases the values 1/2/3 (anything else, including negative
    // values or values >= 4, renders unrotated), so the value is
    // normalized into 0-3 here to reproduce the same visual cycling.
    int rot = (int)floor(t->t / 10) % 4;
    if (rot < 0) {
      rot += 4;
    }
    characterOptions.rotation = rot;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    if (isJumping) {
      text("*", VEC_XY(t->pos));
    } else {
      character("c", VEC_XY(t->pos));
    }
    characterOptions.rotation = 0;
    t->t += t->vel.x;
    if (!t->isScoreAdded && t->pos.x < p.x) {
      play(COIN);
      addScore(addedScore, VEC_XY(t->pos));
      addedScore++;
      t->isScoreAdded = true;
    }
    if (t->pos.x <= 0) {
      t->isAlive = false;
      continue;
    }
  }
  rect(0, 50, 120, 9);
  if (isJumping) {
    v.y += 0.1;
    if (p.y > 47) {
      isJumping = false;
      p.y = 47;
      v.y = 0;
    }
  } else {
    vectorMul(&v, 0.95);
    addedScore = 1;
  }
  if (p.x < 0 || p.x > 120) {
    p.x = clamp(p.x, 0, 120);
    v.x = 0;
  }
  float steerFactor;
  if (isJumping) {
    steerFactor = 0.004;
  } else {
    steerFactor = 0.01;
  }
  v.x += clamp(input.pos.x - p.x, -7, 7) * steerFactor;
  vectorAdd(&p, v.x, v.y);
  characterOptions.rotation = 0;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  char playerChar[2];
  playerChar[0] = 'a' + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  Collision c = character(playerChar, VEC_XY(p));
  if (c.isColliding.character['c']) {
    play(JUMP);
    isJumping = true;
    p.y = 42;
    v.y = -1.2 - fabs(v.x);
  }
  if (c.isColliding.text['*']) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameTarutobi() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
