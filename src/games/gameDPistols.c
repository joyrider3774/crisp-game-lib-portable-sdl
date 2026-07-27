#include "cglp.h"

static char *title = "D PISTOLS";
static char *description = "[Tap]\n Turn & Fire\n[Hold]\n Cross Fire";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

typedef struct {
  Vector pos;
  int my;
} Player;
static Player player;
static float holdTicks;

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} Shot;
#define DPISTOLS_MAX_SHOT_COUNT 32
static Shot shots[DPISTOLS_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  Vector vel;
  int ticks;
  bool isAlive;
} Enemy;
// Enemies only ever expire via being shot or hitting the player - once past
// x==50 they turn to vertical flight with no offscreen/bounds removal at
// all, so any enemy the player merely lets fly by (not directly hit) stays
// "alive" in this array forever; spawn rate also grows with difficulty
// (~2*difficulty/sec), so 32 fills up within seconds of normal play.
#define DPISTOLS_MAX_ENEMY_COUNT 1024
static Enemy enemies[DPISTOLS_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;
static float nextEnemyY;
static float prevEnemyVx;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 20);
    player.my = 1;
    holdTicks = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    nextEnemyY = 50;
    prevEnemyVx = 1;
    multiplier = 0;
  }
  if (input.isJustPressed) {
    play(LASER);
    player.my *= -1;
    TIMES(2, i) {
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, s);
      vectorSet(&s->pos, player.pos.x, player.pos.y + player.my);
      s->angle = i * M_PI;
      s->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, DPISTOLS_MAX_SHOT_COUNT);
    }
    multiplier = (int)clamp(multiplier - 1, 0, 99);
  }
  if (input.isPressed) {
    holdTicks += difficulty;
    if (holdTicks > 30) {
      play(SELECT);
      TIMES(4, i) {
        ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, s2);
        vectorSet(&s2->pos, player.pos.x, player.pos.y);
        s2->angle = (i * M_PI) / 2;
        s2->isAlive = true;
        shotIndex = wrap(shotIndex + 1, 0, DPISTOLS_MAX_SHOT_COUNT);
      }
      holdTicks = 0;
      multiplier = (int)clamp(multiplier - 5, 1, 99);
    }
  } else {
    holdTicks = 0;
  }
  player.pos.y += player.my * difficulty * (1 - holdTicks / 30);
  if ((player.pos.y < 0 && player.my < 0) ||
      (player.pos.y > 99 && player.my > 0)) {
    player.my *= -1;
  }
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (int)floor(ticks / 15) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = player.my == -1;
  character(pc, VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, difficulty * 2);
    thickness = 6;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, 1, s->angle);
    bool inRect = s->pos.x >= -3 && s->pos.x < 103 && s->pos.y >= -3 && s->pos.y < 103;
    s->isAlive = inRect;
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (aliveEnemyCount == 0) {
    nextEnemyTicks = 0;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    if (rnd(0, 1) < 0.3) {
      nextEnemyY = rnd(9, 90);
      prevEnemyVx *= -1;
    }
    Vector vel;
    vectorSet(&vel, prevEnemyVx, 0);
    vel.x *= rnd(1, difficulty) * 0.3;
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    float ex;
    if (vel.x > 0) {
      ex = -3;
    } else {
      ex = 103;
    }
    vectorSet(&ne->pos, ex, nextEnemyY);
    ne->vel = vel;
    ne->ticks = 0;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, DPISTOLS_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(20, 40) / difficulty;
  }
  color = RED;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    if ((e->pos.x > 50 && e->vel.x > 0.1) || (e->pos.x < 50 && e->vel.x < -0.1)) {
      e->pos.x = 50;
      if (player.pos.y < e->pos.y) {
        e->vel.y = -fabs(e->vel.x);
      } else {
        e->vel.y = fabs(e->vel.x);
      }
      e->vel.x *= 0.0001;
    }
    e->ticks++;
    char ec[2];
    ec[0] = 'c' + (int)floor(e->ticks / 15) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = e->vel.x < 0;
    Collision c = character(ec, VEC_XY(e->pos));
    characterOptions.isMirrorX = false;
    if (c.isColliding.rect[BLACK]) {
      play(HIT);
      multiplier = (int)clamp(multiplier + 1, 0, 99);
      addScore(multiplier, VEC_XY(e->pos));
      particle(e->pos.x, e->pos.y, 16, 1, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (c.isColliding.character['a'] || c.isColliding.character['b']) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameDPistols() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
