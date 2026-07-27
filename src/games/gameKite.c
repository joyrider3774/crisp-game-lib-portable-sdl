#include "cglp.h"

static char *title = "KITE";
static char *description = "[Hold] Blow wind";

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
        "     l",
        "lllll ",
        "      ",
        "llll  ",
        "      ",
        "llllll",
    },
    {
        " yyyy ",
        "yY YYy",
        "yY YYy",
        "yY YYy",
        "yY YYy",
        " yyyy ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define KITE_STRING_DIST 50

typedef struct {
  Vector pos;
  Vector vel;
} KiteEntity;
static KiteEntity kite;

typedef struct {
  Vector pos;
  Vector vel;
  float ticks;
} KitePlayer;
static KitePlayer player;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} KiteWind;
#define KITE_MAX_WIND_COUNT 32
static KiteWind winds[KITE_MAX_WIND_COUNT];
static int windIndex;
static float nextWindTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} KiteCoin;
#define KITE_MAX_COIN_COUNT 32
static KiteCoin coins[KITE_MAX_COIN_COUNT];
static int coinIndex;
static float nextCoinDist;

static float groundX;

typedef struct {
  Vector pos;
  float height;
  bool isAlive;
} KiteSpike;
#define KITE_MAX_SPIKE_COUNT 64
static KiteSpike spikes[KITE_MAX_SPIKE_COUNT];
static int spikeIndex;

typedef struct {
  float yAngle;
  float hAngle;
} KiteSpikeAngle;
#define KITE_SPIKE_ANGLE_COUNT 2
static KiteSpikeAngle spikeAngles[KITE_SPIKE_ANGLE_COUNT];
static float nextSpikeDist;

static void update() {
  if (!ticks) {
    vectorSet(&kite.pos, 40, 50);
    vectorSet(&kite.vel, 0, 0);
    vectorSet(&player.pos, 20, 87);
    vectorSet(&player.vel, 0, 0);
    player.ticks = 0;
    groundX = 0;
    INIT_UNALIVED_ARRAY(winds);
    windIndex = 0;
    nextWindTicks = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    nextCoinDist = 0;
    INIT_UNALIVED_ARRAY(spikes);
    spikeIndex = 0;
    TIMES(2, i) {
      spikeAngles[i].yAngle = i * M_PI / 2;
      spikeAngles[i].hAngle = i * M_PI;
    }
    nextSpikeDist = 0;
  }
  float sd = sqrt(difficulty);
  float scr;
  if (kite.pos.x > 60) {
    scr = (kite.pos.x - 60) * 0.1;
  } else {
    scr = 0;
  }
  nextSpikeDist -= scr;
  if (nextSpikeDist < 0) {
    addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    TIMES(2, i) {
      KiteSpikeAngle *a = &spikeAngles[i];
      float height = sin(a->yAngle) * 9 + sin(a->hAngle) * 20 - 10;
      a->yAngle += rnd(0, sd) * 0.2;
      a->hAngle += rnd(0, sd) * 0.4;
      if (height > 0) {
        ASSIGN_ARRAY_ITEM(spikes, spikeIndex, KiteSpike, ns);
        float spikeYSign;
        if (i == 0) {
          spikeYSign = 1;
        } else {
          spikeYSign = -1;
        }
        float py;
        if (i == 0) {
          py = 0;
        } else {
          py = 90;
        }
        vectorSet(&ns->pos, 205, py);
        ns->height = height * spikeYSign;
        ns->isAlive = true;
        spikeIndex = wrap(spikeIndex + 1, 0, KITE_MAX_SPIKE_COUNT);
      }
    }
    nextSpikeDist += 10;
  }
  color = RED;
  FOR_EACH(spikes, i) {
    ASSIGN_ARRAY_ITEM(spikes, i, KiteSpike, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.x -= scr;
    line(s->pos.x - 5, s->pos.y, s->pos.x, s->pos.y + s->height);
    line(s->pos.x + 5, s->pos.y, s->pos.x, s->pos.y + s->height);
    if (s->pos.x < -5) {
      s->isAlive = false;
      continue;
    }
  }
  if (input.isJustPressed) {
    play(SELECT);
  }
  if (input.isPressed) {
    vectorAdd(&kite.vel, difficulty * 0.2, -difficulty * 0.2);
  }
  kite.vel.y += difficulty * 0.01;
  vectorMul(&kite.vel, 0.95);
  vectorAdd(&kite.pos, kite.vel.x, kite.vel.y);
  kite.pos.x -= scr;
  if (kite.pos.y < 0) {
    kite.pos.y = 0;
    kite.vel.y = 0;
  } else if (kite.pos.y > 87) {
    kite.pos.y = 87;
    kite.vel.y = 0;
  }
  color = BLUE;
  Collision kc = box(VEC_XY(kite.pos), 6, 6);
  if (kc.isColliding.rect[RED]) {
    float s;
    if (kite.pos.y < 50) {
      s = 1;
    } else {
      s = -1;
    }
    play(HIT);
    color = TRANSPARENT;
    int c = 0;
    Collision kc2 = box(VEC_XY(kite.pos), 6, 6);
    while (kc2.isColliding.rect[RED] && c < 9) {
      kite.pos.y += 3 * s;
      c++;
      kc2 = box(VEC_XY(kite.pos), 6, 6);
    }
    kite.vel.y = sqrt(c) * s * 2 * sd;
  }
  color = LIGHT_BLACK;
  Vector p;
  vectorSet(&p, kite.pos.x - 3, kite.pos.y + 3);
  thickness = 2;
  line(p.x, p.y, p.x - kite.vel.x * 3, p.y + kite.vel.y + 9);
  vectorSet(&p, kite.pos.x + 3, kite.pos.y + 3);
  thickness = 2;
  line(p.x, p.y, p.x - kite.vel.x * 3, p.y + kite.vel.y + 9);
  thickness = 2;
  line(kite.pos.x, kite.pos.y, player.pos.x, player.pos.y);
  vectorAdd(&player.pos, player.vel.x, player.vel.y);
  player.pos.x -= scr;
  if (player.pos.y < 87) {
    player.vel.y += sd * sqrt(99 - player.pos.y) * 0.01;
    player.vel.x *= 0.95;
    player.ticks = 0;
  } else {
    player.vel.y = 0;
    player.pos.y = 87;
    player.vel.x *= 0.9;
    player.ticks++;
  }
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (int)floor(player.ticks / 15) % 2;
  pc[1] = 0;
  Collision pcColl = character(pc, VEC_XY(player.pos));
  if (pcColl.isColliding.rect[RED]) {
    play(EXPLOSION);
    gameOver();
  }
  float d = distanceTo(&player.pos, VEC_XY(kite.pos));
  if (d > KITE_STRING_DIST) {
    float a = angleTo(&player.pos, VEC_XY(kite.pos));
    addWithAngle(&player.vel, a, (d - KITE_STRING_DIST) * 0.05);
    addWithAngle(&kite.vel, a + M_PI, (d - KITE_STRING_DIST) * 0.01);
    addWithAngle(&kite.pos, a + M_PI, d - KITE_STRING_DIST);
  }
  if (input.isPressed) {
    nextWindTicks -= 3;
  } else {
    nextWindTicks -= 1;
  }
  while (nextWindTicks < 0) {
    ASSIGN_ARRAY_ITEM(winds, windIndex, KiteWind, nw);
    vectorSet(&nw->pos, -3, rnd(0, 87));
    float windSpeedMul;
    if (input.isPressed) {
      windSpeedMul = 2;
    } else {
      windSpeedMul = 1;
    }
    vectorSet(&nw->vel, rnd(1, 2) * sd * windSpeedMul, 0);
    nw->isAlive = true;
    windIndex = wrap(windIndex + 1, 0, KITE_MAX_WIND_COUNT);
    nextWindTicks += 30 / sd;
  }
  color = LIGHT_CYAN;
  FOR_EACH(winds, i) {
    ASSIGN_ARRAY_ITEM(winds, i, KiteWind, w);
    SKIP_IS_NOT_ALIVE(w);
    vectorAdd(&w->pos, w->vel.x, w->vel.y);
    Collision wc = character("c", VEC_XY(w->pos));
    if (wc.isColliding.rect[BLUE]) {
      play(HIT);
      vectorAdd(&kite.vel, w->vel.x, w->vel.y);
      w->isAlive = false;
      continue;
    }
    if (w->pos.x > 203) {
      w->isAlive = false;
      continue;
    }
  }
  nextCoinDist -= scr;
  if (nextCoinDist < 0) {
    ASSIGN_ARRAY_ITEM(coins, coinIndex, KiteCoin, nc);
    vectorSet(&nc->pos, 203, rnd(30, 80));
    vectorSet(&nc->vel, rnd(1, sd) * -0.5, 0);
    nc->isAlive = true;
    coinIndex = wrap(coinIndex + 1, 0, KITE_MAX_COIN_COUNT);
    nextCoinDist += rnd(199, 299);
  }
  color = BLACK;
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, KiteCoin, c);
    SKIP_IS_NOT_ALIVE(c);
    vectorAdd(&c->pos, c->vel.x, c->vel.y);
    c->pos.x -= scr;
    Collision cl = character("d", VEC_XY(c->pos));
    if (cl.isColliding.rect[RED]) {
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.character['a'] || cl.isColliding.character['b']) {
      play(COIN);
      addScore(clamp(ceil(player.vel.x * 5), 1, 99) * 50, VEC_XY(c->pos));
      c->isAlive = false;
      continue;
    }
    if (cl.isColliding.character['c']) {
      c->pos.x++;
    }
    if (distanceTo(&c->pos, VEC_XY(player.pos)) < 24) {
      addWithAngle(&c->vel, angleTo(&c->pos, VEC_XY(player.pos)), sd);
      vectorMul(&c->vel, 0.9);
    }
    bool inRect = c->pos.x >= -3 && c->pos.x < 207 && c->pos.y >= -3 && c->pos.y < 90;
    if (!inRect) {
      c->isAlive = false;
      continue;
    }
  }
  groundX = wrap(groundX - scr, -9, 209);
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10);
  color = WHITE;
  rect(groundX, 90, 2, 10);
}

void addGameKite() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
