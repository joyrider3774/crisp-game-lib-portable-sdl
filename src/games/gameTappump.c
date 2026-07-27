#include "cglp.h"

static char *title = "TAPPUMP";
static char *description = "[Tap]  Jump\n[Hold] Pump";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "r r r ",
        " r r  ",
        "rrRrr ",
        " r r  ",
        "r r r ",
            "      ",
},
    {
        " yyyy ",
        "y yyYy",
        "y yyYy",
        "y yyYy",
        "y yyYy",
        " yyyy ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector vel;
  float radius;
  float rv;
} Player;
static Player player;

typedef struct {
  Vector pos;
  bool isAlive;
} Spike;
#define TAPPUMP_MAX_SPIKE_COUNT 32
static Spike spikes[TAPPUMP_MAX_SPIKE_COUNT];
static int spikeIndex;
static float nextSpikeDist;

typedef struct {
  Vector pos;
  bool isAlive;
} Coin;
#define TAPPUMP_MAX_COIN_COUNT 32
static Coin coins[TAPPUMP_MAX_COIN_COUNT];
static int coinIndex;
static float coinY;
static float coinVy;
static float nextCoinDist;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 10, 20);
    vectorSet(&player.vel, 0, 0);
    player.radius = 1;
    player.rv = 0;
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    coinY = 50;
    coinVy = 0;
    nextSpikeDist = 40;
    nextCoinDist = 0;
  }
  float scr;
  if (player.pos.x > 20) {
    scr = (player.pos.x - 20) * 0.2;
  } else {
    scr = 0;
  }
  nextSpikeDist -= scr;
  if (nextSpikeDist < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(spikes, spikeIndex, Spike, sp);
    vectorSet(&sp->pos, 103, rnd(0, 99));
    sp->isAlive = true;
    spikeIndex = wrap(spikeIndex + 1, 0, TAPPUMP_MAX_SPIKE_COUNT);
    nextSpikeDist += rnd(40, 140);
  }
  color = BLACK;
  FOR_EACH(spikes, i) {
    ASSIGN_ARRAY_ITEM(spikes, i, Spike, sp);
    SKIP_IS_NOT_ALIVE(sp);
    sp->pos.x -= scr;
    character("a", VEC_XY(sp->pos));
    sp->isAlive = sp->pos.x >= -2;
  }
  color = GREEN;
  player.vel.x = difficulty;
  if (input.isJustPressed) {
    play(SELECT);
    player.vel.y -= sqrt(difficulty) * 2;
  }
  if (input.isPressed) {
    player.vel.y -= sqrt(difficulty) * 0.03;
  } else {
    player.vel.y -= sqrt(difficulty) * -0.12;
  }
  if (input.isPressed) {
    player.rv += difficulty * 0.08;
    player.radius += player.rv;
  } else {
    player.radius += (1 - player.radius) * 0.04 * difficulty;
    player.rv = 0;
  }
  vectorAdd(&player.pos, player.vel.x, player.vel.y);
  player.pos.x -= scr;
  thickness = 5;
  Collision ac = arc(player.pos.x, player.pos.y, player.radius, 0, M_PI * 2);
  if (ac.isColliding.character['a'] ||
      player.pos.y < -5 - player.radius ||
      player.pos.y > 105 + player.radius) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  nextCoinDist -= scr;
  coinVy += rnd(0, 0.1) * RNDPM();
  coinVy *= 0.98;
  coinY += coinVy;
  if ((coinY < 10 && coinVy < 0) ||
      (coinY > 90 && coinVy > 0)) {
    coinVy *= -1;
  }
  if (nextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, co);
    vectorSet(&co->pos, 103, coinY + rnd(0, 9) * RNDPM());
    co->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, TAPPUMP_MAX_COIN_COUNT);
    nextCoinDist += rnd(6, 9);
  }
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, Coin, co);
    SKIP_IS_NOT_ALIVE(co);
    co->pos.x -= scr;
    Collision cl = character("b", VEC_XY(co->pos));
    if (cl.isColliding.rect[GREEN]) {
      int sc = ceil(player.radius);
      if (sc < 20) {
        play(COIN);
      } else {
        play(POWER_UP);
      }
      addScore(sc, VEC_XY(co->pos));
      co->isAlive = false;
      continue;
    }
    co->isAlive = !(cl.isColliding.character['a'] || co->pos.x < -3);
  }
}

void addGameTappump() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
