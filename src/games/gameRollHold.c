#include "cglp.h"

static char *title = "ROLL HOLD";
static char *description = "[Hold]\n Hold an angle";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " ll   ",
        "lll   ",
        "lllll ",
        " bb   ",
        "  cc  ",
        " bb   ",
    },
    {
        "lll   ",
        "rr    ",
        "llrrll",
        "LLRRll",
        "rr    ",
        "lll   ",
    },
    {
        "lrl   ",
        "llrrll",
        "LLRRll",
        "lrl   ",
        "      ",
        "      ",
    },
    {
        "lrlLll",
        "lrlLll",
        "      ",
        "      ",
        "      ",
        "      ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = true};

#define ROLLHOLD_TURRET_RADIUS 12

static Vector playerPos;
static float turretAngle;
static float turretVa;
static float fireTicks;
static float animTicks;

static int enemyFrameOffsets[4] = {0, 1, 2, 1};

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} Shot;
#define ROLLHOLD_MAX_SHOT_COUNT 32
static Shot shots[ROLLHOLD_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  float vx;
  float score;
  bool isFired;
  bool isAlive;
} Enemy;
#define ROLLHOLD_MAX_ENEMY_COUNT 48
static Enemy enemies[ROLLHOLD_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  float vx;
} NextEnemy;
static NextEnemy nextEnemy;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Bullet;
#define ROLLHOLD_MAX_BULLET_COUNT 32
static Bullet bullets[ROLLHOLD_MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  Vector size;
  bool isAlive;
} Building;
#define ROLLHOLD_MAX_BUILDING_COUNT 32
static Building buildings[ROLLHOLD_MAX_BUILDING_COUNT];
static int buildingIndex;
static float nextBuildingTicks;

static void setNextEnemy() {
  float vx = sqrt(difficulty) * RNDPM() * 0.4;
  float ex;
  if (vx > 0) {
    ex = -3;
  } else {
    ex = 103;
  }
  vectorSet(&nextEnemy.pos, ex, rnd(9, 90));
  nextEnemy.vx = vx;
}

static void update() {
  if (!ticks) {
    vectorSet(&playerPos, 50, 50);
    turretAngle = 0;
    turretVa = 1;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    fireTicks = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    vectorSet(&nextEnemy.pos, 0, 0);
    nextEnemy.vx = 0;
    setNextEnemy();
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    INIT_UNALIVED_ARRAY(buildings);
    buildingIndex = 0;
    nextBuildingTicks = 0;
    animTicks = 0;
  }

  float df = sqrt(difficulty);
  nextBuildingTicks--;
  if (nextBuildingTicks <= 0) {
    ASSIGN_ARRAY_ITEM(buildings, buildingIndex, Building, nb);
    vectorSet(&nb->pos, 100, 85);
    vectorSet(&nb->size, rndi(10, 20), -rndi(30, 60));
    nb->isAlive = true;
    buildingIndex = wrap(buildingIndex + 1, 0, ROLLHOLD_MAX_BUILDING_COUNT);
    nextBuildingTicks = rndi(5, 50) * 10;
  }

  FOR_EACH(buildings, bi) {
    ASSIGN_ARRAY_ITEM(buildings, bi, Building, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x -= 0.1;
    color = LIGHT_BLACK;
    rect(VEC_XY(b->pos), VEC_XY(b->size));
    color = WHITE;
    rect(b->pos.x + 1, b->pos.y, b->size.x - 2, b->size.y + 1);
    if (b->pos.x + b->size.x < 0) {
      b->isAlive = false;
      continue;
    }
  }

  color = LIGHT_CYAN;
  rect(0, 85, 100, 20);
  color = LIGHT_PURPLE;
  rect(0, 70, 100, 9);
  rect(0, 92, 100, 3);

  animTicks += df;
  if (input.isJustPressed) {
    play(SELECT);
    turretVa *= -1;
    fireTicks = 0;
  }

  Vector tp;
  vectorSet(&tp, ROLLHOLD_TURRET_RADIUS, 0);
  rotate(&tp, turretAngle);
  vectorAdd(&tp, VEC_XY(playerPos));

  color = LIGHT_CYAN;
  if (!input.isPressed) {
    turretAngle = wrap(turretAngle + turretVa * 0.07 * df, -M_PI, M_PI);
  } else {
    fireTicks -= df;
    if (fireTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
      vectorSet(&ns->pos, VEC_XY(tp));
      ns->angle = turretAngle;
      ns->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, ROLLHOLD_MAX_SHOT_COUNT);
      fireTicks = 9;
      particle(VEC_XY(tp), 3, 1, turretAngle, 0.5);
    }
    thickness = 1;
    barCenterPosRatio = -0.5;
    float gaugeAngle;
    if (turretVa > 0) {
      gaugeAngle = turretAngle + M_PI_2;
    } else {
      gaugeAngle = turretAngle - M_PI_2;
    }
    bar(VEC_XY(tp), 4, gaugeAngle);
  }

  color = CYAN;
  FOR_EACH(shots, si) {
    ASSIGN_ARRAY_ITEM(shots, si, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, df * 2);
    thickness = 2;
    barCenterPosRatio = 0.5;
    bar(VEC_XY(s->pos), 3, s->angle);
    if (!(s->pos.x >= -9 && s->pos.x <= 111 && s->pos.y >= -9 && s->pos.y <= 111)) {
      s->isAlive = false;
      continue;
    }
  }

  color = BLACK;
  characterOptions.isMirrorX = (turretAngle < -M_PI_2 || turretAngle > M_PI_2);
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  character("a", VEC_XY(playerPos));

  color = BLUE;
  thickness = 3;
  barCenterPosRatio = 0.5;
  bar(VEC_XY(tp), 2, turretAngle);

  nextEnemyTicks -= df;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, VEC_XY(nextEnemy.pos));
    ne->vx = nextEnemy.vx;
    ne->score = 9;
    ne->isFired = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, ROLLHOLD_MAX_ENEMY_COUNT);
    if (rnd(0, 1) < 0.25) {
      setNextEnemy();
      nextEnemyTicks = 120 / df;
    } else {
      nextEnemyTicks = 25 / df;
    }
  }

  color = BLACK;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x += e->vx;
    if (!e->isFired && ((e->vx > 0 && e->pos.x > 90) || (e->vx < 0 && e->pos.x < 9))) {
      play(CLICK);
      float a = angleTo(&e->pos, VEC_XY(playerPos));
      ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nbul);
      vectorSet(&nbul->pos, VEC_XY(e->pos));
      vectorSet(&nbul->vel, df * 0.3, 0);
      rotate(&nbul->vel, a);
      nbul->isAlive = true;
      bulletIndex = wrap(bulletIndex + 1, 0, ROLLHOLD_MAX_BULLET_COUNT);
      particle(VEC_XY(e->pos), 3, 2, a, 0.2);
      e->isFired = true;
    }
    int frame = enemyFrameOffsets[(int)(animTicks / 20) % 4];
    char enemyChar[2];
    enemyChar[0] = 'b' + frame;
    enemyChar[1] = 0;
    characterOptions.isMirrorX = !(e->vx > 0);
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    Collision cc = character(enemyChar, VEC_XY(e->pos));
    if (cc.isColliding.rect[CYAN] || cc.isColliding.rect[BLUE]) {
      play(POWER_UP);
      float scoreValue;
      if (cc.isColliding.rect[BLUE]) {
        scoreValue = 10;
      } else {
        scoreValue = ceil(e->score);
      }
      addScore(scoreValue, VEC_XY(e->pos));
      particle(VEC_XY(e->pos), 16, 1, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    e->score -= 0.033 * df;
    if (e->pos.x < -5 || e->pos.x > 105) {
      e->isAlive = false;
      continue;
    }
  }

  color = RED;
  FOR_EACH(bullets, bui) {
    ASSIGN_ARRAY_ITEM(bullets, bui, Bullet, bul);
    SKIP_IS_NOT_ALIVE(bul);
    vectorAdd(&bul->pos, VEC_XY(bul->vel));
    Collision bc = box(VEC_XY(bul->pos), 4, 4);
    if (bc.isColliding.rect[BLUE]) {
      play(POWER_UP);
      addScore(10, VEC_XY(bul->pos));
      particle(VEC_XY(bul->pos), 16, 1, 0, M_PI * 2);
      bul->isAlive = false;
      continue;
    } else if (bc.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
  }
}

void addGameRollHold() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
