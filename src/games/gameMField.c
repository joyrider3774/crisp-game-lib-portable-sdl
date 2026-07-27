#include "cglp.h"

static char *title = "M FIELD";
static char *description = "[Tap]\n Jump / Double jump\n[Hold]\n Speed up";

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
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
        "      ",
    },
    {
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  float x;
  float ticks;
  bool isBlinking;
  bool isAlive;
} MfieldMine;
#define MFIELD_MAX_MINE_COUNT 32
static MfieldMine mines[MFIELD_MAX_MINE_COUNT];
static int mineIndex;
static float nextMineDist;

typedef struct {
  float x;
  float ticks;
  bool isAlive;
} MfieldExplosion;
#define MFIELD_MAX_EXPLOSION_COUNT 16
static MfieldExplosion explosions[MFIELD_MAX_EXPLOSION_COUNT];
static int explosionIndex;

typedef struct {
  float x;
  float vx;
  bool isAlive;
} MfieldEnemy;
#define MFIELD_MAX_ENEMY_COUNT 32
static MfieldEnemy enemies[MFIELD_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;

typedef struct {
  Vector pos;
  Vector vel;
  int jumpCount;
} MfieldPlayer;
static MfieldPlayer player;
static float gx;
static int multiplier;

static void explode(float x) {
  play(EXPLOSION);
  ASSIGN_ARRAY_ITEM(explosions, explosionIndex, MfieldExplosion, ne);
  ne->x = x;
  ne->ticks = 0;
  ne->isAlive = true;
  explosionIndex = wrap(explosionIndex + 1, 0, MFIELD_MAX_EXPLOSION_COUNT);
  color = RED;
  particle(x, 89, 9, 2, -M_PI / 2, M_PI / 2);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(mines);
    mineIndex = 0;
    nextMineDist = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 0;
    vectorSet(&player.pos, 80, 87);
    vectorSet(&player.vel, 0, 0);
    player.jumpCount = 0;
    gx = 0;
    multiplier = 1;
  }
  float scr;
  if (player.pos.x > 99) {
    scr = (player.pos.x - 99) * 0.1 * sqrt(difficulty);
  } else {
    scr = 0;
  }
  color = LIGHT_BLACK;
  rect(0, 90, 200, 10);
  gx = wrap(gx - scr, 0, 200);
  color = WHITE;
  rect(gx, 90, 2, 10);
  color = RED;
  FOR_EACH(explosions, i) {
    ASSIGN_ARRAY_ITEM(explosions, i, MfieldExplosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x -= scr;
    e->ticks += sqrt(difficulty);
    box(e->x, 86, sin(e->ticks * 0.1) * 50, 8);
    if (e->ticks > M_PI / 0.1) {
      e->isAlive = false;
      continue;
    }
  }
  nextEnemyDist -= scr;
  if (nextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, MfieldEnemy, ne);
    if (rnd(0, 1) < 0.7) {
      ne->x = 203;
      ne->vx = -rnd(1, 2) * sqrt(difficulty) * 0.4;
    } else {
      ne->x = -3;
      ne->vx = rnd(1.5, 2) * sqrt(difficulty) * 0.4;
    }
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MFIELD_MAX_ENEMY_COUNT);
    nextEnemyDist += rnd(20, 30);
  }
  color = PURPLE;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, MfieldEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->x += e->vx - scr;
    if (player.jumpCount == 0 &&
        ((e->x < player.pos.x - 20 && e->vx < 0) ||
         (e->x > player.pos.x + 20 && e->vx > 0))) {
      e->vx *= -1;
    }
    char ec[2];
    ec[0] = 'c' + (int)floor(ticks / 20) % 2;
    ec[1] = 0;
    if (e->vx > 0) {
      characterOptions.isMirrorX = true;
    } else {
      characterOptions.isMirrorX = false;
    }
    Collision ecColl = character(ec, e->x, 87);
    characterOptions.isMirrorX = false;
    if (ecColl.isColliding.rect[RED]) {
      play(POWER_UP);
      addScore(multiplier, e->x, 87);
      particle(e->x, 87, 16, 1, 0, M_PI * 2);
      if (multiplier < 64) {
        multiplier *= 2;
      }
      e->isAlive = false;
      continue;
    }
    if (e->x < -9) {
      e->isAlive = false;
      continue;
    }
  }
  float pressMul;
  if (input.isPressed) {
    pressMul = 1;
  } else {
    pressMul = 0.3;
  }
  player.vel.x += (pressMul * sqrt(difficulty) - player.vel.x) * 0.1;
  if (player.jumpCount > 0) {
    float gravMul;
    if (input.isPressed) {
      gravMul = 0.05;
    } else {
      gravMul = 0.1;
    }
    player.vel.y += gravMul * difficulty;
    if (player.pos.y > 87) {
      player.pos.y = 87;
      player.vel.y = 0;
      player.jumpCount = 0;
    }
  }
  if (player.jumpCount < 2 && input.isJustPressed) {
    play(JUMP);
    player.vel.y = -2 * sqrt(difficulty);
    player.pos.y -= 6;
    player.jumpCount++;
  }
  vectorAdd(&player.pos, player.vel.x, player.vel.y);
  player.pos.x -= scr;
  color = BLACK;
  char pc[2];
  if (player.jumpCount > 0 || ticks % 30 > 15) {
    pc[0] = 'a';
  } else {
    pc[0] = 'b';
  }
  pc[1] = 0;
  Collision c = character(pc, player.pos.x, player.pos.y);
  if (c.isColliding.rect[RED]) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  if (c.isColliding.character['c'] || c.isColliding.character['d']) {
    if (player.vel.y > 0) {
      play(JUMP);
      player.vel.y *= -0.8;
      player.pos.y = 80;
    } else {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  nextMineDist -= scr;
  if (nextMineDist < 0) {
    ASSIGN_ARRAY_ITEM(mines, mineIndex, MfieldMine, nm);
    nm->x = 203;
    nm->ticks = 0;
    nm->isBlinking = false;
    nm->isAlive = true;
    mineIndex = wrap(mineIndex + 1, 0, MFIELD_MAX_MINE_COUNT);
    if (rnd(0, 1) < 0.6) {
      nextMineDist = rnd(9, 20);
    } else {
      nextMineDist = rnd(50, 80);
    }
  }
  FOR_EACH(mines, i) {
    ASSIGN_ARRAY_ITEM(mines, i, MfieldMine, m);
    SKIP_IS_NOT_ALIVE(m);
    m->x -= scr;
    float y;
    if (m->ticks > 0) {
      y = 89;
    } else {
      y = 91;
    }
    color = PURPLE;
    if (m->ticks > 0) {
      m->ticks += sqrt(difficulty);
      if (m->ticks > 59) {
        explode(m->x);
        m->isAlive = false;
        continue;
      } else if (fmod(m->ticks, 30) > 15) {
        if (!m->isBlinking) {
          play(LASER);
        }
        m->isBlinking = true;
      } else {
        color = TRANSPARENT;
        m->isBlinking = false;
      }
    }
    Collision mc = box(m->x, y, 6, 3);
    if (mc.isColliding.rect[RED]) {
      explode(m->x);
      m->isAlive = false;
      continue;
    } else if (m->ticks == 0 && (mc.isColliding.character['a'] || mc.isColliding.character['b'])) {
      play(HIT);
      multiplier = 1;
      m->ticks = 1;
    }
    if (m->x < -3) {
      m->isAlive = false;
      continue;
    }
  }
}

void addGameMField() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
