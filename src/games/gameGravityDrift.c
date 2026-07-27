#include "cglp.h"
#include "string.h"

static char *title = "GRAVITY DRIFT";
static char *description = "[Hold] Thrust up";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "l ll l",
    "l ll l",
    "llllll",
    " l  l ",
    "ll  ll",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  float x;
  float y;
  float vy;
} Player;
static Player player;

typedef struct {
  float x;
  float y;
  float size;
  bool isAlive;
} Asteroid;
// Concurrent count stays ~self-similar (both spawn interval and lifetime
// scale as 1/spd), roughly 4-5 in practice - generous headroom applied.
#define GRAVITYDRIFT_MAX_ASTEROID_COUNT 32
static Asteroid asteroids[GRAVITYDRIFT_MAX_ASTEROID_COUNT];
static int asteroidIndex;

typedef struct {
  float x;
  float y;
  bool isAlive;
} Star;
#define GRAVITYDRIFT_MAX_STAR_COUNT 8
static Star stars[GRAVITYDRIFT_MAX_STAR_COUNT];
static int starIndex;

static bool reversed;
static int multiplier;

static void update() {
  if (!ticks) {
    player.x = 20;
    player.y = 50;
    player.vy = 0;
    INIT_UNALIVED_ARRAY(asteroids);
    asteroidIndex = 0;
    INIT_UNALIVED_ARRAY(stars);
    starIndex = 0;
    reversed = false;
    multiplier = 1;
  }

  float spd = 0.5 + difficulty * 0.5;

  float targetX;
  if (reversed) {
    targetX = 80;
  } else {
    targetX = 20;
  }
  player.x += (targetX - player.x) * 0.05;

  if (input.isPressed) {
    player.vy -= 0.25;
    color = CYAN;
    particle(player.x, player.y + 2, 1, 1, M_PI / 2, 0.5);
  }
  if (input.isJustPressed) {
    play(CLICK);
  }
  player.vy += 0.12;
  player.vy *= 0.95;
  player.y += player.vy;

  if (player.y < 0 || player.y > 100) {
    play(EXPLOSION);
    gameOver();
  }

  if (ticks % (int)ceil(25 / spd) == 0) {
    float targetY = player.y + rnd(-9, 9);
    targetY = clamp(targetY, 0, 100);
    ASSIGN_ARRAY_ITEM(asteroids, asteroidIndex, Asteroid, na);
    if (reversed) {
      na->x = -5;
    } else {
      na->x = 105;
    }
    na->y = targetY;
    na->size = rnd(5, 10);
    na->isAlive = true;
    asteroidIndex = wrap(asteroidIndex + 1, 0, GRAVITYDRIFT_MAX_ASTEROID_COUNT);
  }

  if (ticks % 120 == 0) {
    ASSIGN_ARRAY_ITEM(stars, starIndex, Star, ns);
    if (reversed) {
      ns->x = -5;
    } else {
      ns->x = 105;
    }
    ns->y = rnd(20, 80);
    ns->isAlive = true;
    starIndex = wrap(starIndex + 1, 0, GRAVITYDRIFT_MAX_STAR_COUNT);
  }

  color = CYAN;
  character("a", VEC_XY(player));

  color = YELLOW;
  FOR_EACH(stars, si) {
    ASSIGN_ARRAY_ITEM(stars, si, Star, s);
    SKIP_IS_NOT_ALIVE(s);
    if (reversed) {
      s->x += spd * 0.5;
    } else {
      s->x -= spd * 0.5;
    }
    Collision c = box(s->x, s->y, 4, 4);
    if (c.isColliding.character['a']) {
      play(POWER_UP);
      reversed = !reversed;
      multiplier++;
      s->isAlive = false;
      continue;
    }
    if (!(s->x > -10 && s->x < 110)) {
      s->isAlive = false;
    }
  }

  if (reversed) {
    color = BLUE;
  } else {
    color = RED;
  }
  FOR_EACH(asteroids, ai) {
    ASSIGN_ARRAY_ITEM(asteroids, ai, Asteroid, a);
    SKIP_IS_NOT_ALIVE(a);
    if (reversed) {
      a->x += spd;
    } else {
      a->x -= spd;
    }
    Collision c = box(a->x, a->y, a->size, a->size);
    if (c.isColliding.character['a']) {
      play(EXPLOSION);
      gameOver();
    }
    if (!(a->x > -10 && a->x < 110)) {
      a->isAlive = false;
    }
  }

  addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(multText, 3, 9);
}

void addGameGravityDrift() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
