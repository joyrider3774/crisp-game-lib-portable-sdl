#include "cglp.h"

static char *title = "FOOT LASER";
static char *description = "[Tap]\n Jump / Double jump / Descent";

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
        "ll    ",
        " ll   ",
        " ll l ",
        "llllll",
        "      ",
        "      ",
    },
    {
        "      ",
        "    l ",
        "llllll",
        " ll   ",
        " ll   ",
        "ll    ",
    },
};
static int charactersCount = 6;

// Vircon32 port note: dropped isDrawingScoreFront (no equivalent option
// in this port). Also, "rewind()" on death has no equivalent here - this
// engine has no rewind/replay-snapshot system at all - so it becomes
// gameOver() below (see the two call sites), same adaptation as
// gameFoosan.c.
static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

#define FOOTLASER_FLOOR_HEIGHT 90
#define FOOTLASER_MAX_JUMP_COUNT 2

typedef struct {
  Vector pos;
  bool isAlive;
} FootlaserShot;
#define FOOTLASER_MAX_SHOT_COUNT 16

typedef struct {
  Vector pos;
  float vy;
  int jumpCount;
  bool isOnFloor;
  int multiplier;
  FootlaserShot shots[FOOTLASER_MAX_SHOT_COUNT];
  int shotIndex;
  float nextShotTicks;
} FootlaserPlayer;
static FootlaserPlayer footlaserPlayer;

typedef struct {
  Vector pos;
  float vx;
  bool isFlying;
  bool isAlive;
} FootlaserEnemy;
#define FOOTLASER_MAX_ENEMY_COUNT 64
static FootlaserEnemy footlaserEnemies[FOOTLASER_MAX_ENEMY_COUNT];
static int footlaserEnemyIndex;
static float footlaserNextEnemyTicks;
static float footlaserNextWallTicks;
static float footlaserNextFlyingTicks;
static float footlaserFloorX;
static float footlaserAnimTicks;

static void update() {
  if (!ticks) {
    vectorSet(&footlaserPlayer.pos, 20, 50);
    footlaserPlayer.vy = 0;
    footlaserPlayer.jumpCount = 9;
    footlaserPlayer.isOnFloor = false;
    footlaserPlayer.multiplier = 1;
    INIT_UNALIVED_ARRAY(footlaserPlayer.shots);
    footlaserPlayer.shotIndex = 0;
    footlaserPlayer.nextShotTicks = 0;
    INIT_UNALIVED_ARRAY(footlaserEnemies);
    footlaserEnemyIndex = 0;
    footlaserNextEnemyTicks = 0;
    footlaserNextWallTicks = rnd(300, 400);
    footlaserNextFlyingTicks = rnd(200, 300);
    footlaserFloorX = 0;
    footlaserAnimTicks = 0;
  }
  float df = sqrt(difficulty);
  footlaserAnimTicks += df;
  color = LIGHT_BLACK;
  rect(footlaserFloorX, FOOTLASER_FLOOR_HEIGHT, 210, 9);
  rect(footlaserFloorX + 212, FOOTLASER_FLOOR_HEIGHT, 210, 9);
  footlaserFloorX -= df;
  if (footlaserFloorX < -209) {
    footlaserFloorX += 212;
  }
  if (!footlaserPlayer.isOnFloor) {
    float vyAdd;
    if (input.isPressed) {
      vyAdd = 0.1;
    } else {
      vyAdd = 0.3;
    }
    footlaserPlayer.vy += vyAdd * df;
    footlaserPlayer.pos.y += footlaserPlayer.vy;
    if (footlaserPlayer.pos.y > FOOTLASER_FLOOR_HEIGHT) {
      play(HIT);
      footlaserPlayer.pos.y = FOOTLASER_FLOOR_HEIGHT;
      footlaserPlayer.isOnFloor = true;
      footlaserPlayer.jumpCount = 0;
      footlaserPlayer.multiplier = 1;
    }
    footlaserPlayer.nextShotTicks--;
    if (footlaserPlayer.nextShotTicks < 0) {
      ASSIGN_ARRAY_ITEM(footlaserPlayer.shots, footlaserPlayer.shotIndex, FootlaserShot, ns);
      vectorSet(&ns->pos, footlaserPlayer.pos.x + 2, footlaserPlayer.pos.y + 9);
      ns->isAlive = true;
      footlaserPlayer.shotIndex =
          wrap(footlaserPlayer.shotIndex + 1, 0, FOOTLASER_MAX_SHOT_COUNT);
      footlaserPlayer.nextShotTicks += rnd(4, 9);
    }
  }
  if (input.isJustPressed) {
    if (footlaserPlayer.jumpCount == FOOTLASER_MAX_JUMP_COUNT) {
      play(LASER);
      footlaserPlayer.vy += 9 * sqrt(df);
    } else if (footlaserPlayer.jumpCount < FOOTLASER_MAX_JUMP_COUNT) {
      play(JUMP);
      footlaserPlayer.vy = -3 * sqrt(df);
      footlaserPlayer.isOnFloor = false;
    }
    footlaserPlayer.jumpCount++;
  }
  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  char pc[2];
  pc[0] = 'a' + ((int)(footlaserAnimTicks / 15) % 2);
  pc[1] = 0;
  character(pc, footlaserPlayer.pos.x + 3, footlaserPlayer.pos.y - 3);
  if (!footlaserPlayer.isOnFloor) {
    color = LIGHT_BLUE;
    rect(footlaserPlayer.pos.x + 2, footlaserPlayer.pos.y, 2,
         FOOTLASER_FLOOR_HEIGHT - footlaserPlayer.pos.y);
  }
  color = PURPLE;
  FOR_EACH(footlaserPlayer.shots, i) {
    ASSIGN_ARRAY_ITEM(footlaserPlayer.shots, i, FootlaserShot, s);
    SKIP_IS_NOT_ALIVE(s);
    if (s->pos.y > FOOTLASER_FLOOR_HEIGHT) {
      particle(footlaserPlayer.pos.x + 3, FOOTLASER_FLOOR_HEIGHT, 3, 3, -M_PI_2, M_PI / 7);
      s->isAlive = false;
      continue;
    }
    rect(s->pos.x, s->pos.y, 2, -9);
    s->pos.y += 6;
  }
  footlaserNextEnemyTicks--;
  footlaserNextWallTicks--;
  footlaserNextFlyingTicks--;
  if (footlaserNextEnemyTicks < 0) {
    float vx = -rnd(1, 2) * df;
    ASSIGN_ARRAY_ITEM(footlaserEnemies, footlaserEnemyIndex, FootlaserEnemy, ne);
    vectorSet(&ne->pos, 200, FOOTLASER_FLOOR_HEIGHT);
    ne->vx = vx;
    ne->isFlying = false;
    ne->isAlive = true;
    footlaserEnemyIndex = wrap(footlaserEnemyIndex + 1, 0, FOOTLASER_MAX_ENEMY_COUNT);
    footlaserNextEnemyTicks = rnd(30, 60) / difficulty;
  }
  if (footlaserNextWallTicks < 0) {
    float vx = -rnd(1, 2) * df;
    int c = rndi(3, 6);
    TIMES(c, i) {
      ASSIGN_ARRAY_ITEM(footlaserEnemies, footlaserEnemyIndex, FootlaserEnemy, ne);
      vectorSet(&ne->pos, 200, FOOTLASER_FLOOR_HEIGHT - i * 6);
      ne->vx = vx;
      ne->isFlying = false;
      ne->isAlive = true;
      footlaserEnemyIndex = wrap(footlaserEnemyIndex + 1, 0, FOOTLASER_MAX_ENEMY_COUNT);
    }
    footlaserNextWallTicks = rnd(100, 600) / difficulty;
    footlaserNextEnemyTicks += 9 / difficulty;
  }
  if (footlaserNextFlyingTicks < 0) {
    float vx2 = -rnd(1, 2) * df;
    int c2 = rndi(1, 5);
    Vector p;
    vectorSet(&p, 206, rnd(50, 80));
    TIMES(c2, i) {
      ASSIGN_ARRAY_ITEM(footlaserEnemies, footlaserEnemyIndex, FootlaserEnemy, ne);
      ne->pos = p;
      ne->vx = vx2;
      ne->isFlying = true;
      ne->isAlive = true;
      footlaserEnemyIndex = wrap(footlaserEnemyIndex + 1, 0, FOOTLASER_MAX_ENEMY_COUNT);
      p.x += 7;
    }
    footlaserNextFlyingTicks = rnd(100, 400) / difficulty;
    footlaserNextEnemyTicks += 9 / difficulty;
  }
  color = RED;
  characterOptions.isMirrorX = true;
  FOR_EACH(footlaserEnemies, i) {
    ASSIGN_ARRAY_ITEM(footlaserEnemies, i, FootlaserEnemy, e);
    SKIP_IS_NOT_ALIVE(e);
    e->pos.x += e->vx;
    char ec[2];
    if (e->isFlying) {
      ec[0] = 'e' + ((int)(footlaserAnimTicks / 20) % 2);
    } else {
      ec[0] = 'c' + ((int)(footlaserAnimTicks / 20) % 2);
    }
    ec[1] = 0;
    characterOptions.isMirrorX = true;
    Collision scratch = character(ec, e->pos.x + 3, e->pos.y - 3);
    if (scratch.isColliding.rect[LIGHT_BLUE]) {
      play(COIN);
      addScore(footlaserPlayer.multiplier, e->pos.x + footlaserPlayer.multiplier * 2, e->pos.y);
      particle(e->pos.x + 2, e->pos.y, 3, 2, -M_PI_2, M_PI);
      footlaserPlayer.multiplier++;
      e->isAlive = false;
      continue;
    } else if (scratch.isColliding.character['a'] || scratch.isColliding.character['b']) {
      play(EXPLOSION);
      gameOver();
    }
    if (e->pos.x < -6) {
      e->isAlive = false;
      continue;
    }
  }
  characterOptions.isMirrorX = false;
}

void addGameFootLaser() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
