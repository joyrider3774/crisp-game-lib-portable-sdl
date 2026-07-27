#include "cglp.h"
#include "string.h"

static char *title = "G TAIL";
static char *description = "[Slide] Move";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " llll ",
        "llccll",
        "llccll",
        "llllll",
        "bbbbbb",
        "bb  bb",
    },
    {
        " rrrr ",
        "rRRrRr",
        "rrRRRr",
        "rRRrrr",
        "rrRrRr",
        " rrrr ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = true};

typedef struct {
  Vector pos;
  float vy;
  int color;
} Star;
#define GTAIL_MAX_STAR_COUNT 30
static Star stars[GTAIL_MAX_STAR_COUNT];
static int starColors[3] = {LIGHT_CYAN, LIGHT_RED, LIGHT_YELLOW};

// The original JS keeps up to 200 positions of history per meteor "just in
// case", but golds only ever reference an index up to
// (max gold count - 1 + 2) * 5 = 40 (see addMeteor's golds init below),
// so a much smaller cap already behaves identically while using far less
// RAM for MAX_METEOR_COUNT meteors at once.
#define GTAIL_MAX_POS_HISTORY 50
#define GTAIL_MAX_GOLD_COUNT 8
typedef struct {
  Vector pos;
  Vector vel;
  float accel;
  Vector posHistory[GTAIL_MAX_POS_HISTORY];
  int posHistoryCount;
  int golds[GTAIL_MAX_GOLD_COUNT];
  int goldCount;
  bool isAlive;
} Meteor;
#define GTAIL_MAX_METEOR_COUNT 32
static Meteor meteors[GTAIL_MAX_METEOR_COUNT];
static int meteorIndex;
static int nextMeteorTicks;
static int multiplier;
static Vector shipPos;

static void addMeteor() {
  ASSIGN_ARRAY_ITEM(meteors, meteorIndex, Meteor, m);
  vectorSet(&m->pos, rnd(10, 90), -5);
  vectorSet(&m->vel, rnd(0, 0.5) * RNDPM(), 0);
  m->accel = rnd(1, sqrt(difficulty));
  m->posHistoryCount = 0;
  m->goldCount = rndi(3, 8);
  TIMES(m->goldCount, i) { m->golds[i] = (i + 2) * 5; }
  m->isAlive = true;
  meteorIndex = wrap(meteorIndex + 1, 0, GTAIL_MAX_METEOR_COUNT);
}

static void shiftGoldsDown(Meteor *m, int removedIndex) {
  memcpy(&m->golds[removedIndex], &m->golds[removedIndex + 1],
         (m->goldCount - 1 - removedIndex) * sizeof(m->golds[0]));
  m->goldCount--;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(meteors);
    meteorIndex = 0;
    nextMeteorTicks = 0;
    multiplier = 1;
    vectorSet(&shipPos, 50, 50);
    TIMES(GTAIL_MAX_STAR_COUNT, i) {
      ASSIGN_ARRAY_ITEM(stars, i, Star, s);
      vectorSet(&s->pos, rnd(0, 99), rnd(0, 99));
      s->vy = rnd(1, 2);
      s->color = starColors[rndi(0, 3)];
    }
  }
  FOR_EACH(stars, i) {
    ASSIGN_ARRAY_ITEM(stars, i, Star, s);
    s->pos.y += s->vy;
    color = s->color;
    rect(s->pos.x, s->pos.y, 1, 1);
    if (s->pos.y > 110) {
      vectorSet(&s->pos, rnd(0, 99), -rnd(0, 9));
    }
  }
  if (shipPos.y > 99) {
    shipPos.y = 99;
    gameOver();
  }
  shipPos.x = clamp(input.pos.x, 0, 99);
  shipPos.y += (50 - shipPos.y) * 0.001;
  color = BLACK;
  float oy = clamp(shipPos.y - 55, 0, 99);
  character("a", shipPos.x + rnd(0, oy * 0.01) * RNDPM(),
            shipPos.y + rnd(0, oy * 0.01) * RNDPM());
  color = RED;
  particle(shipPos.x, shipPos.y + 2, 1, 2, M_PI / 2, M_PI / 8);
  nextMeteorTicks--;
  if (nextMeteorTicks < 0) {
    addMeteor();
    COUNT_IS_ALIVE(meteors, aliveMeteorCount);
    nextMeteorTicks =
        (rnd(99, 120) / sqrt(difficulty)) * sqrt(aliveMeteorCount);
  }
  FOR_EACH(meteors, i) {
    ASSIGN_ARRAY_ITEM(meteors, i, Meteor, m);
    SKIP_IS_NOT_ALIVE(m);

    Vector delta;
    vectorSet(&delta, shipPos.x, shipPos.y);
    vectorAdd(&delta, -m->pos.x, -m->pos.y);
    vectorMul(&delta, 1.0 / 999);
    vectorMul(&delta, m->accel);
    vectorAdd(&m->vel, delta.x, delta.y);

    m->vel.x = clamp(m->vel.x, -2, 2);
    m->vel.y = clamp(m->vel.y, -2, 2);
    vectorAdd(&m->pos, m->vel.x, m->vel.y);

    int shiftCount = m->posHistoryCount;
    if (shiftCount > GTAIL_MAX_POS_HISTORY - 1) {
      shiftCount = GTAIL_MAX_POS_HISTORY - 1;
    }
    for (int k = shiftCount - 1; k >= 0; k--) {
      m->posHistory[k + 1] = m->posHistory[k];
    }
    m->posHistory[0] = m->pos;
    if (m->posHistoryCount < GTAIL_MAX_POS_HISTORY) {
      m->posHistoryCount++;
    }

    color = YELLOW;
    int gi = 0;
    while (gi < m->goldCount) {
      bool goldRemoved = false;
      if (m->posHistoryCount > m->golds[gi]) {
        Vector gp = m->posHistory[m->golds[gi]];
        Collision gc = text("$", gp.x, gp.y);
        if (gc.isColliding.character['a']) {
          play(COIN);
          particle(gp.x, gp.y, 16, 1, 0, M_PI * 2);
          addScore(multiplier, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
          goldRemoved = true;
        }
      }
      if (goldRemoved) {
        shiftGoldsDown(m, gi);
      } else {
        gi++;
      }
    }

    if (m->goldCount == 0) {
      play(POWER_UP);
      m->pos.x = clamp(m->pos.x, 5, 95);
      m->pos.y = clamp(m->pos.y, 5, 95);
      addScore(multiplier, VEC_XY(m->pos));
      color = RED;
      particle(m->pos.x, m->pos.y, 20, 2, 0, M_PI * 2);
      multiplier++;
      m->isAlive = false;
      continue;
    }

    color = BLACK;
    Collision bc = character("b", VEC_XY(m->pos));
    if (bc.isColliding.character['a']) {
      play(EXPLOSION);
      color = BLACK;
      particle(shipPos.x, shipPos.y, 30, 3, 0, M_PI * 2);
      shipPos.y += 10;
      m->isAlive = false;
      continue;
    }

    bool isInSmallRect = m->pos.x >= -3 && m->pos.x < 103 && m->pos.y >= -3 &&
                         m->pos.y < 103;
    if (!isInSmallRect) {
      color = RED;
      float bx = clamp(m->pos.x, -3, 103);
      float by = clamp(m->pos.y, -3, 103);
      box(bx, by, 8, 8);
    }
    bool isInBigRect = m->pos.x >= -100 && m->pos.x < 200 && m->pos.y >= -100 &&
                       m->pos.y < 200;
    m->isAlive = isInBigRect;
  }
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameGTail() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
