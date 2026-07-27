#include "cglp.h"
#include "string.h"

static char *title = "TROJAN DEFENSE";
static char *description = "[Tap]\n Change Direction\n[Hold]\n Extend Shield";

// Character 5 ('e') is a placeholder: upstream references it via addWithCharCode but never defines it.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "   ll ",
        "   l  ",
        " lLll ",
        "llllll",
        "l    l",
        "l    l",
    },
    {
        "   ll ",
        "   l  ",
        " lLll ",
        "llllll",
        "l    l",
        "l  l  ",
    },
    {
        "  rrr ",
        " rrrrr",
        "rrrrrr",
        " rrrrr",
        "  rrr ",
        "   r  ",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
    {
        "      ",
        " rrrr ",
        "rrrrrr",
        "rrrrrr",
        " rrrr ",
        "      ",
    },
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = true};

typedef struct {
  Vector pos;
  float radius;
} Horse;
static Horse horse;

typedef struct {
  float angle;
  float length;
  float arcAngle;
  float rotationSpeed;
  int rotationDirection;
  bool extending;
} Shield;
static Shield shield;

typedef struct {
  Vector pos;
  int type; // 0, 1, or 2 - selects sprite 'c'/'d'/'e' and fire stats
  float fireRate;
  float nextFire;
  float fireSpeed;
  bool isAlive;
} Enemy;
// Enemies only die by deflecting one of their OWN shots back with the
// shield (no offscreen/timeout expiry), and new waves spawn on their own
// timer (rnd(180,300)/sqrt(difficulty), shrinking) regardless of whether
// the previous wave was cleared - a player who can't out-deflect the
// spawn rate keeps accumulating enemies across waves, so sized with real
// headroom above the original 64.
#define TROJANDEFENSE_MAX_ENEMY_COUNT 256
static Enemy enemies[TROJANDEFENSE_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isDeflected;
  int deflectedTicks;
  bool isAlive;
} Projectile;
// Scales with the enemy count above - more surviving enemies each firing
// periodically means more concurrent projectiles too.
#define TROJANDEFENSE_MAX_PROJECTILE_COUNT 384
static Projectile projectiles[TROJANDEFENSE_MAX_PROJECTILE_COUNT];
static int projectileIndex;

typedef struct {
  Vector pos;
  float radius;
  int duration;
  int ticks;
  bool isAlive;
} Explosion;
#define TROJANDEFENSE_MAX_EXPLOSION_COUNT 32
static Explosion explosions[TROJANDEFENSE_MAX_EXPLOSION_COUNT];
static int explosionIndex;

static int multiplier;
static int waveCount;
static float nextEnemyTicks;

static void update() {
  if (!ticks) {
    vectorSet(&horse.pos, 50, 50);
    horse.radius = 5;
    shield.angle = 0;
    shield.length = 10;
    shield.arcAngle = M_PI / 8;
    shield.rotationSpeed = 0.06;
    shield.rotationDirection = 1;
    shield.extending = false;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(projectiles);
    projectileIndex = 0;
    INIT_UNALIVED_ARRAY(explosions);
    explosionIndex = 0;
    multiplier = 1;
    waveCount = 0;
    nextEnemyTicks = 60;
  }

  color = LIGHT_BLACK;
  thickness = 2;
  arc(horse.pos.x, horse.pos.y, 60, 0, M_PI * 2);

  if (input.isJustPressed) {
    play(SELECT);
    shield.rotationDirection *= -1;
    multiplier = 1;
  }
  if (input.isPressed) {
    shield.extending = true;
    if (shield.arcAngle < M_PI / 3) {
      shield.arcAngle += 0.02;
    }
    if (shield.length < 20) {
      shield.length += 0.5;
    }
  } else {
    shield.extending = false;
    if (shield.arcAngle > M_PI / 8) {
      shield.arcAngle -= 0.015;
    }
    if (shield.length > 10) {
      shield.length -= 0.3;
    }
  }
  float speedFactor;
  if (shield.extending) {
    speedFactor = 1 - (shield.arcAngle - M_PI / 8) / (M_PI / 3 - M_PI / 8) * 0.7;
  } else {
    speedFactor = 1.2;
  }
  shield.angle += shield.rotationDirection * shield.rotationSpeed * speedFactor * difficulty;

  color = BLACK;
  if (shield.extending) {
    character("b", VEC_XY(horse.pos));
  } else {
    character("a", VEC_XY(horse.pos));
  }

  color = CYAN;
  float shieldStartAngle = shield.angle - shield.arcAngle / 2;
  float shieldEndAngle = shield.angle + shield.arcAngle / 2;
  thickness = 1;
  for (float r = horse.radius; r <= horse.radius + shield.length; r += 1.5) {
    arc(horse.pos.x, horse.pos.y, r, shieldStartAngle, shieldEndAngle);
  }

  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    waveCount++;
    int enemyCount = (int)floor(1 + waveCount / 3.0);
    if (enemyCount > 5) {
      enemyCount = 5;
    }
    float baseAngle = rnd(0, M_PI * 2);
    TIMES(enemyCount, wi) {
      float angleVariation = rnd(-M_PI / 4, M_PI / 4);
      float angle = baseAngle + angleVariation;
      float edistance = 50;
      Vector enemyPos;
      vectorSet(&enemyPos, horse.pos.x + cos(angle) * edistance,
                horse.pos.y + sin(angle) * edistance);
      int enemyType;
      if (rnd(0, 1) < 0.7) {
        enemyType = 0;
      } else if (rnd(0, 1) < 0.5) {
        enemyType = 1;
      } else {
        enemyType = 2;
      }
      float fireRate;
      float fireSpeed;
      if (enemyType == 0) {
        fireRate = rnd(120, 180) / sqrt(difficulty);
        fireSpeed = 0.5 * sqrt(difficulty);
      } else if (enemyType == 1) {
        fireRate = rnd(80, 120) / sqrt(difficulty);
        fireSpeed = 0.8 * sqrt(difficulty);
      } else {
        fireRate = rnd(150, 200) / sqrt(difficulty);
        fireSpeed = 0.4 * sqrt(difficulty);
      }
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      ne->pos = enemyPos;
      ne->type = enemyType;
      ne->fireRate = fireRate;
      ne->nextFire = rnd(30, 60);
      ne->fireSpeed = fireSpeed;
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, TROJANDEFENSE_MAX_ENEMY_COUNT);
    }
    nextEnemyTicks = rnd(180, 300) / sqrt(difficulty);
  }

  color = LIGHT_RED;
  FOR_EACH(explosions, exi) {
    ASSIGN_ARRAY_ITEM(explosions, exi, Explosion, ex);
    SKIP_IS_NOT_ALIVE(ex);
    ex->ticks--;
    float radiusRatio = (float)ex->ticks / (float)ex->duration;
    float er = ex->radius * sin(radiusRatio * M_PI);
    thickness = 3;
    arc(ex->pos.x, ex->pos.y, er, 0, M_PI * 2);
    if (ex->ticks < 0) {
      ex->isAlive = false;
      continue;
    }
  }

  FOR_EACH(projectiles, projIdx) {
    ASSIGN_ARRAY_ITEM(projectiles, projIdx, Projectile, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    color = RED;
    Collision pc = box(p->pos.x, p->pos.y, 3, 3);
    if (p->isDeflected) {
      color = YELLOW;
      box(p->pos.x, p->pos.y, 2.5, 2.5);
    }
    if (!p->isDeflected) {
      if (pc.isColliding.rect[CYAN]) {
        p->isDeflected = true;
        p->deflectedTicks = 0;
        play(COIN);
        float angleToProjectile = angleTo(&horse.pos, p->pos.x, p->pos.y);
        float reflectionAngle = angleToProjectile;
        float speed = vectorLength(&p->vel) * 1.5;
        vectorSet(&p->vel, speed, 0);
        rotate(&p->vel, reflectionAngle);
        particle(p->pos.x, p->pos.y, 5, 1, reflectionAngle, M_PI / 4);
        continue;
      }
      if (pc.isColliding.character['a'] || pc.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
        p->isAlive = false;
        continue;
      }
    } else {
      p->deflectedTicks++;
      if (pc.isColliding.character['c'] || pc.isColliding.character['d'] || pc.isColliding.character['e']) {
        p->isAlive = false;
        continue;
      }
    }
    float distanceFromCenter = distanceTo(&p->pos, horse.pos.x, horse.pos.y);
    if (distanceFromCenter > 55) {
      p->isAlive = false;
      continue;
    }
  }

  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = RED;
    char enemyChar[2];
    enemyChar[0] = 'c' + e->type;
    enemyChar[1] = 0;
    Collision ec = character(enemyChar, VEC_XY(e->pos));
    bool isHit = ec.isColliding.rect[YELLOW];
    if (isHit) {
      play(POWER_UP);
      addScore(multiplier * (e->type + 1) * 10, VEC_XY(e->pos));
      multiplier++;
      ASSIGN_ARRAY_ITEM(explosions, explosionIndex, Explosion, nex);
      nex->pos = e->pos;
      nex->radius = 8;
      nex->duration = 20;
      nex->ticks = 20;
      nex->isAlive = true;
      explosionIndex = wrap(explosionIndex + 1, 0, TROJANDEFENSE_MAX_EXPLOSION_COUNT);
      particle(e->pos.x, e->pos.y, 15, 2, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    e->nextFire--;
    if (e->nextFire <= 0) {
      float fireAngle = angleTo(&e->pos, horse.pos.x, horse.pos.y);
      ASSIGN_ARRAY_ITEM(projectiles, projectileIndex, Projectile, np);
      np->pos = e->pos;
      vectorSet(&np->vel, e->fireSpeed, 0);
      rotate(&np->vel, fireAngle);
      np->isDeflected = false;
      np->deflectedTicks = 0;
      np->isAlive = true;
      projectileIndex = wrap(projectileIndex + 1, 0, TROJANDEFENSE_MAX_PROJECTILE_COUNT);
      play(LASER);
      e->nextFire = e->fireRate * rnd(0.8, 1.2);
    }
  }

  if (multiplier > 1) {
    color = YELLOW;
    char multText[16];
    strcpy(multText, "x");
    strcat(multText, intToChar(multiplier));
    text(multText, 3, 9);
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (aliveEnemyCount == 0 && ticks > 100) {
    difficulty += 0.0005;
  }
}

void addGameTrojanDefense() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
