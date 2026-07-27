#include "cglp.h"

static char *title = "VINE CLIMBER";
static char *description = "[Hold] Climb\n[Release] Slide";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        "l    l",
    },
    {
        "  ll  ",
        "  ll  ",
        " llll ",
        "l ll l",
        " l  l ",
        " l  l ",
    },
    {
        " lll  ",
        "  l   ",
        " lll  ",
        "l lll ",
        " lll  ",
        "      ",
    },
    {
        " llll ",
        "l llll",
        "l llll",
        "l llll",
        "l llll",
        " llll ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  float centerX;
  float ticks;
  float amplitude;
  float frequency;
  float bending;
  float targetAmplitude;
  float targetFrequency;
  float targetBending;
} Vine;
static Vine vine;

typedef struct {
  Vector pos;
  float speed;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float speed;
  bool isAlive;
} Obstacle;
#define VINECLIMBER_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[VINECLIMBER_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleTicks;

typedef struct {
  Vector pos;
  float vx;
} Coin;
static Coin coin;
static bool coinExists;

static float multiplier;

// Shared helper for the upstream vine x-position formula, used both for the
// player's own x position and for drawing the vine's body.
static float vineX(float y) {
  return vine.centerX +
         pow(sin((vine.ticks + y * vine.bending) * vine.frequency / 2), 2) *
             vine.amplitude * 2 -
         vine.amplitude;
}

static void update() {
  if (!ticks) {
    vine.centerX = 50;
    vine.ticks = 0;
    vine.amplitude = 30;
    vine.frequency = 0.03;
    vine.bending = 0.5;
    vine.targetAmplitude = 30;
    vine.targetFrequency = 0.03;
    vine.targetBending = 0.5;
    vectorSet(&player.pos, 50, 80);
    player.speed = 1;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    coinExists = false;
    multiplier = 1;
  }

  int animIndex = 1;
  if (input.isPressed) {
    player.pos.y -= player.speed * 0.8 * difficulty;
    animIndex = (int)floor(ticks / 20) % 2;
    if (ticks % 20 == 0) {
      play(CLICK);
    }
  } else {
    if (input.isJustReleased) {
      play(HIT);
    }
    player.pos.y += player.speed * 1.2 * difficulty;
  }
  player.pos.y = clamp(player.pos.y, 0, 99);
  player.pos.x = vineX(player.pos.y);
  vine.ticks++;
  vine.amplitude += (vine.targetAmplitude - vine.amplitude) * 0.01;
  vine.frequency += (vine.targetFrequency - vine.frequency) * 0.01;
  vine.bending += (vine.targetBending - vine.bending) * 0.01;
  if (rnd(0, 1) < 0.005) {
    vine.targetAmplitude = rnd(25, 40);
    vine.targetFrequency = rnd(0.025, 0.04);
    vine.targetBending = rnd(0.45, 0.6);
  }

  color = GREEN;
  for (float y = 0; y <= 100; y += 5) {
    float x = vineX(y);
    box(x, y, 2, 5);
  }
  if (vine.ticks * vine.frequency >= M_PI * 2) {
    vine.ticks = 0;
  }

  color = RED;
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->speed += 0.01 * sqrt(difficulty);
    o->pos.y += o->speed;
    character("c", VEC_XY(o->pos));
    if (o->pos.y > 105) {
      o->isAlive = false;
      continue;
    }
  }
  nextObstacleTicks--;
  if (nextObstacleTicks < 0) {
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, rnd(10, 90), 0);
    no->speed = rnd(0, 0.5) * difficulty;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, VINECLIMBER_MAX_OBSTACLE_COUNT);
    nextObstacleTicks = rnd(70, 90) / difficulty;
  }

  color = BLUE;
  char playerChar[2];
  playerChar[0] = 'a' + animIndex;
  playerChar[1] = 0;
  Collision scratch = character(playerChar, VEC_XY(player.pos));
  if (scratch.isColliding.character['c']) {
    play(EXPLOSION);
    gameOver();
  }

  if (!coinExists) {
    float vx = rnd(0.18, 0.25) * RNDPM() * difficulty;
    float cx;
    if (vx > 0) {
      cx = -10;
    } else {
      cx = 110;
    }
    vectorSet(&coin.pos, cx, rnd(10, 90));
    coin.vx = vx;
    coinExists = true;
  }
  color = YELLOW;
  coin.pos.x += coin.vx;
  Collision coinCollision = character("d", VEC_XY(coin.pos));
  if (coinCollision.isColliding.character['a'] || coinCollision.isColliding.character['b']) {
    play(COIN);
    addScore(multiplier, VEC_XY(coin.pos));
    multiplier = clamp(multiplier + 1, 1, 9);
    coinExists = false;
  } else if (coin.pos.x < -10 || coin.pos.x > 110) {
    multiplier = 1;
    coinExists = false;
  }
}

void addGameVineClimber() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
