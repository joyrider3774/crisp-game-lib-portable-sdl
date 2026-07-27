#include "cglp.h"
#include "string.h"

static char *title = "ZONE B";
static char *description = "[Tap]  Turn\n[Hold] Shot";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "l     ",
        "ll    ",
        "ll    ",
        "l     ",
        "      ",
        "      ",
    },
    {
        "llll  ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        "lll   ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        "lll   ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
};
static int charactersCount = 4;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 15, .isDarkColor = false};

static int angleVels[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
#define ZONEB_SHOWN_RANGE 30

#define ZONEB_MAX_ENEMY_COUNT 256
typedef struct {
  Vector pos;
  int angle;
  float speed;
  float shotTicks;
  int burstCount;
  float turnTicks;
  bool isReflecting;
  bool isAlive;
} Enemy;
static Enemy enemies[ZONEB_MAX_ENEMY_COUNT];

#define ZONEB_WALL_COUNT 19
typedef struct {
  Vector pos;
  float width;
  int angle;
  bool isAlive;
} Wall;
static Wall walls[ZONEB_WALL_COUNT];

#define ZONEB_SIDE_PLAYER 0
#define ZONEB_SIDE_ENEMY 1
typedef struct {
  Vector pos;
  int angle;
  float range;
  int side;
  bool isAlive;
} Bullet;
// At the level cap (256 enemies), each fires ~5 bullets per ~185-tick burst
// cycle with a 20-tick range, giving ~256*5/185*20 =~ 140 concurrent enemy
// bullets alone - 128 overflowed.
#define ZONEB_MAX_BULLET_COUNT 256
static Bullet bullets[ZONEB_MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  int angle;
  float speed;
  bool isReflecting;
  float shotTicks;
} Player;
static Player player;

static int level;
static bool isLevelCleared;
static float levelClearTicks;

typedef struct {
  Vector pos;
  float radius;
} CircleLike;
static CircleLike nextCircleTarget;
static CircleLike nextCircle;
static CircleLike circle;
static float circleTicks;
static int multiplier;

static void checkCircleReflect(Vector *pos, int *angle) {
  if (distanceTo(pos, VEC_XY(circle.pos)) < circle.radius) {
    return;
  }
  float a = angleTo(pos, VEC_XY(circle.pos));
  if (a < (-M_PI / 4) * 3 || a > (M_PI / 4) * 3) {
    *angle = 2;
  } else if (a > M_PI / 4) {
    *angle = 1;
  } else if (a < -M_PI / 4) {
    *angle = 3;
  } else {
    *angle = 0;
  }
}

static void update() {
  if (!ticks) {
    level = 0;
    isLevelCleared = true;
    levelClearTicks = 0;
  }
  if (isLevelCleared) {
    circleTicks = 99;
    vectorSet(&circle.pos, 50, 50);
    circle.radius = 120;
    vectorSet(&nextCircle.pos, 50, 50);
    nextCircle.radius = 60;
    vectorSet(&nextCircleTarget.pos, 0, 0);
    nextCircleTarget.radius = 0;
    int enemyCount = 9 + level * 7;
    if (enemyCount > ZONEB_MAX_ENEMY_COUNT) {
      enemyCount = ZONEB_MAX_ENEMY_COUNT;
    }
    INIT_UNALIVED_ARRAY(enemies);
    TIMES(enemyCount, ei) {
      Enemy *e = &enemies[ei];
      vectorSet(&e->pos, rnd(0, 99), rnd(0, 99));
      e->angle = rndi(0, 4);
      e->speed = 0;
      e->shotTicks = rnd(200, 300);
      e->burstCount = 0;
      e->turnTicks = 0;
      e->isReflecting = false;
      e->isAlive = true;
    }
    TIMES(ZONEB_WALL_COUNT, wi) {
      Wall *w = &walls[wi];
      vectorSet(&w->pos, rnd(9, 89), rnd(9, 89));
      w->width = rnd(5, 15);
      w->angle = rndi(0, 2);
      w->isAlive = true;
    }
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    vectorSet(&player.pos, 50, 80);
    player.angle = 3;
    player.speed = 0;
    player.isReflecting = false;
    player.shotTicks = 0;
    level++;
    isLevelCleared = false;
    multiplier = 1;
  }
  circleTicks--;
  if (circleTicks == 9) {
    nextCircleTarget.radius = nextCircle.radius - 10;
    nextCircleTarget.pos = nextCircle.pos;
    TIMES(99, ci) {
      vectorSet(&nextCircleTarget.pos, rnd(10, 90), rnd(10, 90));
      if (distanceTo(&nextCircleTarget.pos, VEC_XY(nextCircle.pos)) <
          nextCircle.radius - nextCircleTarget.radius) {
        break;
      }
    }
  }
  if (circleTicks < 0) {
    circleTicks = 600;
  }
  if (circleTicks < 9) {
    vectorAdd(&nextCircle.pos,
              (nextCircleTarget.pos.x - nextCircle.pos.x) / (circleTicks + 1),
              (nextCircleTarget.pos.y - nextCircle.pos.y) / (circleTicks + 1));
    nextCircle.radius +=
        (nextCircleTarget.radius - nextCircle.radius) / (circleTicks + 1);
  }
  if (nextCircle.radius < 60) {
    color = LIGHT_BLACK;
    thickness = 2;
    arc(VEC_XY(nextCircle.pos), nextCircle.radius, 0, M_PI * 2);
    if (circleTicks > 9) {
      vectorAdd(&circle.pos, (nextCircle.pos.x - circle.pos.x) / circleTicks,
                (nextCircle.pos.y - circle.pos.y) / circleTicks);
      circle.radius += (nextCircle.radius - circle.radius) / circleTicks;
    }
    color = BLUE;
    thickness = 3;
    arc(VEC_XY(circle.pos), circle.radius, 0, M_PI * 2);
  }
  color = YELLOW;
  FOR_EACH(walls, wi2) {
    ASSIGN_ARRAY_ITEM(walls, wi2, Wall, w);
    SKIP_IS_NOT_ALIVE(w);
    Collision wc;
    if (w->angle == 0) {
      wc = box(w->pos.x, w->pos.y, w->width, 2);
    } else {
      wc = box(w->pos.x, w->pos.y, 2, w->width);
    }
    if (wc.isColliding.rect[BLUE]) {
      w->isAlive = false;
      continue;
    }
  }
  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    int avx = angleVels[b->angle][0];
    int avy = angleVels[b->angle][1];
    vectorAdd(&b->pos, avx, avy);
    bool isShown = b->side == ZONEB_SIDE_PLAYER ||
                   distanceTo(&b->pos, VEC_XY(player.pos)) < ZONEB_SHOWN_RANGE;
    if (isShown) {
      if (b->side == ZONEB_SIDE_ENEMY) {
        color = PURPLE;
      } else {
        color = CYAN;
      }
    } else {
      color = TRANSPARENT;
    }
    characterOptions.rotation = b->angle;
    Collision bc;
    if (b->side == ZONEB_SIDE_ENEMY) {
      bc = character("c", VEC_XY(b->pos));
    } else {
      bc = character("d", VEC_XY(b->pos));
    }
    characterOptions.rotation = 0;
    bool outOfRect = !(b->pos.x >= 0 && b->pos.x < 99 && b->pos.y >= 0 && b->pos.y < 99);
    if (bc.isColliding.rect[YELLOW] || outOfRect) {
      b->isAlive = false;
      continue;
    }
    b->range--;
    if (b->range < 0) {
      b->isAlive = false;
      continue;
    }
  }
  FOR_EACH(enemies, ei2) {
    ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    int eavx = angleVels[e->angle][0];
    int eavy = angleVels[e->angle][1];
    vectorAdd(&e->pos, eavx * e->speed, eavy * e->speed);
    bool isShown = distanceTo(&e->pos, VEC_XY(player.pos)) < ZONEB_SHOWN_RANGE;
    if (isShown) {
      color = BLACK;
      characterOptions.rotation = e->angle;
      character("b", e->pos.x + eavx * 2, e->pos.y + eavy * 2);
      characterOptions.rotation = 0;
    }
    if (isShown) {
      color = RED;
    } else {
      color = TRANSPARENT;
    }
    characterOptions.rotation = e->angle;
    Collision ec = character("a", VEC_XY(e->pos));
    characterOptions.rotation = 0;
    if (ec.isColliding.character['d']) {
      play(EXPLOSION);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      e->isAlive = false;
      continue;
    }
    bool eOutOfRect = !(e->pos.x >= 0 && e->pos.x < 99 && e->pos.y >= 0 && e->pos.y < 99);
    if (ec.isColliding.rect[YELLOW] || eOutOfRect) {
      if (!e->isReflecting) {
        e->angle += 2;
        e->isReflecting = true;
      }
    } else {
      e->isReflecting = false;
    }
    if (e->shotTicks > 7) {
      e->turnTicks--;
    }
    if (e->turnTicks < 0) {
      e->angle++;
      e->turnTicks = rnd(200, 300);
    }
    e->angle = (int)wrap(e->angle, 0, 4);
    e->shotTicks--;
    if (e->shotTicks < 0) {
      e->burstCount--;
      if (e->burstCount < 0) {
        e->shotTicks = rnd(100, 200);
        e->burstCount = rndi(3, 7);
      } else {
        ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
        vectorSet(&nb->pos, e->pos.x + eavx * 5, e->pos.y + eavy * 5);
        nb->angle = e->angle;
        nb->range = 20;
        nb->side = ZONEB_SIDE_ENEMY;
        nb->isAlive = true;
        bulletIndex = wrap(bulletIndex + 1, 0, ZONEB_MAX_BULLET_COUNT);
        e->shotTicks += 7;
      }
    }
    float speedTarget;
    if (e->shotTicks < 7) {
      speedTarget = 0;
    } else {
      speedTarget = 0.2;
    }
    e->speed += (speedTarget - e->speed) * 0.1;
    checkCircleReflect(&e->pos, &e->angle);
    e->pos.x = clamp(e->pos.x, 0, 99);
    e->pos.y = clamp(e->pos.y, 0, 99);
  }
  int pavx = angleVels[player.angle][0];
  int pavy = angleVels[player.angle][1];
  float speedTarget2;
  if (input.isPressed) {
    speedTarget2 = 0;
  } else {
    speedTarget2 = 0.2;
  }
  player.speed += (speedTarget2 - player.speed) * 0.1;
  if (input.isJustReleased) {
    if (player.speed > 0.04) {
      play(LASER);
      player.angle = (int)wrap(player.angle + 1, 0, 4);
    }
  }
  player.shotTicks--;
  if (input.isPressed && player.speed < 0.04) {
    if (player.shotTicks < 0) {
      play(HIT);
      ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, npb);
      vectorSet(&npb->pos, player.pos.x + pavx * 5, player.pos.y + pavy * 5);
      npb->angle = player.angle;
      npb->range = 20;
      npb->side = ZONEB_SIDE_PLAYER;
      npb->isAlive = true;
      bulletIndex = wrap(bulletIndex + 1, 0, ZONEB_MAX_BULLET_COUNT);
      player.shotTicks = 7;
    }
  }
  vectorAdd(&player.pos, pavx * player.speed, pavy * player.speed);
  checkCircleReflect(&player.pos, &player.angle);
  color = BLACK;
  characterOptions.rotation = player.angle;
  character("b", player.pos.x + pavx * 2, player.pos.y + pavy * 2);
  characterOptions.rotation = 0;
  color = BLUE;
  characterOptions.rotation = player.angle;
  Collision pc = character("a", VEC_XY(player.pos));
  characterOptions.rotation = 0;
  if (pc.isColliding.character['c'] || circle.radius < 1 ||
      distanceTo(&player.pos, VEC_XY(circle.pos)) >
          circle.radius * 1.05) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
  player.pos.x = clamp(player.pos.x, 0, 99);
  player.pos.y = clamp(player.pos.y, 0, 99);
  // Upstream clamps player.pos before this yellow/out-of-rect check, so the
  // "out of rect" half is unreachable here (position is always in-bounds by
  // this point) - kept as-is to match the original logic exactly.
  if (pc.isColliding.rect[YELLOW]) {
    if (!player.isReflecting) {
      player.angle += 2;
      player.isReflecting = true;
    }
  } else {
    player.isReflecting = false;
  }
  player.angle = (int)wrap(player.angle, 0, 4);
  COUNT_IS_ALIVE(enemies, aliveEnemyCount);
  if (levelClearTicks <= -60 && aliveEnemyCount == 0) {
    play(POWER_UP);
    levelClearTicks = 60;
  }
  if (levelClearTicks > 0) {
    color = BLACK;
    text("WINNER!", 30, 50);
    levelClearTicks--;
    if (levelClearTicks == 0) {
      isLevelCleared = true;
    }
  } else if (levelClearTicks > -60) {
    color = BLACK;
    char levelText[16];
    strcpy(levelText, "LEVEL ");
    strcat(levelText, intToChar(level));
    text(levelText, 30, 50);
    levelClearTicks--;
  }
}

void addGameZoneB() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
