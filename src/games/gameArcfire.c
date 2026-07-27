#include "cglp.h"

static char *title = "ARCFIRE";
static char *description = "[Hold]\n  Set arc\n[Release]\n  Fire";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l  l",
        " llll ",
        "l l   ",
        "  lll ",
        " l    ",
    },
    {
        "  ll  ",
        "l l   ",
        " llll ",
        "  l  l",
        "llll  ",
        "    l ",
    },
    {
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
        "      ",
    },
    {
        " llll ",
        "  l   ",
        " lllll",
        "l l   ",
        "  lll ",
        " l    ",
    },
    {
        " llll ",
        "  l   ",
        "lllll ",
        "  l  l",
        "llll  ",
        "    l ",
    },
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 16, .isDarkColor = false};

typedef struct {
  Vector p;
  Vector v;
  bool isAlive;
} ArcfireEnemy;
#define ARCFIRE_MAX_ENEMY_COUNT 64
static ArcfireEnemy enemies[ARCFIRE_MAX_ENEMY_COUNT];
static int enemyIndex;

static Vector pos;
static float moveAngle;
static float moveDist;
static float angle;
static float arcFrom;
static float arcTo;
static bool isPressing;
static float enemyAddAngle;
static float enemyAddTicks;
static int multiplier;

static bool shotIsAlive;
static float shotD;
static float shotRange;
static float shotArcFrom;
static float shotArcTo;

static void update() {
  if (!ticks) {
    vectorSet(&pos, 50, 50);
    angle = 0;
    shotIsAlive = false;
    isPressing = false;
    moveAngle = 0;
    moveDist = 0;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    enemyAddAngle = rnd(0, M_PI * 2);
    enemyAddTicks = 0;
    multiplier = 1;
  }
  if (moveDist > 1) {
    Vector d;
    vectorSet(&d, moveDist * 0.2, 0);
    rotate(&d, moveAngle);
    vectorAdd(&pos, d.x, d.y);
    moveDist *= 0.2;
    if (!(pos.x >= 10 && pos.x <= 100 && pos.y >= 10 && pos.y <= 100)) {
      moveAngle += M_PI;
    }
    pos.x = clamp(pos.x, 10, 90);
    pos.y = clamp(pos.y, 10, 90);
  }
  angle += 0.07 * difficulty;
  color = LIGHT_BLUE;
  thickness = 4;
  arc(50, 50, 7, 0, M_PI * 2);
  color = LIGHT_BLACK;
  thickness = 2;
  Vector tip;
  vectorSet(&tip, 9, 0);
  rotate(&tip, angle);
  vectorAdd(&tip, pos.x, pos.y);
  line(pos.x, pos.y, tip.x, tip.y);
  color = BLACK;
  characterOptions.isMirrorX = cos(moveAngle) < 0;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  char playerChar[2];
  playerChar[0] = 'a' + ((int)(ticks / 30) % 2);
  playerChar[1] = 0;
  character(playerChar, pos.x, pos.y);

  float range = 0;
  if (isPressing) {
    arcTo = angle;
    range = 300 / sqrt((arcTo - arcFrom) * 30);
    color = GREEN;
    thickness = 3;
    Vector l1;
    vectorSet(&l1, range, 0);
    rotate(&l1, arcFrom);
    vectorAdd(&l1, pos.x, pos.y);
    line(pos.x, pos.y, l1.x, l1.y);
    Vector l2;
    vectorSet(&l2, range, 0);
    rotate(&l2, arcTo);
    vectorAdd(&l2, pos.x, pos.y);
    line(pos.x, pos.y, l2.x, l2.y);
    thickness = 3;
    arc(pos.x, pos.y, range, arcFrom, arcTo);
  }
  if (isPressing && arcTo - arcFrom > M_PI) {
    isPressing = false;
  }
  if (isPressing && input.isJustReleased) {
    isPressing = false;
    if (!shotIsAlive) {
      play(SELECT);
      shotIsAlive = true;
      shotD = 0;
      shotRange = range;
      shotArcFrom = arcFrom;
      shotArcTo = arcTo;
    }
    moveAngle = (arcTo + arcFrom) / 2;
    moveDist = range / 2;
  }
  if (input.isJustPressed) {
    play(LASER);
    arcFrom = angle;
    isPressing = true;
    multiplier = 1;
  }
  color = CYAN;
  if (shotIsAlive) {
    shotD += 2;
    thickness = 5;
    arc(pos.x, pos.y, shotD, shotArcFrom, shotArcTo);
    shotIsAlive = shotD < shotRange;
  }

  enemyAddTicks -= difficulty;
  if (enemyAddTicks < 0) {
    Vector p;
    vectorSet(&p, 70, 0);
    rotate(&p, enemyAddAngle);
    vectorAdd(&p, 50, 50);
    Vector v;
    vectorSet(&v, rnd(0, 10), 0);
    rotate(&v, rnd(0, M_PI * 2));
    vectorAdd(&v, 50, 50);
    vectorAdd(&v, -p.x, -p.y);
    vectorMul(&v, 1.0 / (500 / rnd(1, difficulty)));
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, ArcfireEnemy, e);
    e->p = p;
    e->v = v;
    e->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, ARCFIRE_MAX_ENEMY_COUNT);
    enemyAddTicks += rnd(40, 60);
    if (rnd(0, 1) < 0.1) {
      enemyAddAngle = rnd(0, M_PI * 2);
    } else {
      enemyAddAngle += rnd(0, 0.05) * RNDPM();
    }
  }
  color = RED;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, ArcfireEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->p, e->v.x, e->v.y);
    characterOptions.isMirrorX = cos(vectorAngle(&e->v)) < 0;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    char enemyChar[2];
    enemyChar[0] = 'd' + ((int)(ticks / 30) % 2);
    enemyChar[1] = 0;
    Collision c = character(enemyChar, e->p.x, e->p.y);
    if (c.isColliding.rect[CYAN]) {
      play(POWER_UP);
      particle(e->p.x, e->p.y, 9, 1, 0, M_PI * 2);
      addScore(multiplier, e->p.x, e->p.y);
      multiplier++;
      e->isAlive = false;
      continue;
    }
    if (c.isColliding.character['a'] || c.isColliding.character['b'] ||
        c.isColliding.rect[LIGHT_BLUE]) {
      color = RED;
      if (c.isColliding.rect[LIGHT_BLUE]) {
        float tx = (e->p.x - 50) / 2 + 50;
        float ty = (e->p.y - 50) / 2 + 50;
        text("X", tx, ty);
      } else {
        text("X", pos.x, pos.y);
      }
      play(RANDOM);
      gameOver();
    }
  }
}

void addGameArcfire() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
