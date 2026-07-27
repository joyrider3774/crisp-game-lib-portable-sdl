#include "cglp.h"

static char *title = "LIE DOWN";
static char *description = "[Hold] Lie down";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "    ll",
        "   l  ",
        " llll ",
        "l l   ",
        "ll l  ",
        "    l ",
    },
    {
        "    ll",
        "   l  ",
        " lll  ",
        "  l   ",
        " ll   ",
        " ll   ",
    },
    {
        "l   ll",
        "llllll",
            "      ",
        "      ",
        "      ",
        "      ",
},
    {
        " ll l ",
        "  l l ",
        "  ll  ",
        "  l   ",
        "ll l  ",
        "    l ",
    },
    {
        "ll    ",
        " l    ",
        " lll  ",
        "l l   ",
        " l l  ",
        " l  l ",
    },
    {
        " l    ",
        "lll   ",
        " l    ",
            "      ",
        "      ",
        "      ",
},
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 50, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  float x;
  float height;
  float speed;
  bool isAlive;
} LiedownHole;
#define LIEDOWN_MAX_HOLE_COUNT 32
static LiedownHole holes[LIEDOWN_MAX_HOLE_COUNT];
static int holeIndex;
static float nextHoleTicks;

typedef struct {
  float x;
  float height;
  float speed;
  bool isAlive;
} LiedownEnemy;
#define LIEDOWN_MAX_ENEMY_COUNT 32
static LiedownEnemy enemies[LIEDOWN_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  float x;
  float vx;
  float targetVx;
  bool isAlive;
} LiedownRock;
#define LIEDOWN_MAX_ROCK_COUNT 32
static LiedownRock rocks[LIEDOWN_MAX_ROCK_COUNT];
static int rockIndex;

typedef struct {
  float x;
  float vx;
  float y;
} LiedownPlayer;
static LiedownPlayer player;

static float separateLine;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(holes);
    holeIndex = 0;
    nextHoleTicks = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 60;
    INIT_UNALIVED_ARRAY(rocks);
    rockIndex = 0;
    player.x = 20;
    player.vx = 0;
    player.y = 0;
    separateLine = 100;
    multiplier = 1;
  }
  float scr = difficulty * 0.1;
  if (player.x > 30) {
    scr += (player.x - 30) * 0.1;
  }
  if (player.y > 0) {
    scr = 0;
  }
  color = BLACK;
  rect(0, 40, 200, 10);
  color = LIGHT_BLACK;
  separateLine = wrap(separateLine - scr, 0, 200);
  rect(separateLine, 40, 1, 10);
  color = BLACK;
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    play(LASER);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, LiedownEnemy, ne);
    ne->x = rnd(100, 250);
    ne->height = 0;
    ne->speed = (0.1 + rnd(0, 2) * rnd(0, 2) * 0.1) * difficulty;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, LIEDOWN_MAX_ENEMY_COUNT);
    nextEnemyTicks = 50 / sqrt(difficulty);
  }
  color = RED;
  FOR_EACH(rocks, i) {
    ASSIGN_ARRAY_ITEM(rocks, i, LiedownRock, r);
    SKIP_IS_NOT_ALIVE(r);
    r->x -= scr + r->vx;
    r->vx += (r->targetVx - r->vx) * 0.1;
    if (r->vx > r->targetVx * 0.5) {
      character("f", r->x, 34);
    }
    if (r->x < -3 || r->x > 203) {
      r->isAlive = false;
      continue;
    }
  }
  nextHoleTicks--;
  if (nextHoleTicks < 0) {
    ASSIGN_ARRAY_ITEM(holes, holeIndex, LiedownHole, nh);
    nh->x = rnd(0, 300);
    nh->height = 0;
    nh->speed = rnd(0.1, 0.2) * difficulty;
    nh->isAlive = true;
    holeIndex = wrap(holeIndex + 1, 0, LIEDOWN_MAX_HOLE_COUNT);
    nextHoleTicks = 100 / sqrt(difficulty);
  }
  color = WHITE;
  FOR_EACH(holes, i) {
    ASSIGN_ARRAY_ITEM(holes, i, LiedownHole, h);
    SKIP_IS_NOT_ALIVE(h);
    h->x -= scr;
    h->height += h->speed;
    if (h->height > 11 && h->speed > 0) {
      h->speed *= -1;
    }
    float hg = clamp(h->height, 0, 10);
    rect(h->x, 50 - hg, 9, hg + 1);
    if (h->height < 0) {
      h->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (player.y > 0) {
    player.y++;
    characterOptions.isMirrorY = true;
    character("a", player.x, 37 + player.y);
    characterOptions.isMirrorY = false;
    if (37 + player.y > 45) {
      play(EXPLOSION);
      gameOver();
    }
  } else {
    if (input.isJustPressed) {
      play(SELECT);
      if (multiplier > 1) {
        multiplier--;
      }
    }
    if (!input.isPressed) {
      player.vx += difficulty * 0.02;
    }
    player.vx *= 0.99;
    player.x += player.vx - scr;
    if (player.x < 0) {
      play(EXPLOSION);
      gameOver();
    }
    char pc[2];
    float py;
    if (input.isPressed) {
      pc[0] = 'c';
      py = 39;
    } else {
      pc[0] = 'a' + (int)floor(ticks / 20) % 2;
      py = 37;
    }
    pc[1] = 0;
    Collision pcColl = character(pc, player.x, py);
    if (pcColl.isColliding.character['f']) {
      play(EXPLOSION);
      gameOver();
    }
    color = TRANSPARENT;
    Collision c1 = character("a", player.x - 5, 38);
    Collision c2 = character("a", player.x + 5, 38);
    if (c1.isColliding.rect[WHITE] && c2.isColliding.rect[WHITE]) {
      player.vx = 0;
      if (input.isPressed) {
        player.x -= 5;
      } else {
        player.y = 1;
      }
    }
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, LiedownEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    if (e->speed > 0) {
      e->height += sqrt(e->speed * 10) / 10;
    } else {
      e->height += e->speed;
    }
    if (e->height > 15 && e->speed > 0) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(rocks, rockIndex, LiedownRock, nr);
      nr->x = e->x;
      nr->vx = 0;
      nr->targetVx = e->speed * 3;
      nr->isAlive = true;
      rockIndex = wrap(rockIndex + 1, 0, LIEDOWN_MAX_ROCK_COUNT);
      e->speed *= -1;
    }
    float hg = 10 - clamp(e->height, 0, 10);
    float y = 37 + (hg * hg) / 10;
    if (e->height > 12 && e->speed > 0) {
      color = RED;
    } else {
      color = BLACK;
    }
    char ec[2];
    if (e->speed > 0) {
      ec[0] = 'd';
    } else {
      ec[0] = 'd' + 1;
    }
    ec[1] = 0;
    Collision ecColl = character(ec, e->x, y);
    if (ecColl.isColliding.character['a'] || ecColl.isColliding.character['b'] ||
        ecColl.isColliding.character['c']) {
      play(COIN);
      addScore(multiplier, e->x, y);
      multiplier++;
      particle(e->x, y, 16, 1, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->height < 0) {
      e->isAlive = false;
      continue;
    }
  }
}

void addGameLieDown() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
