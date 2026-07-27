#include "cglp.h"

static char *title = "REVOLVE A";
static char *description = "[Tap]\n Go forward";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " rrr  ",
    "rrRrr ",
    "rRrLr ",
    "rrLLr ",
    " rrr  ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define LINE_DIST 30

typedef struct {
  Vector pos;
  Vector vel;
  bool isRemoved;
  bool isAlive;
} Enemy;
// Spawn interval ~ 1/difficulty but enemy speed (hence lifetime) only scales
// ~sqrt(difficulty), so concurrent enemies grow ~sqrt(difficulty) - already
// tight within the first few minutes of play, not just at extreme difficulty.
#define MAX_ENEMY_COUNT 512
static Enemy enemies[MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyTicks;
static int multiplier;

typedef struct {
  Vector pos;
  Vector vel;
  float angle;
} Arrow;
static Arrow arrow;

static void removeAroundEnemy(float px, float py) {
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (!e->isRemoved && distanceTo(&e->pos, px, py) < LINE_DIST) {
      e->isRemoved = true;
      removeAroundEnemy(VEC_XY(e->pos));
    }
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&arrow.pos, 50, 50);
    vectorSet(&arrow.vel, 1, 0);
    rotate(&arrow.vel, M_PI / 4);
    arrow.angle = -M_PI / 2;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    multiplier = 1;
  }
  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    float px = rnd(0, 99);
    float py;
    if (rnd(0, 1) < 0.5) {
      py = -3;
    } else {
      py = 103;
    }
    if (rnd(0, 1) < 0.5) {
      float tmp = px;
      px = py;
      py = tmp;
    }
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, px, py);
    float velAngle = angleTo(&ne->pos, rnd(10, 90), rnd(10, 90));
    vectorSet(&ne->vel, rnd(1, sqrt(difficulty)) * 0.3, 0);
    rotate(&ne->vel, velAngle);
    ne->isRemoved = false;
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MAX_ENEMY_COUNT);
    nextEnemyTicks = rnd(30, 40) / difficulty;
  }
  multiplier = 1;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    color = GREEN;
    if (e->isRemoved) {
      particle(VEC_XY(e->pos), 16, 1, 0, M_PI * 2);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      e->isAlive = false;
      continue;
    }
    vectorAdd(&e->pos, VEC_XY(e->vel));
    FOR_EACH(enemies, j) {
      ASSIGN_ARRAY_ITEM(enemies, j, Enemy, ae);
      SKIP_IS_NOT_ALIVE(ae);
      if (i == j || distanceTo(&e->pos, VEC_XY(ae->pos)) >= LINE_DIST) {
        continue;
      }
      thickness = 3;
      line(VEC_XY(e->pos), VEC_XY(ae->pos));
    }
    bool eInRect = e->pos.x >= -5 && e->pos.x < 105 && e->pos.y >= -5 && e->pos.y < 105;
    e->isAlive = eInRect;
  }
  color = BLACK;
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    character("a", VEC_XY(e->pos));
  }
  if (input.isJustPressed) {
    play(LASER);
    vectorSet(&arrow.vel, 1, 0);
    rotate(&arrow.vel, arrow.angle);
  }
  if ((arrow.pos.x < 3 && arrow.vel.x < 0) || (arrow.pos.x > 97 && arrow.vel.x > 0)) {
    arrow.vel.x *= -1;
  }
  if ((arrow.pos.y < 3 && arrow.vel.y < 0) || (arrow.pos.y > 97 && arrow.vel.y > 0)) {
    arrow.vel.y *= -1;
  }
  vectorAdd(&arrow.pos, arrow.vel.x * sqrt(difficulty) * 0.4, arrow.vel.y * sqrt(difficulty) * 0.4);
  arrow.angle += 0.08 * sqrt(difficulty);
  if (input.isJustPressed) {
    color = RED;
  } else {
    color = BLUE;
  }
  Vector p;
  vectorSet(&p, VEC_XY(arrow.pos));
  addWithAngle(&p, arrow.angle, 2);
  Vector q1;
  vectorSet(&q1, VEC_XY(arrow.pos));
  addWithAngle(&q1, arrow.angle + M_PI, 2);
  thickness = 2;
  Collision c1 = line(VEC_XY(p), VEC_XY(q1));
  Vector q2;
  vectorSet(&q2, VEC_XY(arrow.pos));
  addWithAngle(&q2, arrow.angle + M_PI / 2, 2);
  thickness = 2;
  Collision c2 = line(VEC_XY(p), VEC_XY(q2));
  Vector q3;
  vectorSet(&q3, VEC_XY(arrow.pos));
  addWithAngle(&q3, arrow.angle - M_PI / 2, 2);
  thickness = 2;
  Collision c3 = line(VEC_XY(p), VEC_XY(q3));
  bool hitChar = c1.isColliding.character['a'] || c2.isColliding.character['a'] ||
                 c3.isColliding.character['a'];
  bool hitGreen = c1.isColliding.rect[GREEN] || c2.isColliding.rect[GREEN] ||
                  c3.isColliding.rect[GREEN];
  if (hitChar) {
    play(EXPLOSION);
    gameOver();
  } else if (hitGreen) {
    play(POWER_UP);
    removeAroundEnemy(VEC_XY(arrow.pos));
  }
}

void addGameRevolveA() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
