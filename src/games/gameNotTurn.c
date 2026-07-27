#include "cglp.h"
#include "string.h"

static char *title = "NOT TURN";
static char *description = "[Hold]\n Turn & Speed up";

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
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "l ll l",
        "l ll l",
        "llllll",
        " l  l ",
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
    {
        "  lll ",
        "ll l l",
        " llll ",
        " l  l ",
        "ll  ll",
            "      ",
},
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        " llll ",
        "l ll l",
        " llll ",
        "  ll  ",
        " l  l ",
            "      ",
},
    {
        "  ll  ",
        " l ll ",
        " llll ",
        "  ll  ",
            "      ",
        "      ",
},
};
static int charactersCount = 9;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 80, .isDarkColor = false};

#define NOTTURN_X_BAR_COUNT 5
#define NOTTURN_X_BAR_INTERVAL 20
#define NOTTURN_X_BAR_PADDING 10.0

static float calcBarX(int i) {
  return i * NOTTURN_X_BAR_INTERVAL + NOTTURN_X_BAR_PADDING;
}

typedef struct {
  Vector pos;
  bool isAlive;
} NotTurnBar;
#define NOTTURN_MAX_BAR_COUNT 64
static NotTurnBar bars[NOTTURN_MAX_BAR_COUNT];
static int barIndex;
static float nextBarDist;

typedef struct {
  Vector pos;
  Vector vel;
  int xIndex;
  float noTurnDist;
  float dotDist;
  bool isAlive;
} NotTurnEnemy;
#define NOTTURN_MAX_ENEMY_COUNT 64
static NotTurnEnemy enemies[NOTTURN_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  Vector vel;
  int xIndex;
  float noTurnDist;
  float speed;
} NotTurnPlayer;
static NotTurnPlayer player;

typedef struct {
  Vector pos;
  bool isAlive;
} NotTurnDot;
#define NOTTURN_MAX_DOT_COUNT 64
static NotTurnDot dots[NOTTURN_MAX_DOT_COUNT];
static int dotIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(bars);
    barIndex = 0;
    nextBarDist = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 20;
    int xIndex = 2;
    vectorSet(&player.pos, calcBarX(xIndex), 99);
    vectorSet(&player.vel, 0, 1);
    player.xIndex = xIndex;
    player.noTurnDist = 5;
    player.speed = 1;
    INIT_UNALIVED_ARRAY(dots);
    dotIndex = 0;
    multiplier = 1;
  }
  float scr = 0;
  if (player.pos.y > 20) {
    scr = (player.pos.y - 20) * 0.1;
  }
  nextBarDist -= scr;
  if (nextBarDist < 0) {
    int i1 = rndi(0, NOTTURN_X_BAR_COUNT - 1);
    int i2 = rndi(0, NOTTURN_X_BAR_COUNT - 1);
    ASSIGN_ARRAY_ITEM(bars, barIndex, NotTurnBar, nb1);
    vectorSet(&nb1->pos, calcBarX(i1) + NOTTURN_X_BAR_INTERVAL / 2.0, 102);
    nb1->isAlive = true;
    barIndex = wrap(barIndex + 1, 0, NOTTURN_MAX_BAR_COUNT);
    if (fabs((float)(i2 - i1)) > 1) {
      ASSIGN_ARRAY_ITEM(bars, barIndex, NotTurnBar, nb2);
      vectorSet(&nb2->pos, calcBarX(i2) + NOTTURN_X_BAR_INTERVAL / 2.0, 102);
      nb2->isAlive = true;
      barIndex = wrap(barIndex + 1, 0, NOTTURN_MAX_BAR_COUNT);
    }
    nextBarDist += rnd(9, 12);
  }
  color = LIGHT_BLACK;
  TIMES(NOTTURN_X_BAR_COUNT, i) { box(calcBarX(i), 50, 3, 100); }
  color = LIGHT_BLUE;
  FOR_EACH(bars, i) {
    ASSIGN_ARRAY_ITEM(bars, i, NotTurnBar, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.y -= scr;
    box(b->pos.x, b->pos.y, NOTTURN_X_BAR_INTERVAL - 3, 3);
    if (b->pos.y < -1) {
      b->isAlive = false;
      continue;
    }
  }
  float sp = clamp(sqrt(difficulty) * player.speed * 0.2, 0, 3);
  vectorAdd(&player.pos, player.vel.x * sp, player.vel.y * sp);
  player.noTurnDist -= player.vel.y * sp;
  char pc[2];
  if (player.vel.x != 0) {
    float bx = calcBarX(player.xIndex);
    if ((player.pos.x - bx) * player.vel.x > 0) {
      player.pos.x = bx;
      vectorSet(&player.vel, 0, 1);
    }
    int divisor;
    if (input.isPressed) {
      divisor = 10;
    } else {
      divisor = 20;
    }
    pc[0] = 'a' + (int)floor(ticks / divisor) % 2;
  } else {
    if (player.noTurnDist < 0 && input.isPressed) {
      color = TRANSPARENT;
      Collision c1 = box(player.pos.x + 6, player.pos.y + 3, 1, 1);
      if (c1.isColliding.rect[LIGHT_BLUE]) {
        play(SELECT);
        vectorSet(&player.vel, 1, 0);
        player.xIndex++;
        player.noTurnDist = 5;
      } else {
        Collision c2 = box(player.pos.x - 6, player.pos.y + 3, 1, 1);
        if (c2.isColliding.rect[LIGHT_BLUE]) {
          play(SELECT);
          vectorSet(&player.vel, -1, 0);
          player.xIndex--;
          player.noTurnDist = 5;
        }
      }
    }
    int divisor2;
    if (input.isPressed) {
      divisor2 = 10;
    } else {
      divisor2 = 20;
    }
    pc[0] = 'c' + (int)floor(ticks / divisor2) % 2;
  }
  pc[1] = 0;
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isJustReleased) {
    play(HIT);
  }
  float speedTarget;
  if (input.isPressed) {
    speedTarget = 4;
  } else {
    speedTarget = 1;
  }
  player.speed += (speedTarget - player.speed) * 0.2;
  color = BLACK;
  characterOptions.isMirrorX = player.vel.x <= 0;
  character(pc, player.pos.x, player.pos.y);
  characterOptions.isMirrorX = false;
  player.pos.y -= scr;
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    int xIndex = rndi(0, NOTTURN_X_BAR_COUNT);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, NotTurnEnemy, ne);
    vectorSet(&ne->pos, calcBarX(xIndex), 103);
    vectorSet(&ne->vel, 0, -1);
    ne->xIndex = xIndex;
    ne->noTurnDist = 0;
    ne->dotDist = 0;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, NOTTURN_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(100, 150) / difficulty;
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, NotTurnEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float esp = clamp(sqrt(difficulty) * 0.3, 0, 3);
    vectorAdd(&e->pos, e->vel.x * esp, e->vel.y * esp);
    e->noTurnDist += e->vel.y * esp;
    if (e->vel.x != 0) {
      float ebx = calcBarX(e->xIndex);
      if ((e->pos.x - ebx) * e->vel.x > 0) {
        e->pos.x = ebx;
        vectorSet(&e->vel, 0, -1);
      }
    } else if (e->noTurnDist < 0) {
      color = TRANSPARENT;
      Collision ec1 = box(e->pos.x + 6, e->pos.y + 3, 1, 1);
      if (ec1.isColliding.rect[LIGHT_BLUE]) {
        vectorSet(&e->vel, 1, 0);
        e->xIndex++;
        e->noTurnDist = 5;
      } else {
        Collision ec2 = box(e->pos.x - 6, e->pos.y + 3, 1, 1);
        if (ec2.isColliding.rect[LIGHT_BLUE]) {
          vectorSet(&e->vel, -1, 0);
          e->xIndex--;
          e->noTurnDist = 5;
        }
      }
    }
    color = RED;
    Collision ecc = character("e", e->pos.x, e->pos.y);
    if (ecc.isColliding.character['a'] || ecc.isColliding.character['b'] ||
        ecc.isColliding.character['c'] || ecc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    e->pos.y -= scr;
    e->dotDist -= esp;
    if (e->dotDist < 0) {
      e->dotDist += 6;
      ASSIGN_ARRAY_ITEM(dots, dotIndex, NotTurnDot, nd);
      nd->pos = e->pos;
      nd->isAlive = true;
      dotIndex = wrap(dotIndex + 1, 0, NOTTURN_MAX_DOT_COUNT);
    }
    if (e->pos.y < -3) {
      if (multiplier > 1) {
        multiplier--;
      }
      e->isAlive = false;
      continue;
    }
  }
  color = YELLOW;
  FOR_EACH(dots, i) {
    ASSIGN_ARRAY_ITEM(dots, i, NotTurnDot, d);
    SKIP_IS_NOT_ALIVE(d);
    Collision dc = character("i", d->pos.x, d->pos.y);
    if (dc.isColliding.character['a'] || dc.isColliding.character['b'] ||
        dc.isColliding.character['c'] || dc.isColliding.character['d']) {
      play(COIN);
      addScore(multiplier, d->pos.x, d->pos.y);
      multiplier++;
      d->isAlive = false;
      continue;
    }
    d->pos.y -= scr;
    if (d->pos.y < -3) {
      d->isAlive = false;
      continue;
    }
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 10);
}

void addGameNotTurn() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
