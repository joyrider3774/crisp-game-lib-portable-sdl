#include "cglp.h"
#include "string.h"

static char *title = "CLOCK TURRET";
static char *description = "[Hold]\n Stop and Shoot";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  float angle;
  float angleVel;
  float shootTicks;
} Hand;
static Hand hand;

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} Shot;
#define CLOCKTURRET_MAX_SHOT_COUNT 32
static Shot shots[CLOCKTURRET_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  float angle;
  float angleToCenter;
  float speed;
  float bulletTicks;
  bool wasInScreen;
  bool isAlive;
} Enemy;
#define CLOCKTURRET_MAX_ENEMY_COUNT 16
static Enemy enemies[CLOCKTURRET_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
} Bullet;
#define CLOCKTURRET_MAX_BULLET_COUNT 16
static Bullet bullets[CLOCKTURRET_MAX_BULLET_COUNT];
static int bulletIndex;
static float nextBulletTicks;

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} Coin;
#define CLOCKTURRET_MAX_COIN_COUNT 16
static Coin coins[CLOCKTURRET_MAX_COIN_COUNT];
static int coinIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    hand.angle = -M_PI_2;
    hand.angleVel = 1;
    hand.shootTicks = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    nextBulletTicks = 0;
    INIT_UNALIVED_ARRAY(coins);
    coinIndex = 0;
    multiplier = 1;
  }
  color = LIGHT_BLACK;
  TIMES(12, i) {
    Vector lp;
    vectorSet(&lp, 50, 50);
    addWithAngle(&lp, -M_PI_2 + (i + 1) * M_PI * 2 / 12, 25);
    if (i >= 9) {
      lp.x -= 3;
    }
    text(intToChar(i + 1), VEC_XY(lp));
  }
  color = BLUE;
  barCenterPosRatio = 0.5;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, sqrt(difficulty) * 3);
    thickness = 2;
    bar(s->pos.x, s->pos.y, 4, s->angle);
    if (!(s->pos.x >= -5 && s->pos.x <= 105 && s->pos.y >= -5 && s->pos.y <= 105)) {
      s->isAlive = false;
    }
  }
  color = BLACK;
  if (input.isJustPressed) {
    play(SELECT);
    hand.angleVel *= -1;
  }
  if (input.isPressed) {
    hand.shootTicks += difficulty;
    if (hand.shootTicks > 7) {
      play(HIT);
      hand.shootTicks -= 7;
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns1);
      vectorSet(&ns1->pos, 50, 50);
      addWithAngle(&ns1->pos, hand.angle, 12);
      ns1->angle = hand.angle;
      ns1->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, CLOCKTURRET_MAX_SHOT_COUNT);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns2);
      vectorSet(&ns2->pos, 50, 50);
      addWithAngle(&ns2->pos, hand.angle, -5);
      ns2->angle = hand.angle + M_PI;
      ns2->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, CLOCKTURRET_MAX_SHOT_COUNT);
    }
  } else {
    hand.angle += difficulty * hand.angleVel * 0.1;
  }
  thickness = 3;
  barCenterPosRatio = 0.2;
  bar(50, 50, 20, hand.angle);
  color = BLUE;
  box(50, 50, 5, 5);
  nextBulletTicks -= difficulty;
  if (nextBulletTicks < 0) {
    COUNT_IS_ALIVE(enemies, aliveEnemyCount);
    if (aliveEnemyCount > 0) {
      int target = rndi(0, aliveEnemyCount);
      int seen = 0;
      Enemy *e = &enemies[0];
      FOR_EACH(enemies, i) {
        ASSIGN_ARRAY_ITEM(enemies, i, Enemy, ce);
        SKIP_IS_NOT_ALIVE(ce);
        if (seen == target) {
          e = ce;
          break;
        }
        seen++;
      }
      Vector fp;
      fp = e->pos;
      addWithAngle(&fp, e->angle, e->speed * 45 / difficulty);
      float oa = wrap(angleTo(&fp, 50, 50) - e->angleToCenter, -M_PI, M_PI);
      if (fp.x >= 5 && fp.x <= 95 && fp.y >= 5 && fp.y <= 95 && fabs(oa) > M_PI * 0.07) {
        e->bulletTicks = 45;
        nextBulletTicks = rnd(45, 60);
      }
    }
  }
  color = YELLOW;
  FOR_EACH(coins, i) {
    ASSIGN_ARRAY_ITEM(coins, i, Coin, c);
    SKIP_IS_NOT_ALIVE(c);
    addWithAngle(&c->pos, c->angle, difficulty);
    Collision cCol = box(VEC_XY(c->pos), 6, 6);
    if (cCol.isColliding.rect[BLACK]) {
      play(COIN);
      multiplier++;
      c->isAlive = false;
      continue;
    }
    if (!(c->pos.x >= -5 && c->pos.x <= 105 && c->pos.y >= -5 && c->pos.y <= 105)) {
      c->isAlive = false;
      continue;
    }
  }
  nextEnemyTicks -= difficulty;
  if (nextEnemyTicks < 0) {
    float angle = rnd(0, M_PI * 2);
    float sign;
    if (rndi(0, 2) == 1) {
      sign = 1;
    } else {
      sign = -1;
    }
    float angleToCenter = angle + M_PI_2 * sign;
    float d = rnd(30, 40);
    Vector pos;
    vectorSet(&pos, 50, 50);
    addWithAngle(&pos, angleToCenter, -d);
    addWithAngle(&pos, angle + M_PI, 60);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    ne->pos = pos;
    ne->angle = angle;
    ne->angleToCenter = angleToCenter;
    ne->speed = rnd(1, difficulty) * 0.5;
    ne->bulletTicks = 0;
    ne->wasInScreen = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, CLOCKTURRET_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(50, 90);
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    addWithAngle(&e->pos, e->angle, e->speed);
    if (e->bulletTicks > 0) {
      e->bulletTicks -= difficulty;
      color = LIGHT_RED;
      float w = e->bulletTicks / 6 + 1;
      thickness = w;
      barCenterPosRatio = 0;
      bar(e->pos.x, e->pos.y, 100, e->angleToCenter);
      if (e->bulletTicks <= 0) {
        play(LASER);
        ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
        nb->pos = e->pos;
        nb->angle = e->angleToCenter;
        nb->speed = difficulty * 9;
        nb->isAlive = true;
        bulletIndex = wrap(bulletIndex + 1, 0, CLOCKTURRET_MAX_BULLET_COUNT);
      }
    }
    color = PURPLE;
    thickness = 5;
    barCenterPosRatio = 0.5;
    Collision eCol = bar(e->pos.x, e->pos.y, 7, e->angleToCenter);
    if (eCol.isColliding.rect[BLUE]) {
      play(POWER_UP);
      particle(e->pos.x, e->pos.y, 9, 1, 0, M_PI * 2);
      addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      ASSIGN_ARRAY_ITEM(coins, coinIndex, Coin, nc);
      nc->pos = e->pos;
      nc->angle = e->angleToCenter;
      nc->isAlive = true;
      coinIndex = wrap(coinIndex + 1, 0, CLOCKTURRET_MAX_COIN_COUNT);
      e->isAlive = false;
      continue;
    }
    if (!e->wasInScreen && e->pos.x >= 0 && e->pos.x <= 100 && e->pos.y >= 0 && e->pos.y <= 100) {
      e->wasInScreen = true;
    }
    if (e->wasInScreen &&
        !(e->pos.x >= -5 && e->pos.x <= 105 && e->pos.y >= -5 && e->pos.y <= 105)) {
      e->isAlive = false;
      continue;
    }
  }
  color = RED;
  barCenterPosRatio = 0.5;
  FOR_EACH(bullets, i) {
    ASSIGN_ARRAY_ITEM(bullets, i, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    addWithAngle(&b->pos, b->angle, b->speed);
    thickness = 4;
    Collision bCol = bar(b->pos.x, b->pos.y, b->speed * 1.2, b->angle);
    if (bCol.isColliding.rect[BLACK]) {
      play(EXPLOSION);
      gameOver();
    }
    if (!(b->pos.x >= -5 && b->pos.x <= 105 && b->pos.y >= -5 && b->pos.y <= 105)) {
      b->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameClockTurret() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
