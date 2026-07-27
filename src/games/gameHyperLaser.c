#include "cglp.h"

static char *title = "HYPER LASER";
static char *description = "[Hold] Laser";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "LllllL",
        "lLLLLl",
        "lLLLLl",
        "lLLLLl",
        "lLLLLl",
        "LllllL",
    },
    {
        "  l   ",
        "  l   ",
        "ll ll ",
        "  l   ",
        "  l   ",
            "      ",
},
    {
        " yy   ",
        "ylly  ",
        "ylly  ",
        " yy   ",
            "      ",
        "      ",
},
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 20, .isDarkColor = false};

static int laserColors[4] = {BLUE, PURPLE, CYAN, GREEN};

typedef struct {
  Vector pos;
  Vector vel;
  float ticks;
  bool isAlive;
} Laser;
#define HYPERLASER_MAX_LASER_COUNT 32
static Laser lasers[HYPERLASER_MAX_LASER_COUNT];
static int laserIndex;

typedef struct {
  Vector pos;
  bool isAlive;
} Block;
#define HYPERLASER_MAX_BLOCK_COUNT 64
static Block blocks[HYPERLASER_MAX_BLOCK_COUNT];
static int blockIndex;
static float nextBlockDist;

typedef struct {
  Vector pos;
  float angle;
  float laserTicks;
  bool hasShield;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float angle;
  float targetAngle;
  float turretAngle;
  float burstTicks;
  int burstIndex;
  int burstCount;
  float shotTicks;
  float shotInterval;
  float turnTicks;
  bool isAlive;
} Enemy;
#define HYPERLASER_MAX_ENEMY_COUNT 32
static Enemy enemies[HYPERLASER_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Shot;
#define HYPERLASER_MAX_SHOT_COUNT 32
static Shot shots[HYPERLASER_MAX_SHOT_COUNT];
static int shotIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(lasers);
    laserIndex = 0;
    INIT_UNALIVED_ARRAY(blocks);
    blockIndex = 0;
    nextBlockDist = 0;
    vectorSet(&player.pos, 50, 95);
    player.angle = -M_PI_2;
    player.laserTicks = 0;
    player.hasShield = false;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    nextEnemyDist = 0;
    multiplier = 1;
  }
  color = LIGHT_CYAN;
  rect(0, 0, 5, 100);
  rect(95, 0, 5, 100);
  float scr = difficulty * 0.1;
  nextBlockDist -= scr;
  if (nextBlockDist < 0) {
    ASSIGN_ARRAY_ITEM(blocks, blockIndex, Block, nb);
    vectorSet(&nb->pos, rnd(8, 92), -9);
    nb->isAlive = true;
    blockIndex = wrap(blockIndex + 1, 0, HYPERLASER_MAX_BLOCK_COUNT);
    nextBlockDist = rnd(0, 16);
  }
  color = RED;
  Vector sp;
  vectorSet(&sp, clamp(input.pos.x, 1, 99), clamp(input.pos.y, 1, 99));
  character("b", VEC_XY(sp));
  float ta = angleTo(&player.pos, VEC_XY(sp));
  if (input.isPressed) {
    player.hasShield = false;
    player.laserTicks--;
    if (player.laserTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(lasers, laserIndex, Laser, nl);
      nl->pos = player.pos;
      vectorSet(&nl->vel, 0, 0);
      addWithAngle(&nl->vel, ta, sqrt(difficulty) * 3);
      nl->ticks = 0;
      nl->isAlive = true;
      laserIndex = wrap(laserIndex + 1, 0, HYPERLASER_MAX_LASER_COUNT);
      player.laserTicks = 9 / sqrt(difficulty);
    }
  } else {
    player.hasShield = true;
    float x = clamp(input.pos.x, 7, 93);
    float s = sqrt(difficulty) * 0.3;
    if (player.pos.x < x - 1) {
      player.pos.x += s;
      player.angle += wrap(-player.angle, -M_PI, M_PI) * 0.1;
    }
    if (player.pos.x > x + 1) {
      player.pos.x -= s;
      player.angle += wrap(M_PI - player.angle, -M_PI, M_PI) * 0.1;
    }
    color = LIGHT_BLUE;
    thickness = 2;
    arc(VEC_XY(player.pos), 5, ticks * 0.03, ticks * 0.1 + M_PI);
    arc(VEC_XY(player.pos), 5, ticks * 0.03 + M_PI, ticks * 0.1 + M_PI * 2);
  }
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(player.pos), 1, player.angle);
  Vector p;
  vectorSet(&p, player.pos.x, player.pos.y);
  color = LIGHT_BLACK;
  addWithAngle(&p, player.angle + M_PI_2, 2);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(p), 4, player.angle);
  addWithAngle(&p, player.angle + M_PI_2, -4);
  thickness = 2;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(p), 4, player.angle);
  color = RED;
  thickness = 2;
  barCenterPosRatio = -0.5;
  bar(VEC_XY(player.pos), 2, ta);
  color = BLACK;
  FOR_EACH(blocks, i) {
    ASSIGN_ARRAY_ITEM(blocks, i, Block, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y += scr;
    Collision bc = character("a", VEC_XY(b->pos));
    if (bc.isColliding.rect[BLACK]) {
      particle(b->pos.x, b->pos.y, 16, 1, 0, M_PI * 2);
      b->isAlive = false;
      continue;
    }
    bool removeOffTop = bc.isColliding.character['a'] && b->pos.y < -3;
    if (removeOffTop || b->pos.y > 103) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(lasers, i) {
    ASSIGN_ARRAY_ITEM(lasers, i, Laser, l);
    SKIP_IS_NOT_ALIVE(l);
    l->pos.y += scr;
    Vector pp;
    vectorSet(&pp, l->pos.x, l->pos.y);
    color = TRANSPARENT;
    vectorAdd(&l->pos, l->vel.x, l->vel.y);
    bool isColliding = false;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc1 = bar(l->pos.x, pp.y, 5, vectorAngle(&l->vel));
    if (lc1.isColliding.character['a'] || (l->pos.x < 5 && l->vel.x < 0) ||
        (l->pos.x > 95 && l->vel.x > 0)) {
      l->vel.x *= -1;
      isColliding = true;
    }
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc2 = bar(pp.x, l->pos.y, 5, vectorAngle(&l->vel));
    if (lc2.isColliding.character['a']) {
      l->vel.y *= -1;
      isColliding = true;
    }
    if (!isColliding) {
      thickness = 2;
      barCenterPosRatio = 0.5;
      Collision lc3 = bar(VEC_XY(l->pos), 5, vectorAngle(&l->vel));
      if (lc3.isColliding.character['a']) {
        l->vel.x *= -1;
        l->vel.y *= -1;
        isColliding = true;
      }
    }
    if (isColliding) {
      l->pos = pp;
      vectorAdd(&l->pos, l->vel.x, l->vel.y);
    }
    color = laserColors[(int)l->ticks % 4];
    l->ticks++;
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc4 = bar(VEC_XY(l->pos), 5, vectorAngle(&l->vel));
    if (lc4.isColliding.character['a']) {
      l->isAlive = false;
      continue;
    }
    if (l->pos.y < -5 || l->pos.y > 105 || l->ticks > 120 / sqrt(difficulty)) {
      l->isAlive = false;
      continue;
    }
  }
  COUNT_IS_ALIVE(enemies, aliveEnemyCount0);
  if (aliveEnemyCount0 == 0) {
    // Matches the JS source exactly: it resets nextBlockDist (not
    // nextEnemyDist) here - likely an upstream quirk, preserved as-is.
    nextBlockDist = 0;
  }
  nextEnemyDist--;
  if (nextEnemyDist < 0) {
    float shotInterval = rnd(60, 120) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, rnd(10, 90), -9);
    ne->angle = M_PI_2;
    ne->targetAngle = M_PI_2;
    ne->turretAngle = M_PI_2;
    ne->burstTicks = 9999;
    ne->burstIndex = 0;
    ne->burstCount = rndi(3, 9);
    ne->shotTicks = rnd(0, shotInterval);
    ne->shotInterval = shotInterval;
    ne->turnTicks = 0;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, HYPERLASER_MAX_ENEMY_COUNT);
    nextEnemyDist = rnd(150, 200) / sqrt(difficulty);
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += scr;
    Vector pp;
    vectorSet(&pp, e->pos.x, e->pos.y);
    bool isMoving = false;
    if (fabs(wrap(e->angle - e->targetAngle, -M_PI, M_PI)) < 0.1) {
      e->angle = e->targetAngle;
      addWithAngle(&e->pos, e->angle, sqrt(difficulty) * 0.1);
      isMoving = true;
      e->turnTicks--;
    } else {
      e->angle += wrap(e->targetAngle - e->angle, -M_PI, M_PI) * 0.1;
    }
    color = YELLOW;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision ec = bar(VEC_XY(e->pos), 2, e->angle);
    Vector ep;
    vectorSet(&ep, e->pos.x, e->pos.y);
    color = LIGHT_BLACK;
    addWithAngle(&ep, e->angle + M_PI_2, 2);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(VEC_XY(ep), 4, e->angle);
    addWithAngle(&ep, e->angle + M_PI_2, -4);
    thickness = 1;
    barCenterPosRatio = 0.5;
    bar(VEC_XY(ep), 4, e->angle);
    e->shotTicks--;
    bool hasShield = e->shotTicks > 0 && e->pos.y < 70;
    if (hasShield) {
      e->turretAngle =
          round(angleTo(&e->pos, VEC_XY(player.pos)) / (M_PI_4)) * M_PI_4;
    }
    color = RED;
    thickness = 2;
    barCenterPosRatio = -0.5;
    bar(VEC_XY(e->pos), 2, e->turretAngle);
    if (e->shotTicks < 0 && e->burstIndex <= 0) {
      e->burstTicks = 0;
      e->burstIndex = e->burstCount;
    }
    e->burstTicks--;
    if (e->burstTicks < 0) {
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
      ns->pos = e->pos;
      vectorSet(&ns->vel, 0, 0);
      addWithAngle(&ns->vel, e->turretAngle, sqrt(difficulty) * 2);
      ns->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, HYPERLASER_MAX_SHOT_COUNT);
      e->burstIndex--;
      if (e->burstIndex > 0) {
        e->burstTicks = 20 / sqrt(difficulty);
      } else {
        e->burstTicks = 9999;
        e->shotTicks = e->shotInterval;
      }
    }
    if (isMoving && (ec.isColliding.character['a'] || e->turnTicks < 0 || e->pos.x < 9 ||
                     e->pos.x > 90)) {
      e->pos = pp;
      e->targetAngle = wrap(e->targetAngle + (rndi(0, 2) * 2 - 1) * M_PI_2, -M_PI, M_PI);
      e->turnTicks = rnd(200, 300) / sqrt(difficulty);
    }
    color = LIGHT_YELLOW;
    if (hasShield) {
      thickness = 2;
      arc(VEC_XY(e->pos), 4, ticks * 0.03, ticks * 0.1 + M_PI);
      arc(VEC_XY(e->pos), 4, ticks * 0.03 + M_PI, ticks * 0.1 + M_PI * 2);
    } else if (ec.isColliding.rect[BLUE] || ec.isColliding.rect[PURPLE] ||
               ec.isColliding.rect[CYAN] || ec.isColliding.rect[GREEN]) {
      play(EXPLOSION);
      addScore(multiplier * 10, VEC_XY(e->pos));
      color = RED;
      particle(e->pos.x, e->pos.y, 19, 2, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->pos.y > 99) {
      color = RED;
      text("X", e->pos.x, 96);
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  color = TRANSPARENT;
  FOR_EACH(blocks, i) {
    ASSIGN_ARRAY_ITEM(blocks, i, Block, b);
    SKIP_IS_NOT_ALIVE(b);
    Collision bc2 = character("a", VEC_XY(b->pos));
    if (bc2.isColliding.rect[YELLOW] && b->pos.y < -3) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(lasers, i) {
    ASSIGN_ARRAY_ITEM(lasers, i, Laser, l);
    SKIP_IS_NOT_ALIVE(l);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision lc5 = bar(VEC_XY(l->pos), 5, vectorAngle(&l->vel));
    if (lc5.isColliding.rect[LIGHT_YELLOW]) {
      play(HIT);
      l->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    s->pos.y += scr;
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    Collision sc = character("c", VEC_XY(s->pos));
    if (!player.hasShield && sc.isColliding.rect[BLACK]) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    if (player.hasShield && sc.isColliding.rect[LIGHT_BLUE]) {
      play(POWER_UP);
      addScore(multiplier, VEC_XY(player.pos));
      multiplier++;
      s->isAlive = false;
      continue;
    }
    bool inRect = s->pos.x >= 5 && s->pos.x < 95 && s->pos.y >= 0 && s->pos.y < 100;
    if (!inRect || sc.isColliding.character['a']) {
      s->isAlive = false;
      continue;
    }
  }
}

void addGameHyperLaser() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
