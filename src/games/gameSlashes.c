#include "cglp.h"

static char *title = "SLASHES";
static char *description = "[Tap] Turn";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = true};

typedef struct {
  Vector pos;
  float angle;
  float width;
  bool isAppearing;
  bool isAlive;
} Slash;
#define MAX_SLASH_COUNT 32
static Slash slashes[MAX_SLASH_COUNT];
static int slashIndex;
static float slashAddTicks;
static float playerAngle;
static float speed;
static float addingScore;

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(slashes);
    slashIndex = 0;
    ASSIGN_ARRAY_ITEM(slashes, slashIndex, Slash, s0);
    vectorSet(&s0->pos, 22, 50);
    s0->angle = M_PI / 4;
    s0->width = 30;
    s0->isAppearing = false;
    s0->isAlive = true;
    slashIndex = wrap(slashIndex + 1, 0, MAX_SLASH_COUNT);
    slashAddTicks = 0;
    playerAngle = 1;
    speed = 1;
    addingScore = 0;
  }
  if (input.isJustPressed) {
    play(SELECT);
    playerAngle *= -1;
  }
  speed += (difficulty - speed) * 0.03;
  Vector scroll;
  vectorSet(&scroll, speed * playerAngle, speed);
  slashAddTicks -= difficulty;
  if (slashAddTicks < 0) {
    float w = rndi(20, 50) + 20;
    ASSIGN_ARRAY_ITEM(slashes, slashIndex, Slash, ns);
    vectorSet(&ns->pos, rndi(-99, 199), -w);
    if (rnd(0, 1) < 0.5) {
      ns->angle = -M_PI / 4;
    } else {
      ns->angle = M_PI / 4;
    }
    ns->width = w;
    ns->isAppearing = true;
    ns->isAlive = true;
    slashIndex = wrap(slashIndex + 1, 0, MAX_SLASH_COUNT);
    slashAddTicks += rnd(10, 15);
  }
  FOR_EACH(slashes, i) {
    ASSIGN_ARRAY_ITEM(slashes, i, Slash, s);
    SKIP_IS_NOT_ALIVE(s);
    color = LIGHT_PURPLE;
    thickness = 22;
    barCenterPosRatio = 0.5;
    bar(s->pos.x, s->pos.y, s->width, s->angle);
  }
  FOR_EACH(slashes, i) {
    ASSIGN_ARRAY_ITEM(slashes, i, Slash, s);
    SKIP_IS_NOT_ALIVE(s);
    color = PURPLE;
    if (s->isAppearing) {
      thickness = 20;
    } else {
      thickness = 4;
    }
    barCenterPosRatio = 0.5;
    Collision sc = bar(s->pos.x, s->pos.y, s->width, s->angle);
    if (s->isAppearing) {
      if (sc.isColliding.rect[PURPLE]) {
        s->isAlive = false;
        continue;
      }
      s->isAppearing = false;
      s->width -= 20;
    }
    vectorAdd(&s->pos, scroll.x, scroll.y);
    s->isAlive = s->pos.y < 99 + s->width;
  }
  color = BLACK;
  float pa = -(playerAngle * M_PI) / 4 + M_PI / 2;
  thickness = 3;
  barCenterPosRatio = 0.9;
  Collision pc = bar(50, 90, 4, pa);
  if (pc.isColliding.rect[PURPLE]) {
    play(EXPLOSION);
    gameOver();
  } else if (pc.isColliding.rect[LIGHT_PURPLE]) {
    play(LASER);
    speed += 0.03 * difficulty;
    addingScore += speed;
    particle(50, 90, 3, speed * 2, pa, 0.3);
  } else {
    particle(50, 90, 1, speed, pa, 0);
    if (addingScore > 0) {
      play(POWER_UP);
      addScore(addingScore, 50, 90);
      addingScore = 0;
    }
  }
}

void addGameSlashes() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
