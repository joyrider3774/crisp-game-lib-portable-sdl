#include "cglp.h"

static char *title = "METEO PLANET";
static char *description = "[Tap] Move";

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
        " lll  ",
        "l l l ",
        "l lll ",
        "ll ll ",
        " lll  ",
        "      ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = true};

typedef struct {
  float dist;
  float angle;
  int type;
  bool isAlive;
} MeteoplanetFalling;
#define METEOPLANET_MAX_FALLING_COUNT 64
static MeteoplanetFalling fallings[METEOPLANET_MAX_FALLING_COUNT];
static int fallingIndex;
static float nextFallingsTicks;
static float nextFallingAngle;
static float angle;
static float targetAngle;
static float animTicks;

typedef struct {
  float dist;
  float angle;
} MeteoplanetStar;
#define METEOPLANET_STAR_COUNT 24
static MeteoplanetStar stars[METEOPLANET_STAR_COUNT];

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(fallings);
    fallings[0].dist = 53;
    fallings[0].angle = rndi(0, 4) * M_PI_2;
    fallings[0].type = 0;
    fallings[0].isAlive = true;
    fallingIndex = 1;
    nextFallingsTicks = 0;
    nextFallingAngle = rndi(1, 4);
    angle = 0;
    targetAngle = 0;
    animTicks = 0;
    TIMES(METEOPLANET_STAR_COUNT, i) {
      stars[i].dist = rnd(10, 70);
      stars[i].angle = rnd(0, M_PI * 2);
    }
  }
  float sd = sqrt(difficulty);
  color = LIGHT_BLACK;
  TIMES(METEOPLANET_STAR_COUNT, i) {
    MeteoplanetStar *s = &stars[i];
    Vector sp;
    vectorSet(&sp, 50, 50);
    addWithAngle(&sp, s->angle - angle, s->dist);
    box(VEC_XY(sp), 1, 1);
  }
  color = BLACK;
  if (input.isJustPressed) {
    play(SELECT);
    targetAngle += M_PI_2;
  }
  if (angle < targetAngle) {
    angle += 0.3 * sd;
    if (angle > targetAngle) {
      angle = targetAngle;
      if (angle > M_PI * 2.2) {
        angle = M_PI_2;
        targetAngle = M_PI_2;
      }
    }
    animTicks += sd;
  }
  char pc[2];
  pc[0] = 'a' + ((int)(animTicks / 3) % 2);
  pc[1] = 0;
  character(pc, 50, 42);
  thickness = 2;
  arc(50, 50, 3, -angle + M_PI * 0.2, -angle + M_PI * 2.2);
  nextFallingsTicks--;
  if (nextFallingsTicks < 0) {
    int cc = rndi(0, 6);
    float dist = 70;
    float fallAngle = nextFallingAngle * M_PI_2;
    TIMES(11, i) {
      int type = (int)fabs((float)(i - 5));
      if (type <= cc) {
        int newType;
        if (type == 0) {
          newType = 0;
        } else {
          newType = cc - type + 1;
        }
        ASSIGN_ARRAY_ITEM(fallings, fallingIndex, MeteoplanetFalling, nf);
        nf->dist = dist;
        nf->angle = fallAngle;
        nf->type = newType;
        nf->isAlive = true;
        fallingIndex = wrap(fallingIndex + 1, 0, METEOPLANET_MAX_FALLING_COUNT);
      }
      dist += 6;
    }
    nextFallingsTicks = rnd(30, 50) / sqrt(sd);
    nextFallingAngle += rndi(1, 4);
  }
  FOR_EACH(fallings, i) {
    ASSIGN_ARRAY_ITEM(fallings, i, MeteoplanetFalling, f);
    SKIP_IS_NOT_ALIVE(f);
    f->dist -= 0.5 * sd;
    Vector fp;
    vectorSet(&fp, 50, 50);
    addWithAngle(&fp, f->angle - angle, f->dist);
    Collision col;
    if (f->type == 0) {
      color = BLACK;
      col = character("c", VEC_XY(fp));
      if (col.isColliding.character['a'] || col.isColliding.character['b']) {
        play(EXPLOSION);
        gameOver();
      }
    } else {
      color = YELLOW;
      col = box(fp.x, fp.y, f->type, f->type);
      if (col.isColliding.character['a'] || col.isColliding.character['b']) {
        play(POWER_UP);
        addScore(f->type, VEC_XY(fp));
        f->isAlive = false;
        continue;
      }
    }
    if (f->dist < 5) {
      if (f->type == 0) {
        play(HIT);
        particle(fp.x, fp.y, 9, 1, 0, M_PI * 2);
      }
      f->isAlive = false;
      continue;
    }
  }
}

void addGameMeteoPlanet() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
