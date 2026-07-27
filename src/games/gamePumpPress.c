#include "cglp.h"

static char *title = "PUMP PRESS";
static char *description = "[Tap]  Shot\n[Hold] Speed up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " lll  ",
        " lccl ",
        "rrrrrr",
        "rrrrrr",
        " llll ",
        " lll  ",
    },
    {
        "    r ",
        "llllrr",
        "    r ",
            "      ",
        "      ",
        "      ",
},
    {
        "  ll  ",
        " ll ll",
        "yyyyy ",
        "yyyyy ",
        " ll ll",
        "  ll  ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 50, .soundSeed = 7, .isDarkColor = false};

typedef struct {
  float x;
  float vx;
  float size;
  bool isPressed;
  Vector pressedOfs;
  bool isAlive;
} Enemy;
#define MAX_ENEMY_COUNT 64
static Enemy enemies[MAX_ENEMY_COUNT];
static int enemyIndex;
static float nextEnemyDist;

static bool hasRocket;
static float rocketX;
static bool isRocketGoing;
static float nextRocketDist;
static float shipX;
static float shipSpeed;
static bool hasShot;
static float shotX;
static float shotRange;
static int pressedCount;
static float scr;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyDist = 0;
    hasRocket = false;
    rocketX = 0;
    isRocketGoing = false;
    nextRocketDist = 30;
    shipX = 100;
    shipSpeed = 1;
    hasShot = false;
    shotX = 0;
    shotRange = 0;
    pressedCount = 0;
    scr = 0;
  }
  color = LIGHT_PURPLE;
  rect(0, 20, 200, 1);
  rect(0, 29, 200, 1);
  color = BLACK;
  shipX += difficulty * shipSpeed;
  float speedTarget;
  if (input.isPressed) {
    speedTarget = 1;
  } else {
    speedTarget = 0.5;
  }
  shipSpeed += (speedTarget - shipSpeed) * 0.1;
  if (input.isPressed) {
    particle(shipX, 25, 1, 2, M_PI, 0.3);
  }
  scr = difficulty * 0.1;
  if (shipX > 100) {
    scr += (shipX - 100) * 0.1;
  }
  shipX -= scr;
  character("a", shipX, 25);
  if (!hasShot && input.isJustPressed) {
    play(LASER);
    hasShot = true;
    shotX = shipX + 6;
    shotRange = 60;
  }
  if (hasShot) {
    float s = difficulty * 3;
    shotX += s - scr;
    shotRange -= s;
    character("b", shotX, 25);
    if (shotRange < 0) {
      hasShot = false;
    }
  }
  nextEnemyDist -= scr;
  if (nextEnemyDist < 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    ne->x = 205;
    ne->vx = rnd(0.5, 0.4 + 0.1 * sqrt(difficulty)) * difficulty;
    ne->size = 0;
    ne->isPressed = false;
    vectorSet(&ne->pressedOfs, rnd(0, 2) * RNDPM() - 4, rnd(0, 2) * RNDPM());
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, MAX_ENEMY_COUNT);
    nextEnemyDist = rnd(40 - sqrt(difficulty) * 10, 40) / sqrt(difficulty) + 3;
  }
  if (hasRocket) {
    rocketX -= scr;
    if (isRocketGoing) {
      particle(rocketX, 25, 1, difficulty, 0, 0.4);
      rocketX -= difficulty;
    }
    Collision rc = character("c", rocketX, 25);
    if (!isRocketGoing && rc.isColliding.character['b']) {
      isRocketGoing = true;
      hasShot = false;
    }
    if (rocketX < 5) {
      if (pressedCount > 0) {
        play(EXPLOSION);
        addScore(((1 + pressedCount) * pressedCount) / 2.0, 20, 20);
        particle(rocketX, 25, pressedCount * 5, sqrt(pressedCount) * 0.5 + 2, 0, M_PI * 2);
      }
      hasRocket = false;
      pressedCount = 0;
    }
  } else {
    nextRocketDist -= scr;
    if (nextRocketDist < 0) {
      hasRocket = true;
      rocketX = 205;
      isRocketGoing = false;
      nextRocketDist = 100;
    }
  }
  FOR_EACH(enemies, i) {
    ASSIGN_ARRAY_ITEM(enemies, i, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float y = 24;
    if (!e->isPressed) {
      if (e->size > 0) {
        e->size -= difficulty * 0.02;
        if (e->size < 0) {
          e->size = 0;
        }
      } else {
        float dir;
        if (e->x < shipX) {
          dir = 1;
        } else {
          dir = -1;
        }
        e->x += e->vx * dir;
      }
      e->x -= scr;
    } else {
      if (!hasRocket) {
        e->isAlive = false;
        continue;
      }
      e->x = rocketX + e->pressedOfs.x;
      y += e->pressedOfs.y;
    }
    float s = ceil(e->size) * 2 + 2;
    color = RED;
    rect(e->x - s, y - s, s * 2, 2);
    rect(e->x - s, y + s, s * 2, 2);
    rect(e->x - s, y - s, 2, s * 2);
    rect(e->x + s - 2, y - s, 2, s * 2);
    color = YELLOW;
    Collision cc = rect(e->x - s, y, s * 2, 2);
    if (!e->isPressed) {
      if (cc.isColliding.character['b']) {
        hasShot = false;
        e->size = ceil(e->size) + 1;
        if (e->size > 3) {
          play(EXPLOSION);
          color = RED;
          particle(e->x, 25, 16, 1, 0, M_PI * 2);
          e->isAlive = false;
          continue;
        } else {
          play(HIT);
        }
      }
      if (isRocketGoing && cc.isColliding.character['c']) {
        e->isPressed = true;
        pressedCount++;
      }
      if (e->size == 0 && cc.isColliding.character['a']) {
        play(POWER_UP);
        gameOver();
      }
      if (e->x < -5) {
        e->isAlive = false;
        continue;
      }
    }
  }
}

void addGamePumpPress() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
