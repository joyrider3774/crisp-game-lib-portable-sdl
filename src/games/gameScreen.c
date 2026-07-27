#include "cglp.h"
#include "string.h"

static char *title = "SCREEN";
static char *description = "[Tap] Fire";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 500, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vy;
  bool isAlive;
} Enemy;
#define SCREEN_MAX_ENEMY_COUNT 32
static Enemy enemies[SCREEN_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  float tx;
  float angle;
  int shotCount;
} Player;
static Player player;

typedef struct {
  Vector pos;
  float angle;
  float speed;
  bool isAlive;
} Shot;
#define SCREEN_MAX_SHOT_COUNT 16
static Shot shots[SCREEN_MAX_SHOT_COUNT];
static int shotIndex;
static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    vectorSet(&player.pos, 55, 95);
    player.tx = 55;
    player.angle = 0;
    player.shotCount = 3;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    multiplier = 1;
  }
  player.angle += 0.01 * difficulty;
  if (player.angle >= M_PI_2) {
    player.angle -= M_PI_2;
  }
  float sa;
  if (player.pos.x > 50) {
    sa = -player.angle;
  } else {
    sa = M_PI + player.angle;
  }
  if (player.shotCount > 0 && fabs(player.pos.x - player.tx) < 1 &&
      input.isJustPressed) {
    play(COIN);
    ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
    ns->pos = player.pos;
    ns->angle = sa;
    ns->speed = difficulty;
    ns->isAlive = true;
    shotIndex = wrap(shotIndex + 1, 0, SCREEN_MAX_SHOT_COUNT);
    player.shotCount--;
  }
  if (player.shotCount <= 0) {
    if (player.tx == 55) {
      player.tx = 45;
    } else {
      player.tx = 55;
    }
  }
  float pp = player.pos.x;
  player.pos.x += (player.tx - player.pos.x) * 0.2;
  if (fabs(player.pos.x - player.tx) < 1) {
    player.pos.x = player.tx;
  }
  if ((pp - 50) * (player.pos.x - 50) <= 0) {
    play(SELECT);
    player.shotCount = 3;
  }
  color = LIGHT_BLACK;
  thickness = 2;
  barCenterPosRatio = 0;
  bar(VEC_XY(player.pos), 99, sa);
  color = BLACK;
  char pc[2];
  pc[0] = 'a' + (int)floor(ticks / 20) % 2;
  pc[1] = 0;
  characterOptions.isMirrorX = player.tx < 50;
  character(pc, VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  color = BLUE;
  float bx;
  if (player.pos.x < 50) {
    bx = 39;
  } else {
    bx = 61;
  }
  TIMES(player.shotCount, i) {
    box(bx, 97, 4, 2);
    if (player.pos.x < 50) {
      bx -= 5;
    } else {
      bx += 5;
    }
  }
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    addWithAngle(&s->pos, s->angle, s->speed);
    thickness = 3;
    barCenterPosRatio = 0.5;
    bar(VEC_XY(s->pos), 2, s->angle);
    bool inRect = s->pos.x >= -5 && s->pos.x < 105 && s->pos.y >= -5 && s->pos.y < 105;
    if (!inRect) {
      if (multiplier > 1) {
        multiplier--;
      }
      s->isAlive = false;
      continue;
    }
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, 50 + rnd(15, 40) * RNDPM(), -3);
    ne->vy = rnd(1, difficulty) * 0.1;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, SCREEN_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(60, 90) / difficulty;
  }
  color = BLACK;
  rect(49, 0, 2, 100);
  color = RED;
  bool isEnd = false;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += e->vy;
    char ec[2];
    ec[0] = 'c' + (int)floor(fabs(ticks) / 20) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = e->pos.x >= 50;
    Collision ecoll = character(ec, VEC_XY(e->pos));
    characterOptions.isMirrorX = false;
    if (ecoll.isColliding.rect[BLUE]) {
      play(POWER_UP);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      particle(VEC_XY(e->pos), 16, 1, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (e->pos.y > 99) {
      play(EXPLOSION);
      text("X", e->pos.x, 96);
      gameOver();
      isEnd = true;
    }
  }
  color = TRANSPARENT;
  FOR_EACH(shots, i) {
    ASSIGN_ARRAY_ITEM(shots, i, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision sc = bar(VEC_XY(s->pos), 2, s->angle);
    if (sc.isColliding.character['c'] || sc.isColliding.character['d']) {
      s->isAlive = false;
    }
  }
  if (!isEnd) {
    color = LIGHT_BLACK;
    float rx;
    if (player.pos.x < 50) {
      rx = 51;
    } else {
      rx = 0;
    }
    rect(rx, 0, 49, 100);
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "+");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameScreen() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
