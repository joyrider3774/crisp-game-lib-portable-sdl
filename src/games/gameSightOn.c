#include "cglp.h"

static char *title = "SIGHT ON";
static char *description = "[Tap] Fire";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " llll ",
        "l ll l",
        "l ll l",
        " llll ",
        "ll  ll",
        "ll  ll",
    },
    {
        "ll  ll",
        "l    l",
        "  ll  ",
        "  ll  ",
        "l    l",
        "ll  ll",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

static int starColors[3] = {CYAN, GREEN, BLACK};

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Target;
// Target spawn interval is rnd(120,150)/difficulty (scales with difficulty directly) while each
// target's lifetime only shrinks as 1/sqrt(difficulty) (vel scaled by sqrt(difficulty)*0.1) -
// concurrent count grows unboundedly (~11*sqrt(difficulty)), reachable within ~30min of play.
#define MAX_TARGET_COUNT 512
static Target targets[MAX_TARGET_COUNT];
static int targetIndex;

typedef struct {
  Vector pos;
  Vector vel;
  Vector sink;
  int targetIndex;
} CurrentEnemy;
static CurrentEnemy currentEnemy;

typedef struct {
  Vector pos;
  Vector vel;
  Vector sink;
  int targetIndex;
  bool isAlive;
} Enemy;
#define MAX_ENEMY_COUNT 64
static Enemy enemies[MAX_ENEMY_COUNT];
static int enemyIndex;

static int enemyCount;
static float nextEnemyInterval;
static float nextEnemyTicks;
static float nextTargetTicks;

typedef struct {
  Vector pos;
  int ticks;
  bool isAlive;
} Explosion;
#define MAX_EXPLOSION_COUNT 32
static Explosion explosions[MAX_EXPLOSION_COUNT];
static int explosionIndex;

typedef struct {
  Vector pos;
  Vector vel;
} Sight;
static Sight sight;

static int multiplier;
static float sightDownY;

typedef struct {
  Vector pos;
  int color;
  int ticks;
} Star;
static Star stars[30];

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    enemyCount = 0;
    nextEnemyInterval = 0;
    nextEnemyTicks = 0;
    nextTargetTicks = 0;
    INIT_UNALIVED_ARRAY(targets);
    targetIndex = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    vectorSet(&sight.pos, 50, 50);
    vectorSet(&sight.vel, 0, 0);
    multiplier = 1;
    sightDownY = 0;
    TIMES(30, si) {
      vectorSet(&stars[si].pos, rnd(0, 99), rnd(0, 99));
      stars[si].color = starColors[rndi(0, 3)];
      stars[si].ticks = rndi(0, 999);
    }
  }
  TIMES(30, si2) {
    Star *s = &stars[si2];
    s->ticks++;
    if (s->ticks % 150 < 100) {
      color = s->color;
      box(s->pos.x, s->pos.y, 1, 1);
      s->pos.y += sqrt(difficulty) * 0.5;
      if (s->pos.y > 99) {
        s->pos.x = rnd(0, 99);
        s->pos.y = -rnd(0, 9);
      }
    }
  }
  color = LIGHT_RED;
  rect(0, 90, 100, 10);
  nextTargetTicks--;
  if (nextTargetTicks < 0) {
    play(SELECT);
    float sd = sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(targets, targetIndex, Target, nt);
    vectorSet(&nt->pos, rnd(20, 80), 0);
    vectorSet(&nt->vel, rnd(0, sd) * RNDPM() * 0.1, sd * 0.1);
    nt->isAlive = true;
    int newTargetIndex = targetIndex;
    targetIndex = wrap(targetIndex + 1, 0, MAX_TARGET_COUNT);
    vectorSet(&currentEnemy.pos, rnd(10, 90), -5);
    vectorSet(&currentEnemy.vel, rnd(1, 2) * RNDPM(), rnd(1, 2) * RNDPM());
    vectorSet(&currentEnemy.sink, rnd(0.02, 0.07), rnd(0.02, 0.07));
    currentEnemy.targetIndex = newTargetIndex;
    enemyCount = rndi(2, 5);
    nextEnemyInterval = rnd(3, 9) / sqrt(difficulty);
    nextEnemyTicks = 0;
    nextTargetTicks = rnd(120, 150) / difficulty;
  }
  FOR_EACH(targets, ti) {
    ASSIGN_ARRAY_ITEM(targets, ti, Target, t);
    SKIP_IS_NOT_ALIVE(t);
    vectorAdd(&t->pos, t->vel.x, t->vel.y);
    t->pos.x = wrap(t->pos.x, 0, 99);
    if (t->pos.y > 150) {
      t->isAlive = false;
      continue;
    }
  }
  vectorAdd(&currentEnemy.pos, targets[currentEnemy.targetIndex].vel.x,
            targets[currentEnemy.targetIndex].vel.y);
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    ne->pos = currentEnemy.pos;
    ne->vel = currentEnemy.vel;
    ne->sink = currentEnemy.sink;
    ne->targetIndex = currentEnemy.targetIndex;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MAX_ENEMY_COUNT);
    nextEnemyTicks = nextEnemyInterval;
    enemyCount--;
    if (enemyCount == 0) {
      nextEnemyTicks = 9999;
    }
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (aliveEnemyCount == 0 && enemyCount == 0) {
    nextTargetTicks = 0;
  }
  float maxY = 0;
  bool hasSightEnemy = false;
  int sightEnemyIndex2 = -1;
  color = YELLOW;
  FOR_EACH(explosions, ei) {
    ASSIGN_ARRAY_ITEM(explosions, ei, Explosion, e);
    SKIP_IS_NOT_ALIVE(e);
    float s = cos(e->ticks * 0.05) * 15;
    e->ticks++;
    if (s < 0) {
      e->isAlive = false;
      continue;
    }
    box(e->pos.x, e->pos.y, s, s);
  }
  color = RED;
  FOR_EACH(enemies, ei2) {
    ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e2);
    SKIP_IS_NOT_ALIVE(e2);
    Target *tgt = &targets[e2->targetIndex];
    e2->vel.x += wrap(tgt->pos.x - e2->pos.x, -50, 50) * e2->sink.x;
    e2->vel.y += (tgt->pos.y - e2->pos.y) * e2->sink.y;
    vectorMul(&e2->vel, 0.997);
    e2->pos.x += e2->vel.x * (sqrt(difficulty) - 0.8);
    e2->pos.y += e2->vel.y * (sqrt(difficulty) - 0.8);
    e2->pos.x = wrap(e2->pos.x, 0, 99);
    if (character("a", e2->pos.x, e2->pos.y).isColliding.rect[YELLOW]) {
      play(HIT);
      particle(e2->pos.x, e2->pos.y, 16, 1, 0, M_PI * 2);
      addScore(multiplier, e2->pos.x, clamp(e2->pos.y, 20, 99));
      multiplier++;
      e2->isAlive = false;
      continue;
    }
    if (e2->pos.y > maxY) {
      hasSightEnemy = true;
      sightEnemyIndex2 = ei2;
      maxY = e2->pos.y;
    }
    if (e2->pos.y > 90) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  float sex, sey;
  if (hasSightEnemy) {
    sex = enemies[sightEnemyIndex2].pos.x;
    sey = enemies[sightEnemyIndex2].pos.y;
  } else {
    sex = 50;
    sey = 50;
  }
  sight.vel.x += wrap(sex - sight.pos.x, -50, 50) * 0.01;
  sight.vel.y += (sey - sight.pos.y) * 0.01;
  vectorMul(&sight.vel, 0.97);
  sight.pos.x += sight.vel.x * (sqrt(difficulty) - 0.8);
  sight.pos.y += sight.vel.y * (sqrt(difficulty) - 0.8);
  sight.pos.x = wrap(sight.pos.x, 0, 99);
  color = BLACK;
  character("b", sight.pos.x, sight.pos.y);
  if (input.isJustPressed) {
    play(EXPLOSION);
    multiplier = 1;
    thickness = 1;
    line(0, 90, sight.pos.x, sight.pos.y);
    line(99, 90, sight.pos.x, sight.pos.y);
    ASSIGN_ARRAY_ITEM(explosions, explosionIndex, Explosion, nex);
    nex->pos = sight.pos;
    nex->ticks = 0;
    nex->isAlive = true;
    explosionIndex = wrap(explosionIndex + 1, 0, MAX_EXPLOSION_COUNT);
    FOR_EACH(targets, ti2) {
      ASSIGN_ARRAY_ITEM(targets, ti2, Target, t2);
      SKIP_IS_NOT_ALIVE(t2);
      t2->pos.y += sightDownY;
    }
    sightDownY++;
    vectorMul(&sight.vel, 0.1);
  }
  sightDownY *= 0.99;
}

void addGameSightOn() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
