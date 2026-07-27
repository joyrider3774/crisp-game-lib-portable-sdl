#include "cglp.h"

static char *title = "BOTTOP";
static char *description = "[Tap]  Jump\n[Hold] Fly";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        "l l   ",
        " l  l ",
        "l  l  ",
    },
    {
        "  ll  ",
        "  l   ",
        " lll  ",
        "  l   ",
        "  ll  ",
        " ll   ",
    },
    {
        "  ll  ",
        "l l  l",
        " llll ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "ll  ll",
        " llll ",
        "llllll",
        "llllll",
        "  ll  ",
        "  ll  ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

static float y;
static float vy;
static bool isJumping;

typedef struct {
  Vector pos;
  bool isAlive;
} Spike;
#define BOTTOP_MAX_SPIKE_COUNT 32
static Spike spikes[BOTTOP_MAX_SPIKE_COUNT];
static int spikeIndex;
static float spikeAddDist;
static float scrolling;

static void update() {
  if (!ticks) {
    y = 0;
    vy = 0;
    isJumping = false;
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    spikeAddDist = 0;
    scrolling = 1;
  }
  scrolling = difficulty;
  score += scrolling / 10;
  spikeAddDist -= scrolling;
  if (spikeAddDist < 0) {
    float sy;
    if (rnd(0, 1) < 0.33) {
      if (rnd(0, 1) < 0.5) {
        sy = 8;
      } else {
        sy = 92;
      }
    } else {
      sy = rnd(8, 92);
    }
    ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, ns);
    vectorSet(&ns->pos, 103, sy);
    ns->isAlive = true;
    spikeIndex = wrap(spikeIndex + 1, 0, BOTTOP_MAX_SPIKE_COUNT);
    spikeAddDist += rnd(30, 60);
  }
  color = RED;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  FOR_EACH(spikes, i) {
    ASSIGN_ARRAY_ITEM(spikes, i, Spike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scrolling;
    characterOptions.rotation = (int)(ticks / 10) % 4;
    character("d", s->pos.x, s->pos.y);
    if (s->pos.x <= -3) {
      s->isAlive = false;
    }
  }
  characterOptions.rotation = 0;
  if (!isJumping && input.isPressed) {
    play(POWER_UP);
    isJumping = true;
    vy = 3;
  }
  if (isJumping) {
    if (input.isPressed) {
      vy -= 0.1;
    } else {
      vy -= 0.3;
    }
    y += vy;
    if (y < 0) {
      y = 0;
      isJumping = false;
    }
  }
  color = BLACK;
  char c[2];
  if (isJumping) {
    c[0] = 'c';
  } else {
    c[0] = 'a' + ((int)(ticks / 15) % 2);
  }
  c[1] = 0;
  characterOptions.isMirrorY = true;
  Collision c1 = character(c, 9, 8 + y);
  bool hit1 = c1.isColliding.character['d'];
  bool hit2 = false;
  if (!hit1) {
    characterOptions.isMirrorY = false;
    Collision c2 = character(c, 9, 92 - y);
    hit2 = c2.isColliding.character['d'];
  }
  characterOptions.isMirrorY = false;
  if (hit1 || hit2) {
    play(EXPLOSION);
    gameOver();
  }
  color = LIGHT_BLUE;
  rect(0, 0, 99, 5);
  rect(0, 95, 99, 5);
}

void addGameBottop() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
