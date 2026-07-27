#include "cglp.h"

static char *title = "MIRROR FLOOR";
static char *description = "[Tap]  Jump\n[Hold] Speed up";

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
        " llll ",
        "ll lll",
        "ll lll",
        "ll lll",
        "ll lll",
        " llll ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = true};

#define MIRRORFLOOR_STATE_RUN 0
#define MIRRORFLOOR_STATE_JUMP 1
#define MIRRORFLOOR_PLAYER_X 9

typedef struct {
  Vector pos;
  float width;
  bool isAlive;
} MirrorfloorFloor;
#define MIRRORFLOOR_MAX_FLOOR_COUNT 32
static MirrorfloorFloor floors[MIRRORFLOOR_MAX_FLOOR_COUNT];
static int floorIndex;
static float nextFloorDist;

typedef struct {
  Vector pos;
  bool isAlive;
} MirrorfloorCoin;
#define MIRRORFLOOR_MAX_COIN_COUNT 64
static MirrorfloorCoin coins[MIRRORFLOOR_MAX_COIN_COUNT];
static int coinIndex;

typedef struct {
  float y;
  float my;
  float vy;
  float speed;
  int side;
  int state;
} MirrorfloorPlayer;
static MirrorfloorPlayer player;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
    ASSIGN_ARRAY_ITEM(floors, floorIndex, MirrorfloorFloor, f0);
    vectorSet(&f0->pos, 10, 50);
    f0->width = 80;
    f0->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MIRRORFLOOR_MAX_FLOOR_COUNT);
    nextFloorDist = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    ASSIGN_ARRAY_ITEM(coins, coinIndex, MirrorfloorCoin, c0);
    vectorSet(&c0->pos, 60, 47);
    c0->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, MIRRORFLOOR_MAX_COIN_COUNT);
    player.y = 10;
    player.my = 0;
    player.vy = 0;
    player.speed = 1;
    player.side = 1;
    player.state = MIRRORFLOOR_STATE_JUMP;
    multiplier = 1;
  }
  float scr = difficulty * 0.5 * player.speed;
  if (player.state == MIRRORFLOOR_STATE_RUN) {
    if (input.isJustPressed) {
      play(POWER_UP);
      player.vy = -1.5 * sqrt(difficulty) * player.side;
      player.state = MIRRORFLOOR_STATE_JUMP;
    }
  }
  if (player.state == MIRRORFLOOR_STATE_JUMP) {
    player.vy += 0.07 * difficulty * player.side;
    player.y += player.vy;
  }
  if (input.isPressed) {
    player.speed += (2 - player.speed) * 0.05;
  } else {
    player.speed += (1 - player.speed) * 0.2;
  }
  if ((player.y < 0 && player.side == -1) ||
      (player.y > 99 && player.side == 1)) {
    play(RANDOM);
    gameOver();
  }
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  characterOptions.isMirrorY = player.side == -1;
  character(pc, MIRRORFLOOR_PLAYER_X, player.y);
  characterOptions.isMirrorY = false;
  nextFloorDist -= scr;
  if (nextFloorDist < 0) {
    ASSIGN_ARRAY_ITEM(floors, floorIndex, MirrorfloorFloor, nf);
    vectorSet(&nf->pos, 100, rnd(10, 90));
    nf->width = rnd(45, 75);
    nf->isAlive = true;
    floorIndex = wrap(floorIndex + 1, 0, MIRRORFLOOR_MAX_FLOOR_COUNT);
    float cx = rnd(20, 25);
    while (cx < nf->width - 20) {
      ASSIGN_ARRAY_ITEM(coins, coinIndex, MirrorfloorCoin, nc);
      vectorSet(&nc->pos, 100 + cx, nf->pos.y - 3);
      nc->isAlive = true;
      coinIndex = wrap(coinIndex + 1, 0, MIRRORFLOOR_MAX_COIN_COUNT);
      cx += rnd(15, 30);
    }
    nextFloorDist += nf->width + rnd(10, 20);
  }
  bool isOnFloor = false;
  FOR_EACH(floors, i) {
    ASSIGN_ARRAY_ITEM(floors, i, MirrorfloorFloor, f);
    SKIP_IS_NOT_ALIVE(f);
    f->pos.x -= scr;
    if (player.side == 1) {
      color = CYAN;
    } else {
      color = LIGHT_CYAN;
    }
    Collision c1 = rect(f->pos.x, f->pos.y, f->width, 1);
    if (player.side == -1) {
      color = CYAN;
    } else {
      color = LIGHT_CYAN;
    }
    Collision c2 = rect(f->pos.x, f->pos.y + 1, f->width, 1);
    if ((c1.isColliding.character['a'] || c1.isColliding.character['b'] ||
         c2.isColliding.character['a'] || c2.isColliding.character['b']) &&
        player.vy * player.side > 0) {
      play(LASER);
      player.state = MIRRORFLOOR_STATE_RUN;
      float ofs;
      if (player.side == 1) {
        ofs = -3;
      } else {
        ofs = 5;
      }
      player.y = f->pos.y + ofs;
    }
    if (f->pos.x - 3 < MIRRORFLOOR_PLAYER_X &&
        MIRRORFLOOR_PLAYER_X < f->pos.x + f->width + 3) {
      player.my = f->pos.y - (player.y - f->pos.y) + 2;
      color = LIGHT_BLACK;
      char pc2[2];
      pc2[0] = 'a' + (int)floor(ticks / 15) % 2;
      pc2[1] = 0;
      characterOptions.isMirrorY = player.side == 1;
      character(pc2, MIRRORFLOOR_PLAYER_X, player.my);
      characterOptions.isMirrorY = false;
      isOnFloor = true;
    }
    f->isAlive = f->pos.x >= -f->width;
  }
  if (!isOnFloor) {
    player.state = MIRRORFLOOR_STATE_JUMP;
  }
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, MirrorfloorCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.x -= scr;
    if (player.side == 1) {
      color = YELLOW;
    } else {
      color = LIGHT_YELLOW;
    }
    Collision cl = character("c", VEC_XY(c->pos));
    if (player.side == 1) {
      color = LIGHT_YELLOW;
    } else {
      color = YELLOW;
    }
    character("c", c->pos.x, c->pos.y + 8);
    if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
      play(COIN);
      addScore(multiplier, VEC_XY(c->pos));
      multiplier++;
      player.side *= -1;
      player.vy *= -1;
      player.y = player.my;
      c->isAlive = false;
      continue;
    }
    if (c->pos.x < -3) {
      if (multiplier > 1) {
        multiplier--;
      }
      c->isAlive = false;
      continue;
    }
  }
}

void addGameMirrorFloor() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
