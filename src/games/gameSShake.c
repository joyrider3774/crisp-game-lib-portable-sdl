#include "cglp.h"

static char *title = "S SHAKE";
static char *description = "[Tap] Shake";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
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
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  Vector pos;
  float angle;
  float height;
} Ground;
#define SSHAKE_GROUND_COUNT 22
static Ground grounds[SSHAKE_GROUND_COUNT];
static float heightRatio;

typedef struct {
  Vector pos;
  Vector vel;
  bool isOnGround;
  float ticks;
  bool isAlive;
} Enemy;
// Concurrent enemy count grows roughly with difficulty^1.5 (spawn interval
// shrinks as 1/difficulty^2 while lifetime only shrinks as ~1/sqrt(difficulty)
// via the speed formula below) - at difficulty 1 that's already ~11
// concurrent enemies, ~59 at difficulty 4, ~150 at difficulty 9. Sized well
// above what a long play session can reach so the ring buffer never wraps
// into a still-alive enemy (which silently replaces it - looks exactly like
// enemies vanishing when new ones spawn).
#define SSHAKE_MAX_ENEMY_COUNT 512
static Enemy enemies[SSHAKE_MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;
static int multiplier;

static void update() {
  if (!ticks) {
    float angle = 0;
    TIMES(SSHAKE_GROUND_COUNT, i) {
      angle += (M_PI * 4) / 20;
      vectorSet(&grounds[i].pos, i * 10, 0);
      grounds[i].angle = angle;
      grounds[i].height = 9;
    }
    heightRatio = 1;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    multiplier = 1;
  }
  float scr = difficulty * 0.3;
  if (input.isJustPressed) {
    play(JUMP);
    heightRatio = 3;
    multiplier = 1;
  }
  heightRatio += (1 - heightRatio) * 0.05;
  float maxY = 0;
  TIMES(SSHAKE_GROUND_COUNT, i) {
    Ground *g = &grounds[i];
    g->pos.x += scr;
    if (g->pos.x > 210) {
      g->pos.x -= 220;
      Ground *ng = &grounds[(int)wrap(i + 1, 0, 22)];
      g->angle = ng->angle - ((M_PI * 4) / 20) * rnd(0.5, 1.5);
      g->height = ng->height + rnd(0, 1) * RNDPM();
      g->height += (9 - g->height) * 0.05;
    }
    g->pos.y = sin(g->angle) * g->height;
    if (g->pos.y > maxY) {
      maxY = g->pos.y;
    }
  }
  bool hasPp = false;
  Vector pp;
  TIMES(SSHAKE_GROUND_COUNT, i) {
    Ground *g = &grounds[i];
    g->pos.y = (g->pos.y - maxY) * heightRatio + 99;
    if (hasPp && pp.x < g->pos.x) {
      line(VEC_XY(pp), VEC_XY(g->pos));
    }
    pp = g->pos;
    hasPp = true;
  }
  Vector fp = grounds[0].pos;
  Vector lp = grounds[SSHAKE_GROUND_COUNT - 1].pos;
  if (lp.x < fp.x) {
    line(VEC_XY(lp), VEC_XY(fp));
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, 203, 50);
    vectorSet(&ne->vel, -rnd(1, sqrt(difficulty)) * 0.3 * sqrt(difficulty), 0);
    ne->isOnGround = true;
    ne->ticks = 0;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, SSHAKE_MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(0, 120) / difficulty / difficulty;
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, VEC_XY(e->vel));
    e->ticks -= e->vel.x;
    color = TRANSPARENT;
    if (e->isOnGround) {
      if (input.isJustPressed) {
        float vyy = 0;
        TIMES(99, k) {
          e->pos.y--;
          vyy--;
          Collision bc = box(VEC_XY(e->pos), 6, 6);
          if (bc.isColliding.rect[BLACK]) {
            e->vel.y = vyy * sqrt(difficulty) * 0.3;
            e->isOnGround = false;
            break;
          }
        }
      }
      Collision bc2 = box(VEC_XY(e->pos), 6, 6);
      if (bc2.isColliding.rect[BLACK]) {
        TIMES(99, k) {
          e->pos.y--;
          Collision bc3 = box(VEC_XY(e->pos), 6, 6);
          if (!bc3.isColliding.rect[BLACK]) {
            break;
          }
        }
      } else {
        TIMES(99, k) {
          e->pos.y++;
          Collision bc4 = box(VEC_XY(e->pos), 6, 6);
          if (bc4.isColliding.rect[BLACK]) {
            e->pos.y--;
            break;
          }
        }
      }
    } else {
      e->vel.y += 0.03 * difficulty;
      Collision bc5 = box(VEC_XY(e->pos), 6, 6);
      if (bc5.isColliding.rect[BLACK]) {
        e->isOnGround = true;
        e->vel.y = 0;
      } else if (e->vel.y > 0) {
        float ey = e->pos.y;
        TIMES(9, k) {
          ey -= 3;
          Collision bc6 = box(e->pos.x, ey, 6, 6);
          if (bc6.isColliding.rect[BLACK]) {
            e->pos.y = ey - 5;
            e->isOnGround = true;
            e->vel.y = 0;
            break;
          }
        }
      }
    }
    color = BLACK;
    char ec[2];
    ec[0] = 'a' + (int)floor(e->ticks / 9) % 2;
    ec[1] = 0;
    characterOptions.isMirrorX = true;
    character(ec, VEC_XY(e->pos));
    characterOptions.isMirrorX = false;
    if (e->pos.y < -3) {
      play(COIN);
      addScore(multiplier, e->pos.x, clamp(9 + multiplier * 3, 9, 60));
      multiplier++;
      e->isAlive = false;
      continue;
    }
    if (e->pos.x < 3) {
      play(EXPLOSION);
      gameOver();
    }
    e->isAlive = e->pos.y <= 103;
  }
}

void addGameSShake() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
