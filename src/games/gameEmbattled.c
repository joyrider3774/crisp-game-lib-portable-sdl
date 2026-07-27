#include "cglp.h"
#include "string.h"

static char *title = "EMBATTLED";
static char *description = "[Tap]  Turn\n[Hold] Defense";

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
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define EMBATTLED_TANK_ANGLE_VEL 0.02
#define EMBATTLED_TANK_TURRET_ANGLE_VEL 0.03

typedef struct {
  Vector pos;
  float angle;
  float speed;
  float turretAngle;
  bool hasTarget;
  Vector targetPos;
  float fireTicks;
  float fireInterval;
  int side;
  bool isAlive;
} EmbattledTank;
#define EMBATTLED_MAX_TANK_COUNT 32
static EmbattledTank embattledTanks[EMBATTLED_MAX_TANK_COUNT];
static int embattledTankIndex;
static float embattledNextTankTicks;
static int embattledCurrentSide;
static int embattledSideChangeCount;

typedef struct {
  Vector pos;
  Vector vel;
  int side;
  bool isAlive;
} EmbattledBullet;
#define EMBATTLED_MAX_BULLET_COUNT 64
static EmbattledBullet embattledBullets[EMBATTLED_MAX_BULLET_COUNT];
static int embattledBulletIndex;

typedef struct {
  Vector pos;
  float vy;
  float pressedTicks;
} EmbattledPlayer;
static EmbattledPlayer embattledPlayer;
static int embattledMultiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(embattledTanks);
    embattledTankIndex = 0;
    embattledNextTankTicks = 0;
    embattledCurrentSide = 0;
    embattledSideChangeCount = 0;
    INIT_UNALIVED_ARRAY(embattledBullets);
    embattledBulletIndex = 0;
    vectorSet(&embattledPlayer.pos, 50, 30);
    embattledPlayer.vy = 1;
    embattledPlayer.pressedTicks = 0;
    embattledMultiplier = 1;
  }
  if (input.isJustPressed || (embattledPlayer.pos.y < 3 && embattledPlayer.vy < 0) ||
      (embattledPlayer.pos.y > 97 && embattledPlayer.vy > 0)) {
    play(SELECT);
    embattledPlayer.vy *= -1;
  }
  bool pWall = embattledPlayer.pressedTicks > 10 / sqrt(difficulty);
  if (input.isPressed) {
    embattledPlayer.pressedTicks++;
  } else {
    embattledPlayer.pos.y += embattledPlayer.vy * difficulty * 0.5;
    embattledPlayer.pressedTicks = 0;
  }
  bool playerHasWall = embattledPlayer.pressedTicks > 10 / sqrt(difficulty);
  if (!pWall && playerHasWall) {
    play(POWER_UP);
  }
  if (playerHasWall) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  char pc[2];
  pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  pc[1] = 0;
  if (embattledPlayer.vy < 0) {
    characterOptions.isMirrorX = true;
  } else {
    characterOptions.isMirrorX = false;
  }
  character(pc, VEC_XY(embattledPlayer.pos));
  characterOptions.isMirrorX = false;
  float hwr = 1;
  if (playerHasWall) {
    box(embattledPlayer.pos.x - 5, embattledPlayer.pos.y, 5, 15);
    box(embattledPlayer.pos.x + 6, embattledPlayer.pos.y, 5, 15);
    hwr *= 2;
  }
  FOR_EACH(embattledBullets, i) {
    ASSIGN_ARRAY_ITEM(embattledBullets, i, EmbattledBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, b->vel.x * hwr, b->vel.y * hwr);
    if (b->side == 0) {
      color = RED;
    } else {
      color = PURPLE;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc = bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel));
    if (bc.isColliding.rect[CYAN]) {
      play(HIT);
      b->isAlive = false;
      continue;
    }
    if (bc.isColliding.character['a'] || bc.isColliding.character['b']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
  }
  COUNT_IS_ALIVE(embattledTanks, aliveTankCount);
  if (aliveTankCount == 0) {
    embattledNextTankTicks = 0;
  }
  embattledNextTankTicks -= hwr;
  if (embattledNextTankTicks < 0) {
    int side = embattledCurrentSide;
    embattledSideChangeCount--;
    if (embattledSideChangeCount <= 0) {
      if (embattledCurrentSide == 0) {
        embattledCurrentSide = 1;
      } else {
        embattledCurrentSide = 0;
      }
      embattledSideChangeCount = rndi(1, 4);
    }
    Vector npos;
    if (side == 0) {
      vectorSet(&npos, -5, rnd(0, 99));
    } else {
      vectorSet(&npos, 105, rnd(0, 99));
    }
    float angle = angleTo(&npos, embattledPlayer.pos.x, embattledPlayer.pos.y);
    float fireInterval = rnd(300, 400) / difficulty;
    ASSIGN_ARRAY_ITEM(embattledTanks, embattledTankIndex, EmbattledTank, nt);
    nt->pos = npos;
    nt->angle = angle;
    nt->speed = rnd(1, difficulty) * 0.02;
    nt->turretAngle = angle;
    nt->hasTarget = false;
    nt->fireTicks = rnd(0, fireInterval);
    nt->fireInterval = fireInterval;
    nt->side = side;
    nt->isAlive = true;
    embattledTankIndex = wrap(embattledTankIndex + 1, 0, EMBATTLED_MAX_TANK_COUNT);
    embattledNextTankTicks = rnd(60, 80) / difficulty;
  }
  FOR_EACH(embattledTanks, i) {
    ASSIGN_ARRAY_ITEM(embattledTanks, i, EmbattledTank, t);
    SKIP_IS_NOT_ALIVE(t);
    float md;
    if (playerHasWall) {
      md = distanceTo(&t->pos, embattledPlayer.pos.x, embattledPlayer.pos.y);
      t->targetPos = embattledPlayer.pos;
      t->hasTarget = true;
    } else {
      md = 99;
      t->hasTarget = false;
    }
    FOR_EACH(embattledTanks, j) {
      ASSIGN_ARRAY_ITEM(embattledTanks, j, EmbattledTank, ot);
      SKIP_IS_NOT_ALIVE(ot);
      if (t->side == ot->side) {
        continue;
      }
      float d = distanceTo(&t->pos, ot->pos.x, ot->pos.y);
      if (d < md) {
        md = d;
        t->targetPos = ot->pos;
        t->hasTarget = true;
      }
    }
    if (t->hasTarget) {
      float ta = angleTo(&t->pos, t->targetPos.x, t->targetPos.y);
      float oa = wrap(ta - t->turretAngle, -M_PI, M_PI);
      float tv = EMBATTLED_TANK_TURRET_ANGLE_VEL * difficulty * hwr;
      if (fabs(oa) < tv) {
        t->turretAngle = ta;
      } else if (oa > 0) {
        t->turretAngle += tv;
      } else {
        t->turretAngle -= tv;
      }
      oa = wrap(ta - t->angle, -M_PI, M_PI);
      tv = EMBATTLED_TANK_ANGLE_VEL * difficulty;
      if (fabs(oa) < tv) {
        t->angle = ta;
      } else if (oa > 0) {
        t->angle += tv;
      } else {
        t->angle -= tv;
      }
    }
    addWithAngle(&t->pos, t->angle, t->speed * hwr);
    t->fireTicks -= hwr;
    if (t->fireTicks < 0) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(embattledBullets, embattledBulletIndex, EmbattledBullet, nb);
      nb->pos = t->pos;
      vectorSet(&nb->vel, 0, 0);
      addWithAngle(&nb->vel, t->turretAngle, difficulty * 0.5);
      nb->side = t->side;
      nb->isAlive = true;
      embattledBulletIndex = wrap(embattledBulletIndex + 1, 0, EMBATTLED_MAX_BULLET_COUNT);
      t->fireTicks = t->fireInterval;
    }
    if (t->side == 0) {
      color = LIGHT_RED;
    } else {
      color = LIGHT_PURPLE;
    }
    thickness = 6;
    barCenterPosRatio = 0.5;
    Collision tc = bar(t->pos.x, t->pos.y, 1, t->angle);
    int oppositeColor;
    if (t->side == 0) {
      oppositeColor = PURPLE;
    } else {
      oppositeColor = RED;
    }
    if (tc.isColliding.rect[oppositeColor]) {
      play(EXPLOSION);
      color = BLACK;
      particle(t->pos.x, t->pos.y, 16, 1, 0, M_PI * 2);
      addScore(embattledMultiplier, t->pos.x, t->pos.y);
      embattledMultiplier++;
      t->isAlive = false;
      continue;
    }
    if (tc.isColliding.character['a'] || tc.isColliding.character['b']) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    color = BLACK;
    thickness = 3;
    barCenterPosRatio = 0;
    bar(t->pos.x, t->pos.y, 3, t->turretAngle);
    bool inRect = t->pos.x >= -5 && t->pos.x < 105 && t->pos.y >= -5 && t->pos.y < 105;
    if (!inRect) {
      t->isAlive = false;
      continue;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(embattledBullets, i) {
    ASSIGN_ARRAY_ITEM(embattledBullets, i, EmbattledBullet, b);
    SKIP_IS_NOT_ALIVE(b);
    int checkColor;
    if (b->side == 0) {
      checkColor = LIGHT_PURPLE;
    } else {
      checkColor = LIGHT_RED;
    }
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc2 = bar(b->pos.x, b->pos.y, 3, vectorAngle(&b->vel));
    if (bc2.isColliding.rect[checkColor]) {
      b->isAlive = false;
      continue;
    }
  }
  if (ticks % 60 == 0 && embattledMultiplier > 1) {
    embattledMultiplier--;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "+");
  strcat(multText, intToChar(embattledMultiplier));
  text(multText, 3, 9);
}

void addGameEmbattled() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
