#include "cglp.h"
#include "string.h"

static char *title = "BLAST BUOY";
static char *description = "[Hold] Sink\n[Release] Float";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 12, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
  float vx;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float dir;
  float spd;
  float fireT;
  bool isAlive;
} Enemy;
// Concurrent count stays roughly constant across difficulty (spawn interval
// and enemy speed both scale with sqrt(difficulty)), estimated around 5
// typical / ~12 worst-case - sized generously above that.
#define BLASTBUOY_MAX_ENEMY_COUNT 64
static Enemy enemies[BLASTBUOY_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  float vx;
  bool isAlive;
} Bullet;
#define BLASTBUOY_MAX_BULLET_COUNT 48
static Bullet bullets[BLASTBUOY_MAX_BULLET_COUNT];
static int bulletIndex;

static int combo;
static int comboT;
static int iframes;
static float nextSubTicks;

typedef struct {
  Vector pos;
  int t;
  int color;
} BlastFx;
static BlastFx blastFx;

static int hitstop;
static float shakeMag;
static int shakeT;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 30, 40);
    player.vy = 0;
    player.vx = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    combo = 1;
    comboT = 0;
    iframes = 0;
    nextSubTicks = 0;
    vectorSet(&blastFx.pos, 0, 0);
    blastFx.t = 0;
    blastFx.color = LIGHT_RED;
    hitstop = 0;
    shakeMag = 0;
    shakeT = 0;
  }

  bool frozen = hitstop > 0;
  if (frozen) {
    hitstop--;
  }

  if (!frozen) {
    if (input.isPressed) {
      player.vy += 0.06;
    } else {
      player.vy -= 0.05;
    }
    player.vy = clamp(player.vy, -1.1, 1.1);
    player.vx *= 0.95;
    player.pos.y += player.vy;
    player.pos.x += player.vx;
    player.pos.x = clamp(player.pos.x, 5, 95);
    player.pos.y = clamp(player.pos.y, 13, 87);
    if (player.pos.y <= 13 || player.pos.y >= 87) {
      player.vy = 0;
    }

    if (iframes > 0) {
      iframes--;
    }
    if (comboT > 0) {
      comboT--;
    } else {
      combo = 1;
    }

    nextSubTicks--;
    if (nextSubTicks < 0) {
      float dir;
      if (rnd(0, 1) < 0.5) {
        dir = 1;
      } else {
        dir = -1;
      }
      float ex;
      if (dir > 0) {
        ex = -8;
      } else {
        ex = 108;
      }
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      vectorSet(&ne->pos, ex, rnd(13, 87));
      ne->dir = dir;
      ne->spd = rnd(0.2, 0.45) * sqrt(difficulty);
      ne->fireT = rnd(60, 120);
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, BLASTBUOY_MAX_ENEMY_COUNT);
      nextSubTicks = rnd(50, 90) / sqrt(difficulty);
    }
  }

  float shakeY = 0;
  if (shakeT > 0) {
    shakeT--;
    float m = shakeMag * (shakeT / 8.0);
    shakeY = rnd(-m, m);
  }

  color = LIGHT_BLUE;
  rect(0, 8 + shakeY, 100, 1);
  color = LIGHT_BLACK;
  rect(0, 93 + shakeY, 100, 7);

  if (blastFx.t > 0) {
    blastFx.t--;
    color = blastFx.color;
    thickness = 2;
    arc(blastFx.pos.x, blastFx.pos.y + shakeY, 25 * (1 - blastFx.t / 14.0), 0,
        M_PI * 2);
  }

  if (iframes % 8 >= 4) {
    color = BLUE;
  } else {
    color = CYAN;
  }
  box(player.pos.x, player.pos.y, 6, 6);

  bool blastAtSet = false;
  Vector blastAt;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!frozen) {
      e->pos.x += e->dir * e->spd;
      e->fireT--;
      if (e->fireT < 0 && fabs(e->pos.y - player.pos.y) < 10 && e->pos.x > 0 &&
          e->pos.x < 100) {
        if (distanceTo(&e->pos, player.pos.x, player.pos.y) > 36) {
          float d;
          if (player.pos.x > e->pos.x) {
            d = 1;
          } else {
            d = -1;
          }
          ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
          vectorSet(&nb->pos, e->pos.x + d * 6, e->pos.y);
          nb->vx = d * (0.8 + difficulty * 0.15);
          nb->isAlive = true;
          bulletIndex = wrap(bulletIndex + 1, 0, BLASTBUOY_MAX_BULLET_COUNT);
          play(LASER);
        }
        e->fireT = rnd(90, 150);
      }
    }
    color = GREEN;
    Collision ec = box(e->pos.x, e->pos.y, 9, 4);
    if (!blastAtSet && ec.isColliding.rect[CYAN]) {
      vectorSet(&blastAt, e->pos.x, e->pos.y);
      blastAtSet = true;
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < -12 || e->pos.x > 112) {
      e->isAlive = false;
      continue;
    }
  }

  if (blastAtSet) {
    int kills = 1;
    FOR_EACH(enemies, ki) {
      ASSIGN_ARRAY_ITEM(enemies, ki, Enemy, o);
      SKIP_IS_NOT_ALIVE(o);
      if (distanceTo(&o->pos, blastAt.x, blastAt.y) < 25) {
        kills++;
      }
    }
    int chainColor;
    if (kills >= 4) {
      chainColor = PURPLE;
    } else if (kills >= 2) {
      chainColor = RED;
    } else {
      chainColor = LIGHT_RED;
    }

    play(EXPLOSION);
    color = chainColor;
    particle(blastAt.x, blastAt.y, 30 + kills * 5, 4, 0, M_PI * 2);
    vectorSet(&blastFx.pos, blastAt.x, blastAt.y);
    blastFx.t = 14;
    blastFx.color = chainColor;

    int n = 0;
    FOR_EACH(enemies, oi) {
      ASSIGN_ARRAY_ITEM(enemies, oi, Enemy, o2);
      SKIP_IS_NOT_ALIVE(o2);
      if (distanceTo(&o2->pos, blastAt.x, blastAt.y) < 25) {
        color = chainColor;
        particle(o2->pos.x, o2->pos.y, 12 + kills * 2, 3, 0, M_PI * 2);
        n++;
        o2->isAlive = false;
        continue;
      }
    }
    FOR_EACH(bullets, bi) {
      ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
      SKIP_IS_NOT_ALIVE(b);
      if (distanceTo(&b->pos, blastAt.x, blastAt.y) < 25) {
        b->isAlive = false;
        continue;
      }
    }
    addScore((5 + n * 8) * combo, blastAt.x, blastAt.y);
    combo++;
    comboT = 120;
    float a = angleTo(&blastAt, player.pos.x, player.pos.y);
    player.vx = cos(a) * 2.2;
    player.vy = sin(a) * 2.2;
    iframes = 40;

    hitstop = (int)clamp(2 + kills, 2, 8);
    shakeMag = clamp(1.5 + kills, 1.5, 7);
    shakeT = 8;
  }

  FOR_EACH(bullets, bi2) {
    ASSIGN_ARRAY_ITEM(bullets, bi2, Bullet, b2);
    SKIP_IS_NOT_ALIVE(b2);
    if (!frozen) {
      b2->pos.x += b2->vx;
    }
    color = LIGHT_RED;
    box(b2->pos.x - b2->vx * 3, b2->pos.y, 3, 1.5);
    box(b2->pos.x - b2->vx * 6, b2->pos.y, 2, 1);
    color = RED;
    Collision bc = box(b2->pos.x, b2->pos.y, 5, 2);
    if (bc.isColliding.rect[CYAN]) {
      if (iframes > 0) {
        b2->isAlive = false;
        continue;
      }
      play(EXPLOSION);
      particle(player.pos.x, player.pos.y, 25, 3, 0, M_PI * 2);
      gameOver();
    }
    if (b2->pos.x < -5 || b2->pos.x > 105) {
      b2->isAlive = false;
      continue;
    }
  }

  if (combo > 1) {
    color = BLACK;
    char comboText[16];
    strcpy(comboText, "x");
    strcat(comboText, intToChar(combo));
    text(comboText, 3, 12);
  }
}

void addGameBlastBuoy() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
