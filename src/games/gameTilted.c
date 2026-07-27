#include "cglp.h"

static char *title = "TILTED";
static char *description = "[Tap]\n Jump\n Jump & Turn";

// Index 4 is a genuine gap in the upstream character array (elided, never
// drawn) - kept blank here so index 5 ("e") still lines up correctly.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  lll ",
        "  lll ",
        " lll  ",
        "  l   ",
        " l ll ",
        "l    l",
    },
    {
        "  lll ",
        "  lll ",
        "llllll",
        "  l   ",
        " l l  ",
        " l l  ",
    },
    {
        "  lll ",
        "l lll ",
        " ll   ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        " llll ",
        "l llll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        " llll ",
        "l llll",
        "llllll",
        "llllll",
        "llllll",
        " llll ",
    },
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3000, .isDarkColor = false};

typedef struct {
  Vector pos;
  float angle;
  float length;
  bool isAlive;
} Bar;
#define TILTED_MAX_BAR_COUNT 128
static Bar bars[TILTED_MAX_BAR_COUNT];
static int barIndex;
static float nextBarDist;
static int barCount;

typedef struct {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
} Ball;
// Spawn interval shrinks as ~1/difficulty (rnd(60,80)/difficulty) while
// each ball's accelerating fall (vel.y ramps up every tick) only shrinks
// its lifetime roughly as difficulty^-0.25, so concurrent balls grow like
// difficulty^0.75 - already exceeds 32 within ~1-2 hours of continuous
// play, so sized with real headroom.
#define TILTED_MAX_BALL_COUNT 512
static Ball balls[TILTED_MAX_BALL_COUNT];
static int ballIndex;
static float nextBallTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool hasBar;
  int barIndex;
  int jumpCount;
} Player;
static Player player;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(bars);
    barIndex = 0;
    ASSIGN_ARRAY_ITEM(bars, barIndex, Bar, initBar);
    vectorSet(&initBar->pos, 50, 0);
    initBar->angle = 0;
    initBar->length = 90;
    initBar->isAlive = true;
    int initialBarIndex = barIndex;
    barIndex = wrap(barIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    nextBarDist = 10;
    barCount = 0;
    INIT_UNALIVED_ARRAY(balls);
    ballIndex = 0;
    nextBallTicks = 60;
    vectorSet(&player.pos, 50, -6);
    vectorSet(&player.vel, 1, 0);
    player.hasBar = true;
    player.barIndex = initialBarIndex;
    player.jumpCount = 0;
  }
  float scr = difficulty * 0.02;
  if (player.pos.y < 60) {
    scr += (60 - player.pos.y) * 0.1;
  }
  addScore(scr, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  char pc[2];
  if (player.hasBar) {
    Bar *b = &bars[player.barIndex];
    Vector tmp;
    vectorSet(&tmp, player.pos.x, player.pos.y + 5);
    float d = distanceTo(&tmp, b->pos.x, b->pos.y);
    if (player.pos.x < b->pos.x) {
      d = -d;
    }
    d += (player.vel.x / 2) * sqrt(difficulty);
    if (fabs(d) > b->length / 2 + 6) {
      player.hasBar = false;
      player.jumpCount = 0;
    }
    Vector newPos;
    vectorSet(&newPos, b->pos.x, b->pos.y);
    addWithAngle(&newPos, b->angle, d);
    vectorAdd(&newPos, 0, -5);
    player.pos = newPos;
    if (input.isJustPressed) {
      play(JUMP);
      player.hasBar = false;
      player.vel.y = -2;
      player.pos.y -= 2 * sqrt(difficulty);
      player.jumpCount = 1;
    }
    pc[0] = 'a' + (ticks / 15) % 2;
    pc[1] = 0;
  } else {
    if (input.isJustPressed) {
      play(JUMP);
      if (player.jumpCount > 0) {
        player.vel.x *= -1;
      } else {
        player.jumpCount = 1;
      }
      player.vel.y = -2.0 / player.jumpCount / player.jumpCount;
      player.jumpCount++;
    }
    player.pos.y += scr;
    float dvy;
    if (input.isPressed) {
      dvy = 0.05;
    } else {
      dvy = 0.1;
    }
    player.vel.y += dvy;
    vectorAdd(&player.pos, player.vel.x * sqrt(difficulty) / 2,
              player.vel.y * sqrt(difficulty) / 2);
    pc[0] = 'c';
    pc[1] = 0;
  }
  if ((player.pos.x < 8 && player.vel.x < 0) ||
      (player.pos.x > 92 && player.vel.x > 0)) {
    player.vel.x *= -1;
  }
  if (player.pos.y > 99) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  characterOptions.isMirrorX = player.vel.x <= 0;
  character(pc, VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  nextBallTicks--;
  if (nextBallTicks < 0) {
    ASSIGN_ARRAY_ITEM(balls, ballIndex, Ball, nball);
    vectorSet(&nball->pos, rnd(20, 80), -40);
    vectorSet(&nball->vel, rnd(0.5, 1) * RNDPM(), 0);
    nball->ticks = 0;
    nball->isAlive = true;
    ballIndex = wrap(ballIndex + 1, 0, TILTED_MAX_BALL_COUNT);
    nextBallTicks += rnd(60, 80) / difficulty;
  }
  FOR_EACH(balls, bli2) {
    ASSIGN_ARRAY_ITEM(balls, bli2, Ball, bl2);
    SKIP_IS_NOT_ALIVE(bl2);
    bl2->pos.y += scr;
    bl2->vel.y += 0.05;
    vectorMul(&bl2->vel, 0.995);
    vectorAdd(&bl2->pos, bl2->vel.x * sqrt(difficulty) / 2, bl2->vel.y * sqrt(difficulty) / 2);
    if ((bl2->pos.x < 8 && bl2->vel.x < 0) || (bl2->pos.x > 92 && bl2->vel.x > 0)) {
      bl2->vel.x *= -1;
    }
    color = YELLOW;
    Collision bc = character("d", VEC_XY(bl2->pos));
    if (bc.isColliding.character['a'] || bc.isColliding.character['b'] ||
        bc.isColliding.character['c']) {
      play(HIT);
      if (player.hasBar) {
        bars[player.barIndex].isAlive = false;
        player.hasBar = false;
        player.vel.y += 1;
      } else {
        player.vel.y += 4;
      }
      particle(bl2->pos.x, bl2->pos.y, 20, 3, M_PI / 2, M_PI / 3);
      bl2->isAlive = false;
      continue;
    }
    if (bc.isColliding.character['d']) {
      bl2->ticks = 999;
      character("e", VEC_XY(bl2->pos));
    }
    bl2->ticks++;
    if (bl2->ticks > 500) {
      particle(bl2->pos.x, bl2->pos.y, 16, 1, 0, M_PI * 2);
      bl2->isAlive = false;
      continue;
    }
    if (bl2->pos.y > 103) {
      bl2->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(balls, bli3) {
    ASSIGN_ARRAY_ITEM(balls, bli3, Ball, bl3);
    SKIP_IS_NOT_ALIVE(bl3);
    Collision ec = character("d", VEC_XY(bl3->pos));
    if (ec.isColliding.character['e']) {
      bl3->ticks = 999;
    }
  }
  color = LIGHT_BLACK;
  rect(0, 0, 5, 100);
  rect(95, 0, 5, 100);
  FOR_EACH(bars, bai) {
    ASSIGN_ARRAY_ITEM(bars, bai, Bar, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += scr;
    thickness = 3;
    Collision bc2 = bar(b->pos.x, b->pos.y, b->length, b->angle);
    if (!player.hasBar && (bc2.isColliding.character['a'] || bc2.isColliding.character['b'] ||
                            bc2.isColliding.character['c'])) {
      play(LASER);
      if (player.vel.y <= 0) {
        player.vel.y *= -0.5;
        player.pos.y += player.vel.y * 3 * difficulty;
      } else {
        player.hasBar = true;
        player.barIndex = bai;
      }
    }
    if (bc2.isColliding.character['d']) {
      bool hasHb = false;
      int hbIndex = -1;
      float dHb = 99;
      FOR_EACH(balls, blj) {
        ASSIGN_ARRAY_ITEM(balls, blj, Ball, bl4);
        SKIP_IS_NOT_ALIVE(bl4);
        if (!hasHb) {
          hasHb = true;
          hbIndex = blj;
        }
        float bd = distanceTo(&bl4->pos, b->pos.x, b->pos.y);
        if (bd < dHb) {
          dHb = bd;
          hbIndex = blj;
        }
      }
      if (hasHb) {
        Ball *hb = &balls[hbIndex];
        float dd = distanceTo(&hb->pos, b->pos.x, b->pos.y);
        if (dd > b->length / 2) {
          vectorMul(&hb->vel, -1);
        } else {
          float a = vectorAngle(&hb->vel) - (vectorAngle(&hb->vel) - b->angle) * 2;
          float s = vectorLength(&hb->vel);
          vectorSet(&hb->vel, 0, 0);
          addWithAngle(&hb->vel, a, s);
        }
        vectorAdd(&hb->pos, hb->vel.x * sqrt(difficulty), hb->vel.y * sqrt(difficulty));
        hb->ticks += 30;
      }
    }
    if (b->pos.y > 120) {
      b->isAlive = false;
      continue;
    }
  }
  nextBarDist -= scr;
  while (nextBarDist < 0) {
    bool isSide = barCount % 2;
    float length = rnd(15, 22);
    float x;
    if (isSide) {
      x = 50 + rnd(25, 50 - length / 2) * RNDPM();
    } else {
      x = rnd(35, 65);
    }
    float angleThreshold;
    if (isSide) {
      angleThreshold = 0.2;
    } else {
      angleThreshold = 0.7;
    }
    float bAngle;
    if (rnd(0, 1) < angleThreshold) {
      bAngle = 0;
    } else {
      bAngle = rnd(0, M_PI / 5) * RNDPM();
    }
    ASSIGN_ARRAY_ITEM(bars, barIndex, Bar, newB);
    vectorSet(&newB->pos, x, -9 - nextBarDist);
    newB->angle = bAngle;
    newB->length = length;
    newB->isAlive = true;
    float newBY = newB->pos.y;
    barIndex = wrap(barIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    if (isSide) {
      ASSIGN_ARRAY_ITEM(bars, barIndex, Bar, newB2);
      vectorSet(&newB2->pos, 100 - x, newBY);
      newB2->angle = -bAngle;
      newB2->length = length;
      newB2->isAlive = true;
      barIndex = wrap(barIndex + 1, 0, TILTED_MAX_BAR_COUNT);
    }
    nextBarDist += rnd(15, 20);
    barCount++;
  }
}

void addGameTilted() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
