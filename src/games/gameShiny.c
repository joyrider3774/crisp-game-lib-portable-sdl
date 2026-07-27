#include "cglp.h"

static char *title = "SHINY";
static char *description = "[Hold] Rainy";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "  l   ",
        "lllll ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "   ll ",
        "   l  ",
        "lllll ",
        "  l   ",
        "ll ll ",
        "     l",
    },
    {
        "  lll ",
        " l l l",
        "  lll ",
        "   l  ",
        "llll  ",
        "    l ",
    },
    {
        " lll  ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l lll",
        "l     ",
    },
    {
        "l l l ",
        "l l l ",
        " lll  ",
        "  l   ",
        " l l  ",
        "l   l ",
    },
    {
        "  ll  ",
        " llll ",
        "llllll",
        "llllll",
        " llll ",
        "  ll  ",
    },
};
static int charactersCount = 6;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

#define SHINY_GROUND_COUNT 22
#define SHINY_RIGHT_EDGE_GROUND_INDEX 20
#define SHINY_HUMAN_Y 89

typedef struct {
  Vector pos;
  float size;
} Ground;
static Ground grounds[SHINY_GROUND_COUNT];

typedef struct {
  Vector pos;
  Vector rainyPos;
  Vector shinyPos;
  float radius;
  float weakDropTicks;
  float strongDropTicks;
} Cloud;
static Cloud clouds[30];

#define SHINY_MAX_HUMAN_COUNT 16
typedef struct {
  Vector pos;
  float speed;
  float ticks;
  bool isRunning;
  bool isFalling;
  bool isAlive;
} Human;
static Human humans[SHINY_MAX_HUMAN_COUNT];
static int humansCount;

#define SHINY_DROP_TYPE_WEAK 0
#define SHINY_DROP_TYPE_STRONG 1
typedef struct {
  Vector pos;
  Vector vel;
  int type;
  bool isAlive;
} Drop;
#define SHINY_MAX_DROP_COUNT 128
static Drop drops[SHINY_MAX_DROP_COUNT];
static int dropIndex;

static float rainyRatio;
static float windY;

static void addDrop(float x, float y, int type) {
  ASSIGN_ARRAY_ITEM(drops, dropIndex, Drop, nd);
  vectorSet(&nd->pos, x, y);
  vectorSet(&nd->vel, (rnd(0, 0.2) * RNDPM() + windY) * difficulty, difficulty);
  nd->type = type;
  nd->isAlive = true;
  dropIndex = wrap(dropIndex + 1, 0, SHINY_MAX_DROP_COUNT);
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(humans);
    INIT_UNALIVED_ARRAY(drops);
    dropIndex = 0;
    TIMES(SHINY_GROUND_COUNT, gi) {
      vectorSet(&grounds[gi].pos, (gi - 1) * 10 + 5, 95);
      grounds[gi].size = 8;
    }
    TIMES(30, ci) {
      Cloud *c = &clouds[ci];
      vectorSet(&c->rainyPos, ((ci + 1) / 31.0) * 199 + rnd(0, 3) * RNDPM(), rnd(5, 15));
      float shinyX;
      if (c->rainyPos.x < 99) {
        shinyX = -20;
      } else {
        shinyX = 220;
      }
      vectorSet(&c->shinyPos, shinyX, c->rainyPos.y);
      c->pos = c->shinyPos;
      c->radius = rnd(5, 10);
      c->weakDropTicks = rnd(0, 99);
      c->strongDropTicks = rnd(100, 900);
    }
    rainyRatio = 0;
    humansCount = 0;
    windY = 0;
  }
  COUNT_IS_ALIVE(humans, emptyCheckCount);
  if (emptyCheckCount == 0) {
    if (humansCount < 9) {
      humansCount++;
    }
    INIT_UNALIVED_ARRAY(humans);
    TIMES(humansCount, hi) {
      Human *h = &humans[hi];
      vectorSet(&h->pos, -4, SHINY_HUMAN_Y);
      h->speed = rnd(0.1, 0.15) * difficulty;
      h->ticks = rndi(0, 999);
      h->isRunning = false;
      h->isFalling = false;
      h->isAlive = true;
    }
    grounds[SHINY_RIGHT_EDGE_GROUND_INDEX].size = 8;
  }
  bool isRainy = input.isPressed;
  if (input.isJustPressed) {
    play(HIT);
    windY = rnd(0, 0.5) * RNDPM() * sqrt(difficulty);
    grounds[SHINY_RIGHT_EDGE_GROUND_INDEX].size -= 0.1;
  }
  if (input.isJustReleased) {
    play(HIT);
  }
  if (!isRainy && rainyRatio > 0) {
    rainyRatio -= 1;
  } else if (isRainy && rainyRatio < 10) {
    rainyRatio += 1;
  }
  float rr = rainyRatio / 10;
  color = YELLOW;
  Vector p1;
  vectorSet(&p1, 100, 10);
  character("f", VEC_XY(p1));
  if (rr < 1) {
    Vector p2;
    TIMES(7, i) {
      float a = ticks * 0.05 + (i * M_PI * 2) / 7.0;
      float l = fabs(sin(i + ticks * 0.05) * 5 * (1 - rr)) + 10;
      vectorSet(&p1, 100, 10);
      addWithAngle(&p1, a, 7);
      vectorSet(&p2, 100, 10);
      addWithAngle(&p2, a, l);
      thickness = 3;
      line(VEC_XY(p1), VEC_XY(p2));
    }
  }
  color = LIGHT_BLACK;
  TIMES(30, ci2) {
    Cloud *c = &clouds[ci2];
    vectorSet(&c->pos, c->rainyPos.x * rr + c->shinyPos.x * (1 - rr),
              c->rainyPos.y * rr + c->shinyPos.y * (1 - rr));
    if (c->pos.y > -9 && c->pos.y < 209) {
      box(c->pos.x, c->pos.y, c->radius * 2, c->radius * 2);
    }
    c->weakDropTicks--;
    if (c->weakDropTicks < 0) {
      if (isRainy) {
        addDrop(c->pos.x, c->pos.y + c->radius, SHINY_DROP_TYPE_WEAK);
      }
      c->weakDropTicks = rnd(100, 200);
    }
    c->strongDropTicks--;
    if (c->strongDropTicks < 0) {
      if (isRainy) {
        addDrop(c->pos.x, c->pos.y + c->radius, SHINY_DROP_TYPE_STRONG);
      }
      c->strongDropTicks = rnd(500, 999) / difficulty;
    }
  }
  TIMES(SHINY_GROUND_COUNT, gi2) {
    Ground *g = &grounds[gi2];
    if (gi2 == SHINY_RIGHT_EDGE_GROUND_INDEX) {
      color = BLACK;
      if (g->size >= 1) {
        box(g->pos.x, g->pos.y - (4 - g->size / 2), 8, g->size);
        g->size -= 0.005 * difficulty;
      }
    } else {
      if (g->size < 8) {
        color = LIGHT_BLACK;
        box(VEC_XY(g->pos), g->size, g->size);
        g->size += 0.12 * difficulty;
      } else {
        color = BLACK;
        box(VEC_XY(g->pos), 8, 8);
      }
    }
  }
  COUNT_IS_ALIVE(humans, humansAliveThisFrame);
  color = RED;
  FOR_EACH(humans, hi2) {
    ASSIGN_ARRAY_ITEM(humans, hi2, Human, h);
    SKIP_IS_NOT_ALIVE(h);
    int ci3;
    if (h->isRunning) {
      ci3 = 2 + (int)floor(h->ticks / 15) % 2;
    } else {
      ci3 = 0 + (int)floor(h->ticks / 30) % 2;
    }
    if (!h->isFalling) {
      float mul;
      if (h->isRunning) {
        mul = 5;
      } else {
        mul = 1;
      }
      h->pos.x += h->speed * mul;
    } else {
      ci3 = 4;
    }
    char hc[2];
    hc[0] = 'a' + ci3;
    hc[1] = 0;
    Collision hcoll = character(hc, VEC_XY(h->pos));
    if (!hcoll.isColliding.rect[BLACK]) {
      h->isFalling = true;
      h->pos.y += difficulty;
      if (h->pos.y > 103) {
        humansCount--;
        if (humansCount <= 0) {
          play(RANDOM);  // Equivalent to "lucky" in JS
          gameOver();
        } else {
          play(EXPLOSION);
        }
        h->isAlive = false;
        continue;
      }
    } else {
      h->pos.y = SHINY_HUMAN_Y;
      h->isFalling = false;
    }
    h->ticks++;
    int ti = (int)floor(30 / difficulty);
    if (ti > 0 && (int)h->ticks % ti == 0) {
      h->isRunning = isRainy;
    }
    if (h->pos.x > 203) {
      play(POWER_UP);
      int s = (int)floor(grounds[20].size * humansAliveThisFrame);
      if (s > 0) {
        addScore(s, 190, 80);
      }
      h->isAlive = false;
      continue;
    }
  }
  FOR_EACH(drops, di) {
    ASSIGN_ARRAY_ITEM(drops, di, Drop, d);
    SKIP_IS_NOT_ALIVE(d);
    vectorAdd(&d->pos, VEC_XY(d->vel));
    if (d->type == SHINY_DROP_TYPE_STRONG) {
      color = BLUE;
      Vector dEnd;
      dEnd = d->vel;
      vectorMul(&dEnd, 4.0 / vectorLength(&dEnd));
      vectorAdd(&dEnd, VEC_XY(d->pos));
      thickness = 3;
      Collision dc = line(d->pos.x, d->pos.y, dEnd.x, dEnd.y);
      if (dc.isColliding.character['a'] || dc.isColliding.character['b'] ||
          dc.isColliding.character['c'] || dc.isColliding.character['d']) {
        d->isAlive = false;
        continue;
      }
    } else {
      color = LIGHT_BLUE;
      Vector dEnd2;
      dEnd2 = d->vel;
      vectorMul(&dEnd2, 3.0 / vectorLength(&dEnd2));
      vectorAdd(&dEnd2, VEC_XY(d->pos));
      thickness = 2;
      line(d->pos.x, d->pos.y, dEnd2.x, dEnd2.y);
    }
    if (d->pos.y > 90) {
      if (d->type == SHINY_DROP_TYPE_STRONG) {
        int gi3 = (int)floor(d->pos.x / 10) + 1;
        if (gi3 >= 2 && gi3 < 20) {
          play(SELECT);
          grounds[gi3].size = 0;
        }
      }
      d->isAlive = false;
      continue;
    }
  }
}

void addGameShiny() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
