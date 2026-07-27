#include "cglp.h"

static char *title = "V BOMB";
static char *description = "[Tap] Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "ll    ",
        "lllll ",
        "llllll",
        "      ",
        "      ",
        "      ",
    },
    {
        "lll   ",
        "lll   ",
        "lll   ",
        "lll   ",
        " l    ",
        "      ",
    },
    {
        "  ll  ",
        " l ll ",
        "l llll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 200, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vx;
  float targetVx;
} Ship;
static Ship ship;

typedef struct {
  Vector pos;
  Vector vel;
  float bombTicks;
  bool isAlive;
} Enemy;
#define VBOMB_MAX_ENEMY_COUNT 32
static Enemy enemies[VBOMB_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  float count;
  float vy;
  bool isAlive;
} Bomb;
#define VBOMB_MAX_BOMB_COUNT 32
static Bomb bombs[VBOMB_MAX_BOMB_COUNT];
static int bombIndex;

typedef struct {
  Vector pos;
  float height;
  bool isAlive;
} Explosion;
#define VBOMB_MAX_EXPLOSION_COUNT 32
static Explosion explosions[VBOMB_MAX_EXPLOSION_COUNT];
static int explosionIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&ship.pos, 40, 10);
    ship.vx = 1;
    ship.targetVx = 1;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bombs);
    bombIndex = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    multiplier = 1;
  }
  color = BLACK;
  rect(0, 90, 100, 10);
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    float ex;
    if (rnd(0, 1) < 0.5) {
      ex = -3;
    } else {
      ex = 103;
    }
    vectorSet(&ne->pos, ex, rnd(70, 80));
    vectorSet(&ne->vel, 0, -sqrt(difficulty) * rnd(0.01, 0.03));
    ne->bombTicks = 0;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, VBOMB_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(120, 150) / difficulty;
  }
  COUNT_IS_ALIVE(explosions, aliveExplosionCount);
  if (aliveExplosionCount == 0) {
    multiplier = 1;
  }
  color = RED;
  FOR_EACH(explosions, i) {
    ASSIGN_ARRAY_ITEM(explosions, i, Explosion, e);
    SKIP_IS_NOT_ALIVE(e);
    box(VEC_XY(e->pos), 6, e->height);
    e->pos.y -= 7;
    e->height += 5;
    e->isAlive = e->pos.y + e->height >= 0;
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->bombTicks--;
    if (e->bombTicks < 0) {
      if (e->pos.x > 1 && e->pos.x < 99) {
        play(LASER);
        ASSIGN_ARRAY_ITEM(bombs, bombIndex, Bomb, nb);
        nb->pos = e->pos;
        nb->count = rndi(3, 9) + 0.9;
        nb->vy = 0;
        nb->isAlive = true;
        bombIndex = wrap(bombIndex + 1, 0, VBOMB_MAX_BOMB_COUNT);
      }
      e->bombTicks = rnd(200, 300);
    }
    float ms = sqrt(difficulty) * 0.1;
    e->vel.x = clamp(e->vel.x + (ship.pos.x - e->pos.x) * 0.0002 * sqrt(difficulty), -ms, ms);
    if (e->pos.y < 10) {
      e->vel.y = 0;
    }
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    char ec[2];
    bool small = fabs(e->vel.x) < 0.1;
    if (small) {
      ec[0] = 'b';
    } else {
      ec[0] = 'a';
    }
    ec[1] = 0;
    characterOptions.isMirrorX = e->vel.x < 0;
    characterOptions.isMirrorY = small;
    Collision ecoll = character(ec, VEC_XY(e->pos));
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    if (ecoll.isColliding.rect[RED]) {
      play(COIN);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      particle(VEC_XY(e->pos), 9, 3, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
  }
  FOR_EACH(bombs, i) {
    ASSIGN_ARRAY_ITEM(bombs, i, Bomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->vy += 0.1 * difficulty;
    b->pos.y += b->vy;
    if (b->pos.y > 87) {
      b->pos.y = 87;
    }
    color = PURPLE;
    character("c", b->pos.x, b->pos.y + 3 - (10 - b->count) / 2);
    if (b->count <= 1) {
      color = RED;
    } else {
      color = BLACK;
    }
    float ty = b->pos.y - (10 - b->count) * 2;
    if (b->count < 1) {
      ty -= (1 - b->count) * 50;
    }
    Collision tc = text(intToChar((int)ceil(b->count)), b->pos.x, ty);
    if (tc.isColliding.rect[RED]) {
      b->count = 0;
    }
    color = PURPLE;
    particle(b->pos.x, b->pos.y - (10 - b->count) / 2, 0.3, (10 - b->count) * 0.1,
             -M_PI / 2, M_PI / 3);
    float pc = ceil(b->count);
    b->count -= 1.0 / 60;
    if (pc != ceil(b->count)) {
      play(HIT);
    }
    if (b->count <= 0) {
      play(EXPLOSION);
      particle(b->pos.x, b->pos.y, 20, 3, -M_PI / 2, M_PI / 8);
      ASSIGN_ARRAY_ITEM(explosions, explosionIndex, Explosion, ne2);
      ne2->pos = b->pos;
      ne2->height = 6;
      ne2->isAlive = true;
      explosionIndex = wrap(explosionIndex + 1, 0, VBOMB_MAX_EXPLOSION_COUNT);
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  if (input.isJustPressed || (ship.pos.x > 99 && ship.targetVx > 0) ||
      (ship.pos.x < 1 && ship.targetVx < 0)) {
    play(SELECT);
    ship.targetVx *= -1;
  }
  ship.vx += (ship.targetVx - ship.vx) * 0.1;
  ship.pos.x += ship.vx * sqrt(difficulty);
  char sc2[2];
  bool shipSmall = fabs(ship.vx) < 0.5;
  if (shipSmall) {
    sc2[0] = 'b';
  } else {
    sc2[0] = 'a';
  }
  sc2[1] = 0;
  characterOptions.isMirrorX = ship.vx < 0;
  Collision sc = character(sc2, VEC_XY(ship.pos));
  characterOptions.isMirrorX = false;
  if (sc.isColliding.rect[RED] || sc.isColliding.character['a'] ||
      sc.isColliding.character['b']) {
    play(RANDOM);
    gameOver();
  }
}

void addGameVBomb() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
