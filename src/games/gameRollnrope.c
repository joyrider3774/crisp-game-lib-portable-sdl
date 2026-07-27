#include "cglp.h"

static char *title = "ROLLNROPE";
static char *description = "[Tap] Jump / Run";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  llll",
        "  llll",
        "  llll",
        "  ll  ",
        "llllll",
        "llllll",
    },
    {
        " lll  ",
        "  lll ",
        "   ll ",
        "  ll  ",
        "  ll  ",
        " ll   ",
    },
    {
        "  lll ",
        "  l ll",
        " ll ll",
        " ll ll",
        "ll  ll",
        "ll ll ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define ROLLNROPE_MODE_STOP 0
#define ROLLNROPE_MODE_RUN 1
#define ROLLNROPE_MODE_STAND 2
#define ROLLNROPE_MODE_JUMP 3

typedef struct {
  float x;
  float angle;
  float radius;
  float size;
  float speed;
  int count;
} Rope;
static Rope rope;
static bool ropeActive;

typedef struct {
  Vector pos;
  float vy;
  int mode;
} Player;
static Player player;

static void update() {
  if (!ticks) {
    ropeActive = false;
    vectorSet(&player.pos, 5, 84);
    player.vy = 0;
    player.mode = ROLLNROPE_MODE_STOP;
  }
  if (!ropeActive) {
    rope.x = 150;
    rope.angle = rnd(0, M_PI * 2);
    rope.radius = rnd(30, 45);
    rope.size = rnd(5, 10);
    rope.speed = rnd(1, 2) * difficulty;
    if (!ticks) {
      rope.count = 2;
    } else {
      rope.count = rndi(2, 6);
    }
    if (rnd(0, 1) < 0.2) {
      rope.speed *= -1;
    }
    ropeActive = true;
  }
  rect(0, 90, 99, 9);
  rope.x -= (rope.x - 50) * 0.1;
  float pa = wrap(rope.angle - M_PI_2, -M_PI, M_PI);
  rope.angle += rope.speed * 0.05;
  float a = wrap(rope.angle - M_PI_2, -M_PI, M_PI);
  float ry = 91 - rope.size / 2 - rope.radius +
             sin(rope.angle) * rope.radius;
  box(rope.x + cos(rope.angle) * rope.radius, ry, rope.size, rope.size);
  text(intToChar(rope.count), 50, 10);
  if (pa * a < 0 && fabs(a) < M_PI_2) {
    if (player.mode == ROLLNROPE_MODE_JUMP) {
      play(POWER_UP);
      rope.count--;
      float s = 85 - rope.size - player.pos.y;
      if (s < 0) {
        s = 1;
      }
      s = floor(100 / s);
      addScore(s, VEC_XY(player.pos));
      if (rope.count == 0) {
        play(COIN);
        ropeActive = false;
        player.mode = ROLLNROPE_MODE_STOP;
      }
    } else {
      play(HIT);
    }
  }
  if (player.mode == ROLLNROPE_MODE_STOP) {
    player.pos.x += (5 - player.pos.x) * 0.1;
    if (player.pos.y == 84 && input.isJustPressed) {
      play(SELECT);
      player.mode = ROLLNROPE_MODE_RUN;
    }
  }
  if (player.mode == ROLLNROPE_MODE_RUN) {
    player.pos.x += difficulty * 2;
    if (player.pos.x > 50) {
      player.pos.x = 50;
      player.mode = ROLLNROPE_MODE_STAND;
    }
  }
  if (player.mode == ROLLNROPE_MODE_STAND) {
    if (input.isPressed) {
      play(JUMP);
      player.vy = -3;
      player.mode = ROLLNROPE_MODE_JUMP;
    }
  }
  if (player.mode == ROLLNROPE_MODE_JUMP ||
      player.mode == ROLLNROPE_MODE_STOP) {
    float dvy;
    if (input.isPressed) {
      dvy = 0.1;
    } else {
      dvy = 0.2;
    }
    player.vy += dvy * sqrt(difficulty);
    player.pos.y += player.vy;
    if (player.pos.y > 84) {
      player.pos.y = 84;
      if (player.mode == ROLLNROPE_MODE_JUMP) {
        player.mode = ROLLNROPE_MODE_STAND;
      }
    }
  }
  Collision c1 = character("a", player.pos.x, player.pos.y - 3);
  char legChar[2];
  if (player.mode == ROLLNROPE_MODE_JUMP ||
      player.mode == ROLLNROPE_MODE_RUN) {
    legChar[0] = 'c';
  } else {
    legChar[0] = 'b';
  }
  legChar[1] = 0;
  Collision c2 = character(legChar, player.pos.x, player.pos.y + 3);
  if (c1.isColliding.rect[BLACK] || c2.isColliding.rect[BLACK]) {
    play(EXPLOSION);
    gameOver();
  }
}

void addGameRollnrope() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
