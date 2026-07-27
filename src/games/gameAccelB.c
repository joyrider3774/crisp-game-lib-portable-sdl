#include "cglp.h"

static char *title = "ACCEL B";
static char *description = "[Tap]  Fire\n[Hold] Accel";

static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1] = {
    {   // Player 'a'
        "      ",
        "      ",
        "ll    ",
        " lllll",
        "      ",
        "      ",
    },
    {   // Enemy 'b'
        "      ",
        "   lll",
        " lllll",
        "llllll",
        "   ll ",
        "      ",
    }
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 200,
    .viewSizeY = 100,
    .soundSeed = 20,
    .isDarkColor = false
};

typedef struct {
  Vector pos;
  float vx;
} Player;

typedef struct {
  Vector pos;
  Vector size;
} Forest;
#define ACCELB_FOREST_COUNT 7
static Forest forests[ACCELB_FOREST_COUNT];

typedef struct {
  Vector pos;
  Vector vel;
  float ticks;
  bool isEnemy;
  bool isAlive;
} Smoke;
#define ACCELB_MAX_SMOKE_COUNT 128
static Smoke smokes[ACCELB_MAX_SMOKE_COUNT];
static int smokeIndex;

typedef struct {
  Vector pos;
  float vx;
  float ma;
  int fireTicks;
  bool isAlive;
} Enemy;
#define ACCELB_MAX_ENEMY_COUNT 32
static Enemy enemies[ACCELB_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;

typedef struct {
  Vector pos;
  float angle;
  float va;
  float speed;
  float smokeTicks;
  bool isAlive;
} Missile;
#define ACCELB_MAX_MISSILE_COUNT 32
static Missile missiles[ACCELB_MAX_MISSILE_COUNT];
static int missileIndex;

// Vircon32 port note: upstream's playerMissile.target is a live reference to
// an enemy/enemy-missile's Vector (so it keeps homing on that object's
// current position even after the object itself is later removed from its
// own array, since the referenced Vector still exists as long as the
// missile holds it). There are no live references here, so a missile
// instead stores which array/slot to keep reading from - this matches
// upstream as long as that array/slot isn't recycled by a new spawn before
// the missile finishes (enemies/enemy-missiles are capped well above what
// can realistically spawn within a missile's ~2 second homing lifetime, so
// that never happens in practice).
typedef struct {
  Vector pos;
  Vector vel;
  bool targetIsMissile;
  int targetIndex;
  float ticks;
  float exTicks;
  float smokeTicks;
  bool isAlive;
} PlayerMissile;
#define ACCELB_MAX_PLAYER_MISSILE_COUNT 64
static PlayerMissile playerMissiles[ACCELB_MAX_PLAYER_MISSILE_COUNT];
static int playerMissileIndex;

static Player player;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, 20, 45);
    player.vx = 0;
    INIT_UNALIVED_ARRAY(smokes);
    smokeIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 0;
    INIT_UNALIVED_ARRAY(missiles);
    missileIndex = 0;
    INIT_UNALIVED_ARRAY(playerMissiles);
    playerMissileIndex = 0;
    TIMES(ACCELB_FOREST_COUNT, fi) {
      vectorSet(&forests[fi].pos, -99 - fi * 40, 0);
      vectorSet(&forests[fi].size, 0, 0);
    }
    multiplier = 1;
  }

  float scr = (player.pos.x - 20) * 0.1;
  color = YELLOW;
  rect(0, 90, 200, 10);

  color = GREEN;
  TIMES(ACCELB_FOREST_COUNT, fi2) {
    Forest *f = &forests[fi2];
    box(f->pos.x, f->pos.y, f->size.x, f->size.y);
    f->pos.x -= scr;
    if (f->pos.x < -99) {
      f->pos.x += 300 + rnd(0, 99);
      f->pos.y = rnd(90, 99);
      f->size.x = rnd(30, 50);
      f->size.y = rnd(5, 9);
    }
  }

  FOR_EACH(smokes, si) {
    ASSIGN_ARRAY_ITEM(smokes, si, Smoke, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    s->pos.x -= scr;
    vectorMul(&s->vel, 0.95);
    if (s->isEnemy && s->ticks < 20) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_BLACK;
    }
    float ssize = 3 + cos(s->ticks * 0.03) * 5;
    box(s->pos.x, s->pos.y, ssize, ssize);
    s->ticks += sqrt(difficulty);
    if (s->ticks > 60) {
      s->isAlive = false;
    }
  }

  float pressAccel;
  if (input.isPressed) {
    pressAccel = 2;
  } else {
    pressAccel = 0.2;
  }
  player.vx += (pressAccel * sqrt(difficulty) - player.vx) * 0.2;
  player.pos.x += player.vx - scr;
  color = BLUE;
  character("a", player.pos.x, player.pos.y);
  color = PURPLE;
  rect(player.pos.x - 3, player.pos.y, -player.vx * 3, 1);

  COUNT_IS_ALIVE(playerMissiles, playerMissileCountBefore);
  if (playerMissileCountBefore == 0) {
    color = CYAN;
    box(player.pos.x, player.pos.y + 3, 5, 2);
    if (input.isJustPressed) {
      play(SELECT);
      multiplier = 1;
      FOR_EACH(enemies, ei) {
        ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
        SKIP_IS_NOT_ALIVE(e);
        ASSIGN_ARRAY_ITEM(playerMissiles, playerMissileIndex, PlayerMissile, pm);
        vectorSet(&pm->pos, player.pos.x, player.pos.y + 3);
        vectorSet(&pm->vel, sqrt(difficulty) * 2, 0);
        pm->targetIsMissile = false;
        pm->targetIndex = ei;
        pm->ticks = 0;
        pm->exTicks = 0;
        pm->smokeTicks = 0;
        pm->isAlive = true;
        playerMissileIndex = wrap(playerMissileIndex + 1, 0, ACCELB_MAX_PLAYER_MISSILE_COUNT);
      }
      FOR_EACH(missiles, mi) {
        ASSIGN_ARRAY_ITEM(missiles, mi, Missile, m);
        SKIP_IS_NOT_ALIVE(m);
        ASSIGN_ARRAY_ITEM(playerMissiles, playerMissileIndex, PlayerMissile, pm);
        vectorSet(&pm->pos, player.pos.x, player.pos.y + 3);
        vectorSet(&pm->vel, sqrt(difficulty) * 2, 0);
        pm->targetIsMissile = true;
        pm->targetIndex = mi;
        pm->ticks = 0;
        pm->exTicks = 0;
        pm->smokeTicks = 0;
        pm->isAlive = true;
        playerMissileIndex = wrap(playerMissileIndex + 1, 0, ACCELB_MAX_PLAYER_MISSILE_COUNT);
      }
    }
  }

  FOR_EACH(playerMissiles, pmi) {
    ASSIGN_ARRAY_ITEM(playerMissiles, pmi, PlayerMissile, pm);
    SKIP_IS_NOT_ALIVE(pm);
    vectorAdd(&pm->pos, pm->vel.x, pm->vel.y);
    pm->pos.x += sqrt(difficulty) - scr;

    if (pm->exTicks > 0) {
      if (pm->pos.y > 90) {
        vectorSet(&pm->vel, 0, 0);
      }
      pm->exTicks += sqrt(difficulty);
      vectorMul(&pm->vel, 0.9);
      color = RED;
      float esize = 3 + cos(pm->exTicks * 0.05) * 9;
      box(pm->pos.x, pm->pos.y, esize, esize);
      if (pm->exTicks > 30) {
        pm->isAlive = false;
      }
      continue;
    }

    float targetX;
    float targetY;
    if (pm->targetIsMissile) {
      targetX = missiles[pm->targetIndex].pos.x;
      targetY = missiles[pm->targetIndex].pos.y;
    } else {
      targetX = enemies[pm->targetIndex].pos.x;
      targetY = enemies[pm->targetIndex].pos.y;
    }
    float d = distanceTo(&pm->pos, targetX, targetY);
    if (d < 9 || pm->pos.y > 95 || pm->ticks > 120) {
      play(POWER_UP);
      pm->exTicks = 1;
      float s = vectorLength(&pm->vel);
      vectorSet(&pm->vel, 0, 0);
      addWithAngle(&pm->vel, angleTo(&pm->pos, targetX, targetY), s);
    }
    float speedFactor;
    if (pm->ticks < 9) {
      speedFactor = 0.1;
    } else if (pm->ticks < 20) {
      speedFactor = 3;
    } else {
      speedFactor = 1;
    }
    float mv = (sqrt(difficulty) / sqrt(d + 9)) * speedFactor;
    addWithAngle(&pm->vel, angleTo(&pm->pos, targetX, targetY), mv);
    if (pm->ticks < 20) {
      vectorMul(&pm->vel, 0.7);
    } else {
      vectorMul(&pm->vel, 0.95);
    }
    pm->ticks += sqrt(difficulty);
    color = CYAN;
    thickness = 2;
    barCenterPosRatio = 0.5;
    bar(pm->pos.x, pm->pos.y, 3, vectorAngle(&pm->vel));
    pm->smokeTicks += sqrt(difficulty);
    if (pm->smokeTicks > 5) {
      ASSIGN_ARRAY_ITEM(smokes, smokeIndex, Smoke, ns);
      vectorSet(&ns->pos, pm->pos.x, pm->pos.y);
      vectorSet(&ns->vel, pm->vel.x * 0.5, pm->vel.y * 0.5);
      ns->ticks = 0;
      ns->isEnemy = false;
      ns->isAlive = true;
      smokeIndex = wrap(smokeIndex + 1, 0, ACCELB_MAX_SMOKE_COUNT);
      pm->smokeTicks -= 5;
    }
  }
  COUNT_IS_ALIVE(playerMissiles, playerMissileCountAfter);
  if (playerMissileCountBefore > 0 && playerMissileCountAfter == 0) {
    play(COIN);
  }

  nextEnemyDist -= scr;
  if (nextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    float ey;
    if (rnd(0, 1) < 0.5) {
      ey = rnd(5, 35);
    } else {
      ey = rnd(55, 85);
    }
    vectorSet(&ne->pos, 203, ey);
    ne->vx = rnd(0, sqrt(difficulty)) * 0.5;
    ne->ma = M_PI + rnd(0, M_PI / 5) * RNDPM();
    ne->fireTicks = ceil(rnd(10, 30) / sqrt(difficulty));
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, ACCELB_MAX_ENEMY_COUNT);
    nextEnemyDist += rnd(40, 60) / difficulty;
  }

  FOR_EACH(enemies, ei2) {
    ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x -= e->vx + scr;
    e->fireTicks--;
    if (e->fireTicks == 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(missiles, missileIndex, Missile, nm);
      vectorSet(&nm->pos, e->pos.x, e->pos.y + 3);
      nm->angle = e->ma;
      nm->va = 0;
      nm->speed = sqrt(difficulty);
      nm->smokeTicks = 0;
      nm->isAlive = true;
      missileIndex = wrap(missileIndex + 1, 0, ACCELB_MAX_MISSILE_COUNT);
    }
    color = PURPLE;
    Collision ec = character("b", e->pos.x, e->pos.y);
    if (ec.isColliding.rect[RED]) {
      play(EXPLOSION);
      particle(e->pos.x, e->pos.y, 15, 2, 0, M_PI * 2);
      addScore(multiplier, e->pos.x, e->pos.y);
      if (multiplier < 64) {
        multiplier *= 2;
      }
      e->isAlive = false;
      continue;
    }
    if (e->fireTicks > 0) {
      color = BLACK;
      thickness = 3;
      barCenterPosRatio = 0.5;
      bar(e->pos.x, e->pos.y + 3, 3, e->ma);
    }
    if (e->pos.x < -3) {
      e->isAlive = false;
      continue;
    }
  }

  float vva = sqrt(difficulty) * 0.0005;
  FOR_EACH(missiles, mi2) {
    ASSIGN_ARRAY_ITEM(missiles, mi2, Missile, m);
    SKIP_IS_NOT_ALIVE(m);
    addWithAngle(&m->pos, m->angle, m->speed);
    m->pos.x -= scr;
    float ta = angleTo(&m->pos, player.pos.x, player.pos.y);
    float oy = wrap(ta - m->angle, -M_PI, M_PI);
    if (oy > 0) {
      m->va += vva;
    } else {
      m->va -= vva;
    }
    m->angle = clamp(wrap(m->angle + m->va, 0, M_PI * 2), (M_PI / 4) * 3, (M_PI / 4) * 5);
    color = BLACK;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision c = bar(m->pos.x, m->pos.y, 3, m->angle);
    m->smokeTicks += sqrt(difficulty);
    if (m->smokeTicks > 9) {
      ASSIGN_ARRAY_ITEM(smokes, smokeIndex, Smoke, ns2);
      vectorSet(&ns2->pos, m->pos.x, m->pos.y);
      Vector smokeVel;
      vectorSet(&smokeVel, 0, 0);
      addWithAngle(&smokeVel, m->angle, m->speed * 0.3);
      ns2->vel = smokeVel;
      ns2->ticks = 0;
      ns2->isEnemy = true;
      ns2->isAlive = true;
      smokeIndex = wrap(smokeIndex + 1, 0, ACCELB_MAX_SMOKE_COUNT);
      m->smokeTicks -= 9;
    }
    color = RED;
    if (c.isColliding.rect[RED]) {
      play(HIT);
      particle(m->pos.x, m->pos.y, 9, 2, 0, M_PI * 2);
      addScore(multiplier, m->pos.x, m->pos.y);
      if (multiplier < 64) {
        multiplier *= 2;
      }
      m->isAlive = false;
      continue;
    } else if (c.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    if (m->pos.y > 90) {
      particle(m->pos.x, m->pos.y, 16, 1, 0, M_PI * 2);
      m->isAlive = false;
      continue;
    }
    if (m->pos.x < -3) {
      m->isAlive = false;
      continue;
    }
  }
}

void addGameAccelB() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
