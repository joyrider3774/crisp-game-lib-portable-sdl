#include "cglp.h"

static char *title = "TOTOGE";
static char *description = "[Tap] Jump";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " r r  ",
        " lll l",
        "yl lll",
        " lllll",
        "  llyl",
        "     y",
    },
    {
        " r r  ",
        " lll  ",
        "yl ll ",
        " lllll",
        " yl ll",
        "y     ",
    },
    {
        "  rr  ",
        "  rr  ",
        " rRRr ",
        " rRRr ",
        "rRRRRr",
        "rrrrrr",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

typedef struct {
  Vector pos;
  int width;
  bool isAlive;
} Spike;
#define TOTOGE_MAX_SPIKE_COUNT 32
static Spike spikes[TOTOGE_MAX_SPIKE_COUNT];
static int spikeIndex;
static float spikeAddDist;
static float nextSpikeX;
static float nextSpikeVx;

typedef struct {
  Vector pos;
  bool isAlive;
} Wall;
#define TOTOGE_MAX_WALL_COUNT 128
static Wall walls[TOTOGE_MAX_WALL_COUNT];
static int wallIndex;
static float nextWallY;

static Vector pos;
static Vector vel;
static float scr;

static void addWall(float x, float y) {
  ASSIGN_ARRAY_ITEM(walls, wallIndex, Wall, w);
  vectorSet(&w->pos, x, y);
  w->isAlive = true;
  wallIndex = wrap(wallIndex + 1, 0, TOTOGE_MAX_WALL_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    spikeAddDist = 0;
    nextSpikeX = 10;
    nextSpikeVx = 1;
    vectorSet(&pos, 50, 9);
    vectorSet(&vel, 0.33, 0);
    scr = 0;
    INIT_UNALIVED_ARRAY(walls);
    wallIndex = 0;
    for (int i = -10; i < 10; i++) {
      addWall(5, i * 10 + 5);
      addWall(95, i * 10 + 5);
    }
    nextWallY = 105;
  }
  while (nextWallY < 105) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    addWall(5, nextWallY);
    addWall(95, nextWallY);
    nextWallY += 10;
  }
  nextWallY -= scr;
  color = LIGHT_BLUE;
  FOR_EACH(walls, i) {
    ASSIGN_ARRAY_ITEM(walls, i, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    w->pos.y -= scr;
    box(VEC_XY(w->pos), 8, 8);
    w->isAlive = w->pos.y > -99;
  }
  spikeAddDist -= scr;
  if (spikeAddDist < 0) {
    play(LASER);
    int width = rndi(2, 5);
    ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, sp);
    vectorSet(&sp->pos, clamp(nextSpikeX, 12, 93 - width * 6), 199);
    sp->width = width;
    sp->isAlive = true;
    spikeIndex = wrap(spikeIndex + 1, 0, TOTOGE_MAX_SPIKE_COUNT);
    spikeAddDist = rnd(99, 199);
    nextSpikeX += nextSpikeVx * width * 6;
    if ((nextSpikeX < 9 && nextSpikeVx < 0) ||
        (nextSpikeX > 90 && nextSpikeVx > 0)) {
      nextSpikeVx *= -1;
    }
  }
  color = BLACK;
  FOR_EACH(spikes, i) {
    ASSIGN_ARRAY_ITEM(spikes, i, Spike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.y -= scr;
    char spikeChars[5];
    TIMES(s->width, k) { spikeChars[k] = 'c'; }
    spikeChars[s->width] = 0;
    character(spikeChars, s->pos.x, clamp(s->pos.y, -99, 99));
    s->isAlive = s->pos.y > -99;
  }
  color = TRANSPARENT;
  Vector cp;
  vectorSet(&cp, pos.x, pos.y);
  int iterCount = (int)clamp(vel.y / 6, 1, 99);
  TIMES(iterCount, i) {
    Collision cc = character("a", VEC_XY(cp));
    if (cc.isColliding.character['c']) {
      play(RANDOM);
      gameOver();
    }
    cp.y -= 6;
  }
  color = BLACK;
  characterOptions.isMirrorX = vel.x > 0;
  if (vel.y > 0) {
    character("a", VEC_XY(pos));
  } else {
    character("b", VEC_XY(pos));
  }
  characterOptions.isMirrorX = false;
  pos.x += vel.x * difficulty;
  pos.y += vel.y * difficulty - scr;
  if (pos.y < 10) {
    scr = (pos.y - 10) * 0.5;
  } else if (pos.y > 30) {
    scr = (pos.y - 30) * 0.5;
  }
  vel.y += 0.05;
  if ((pos.x < 9 && vel.x < 0) || (pos.x > 90 && vel.x > 0)) {
    vel.x *= -1;
  }
  if (vel.y > 0 && nextWallY < 150 && input.isJustPressed) {
    play(JUMP);
    float s = vel.y * vel.y * difficulty * difficulty;
    if (s > 10) {
      addScore(s, VEC_XY(pos));
    }
    vel.y = -1;
  }
}

void addGameTotoge() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
