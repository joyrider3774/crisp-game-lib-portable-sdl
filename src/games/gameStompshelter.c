#include "cglp.h"

static char *title = "STOMPSHELTER";
static char *description = "[Tap] Double Jump\n[Hold] Slide";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  bool grounded;
  float scaleX;
  float scaleY;
} Player;
static Player player;

typedef struct {
  float x;
  float y;
  float w;
  float hp;
  bool isAlive;
} Platform;
#define STOMPSHELTER_MAX_PLATFORM_COUNT 40
static Platform platforms[STOMPSHELTER_MAX_PLATFORM_COUNT];
static int platformIndex;

typedef struct {
  float x;
  float y;
  int dir;
  float py;
  bool isAlive;
} Enemy;
#define STOMPSHELTER_MAX_ENEMY_COUNT 64
static Enemy enemies[STOMPSHELTER_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  float x;
  float y;
  float speed;
  bool isAlive;
} Debris;
// Sized well past the ~110 estimated at difficulty 30; also guarded against a modulo-by-zero trap past difficulty 70.
#define STOMPSHELTER_MAX_DEBRIS_COUNT 256
static Debris debris[STOMPSHELTER_MAX_DEBRIS_COUNT];
static int debrisIndex;

typedef struct {
  float x;
  float y;
  int life;
  bool isAlive;
} Trail;
#define STOMPSHELTER_MAX_TRAIL_COUNT 32
static Trail trails[STOMPSHELTER_MAX_TRAIL_COUNT];
static int trailIndex;

static float nextEnemyDist;
static float lavaY;
static int combo;
static int jumpCount;
static bool wasGrounded;

static void update() {
  if (!ticks) {
    player.x = 50;
    player.y = 70;
    player.vx = 1;
    player.vy = 0;
    player.grounded = false;
    player.scaleX = 1;
    player.scaleY = 1;
    INIT_UNALIVED_ARRAY(platforms);
    platformIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 100;
    INIT_UNALIVED_ARRAY(debris);
    debrisIndex = 0;
    lavaY = 102;
    combo = 0;
    jumpCount = 0;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    wasGrounded = false;
    TIMES(6, initPlI) {
      ASSIGN_ARRAY_ITEM(platforms, platformIndex, Platform, np);
      np->x = rnd(20, 60);
      np->y = 15 + initPlI * 15;
      np->w = rnd(25, 38);
      np->hp = 120;
      np->isAlive = true;
      platformIndex = wrap(platformIndex + 1, 0, STOMPSHELTER_MAX_PLATFORM_COUNT);
    }
  }

  lavaY -= 0.025;

  float topY = 999;
  FOR_EACH(platforms, tpI) {
    ASSIGN_ARRAY_ITEM(platforms, tpI, Platform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->y < topY) {
      topY = p->y;
    }
  }
  if (topY > 5) {
    ASSIGN_ARRAY_ITEM(platforms, platformIndex, Platform, np2);
    np2->x = rnd(10, 55);
    np2->y = topY - rnd(13, 17);
    np2->w = rnd(25, 38);
    np2->hp = 120;
    np2->isAlive = true;
    platformIndex = wrap(platformIndex + 1, 0, STOMPSHELTER_MAX_PLATFORM_COUNT);
  }

  if (nextEnemyDist < 0) {
    int vpCount = 0;
    FOR_EACH(platforms, vpI) {
      ASSIGN_ARRAY_ITEM(platforms, vpI, Platform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->y < 20 && p->hp > 60) {
        vpCount++;
      }
    }
    if (vpCount > 0) {
      int pick = (int)floor(rnd(0, vpCount));
      int seen = 0;
      Platform *chosen = NULL;
      FOR_EACH(platforms, vpI2) {
        ASSIGN_ARRAY_ITEM(platforms, vpI2, Platform, p);
        SKIP_IS_NOT_ALIVE(p);
        if (p->y < 20 && p->hp > 60) {
          if (seen == pick) {
            chosen = p;
          }
          seen++;
        }
      }
      if (chosen != NULL) {
        ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
        ne->x = chosen->x + chosen->w / 2;
        ne->y = chosen->y - 5;
        ne->dir = rndi(0, 2) * 2 - 1;
        ne->py = chosen->y;
        ne->isAlive = true;
        enemyIndex = wrap(enemyIndex + 1, 0, STOMPSHELTER_MAX_ENEMY_COUNT);
        nextEnemyDist = rnd(100, 200) / sqrt(difficulty);
      }
    }
  }

  int debrisInterval = (int)floor(70.0 / difficulty);
  if (debrisInterval < 1) {
    debrisInterval = 1;
  }
  if (ticks % debrisInterval == 0) {
    ASSIGN_ARRAY_ITEM(debris, debrisIndex, Debris, nd);
    nd->x = rnd(8, 92);
    nd->y = -4;
    nd->speed = 0.5;
    nd->isAlive = true;
    debrisIndex = wrap(debrisIndex + 1, 0, STOMPSHELTER_MAX_DEBRIS_COUNT);
  }

  if (input.isJustPressed && (player.grounded || jumpCount < 2)) {
    player.vy = -2.6;
    player.grounded = false;
    jumpCount++;
    play(JUMP);
    player.scaleX = 0.7;
    player.scaleY = 1.4;
    color = CYAN;
    particle(player.x, player.y + 3, 5, 1, -M_PI_2, M_PI_4);
  }

  player.vy += 0.1;
  player.y += player.vy;

  float driftFactor;
  if (input.isPressed) {
    driftFactor = 1;
  } else {
    driftFactor = 0.1;
  }
  player.x += player.vx * driftFactor;
  if ((player.x > 90 && player.vx > 0) || (player.x < 10 && player.vx < 0)) {
    player.vx *= -1;
  }

  player.grounded = false;
  FOR_EACH(platforms, pcI) {
    ASSIGN_ARRAY_ITEM(platforms, pcI, Platform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (player.vy > 0 && p->hp > 0 && player.x > p->x - 3 && player.x < p->x + p->w + 3 &&
        player.y > p->y - 8 && player.y < p->y + 2) {
      player.y = p->y - 4;
      player.vy = 0;
      player.grounded = true;
      jumpCount = 0;
      combo = 0;
      p->hp -= 1;
      if (!wasGrounded) {
        player.scaleX = 1.4;
        player.scaleY = 0.6;
        color = CYAN;
        particle(player.x, player.y + 3, 4, 0.8, M_PI_2, M_PI / 3);
      }
    }
  }
  wasGrounded = player.grounded;

  if (player.y < 60) {
    float scroll = 60 - player.y;
    player.y = 60;
    lavaY += scroll;
    nextEnemyDist -= scroll;
    FOR_EACH(platforms, spI) {
      ASSIGN_ARRAY_ITEM(platforms, spI, Platform, p);
      SKIP_IS_NOT_ALIVE(p);
      p->y += scroll;
    }
    FOR_EACH(enemies, seI) {
      ASSIGN_ARRAY_ITEM(enemies, seI, Enemy, e);
      SKIP_IS_NOT_ALIVE(e);
      e->y += scroll;
      e->py += scroll;
    }
    FOR_EACH(debris, sdI) {
      ASSIGN_ARRAY_ITEM(debris, sdI, Debris, d);
      SKIP_IS_NOT_ALIVE(d);
      d->y += scroll;
    }
    FOR_EACH(trails, stI) {
      ASSIGN_ARRAY_ITEM(trails, stI, Trail, t);
      SKIP_IS_NOT_ALIVE(t);
      t->y += scroll;
    }
    addScore(floor(scroll), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
  }

  FOR_EACH(platforms, rpI) {
    ASSIGN_ARRAY_ITEM(platforms, rpI, Platform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (!(p->y < 110 && p->hp > 0)) {
      p->isAlive = false;
      continue;
    }
  }
  FOR_EACH(enemies, reI) {
    ASSIGN_ARRAY_ITEM(enemies, reI, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!(e->y < 105)) {
      e->isAlive = false;
      continue;
    }
  }
  FOR_EACH(debris, rdI) {
    ASSIGN_ARRAY_ITEM(debris, rdI, Debris, d);
    SKIP_IS_NOT_ALIVE(d);
    if (!(d->y < 105)) {
      d->isAlive = false;
      continue;
    }
  }

  FOR_EACH(enemies, emI) {
    ASSIGN_ARRAY_ITEM(enemies, emI, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x += e->dir * 0.35;
    FOR_EACH(platforms, epI) {
      ASSIGN_ARRAY_ITEM(platforms, epI, Platform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->hp > 0 && fabs(e->py - p->y) < 2) {
        if (e->x <= p->x + 2) {
          e->dir = 1;
        }
        if (e->x >= p->x + p->w - 2) {
          e->dir = -1;
        }
      }
    }
  }

  FOR_EACH(debris, dfI) {
    ASSIGN_ARRAY_ITEM(debris, dfI, Debris, d);
    SKIP_IS_NOT_ALIVE(d);
    d->y += d->speed;
  }

  player.scaleX += (1 - player.scaleX) * 0.15;
  player.scaleY += (1 - player.scaleY) * 0.15;

  if (player.grounded && fabs(player.scaleX - 1) < 0.05) {
    float breath = sin(ticks * 0.1) * 0.05;
    player.scaleX = 1 + breath;
    player.scaleY = 1 - breath;
  }

  if (fabs(player.vy) > 1 || fabs(player.vx) > 0.5) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
    nt->x = player.x;
    nt->y = player.y;
    nt->life = 8;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, STOMPSHELTER_MAX_TRAIL_COUNT);
  }
  FOR_EACH(trails, tlI) {
    ASSIGN_ARRAY_ITEM(trails, tlI, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->life--;
    if (t->life <= 0) {
      t->isAlive = false;
      continue;
    }
  }

  FOR_EACH(trails, dtI) {
    ASSIGN_ARRAY_ITEM(trails, dtI, Trail, t);
    SKIP_IS_NOT_ALIVE(t);
    color = LIGHT_CYAN;
    float alpha = (float)t->life / 8;
    box(t->x, t->y, 4 * alpha, 4 * alpha);
  }

  color = CYAN;
  float tilt = player.vx * 0.15;
  float pw = 6 * player.scaleX;
  float ph = 6 * player.scaleY;
  thickness = pw;
  bar(player.x, player.y, ph / 2, -M_PI_2 + tilt);

  color = WHITE;
  float eyeOffX;
  if (player.vx > 0) {
    eyeOffX = 1;
  } else {
    eyeOffX = -1;
  }
  float eyeOffY;
  if (player.vy > 0) {
    eyeOffY = 1;
  } else if (player.vy < -1) {
    eyeOffY = -1;
  } else {
    eyeOffY = 0;
  }
  box(player.x - 1.2, player.y - 1, 2, 2);
  box(player.x + 1.2, player.y - 1, 2, 2);
  color = BLACK;
  box(player.x - 1.2 + eyeOffX * 0.4, player.y - 1 + eyeOffY * 0.3, 1, 1);
  box(player.x + 1.2 + eyeOffX * 0.4, player.y - 1 + eyeOffY * 0.3, 1, 1);

  if (lavaY > 100) {
    lavaY += (100 - lavaY) * 0.5;
  }
  color = YELLOW;
  rect(0, lavaY, 100, 120 - lavaY);
  if (player.y > lavaY - 3) {
    play(EXPLOSION);
    gameOver();
  }

  FOR_EACH(platforms, dpI) {
    ASSIGN_ARRAY_ITEM(platforms, dpI, Platform, p);
    SKIP_IS_NOT_ALIVE(p);
    if (p->hp > 80) {
      color = BLACK;
    } else if (p->hp > 40) {
      color = BLUE;
    } else {
      color = LIGHT_RED;
    }
    rect(p->x, p->y, p->w, 4);
  }

  FOR_EACH(enemies, deI) {
    ASSIGN_ARRAY_ITEM(enemies, deI, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = RED;
    Collision c = box(e->x, e->y, 7, 7);

    float eTilt = e->dir * 0.2;
    thickness = 7;
    bar(e->x, e->y, 3.5, -M_PI_2 + eTilt);

    color = WHITE;
    float lookX;
    if (player.x > e->x) {
      lookX = 1;
    } else {
      lookX = -1;
    }
    float lookY;
    if (player.y > e->y) {
      lookY = 1;
    } else if (player.y < e->y - 5) {
      lookY = -1;
    } else {
      lookY = 0;
    }
    box(e->x - 1.5, e->y - 1, 2.5, 2.5);
    box(e->x + 1.5, e->y - 1, 2.5, 2.5);
    color = BLACK;
    box(e->x - 1.5 + lookX * 0.5, e->y - 1 + lookY * 0.4, 1.2, 1.2);
    box(e->x + 1.5 + lookX * 0.5, e->y - 1 + lookY * 0.4, 1.2, 1.2);

    if (c.isColliding.rect[CYAN]) {
      if (player.vy > 0.5 && player.y < e->y - 2) {
        player.vy = -2.2;
        combo++;
        addScore(8 + combo * 4, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
        play(POWER_UP);
        color = RED;
        particle(e->x, e->y, 10, 2, 0, M_PI * 2);
        e->isAlive = false;
        continue;
      } else {
        play(HIT);
        gameOver();
      }
    }
  }

  FOR_EACH(debris, ddI) {
    ASSIGN_ARRAY_ITEM(debris, ddI, Debris, d);
    SKIP_IS_NOT_ALIVE(d);
    color = LIGHT_PURPLE;
    box(d->x, d->y - 3, 2, 2);
    box(d->x, d->y - 6, 1, 1);

    bool blocked = false;
    FOR_EACH(platforms, bpI) {
      ASSIGN_ARRAY_ITEM(platforms, bpI, Platform, p);
      SKIP_IS_NOT_ALIVE(p);
      if (p->hp > 0 && d->x > p->x && d->x < p->x + p->w && d->y > p->y - 2 && d->y < p->y + 5) {
        blocked = true;
        p->hp -= 15;
      }
    }
    if (blocked) {
      color = PURPLE;
      particle(d->x, d->y, 6, 1, 0, M_PI * 2);
      d->isAlive = false;
      continue;
    }
    color = PURPLE;
    Collision dc = box(d->x, d->y, 4, 4);
    if (dc.isColliding.rect[CYAN]) {
      play(HIT);
      gameOver();
    }
  }
}

void addGameStompshelter() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
