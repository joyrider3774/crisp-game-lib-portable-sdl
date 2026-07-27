#include "cglp.h"

static char *title = "INVINCIBLE MAN";
static char *description = "[Tap]\n Turn\n[Hold]\n Walk outward";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l   ",
        " llll ",
        "l l   ",
        " l ll ",
        "l     ",
    },
    {
        "   ll ",
        "  l   ",
        "llll  ",
        "  l   ",
        "ll l  ",
        "    l ",
    },
    {
        "ll ll ",
        "l  l  ",
        " llll ",
        "lllll ",
        "lllll ",
        "l l l ",
    },
    {
        " ll ll",
        " l  l ",
        " llll ",
        "lllll ",
        "lllll ",
        " l l  ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = true};

typedef struct {
  Vector pos;
  float va;
  float ticksVal;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector targetPos;
  Vector vel;
  float ticksVal;
  bool isAlive;
} Human;
#define INVINCIBLEMAN_MAX_HUMAN_COUNT 16
static Human humans[INVINCIBLEMAN_MAX_HUMAN_COUNT];
static int humanIndex;
static float nextHumanTicks;

typedef struct {
  Vector pos;
  Vector vel;
  float ticksVal;
  bool isAlive;
} Enemy;
#define INVINCIBLEMAN_MAX_ENEMY_COUNT 64
static Enemy enemies[INVINCIBLEMAN_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  float radius;
  float rv;
  bool isAlive;
} Explosion;
#define INVINCIBLEMAN_MAX_EXPLOSION_COUNT 16
static Explosion explosions[INVINCIBLEMAN_MAX_EXPLOSION_COUNT];
static int explosionIndex;

static int multiplier;

static int countHumans() {
  COUNT_IS_ALIVE(humans, c);
  return c;
}

static int nearestEnemyIndex(float px, float py) {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float d = distanceTo(&e->pos, px, py);
    if (best < 0 || d < bestDist) {
      best = i;
      bestDist = d;
    }
  }
  return best;
}

static int nearestHumanIndex(float px, float py) {
  int best = -1;
  float bestDist = 0;
  FOR_EACH(humans, i) {
    ASSIGN_ARRAY_ITEM(humans, i, Human, h);
    SKIP_IS_NOT_ALIVE(h);
    float d = distanceTo(&h->pos, px, py);
    if (best < 0 || d < bestDist) {
      best = i;
      bestDist = d;
    }
  }
  return best;
}

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 50, 40);
    player.va = -1;
    player.ticksVal = 0;
    INIT_UNALIVED_ARRAY(humans);
    TIMES(10, i) {
      vectorSet(&humans[i].pos, rnd(40, 60), rnd(40, 60));
      vectorSet(&humans[i].targetPos, rnd(40, 60), rnd(40, 60));
      vectorSet(&humans[i].vel, 0, 0);
      humans[i].ticksVal = rnd(0, 60);
      humans[i].isAlive = true;
    }
    humanIndex = 10 % INVINCIBLEMAN_MAX_HUMAN_COUNT;
    nextHumanTicks = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    multiplier = 1;
  }
  float sd = difficulty;
  float tc = angleTo(&player.pos, 50, 50);
  if (input.isJustPressed) {
    play(LASER);
    player.va *= -1;
  }
  float px = player.pos.x;
  if (input.isPressed) {
    addWithAngle(&player.pos, tc, -sd);
  } else {
    addWithAngle(&player.pos, tc + M_PI_2 * player.va, 0.7 * sd);
  }
  addWithAngle(&player.pos, tc, (distanceTo(&player.pos, 50, 50) + 9) * 0.005 * sd);
  player.pos.x = clamp(player.pos.x, 0, 100);
  player.pos.y = clamp(player.pos.y, 0, 100);
  player.ticksVal += sd;
  color = CYAN;
  if (player.pos.x > px) {
    characterOptions.isMirrorX = false;
  } else {
    characterOptions.isMirrorX = true;
  }
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  char plc[2];
  plc[0] = 'a' + ((int)(player.ticksVal / 30) % 2);
  plc[1] = 0;
  character(plc, VEC_XY(player.pos));
  color = RED;
  FOR_EACH(explosions, i) {
    ASSIGN_ARRAY_ITEM(explosions, i, Explosion, e);
    SKIP_IS_NOT_ALIVE(e);
    e->radius += e->rv * 0.6 * sd;
    if (e->rv > 0) {
      if (e->radius > 12) {
        e->rv = -1;
      }
    } else {
      if (e->radius < 1) {
        e->isAlive = false;
        continue;
      }
    }
    thickness = 3;
    arc(VEC_XY(e->pos), e->radius, 0, M_PI * 2);
  }
  nextHumanTicks -= sd;
  if (nextHumanTicks < 0) {
    if (countHumans() < 9) {
      Vector p;
      vectorSet(&p, 50, 50);
      int hc = 1;
      FOR_EACH(humans, i) {
        ASSIGN_ARRAY_ITEM(humans, i, Human, h);
        SKIP_IS_NOT_ALIVE(h);
        vectorAdd(&p, h->pos.x, h->pos.y);
        hc++;
      }
      vectorMul(&p, 1.0 / hc);
      ASSIGN_ARRAY_ITEM(humans, humanIndex, Human, nh);
      nh->pos = p;
      vectorSet(&nh->targetPos, rnd(40, 60), rnd(40, 60));
      vectorSet(&nh->vel, 0, 0);
      nh->ticksVal = rnd(0, 60);
      nh->isAlive = true;
      humanIndex = wrap(humanIndex + 1, 0, INVINCIBLEMAN_MAX_HUMAN_COUNT);
    }
    nextHumanTicks = 600;
  }
  color = BLACK;
  FOR_EACH(humans, i) {
    ASSIGN_ARRAY_ITEM(humans, i, Human, h);
    SKIP_IS_NOT_ALIVE(h);
    bool hasTa = false;
    float ta = 0;
    int nei = nearestEnemyIndex(h->pos.x, h->pos.y);
    if (nei >= 0) {
      Enemy *ne = &enemies[nei];
      if (distanceTo(&ne->pos, VEC_XY(h->pos)) < 25) {
        ta = angleTo(&ne->pos, VEC_XY(h->pos));
        hasTa = true;
      }
    }
    if (!hasTa) {
      if (distanceTo(&h->pos, VEC_XY(h->targetPos)) < 1) {
        vectorSet(&h->targetPos, rnd(40, 60), rnd(40, 60));
      }
      ta = angleTo(&h->pos, VEC_XY(h->targetPos));
    }
    addWithAngle(&h->vel, ta, 0.01);
    vectorMul(&h->vel, 0.9);
    float hpx = h->pos.x;
    vectorAdd(&h->pos, h->vel.x * sd, h->vel.y * sd);
    h->pos.x = clamp(h->pos.x, 10, 90);
    h->pos.y = clamp(h->pos.y, 10, 90);
    h->ticksVal += sd;
    if (h->pos.x > hpx) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    char hc2[2];
    hc2[0] = 'a' + ((int)(h->ticksVal / 30) % 2);
    hc2[1] = 0;
    Collision hcoll = character(hc2, VEC_XY(h->pos));
    if (hcoll.isColliding.rect[RED]) {
      play(EXPLOSION);
      particle(h->pos.x, h->pos.y, 9, 2, 0, M_PI * 2);
      h->isAlive = false;
      continue;
    }
  }
  nextEnemyTicks -= sd;
  if (nextEnemyTicks < 0) {
    Vector ep;
    vectorSet(&ep, 50, 50);
    addWithAngle(&ep, rnd(0, M_PI * 2), 80);
    int ec = rndi(3, 9);
    TIMES(ec, i) {
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      vectorSet(&ne->pos, ep.x + rnd(0, 9) * RNDPM(), ep.y + rnd(0, 9) * RNDPM());
      vectorSet(&ne->vel, rnd(0, 1 / sd) * RNDPM(), rnd(0, 1 / sd) * RNDPM());
      ne->ticksVal = rnd(0, 60);
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, INVINCIBLEMAN_MAX_ENEMY_COUNT);
    }
    nextEnemyTicks = rnd(70, 99) * sqrt(ec);
  }
  color = RED;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float epx = e->pos.x;
    if (countHumans() > 0) {
      int nhi = nearestHumanIndex(e->pos.x, e->pos.y);
      Human *nh = &humans[nhi];
      addWithAngle(&e->vel, angleTo(&e->pos, VEC_XY(nh->pos)), 0.005);
      vectorMul(&e->vel, 0.95);
      vectorAdd(&e->pos, e->vel.x * sd, e->vel.y * sd);
    }
    e->ticksVal += sd;
    if (e->pos.x > epx) {
      characterOptions.isMirrorX = false;
    } else {
      characterOptions.isMirrorX = true;
    }
    char ec2[2];
    ec2[0] = 'c' + ((int)(e->ticksVal / 30) % 2);
    ec2[1] = 0;
    Collision ecoll = character(ec2, VEC_XY(e->pos));
    if (ecoll.isColliding.rect[RED] || ecoll.isColliding.character['a'] ||
        ecoll.isColliding.character['b']) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(explosions, explosionIndex, Explosion, nex);
      nex->pos = e->pos;
      nex->radius = 1;
      nex->rv = 1;
      nex->isAlive = true;
      explosionIndex = wrap(explosionIndex + 1, 0, INVINCIBLEMAN_MAX_EXPLOSION_COUNT);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      e->isAlive = false;
      continue;
    }
  }
  characterOptions.isMirrorX = false;
  COUNT_IS_ALIVE(explosions, aliveExplosionCount);
  if (aliveExplosionCount == 0) {
    multiplier = 1;
  }
  if (countHumans() == 0) {
    play(RANDOM);
    gameOver();
  }
}

void addGameInvincibleMan() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
