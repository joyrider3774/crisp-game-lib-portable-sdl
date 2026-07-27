#include "cglp.h"
#include "string.h"

static char *title = "ROLL S";
static char *description = "[Tap]\n Change angle\n[Hold]\n Fire";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
    {
        "  ll  ",
        "   ll ",
        "    ll",
        "    ll",
        "   ll ",
        "  ll  ",
    },
    {
        "  lll ",
        "   ll ",
        "  lll ",
        " ll ll",
        "ll  ll",
        "ll  ll",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "llllll",
        "llllll",
    },
    {
        "      ",
        "      ",
        "      ",
        "ll    ",
        "llllll",
        "llllll",
    },
    {
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
        "ll    ",
    },
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 200, .isDarkColor = false};

#define ROLLS_PLAYER_X 20

typedef struct {
  Vector pos;
  float angle;
  float va;
  float ticks;
  float fireTicks;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Shot;
#define ROLLS_MAX_SHOT_COUNT 64
static Shot shots[ROLLS_MAX_SHOT_COUNT];
static int shotIndex;

typedef struct {
  Vector pos;
  Vector vel;
  float angle;
  float ticks;
  float fireTicks;
  bool isAlive;
} Enemy;
#define ROLLS_MAX_ENEMY_COUNT 64
static Enemy enemies[ROLLS_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Bullet;
#define ROLLS_MAX_BULLET_COUNT 64
static Bullet bullets[ROLLS_MAX_BULLET_COUNT];
static int bulletIndex;

static float scrOfs;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&player.pos, ROLLS_PLAYER_X, 50);
    player.angle = 0;
    player.va = 1;
    player.ticks = 0;
    player.fireTicks = 0;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    scrOfs = 0;
    multiplier = 1;
  }
  float scr = 0;
  float pa = floor(player.angle) * M_PI / 4;
  Vector pc;
  vectorSet(&pc, player.pos.x, player.pos.y - 9);
  bool pd = false;
  if (input.isJustReleased) {
    play(SELECT);
    player.angle += player.va;
    if (player.angle < -1 || player.angle > 1) {
      player.va *= -1;
      player.angle += player.va * 2;
    }
    player.fireTicks = 9 / sqrt(difficulty);
  }
  if (input.isPressed) {
    if (player.angle == 0 || player.angle == 4) {
      float pcxofs;
      if (player.angle == 0) {
        pcxofs = 6;
      } else {
        pcxofs = -6;
      }
      vectorSet(&pc, player.pos.x + pcxofs, player.pos.y - 3);
      pd = true;
    }
    player.angle = floor(player.angle);
    player.fireTicks--;
    if (player.fireTicks < 0) {
      play(HIT);
      TIMES(5, i) {
        ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
        ns->pos = pc;
        vectorSet(&ns->vel, 3 * sqrt(difficulty), 0);
        rotate(&ns->vel, pa + i * 0.12 - 0.24);
        ns->isAlive = true;
        shotIndex = wrap(shotIndex + 1, 0, ROLLS_MAX_SHOT_COUNT);
      }
      player.fireTicks = 9 / sqrt(difficulty);
    }
  } else {
    scr = sqrt(difficulty) * 0.5;
    player.ticks += sqrt(difficulty);
  }
  scrOfs += scr;
  if (scrOfs > multiplier * 100) {
    play(COIN);
    multiplier++;
  }
  color = GREEN;
  rect(0, 20, 100, 5);
  rect(0, 50, 100, 5);
  rect(0, 80, 100, 5);
  rect(0, 80, 100, 5);
  color = LIGHT_BLACK;
  rect(0, 25, 100, 25);
  rect(0, 55, 100, 25);
  color = LIGHT_BLUE;
  rect(0, 85, 100, 15);
  color = LIGHT_GREEN;
  rect(wrap(-scrOfs + ROLLS_PLAYER_X, 0, 100), 25, 2, 25);
  rect(wrap(-scrOfs + 67, -10, 110), 55, 2, 25);
  color = BLACK;
  if (pd) {
    character("d", VEC_XY(pc));
    color = BLUE;
    character("e", player.pos.x, player.pos.y - 3);
  } else {
    character("a", VEC_XY(pc));
    color = BLUE;
    char bc[2];
    bc[0] = 'b' + (int)floor(player.ticks / 15) % 2;
    bc[1] = 0;
    character(bc, player.pos.x, player.pos.y - 3);
  }
  color = BLACK;
  thickness = 3;
  barCenterPosRatio = 0;
  bar(VEC_XY(pc), 6, pa);
  barCenterPosRatio = 0.5;
  color = BLUE;
  FOR_EACH(shots, si) {
    ASSIGN_ARRAY_ITEM(shots, si, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, VEC_XY(s->vel));
    s->pos.x -= scr;
    thickness = 3;
    bar(VEC_XY(s->pos), 3, vectorAngle(&s->vel));
    bool inRect = s->pos.x >= -3 && s->pos.x < 103 && s->pos.y >= -3 && s->pos.y < 103;
    if (!inRect) {
      s->isAlive = false;
      continue;
    }
  }
  float fireInterval = ceil(300 / sqrt(difficulty));
  float fireRepeatInterval = ceil(36 / sqrt(difficulty));
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, 105, rndi(0, 3) * 30 + 20);
    vectorSet(&ne->vel, rnd(1, difficulty) * -0.2, 0);
    ne->angle = 0;
    ne->ticks = 0;
    ne->fireTicks = rndi(0, (int)fireInterval);
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, ROLLS_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(50, 60) / difficulty;
  }
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    Vector ec;
    vectorSet(&ec, e->pos.x, e->pos.y - 9);
    e->fireTicks--;
    if (e->fireTicks < 0) {
      if (fmod(-e->fireTicks, fireRepeatInterval) == 0) {
        play(JUMP);
        ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
        nb->pos = ec;
        vectorSet(&nb->vel, sqrt(difficulty), 0);
        rotate(&nb->vel, e->angle);
        nb->isAlive = true;
        bulletIndex = wrap(bulletIndex + 1, 0, ROLLS_MAX_BULLET_COUNT);
      }
      if (-e->fireTicks >= fireRepeatInterval * 3) {
        e->fireTicks = fireInterval;
      }
    } else {
      e->angle = floor((angleTo(&e->pos, VEC_XY(player.pos)) + M_PI / 8) /
                        (M_PI / 4)) *
                 (M_PI / 4);
      vectorAdd(&e->pos, VEC_XY(e->vel));
      e->ticks -= e->vel.x * 5;
    }
    e->pos.x -= scr;
    color = RED;
    characterOptions.isMirrorX = true;
    Collision c1 = character("f", VEC_XY(ec));
    char ebc[2];
    ebc[0] = 'b' + (int)floor(e->ticks / 15) % 2;
    ebc[1] = 0;
    Collision c2 = character(ebc, e->pos.x, e->pos.y - 3);
    characterOptions.isMirrorX = false;
    if (e->fireTicks < 0) {
      color = RED;
    } else {
      color = BLACK;
    }
    thickness = 3;
    barCenterPosRatio = 0;
    bar(VEC_XY(ec), 6, e->angle);
    barCenterPosRatio = 0.5;
    if (c1.isColliding.rect[BLUE] || c2.isColliding.rect[BLUE]) {
      play(POWER_UP);
      color = RED;
      addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      particle(VEC_XY(e->pos), 16, 1, 0, M_PI * 2);
      e->isAlive = false;
      continue;
    }
    if (c1.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    bool inRect2 = e->pos.x >= -5 && e->pos.x < 105 && e->pos.y >= -5 && e->pos.y < 105;
    if (!inRect2) {
      e->isAlive = false;
      continue;
    }
  }
  color = RED;
  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    vectorAdd(&b->pos, VEC_XY(b->vel));
    b->pos.x -= scr;
    thickness = 3;
    barCenterPosRatio = 0.5;
    Collision bc = bar(VEC_XY(b->pos), 3, vectorAngle(&b->vel));
    if (bc.isColliding.character['a'] || bc.isColliding.character['d']) {
      play(EXPLOSION);
      gameOver();
    }
    bool inRect3 = b->pos.x >= -5 && b->pos.x < 105 && b->pos.y >= -5 && b->pos.y < 105;
    if (!inRect3) {
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

void addGameRollS() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
