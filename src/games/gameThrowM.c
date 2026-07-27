#include "cglp.h"

static char *title = "THROW M";
static char *description = "[Hold]\n Set angle\n[Release]\n Shoot";

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
        "  lll ",
        "ll l l",
        " llll ",
        "  ll  ",
        " l  l ",
        " l  l ",
    },
    {
        " lll  ",
        "ll ll ",
        "l lll ",
        "lllll ",
        " lll  ",
        "  ll  ",
    },
    {
        "r  r  ",
        "llllrr",
        "r  r  ",
            "      ",
        "      ",
        "      ",
},
    {
        " yyyy ",
        " y yy ",
        "l yyyl",
        "lyyyyl",
        " yyyy ",
        " yyyy ",
    },
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

static int enemyColors[4] = {RED, CYAN, YELLOW, GREEN};

typedef struct {
  Vector pos;
  float vy;
  float fireInterval;
  float fireTicks;
  int color;
  bool isFalling;
  bool isAlive;
} Enemy;
// Unhit enemies fall until off-screen; spawn interval shrinks as
// difficulty^-1.5 while lifetime only shrinks as difficulty^-1, so
// concurrent count grows ~8*sqrt(difficulty) over a long session - sized
// with headroom well above that.
#define THROWM_MAX_ENEMY_COUNT 512
static Enemy enemies[THROWM_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;

typedef struct {
  Vector pos;
  bool isAlive;
} Bullet;
#define THROWM_MAX_BULLET_COUNT 64
static Bullet bullets[THROWM_MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  Vector pos;
  float vy;
  bool hasFireAngle;
  float fireAngle;
} Player;
static Player player;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Shot;
#define THROWM_MAX_SHOT_COUNT 32
static Shot shots[THROWM_MAX_SHOT_COUNT];
static int shotIndex;

static int multiplier;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    vectorSet(&player.pos, 90, 50);
    player.vy = -1;
    player.hasFireAngle = false;
    INIT_UNALIVED_ARRAY(shots);
    shotIndex = 0;
    multiplier = 1;
  }
  color = LIGHT_BLACK;
  rect(85, 0, 15, 10);
  rect(85, 92, 15, 8);
  color = BLACK;
  FOR_EACH(shots, si) {
    ASSIGN_ARRAY_ITEM(shots, si, Shot, s);
    SKIP_IS_NOT_ALIVE(s);
    vectorAdd(&s->pos, s->vel.x, s->vel.y);
    s->vel.y += difficulty * 0.07;
    character("e", VEC_XY(s->pos));
    if (s->pos.y > 103) {
      s->isAlive = false;
      continue;
    }
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    float fireInterval = rnd(200, 300) / sqrt(difficulty);
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, rnd(3, 50), -5);
    ne->vy = rnd(0.1, 0.4) * difficulty;
    ne->fireInterval = fireInterval;
    ne->fireTicks = rnd(0, fireInterval);
    ne->color = enemyColors[rndi(0, 4)];
    ne->isFalling = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, THROWM_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(50, 60) / difficulty / sqrt(difficulty);
  }
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.y += e->vy;
    bool isHit = false;
    if (e->isFalling) {
      e->vy += 0.1;
    } else {
      e->fireTicks--;
      if (e->fireTicks < 0) {
        ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
        nb->pos = e->pos;
        nb->isAlive = true;
        bulletIndex = wrap(bulletIndex + 1, 0, THROWM_MAX_BULLET_COUNT);
        e->fireTicks = e->fireInterval;
      }
      color = e->color;
      Collision ec = character("c", e->pos.x, e->pos.y - 6);
      if (ec.isColliding.character['e']) {
        isHit = true;
      }
    }
    color = BLACK;
    characterOptions.isMirrorY = e->isFalling;
    Collision ec2 = character("b", VEC_XY(e->pos));
    characterOptions.isMirrorY = false;
    if (ec2.isColliding.character['e'] && !e->isFalling) {
      isHit = true;
    }
    if (isHit) {
      play(POWER_UP);
      particle(e->pos.x, e->pos.y - 6, 16, 1, 0, M_PI * 2);
      e->isFalling = true;
      addScore(multiplier, VEC_XY(e->pos));
      multiplier *= 2;
    }
    if (e->pos.y > 105) {
      e->isAlive = false;
      continue;
    }
  }
  float bs = difficulty * 0.5;
  color = BLACK;
  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->pos.x += bs;
    Collision bc = character("d", VEC_XY(b->pos));
    if (bc.isColliding.character['e']) {
      play(HIT);
      particle(b->pos.x, b->pos.y, 16, 1, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(b->pos));
      multiplier++;
      b->isAlive = false;
      continue;
    }
    if (b->pos.x > 103 || bc.isColliding.rect[LIGHT_BLACK]) {
      b->isAlive = false;
      continue;
    }
  }
  player.pos.y += player.vy * difficulty * 0.5;
  if ((player.pos.y < 19 && player.vy < 0) ||
      (player.pos.y > 90 && player.vy > 0)) {
    player.vy *= -1;
  }
  color = BLUE;
  characterOptions.isMirrorX = true;
  Collision pc = character("c", player.pos.x, player.pos.y - 6);
  characterOptions.isMirrorX = false;
  if (pc.isColliding.character['d']) {
    play(EXPLOSION);
    gameOver();
  }
  color = BLACK;
  characterOptions.isMirrorX = true;
  character("b", VEC_XY(player.pos));
  characterOptions.isMirrorX = false;
  if (!player.hasFireAngle) {
    if (input.isJustPressed) {
      player.fireAngle = (M_PI / 4) * 3;
      player.hasFireAngle = true;
    }
  }
  if (player.hasFireAngle) {
    player.fireAngle += 0.1 * difficulty;
    color = BLACK;
    Vector lineEnd;
    vectorSet(&lineEnd, player.pos.x, player.pos.y);
    addWithAngle(&lineEnd, player.fireAngle, 5);
    thickness = 2;
    line(VEC_XY(player.pos), VEC_XY(lineEnd));
    if (input.isJustReleased || player.fireAngle > (M_PI / 8) * 11) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(shots, shotIndex, Shot, ns);
      ns->pos = player.pos;
      vectorSet(&ns->vel, 0, 0);
      addWithAngle(&ns->vel, player.fireAngle, sqrt(difficulty) * 3);
      ns->isAlive = true;
      shotIndex = wrap(shotIndex + 1, 0, THROWM_MAX_SHOT_COUNT);
      player.hasFireAngle = false;
      multiplier = 1;
    }
  }
}

void addGameThrowM() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
