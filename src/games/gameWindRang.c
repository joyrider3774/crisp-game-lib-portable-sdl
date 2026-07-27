#include "cglp.h"
#include "string.h"

static char *title = "WIND RANG";
static char *description = "[Tap] Toggle wind";

// Upstream's "b"/"e" glyphs only specify 5 rows; the 6th is added here as explicitly blank.
#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        " l  l ",
        " l  l ",
    },
    {
        "llllll",
        "llllll",
        "llllll",
        "llllll",
        "ll  ll",
        "      ",
    },
    {
        "    ll",
        "    ll",
        "    ll",
        "    ll",
        "llllll",
        "llllll",
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
};
static int charactersCount = 5;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define WINDRANG_WIND_FORCE 0.08
#define WINDRANG_WIND_LINE_SPEED 0.5
#define WINDRANG_WIND_LINE_ACCEL 0.05
#define WINDRANG_WIND_LINE_WIDTH 15
#define WINDRANG_WIND_LINE_HEIGHT 1
#define WINDRANG_BOOMERANG_SPEED 1.5
#define WINDRANG_BOOMERANG_INTERVAL 18
#define WINDRANG_ENEMY_SPAWN_INTERVAL 77
#define WINDRANG_ENEMY_SPEED 0.25
#define WINDRANG_CATCH_RADIUS 8
#define WINDRANG_PLAYER_Y 98

typedef struct {
  Vector pos;
  Vector vel;
  bool stateIsReturn;
  float angle;
  bool isAlive;
} Boomerang;
#define WINDRANG_MAX_BOOMERANG_COUNT 32
static Boomerang boomerangs[WINDRANG_MAX_BOOMERANG_COUNT];
static int boomerangIndex;

typedef struct {
  Vector pos;
  Vector vel;
  bool isAlive;
} Enemy;
// Sized well above the ~15-25 typical estimate since speed and spawn rate don't fully cancel out.
#define WINDRANG_MAX_ENEMY_COUNT 128
static Enemy enemies[WINDRANG_MAX_ENEMY_COUNT];
static int enemyIndex;

typedef struct {
  Vector pos;
  Vector vel;
} WindLine;
#define WINDRANG_WIND_LINE_COUNT 5
static WindLine windLines[WINDRANG_WIND_LINE_COUNT];

static float nextBoomerangTicks;
static float nextEnemyTicks;
static bool isWindLeft;
static int multiplier;
static float playerX;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(boomerangs);
    boomerangIndex = 0;
    nextBoomerangTicks = WINDRANG_BOOMERANG_INTERVAL;
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = WINDRANG_ENEMY_SPAWN_INTERVAL;
    isWindLeft = false;
    multiplier = 1;
    playerX = 50;
    TIMES(WINDRANG_WIND_LINE_COUNT, wli) {
      WindLine *w = &windLines[wli];
      vectorSet(&w->pos, rnd(0, 100), 20 + wli * 15);
      vectorSet(&w->vel, WINDRANG_WIND_LINE_SPEED, rnd(-0.2, 0.2));
    }
  }

  if (input.isJustPressed) {
    isWindLeft = !isWindLeft;
    play(SELECT);
  }

  color = LIGHT_BLUE;
  TIMES(WINDRANG_WIND_LINE_COUNT, wli2) {
    WindLine *w = &windLines[wli2];
    if (isWindLeft) {
      w->vel.x -= WINDRANG_WIND_LINE_ACCEL;
    } else {
      w->vel.x += WINDRANG_WIND_LINE_ACCEL;
    }
    w->vel.y += rnd(-WINDRANG_WIND_LINE_ACCEL, WINDRANG_WIND_LINE_ACCEL) * 0.2;
    w->vel.x *= 0.98;
    w->vel.y *= 0.98;
    vectorAdd(&w->pos, w->vel.x, w->vel.y);
    if (w->pos.x < -WINDRANG_WIND_LINE_WIDTH) {
      w->pos.x = 100 + WINDRANG_WIND_LINE_WIDTH;
    } else if (w->pos.x > 100 + WINDRANG_WIND_LINE_WIDTH) {
      w->pos.x = -WINDRANG_WIND_LINE_WIDTH;
    }
    if (w->pos.y < 0) {
      w->pos.y = 100;
    } else if (w->pos.y > 100) {
      w->pos.y = 0;
    }
    box(VEC_XY(w->pos), WINDRANG_WIND_LINE_WIDTH, WINDRANG_WIND_LINE_HEIGHT);
  }

  color = BLACK;
  char playerChar[2];
  playerChar[0] = 'a' + ((int)(ticks / 15) % 2);
  playerChar[1] = 0;
  character(playerChar, playerX, WINDRANG_PLAYER_Y);

  nextBoomerangTicks--;
  if (nextBoomerangTicks <= 0) {
    ASSIGN_ARRAY_ITEM(boomerangs, boomerangIndex, Boomerang, nb);
    vectorSet(&nb->pos, playerX, WINDRANG_PLAYER_Y);
    vectorSet(&nb->vel, 0, -WINDRANG_BOOMERANG_SPEED);
    nb->stateIsReturn = false;
    nb->angle = 0;
    nb->isAlive = true;
    boomerangIndex = wrap(boomerangIndex + 1, 0, WINDRANG_MAX_BOOMERANG_COUNT);
    nextBoomerangTicks = WINDRANG_BOOMERANG_INTERVAL;
    play(CLICK);
  }

  FOR_EACH(boomerangs, bi) {
    ASSIGN_ARRAY_ITEM(boomerangs, bi, Boomerang, b);
    SKIP_IS_NOT_ALIVE(b);
    float windForce;
    if (isWindLeft) {
      windForce = -WINDRANG_WIND_FORCE;
    } else {
      windForce = WINDRANG_WIND_FORCE;
    }
    b->vel.x += windForce;
    vectorAdd(&b->pos, b->vel.x, b->vel.y);
    b->angle += 0.2;

    if (!b->stateIsReturn && b->pos.y < 10) {
      b->stateIsReturn = true;
      b->vel.y = WINDRANG_BOOMERANG_SPEED;
    }

    color = BLUE;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = (int)floor(fmod(b->angle / M_PI_2, 4));
    char boomChar[2];
    boomChar[0] = 'c';
    boomChar[1] = 0;
    character(boomChar, VEC_XY(b->pos));

    if (b->stateIsReturn && b->pos.y > WINDRANG_PLAYER_Y - 5) {
      float dist = fabs(b->pos.x - playerX);
      if (dist < WINDRANG_CATCH_RADIUS) {
        play(POWER_UP);
        multiplier++;
        addScore(10 * multiplier, VEC_XY(b->pos));
        particle(VEC_XY(b->pos), 10, 2, 0, M_PI * 2);
        b->isAlive = false;
        continue;
      } else if (b->pos.y > 105) {
        b->isAlive = false;
        continue;
      }
    }

    if (b->pos.x < -5 || b->pos.x > 105 || b->pos.y > 105) {
      b->isAlive = false;
      continue;
    }
  }

  nextEnemyTicks--;
  if (nextEnemyTicks <= 0) {
    ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
    vectorSet(&ne->pos, rnd(10, 90), 0);
    vectorSet(&ne->vel, 0, WINDRANG_ENEMY_SPEED * rnd(1, difficulty));
    ne->isAlive = true;
    enemyIndex = wrap(enemyIndex + 1, 0, WINDRANG_MAX_ENEMY_COUNT);
    nextEnemyTicks = (WINDRANG_ENEMY_SPAWN_INTERVAL * rnd(0.5, 2)) / difficulty;
  }

  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    float windForce2;
    if (isWindLeft) {
      windForce2 = -WINDRANG_WIND_FORCE * 0.25;
    } else {
      windForce2 = WINDRANG_WIND_FORCE * 0.25;
    }
    e->vel.x += windForce2;
    vectorAdd(&e->pos, e->vel.x, e->vel.y);

    if (e->pos.x < 0) {
      e->pos.x = 100;
    } else if (e->pos.x > 100) {
      e->pos.x = 0;
    }

    if (e->pos.y > 100) {
      play(EXPLOSION);
      gameOver();
    }

    color = RED;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = false;
    characterOptions.rotation = 0;
    char enemyChar[2];
    enemyChar[0] = 'd' + ((int)(ticks / 15) % 2);
    enemyChar[1] = 0;
    Collision scratch = character(enemyChar, VEC_XY(e->pos));

    if (scratch.isColliding.character['c']) {
      play(COIN);
      particle(VEC_XY(e->pos), 10, 3, 0, M_PI * 2);
      addScore(1 * multiplier, VEC_XY(e->pos));
      e->isAlive = false;
      continue;
    }

    if (e->pos.y > WINDRANG_PLAYER_Y - 5 && fabs(e->pos.x - playerX) < 5) {
      play(EXPLOSION);
      gameOver();
    }
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameWindRang() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
