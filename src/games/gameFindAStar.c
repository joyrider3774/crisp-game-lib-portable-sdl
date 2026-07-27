#include "cglp.h"

static char *title = "FIND A STAR";
static char *description = "[Tap] Open";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " lll  ",
        "l   l ",
        "l   l ",
        "lllll ",
        "l l l ",
        "lllll ",
    },
    {
        " lll  ",
        "l  l  ",
        "l l   ",
        "lllll ",
        "l   l ",
        "lllll ",
    },
    {
        "  ll  ",
        "l ll l",
        " llll ",
        " llll ",
        " l  l ",
        "l    l",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

#define FINDASTAR_BOX_COUNT 16
#define FINDASTAR_BOX_LEFT_X 5

typedef struct {
  float y;
  int sx;
  bool isOpened[FINDASTAR_BOX_COUNT];
  bool isAlive;
} FindastarBoxLine;
#define FINDASTAR_MAX_LINE_COUNT 32
static FindastarBoxLine findastarBoxLines[FINDASTAR_MAX_LINE_COUNT];
static int findastarBoxLineIndex;
static float findastarBoxLineAddDist;

typedef struct {
  Vector pos;
  float vy;
  float angle;
  float score;
  bool isAlive;
} FindastarStar;
#define FINDASTAR_MAX_STAR_COUNT 16
static FindastarStar findastarStars[FINDASTAR_MAX_STAR_COUNT];
static int findastarStarIndex;
static float findastarPvy;

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped box
  // column is computed directly from input.pos via grid math (see "ibx"
  // below), so the engine's own O(n^2) hitbox scan (see checkHitBox() in
  // cglp.c) is pure waste here. Restored automatically when the next real
  // game starts, via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    INIT_UNALIVED_ARRAY(findastarBoxLines);
    findastarBoxLineIndex = 0;
    findastarBoxLineAddDist = 0;
    INIT_UNALIVED_ARRAY(findastarStars);
    findastarStarIndex = 0;
    findastarPvy = 0;
  }
  int ibx = floor((input.pos.x - FINDASTAR_BOX_LEFT_X + 3) / 6);
  if (input.isJustPressed && ibx >= 0 && ibx < FINDASTAR_BOX_COUNT) {
    play(LASER);
    color = BLUE;
    rect(FINDASTAR_BOX_LEFT_X + ibx * 6, 0, 1, 99);
    findastarPvy += 2;
  }
  float scr = difficulty * 0.06 + findastarPvy;
  findastarPvy *= 0.07;
  findastarBoxLineAddDist -= scr;
  if (findastarBoxLineAddDist < 0) {
    play(SELECT);
    ASSIGN_ARRAY_ITEM(findastarBoxLines, findastarBoxLineIndex, FindastarBoxLine, nl);
    nl->y = -3;
    nl->sx = rndi(0, FINDASTAR_BOX_COUNT);
    TIMES(FINDASTAR_BOX_COUNT, i) { nl->isOpened[i] = false; }
    nl->isAlive = true;
    findastarBoxLineIndex = wrap(findastarBoxLineIndex + 1, 0, FINDASTAR_MAX_LINE_COUNT);
    findastarBoxLineAddDist += 5 + 5 / difficulty;
  }
  int lc = 0;
  int ml = 1;
  FOR_EACH(findastarBoxLines, li) {
    ASSIGN_ARRAY_ITEM(findastarBoxLines, li, FindastarBoxLine, l);
    SKIP_IS_NOT_ALIVE(l);
    lc++;
    if (l->y < 9) {
      l->y += (9 - l->y) * 0.2;
    } else if (input.isJustPressed && ibx >= 0 && ibx < FINDASTAR_BOX_COUNT) {
      if (ibx == l->sx) {
        play(COIN);
        ASSIGN_ARRAY_ITEM(findastarStars, findastarStarIndex, FindastarStar, ns);
        vectorSet(&ns->pos, FINDASTAR_BOX_LEFT_X + ibx * 6, l->y);
        ns->vy = 1;
        ns->angle = 0;
        ns->score = lc * lc * ml;
        ns->isAlive = true;
        findastarStarIndex = wrap(findastarStarIndex + 1, 0, FINDASTAR_MAX_STAR_COUNT);
        ml++;
        l->isAlive = false;
        continue;
      } else if (ibx > l->sx) {
        for (int i = ibx; i < FINDASTAR_BOX_COUNT; i++) {
          l->isOpened[i] = true;
        }
      } else {
        for (int i = 0; i <= ibx; i++) {
          l->isOpened[i] = true;
        }
      }
    }
    l->y += scr;
    TIMES(FINDASTAR_BOX_COUNT, i) {
      char bc[2];
      if (l->isOpened[i]) {
        color = LIGHT_BLUE;
        bc[0] = 'b';
      } else {
        color = BLUE;
        bc[0] = 'a';
      }
      bc[1] = 0;
      character(bc, FINDASTAR_BOX_LEFT_X + i * 6, l->y);
    }
    if (l->y > 97) {
      play(EXPLOSION);
      gameOver();
    }
  }
  COUNT_IS_ALIVE(findastarBoxLines, aliveLineCount);
  if (aliveLineCount == 0) {
    findastarBoxLineAddDist = 0;
  }
  color = YELLOW;
  FOR_EACH(findastarStars, si) {
    ASSIGN_ARRAY_ITEM(findastarStars, si, FindastarStar, s);
    SKIP_IS_NOT_ALIVE(s);
    // The original scales the sprite horizontally by cos(angle) for a
    // spinning-coin effect; this engine's character() only supports
    // mirror/rotation, not arbitrary scale, so this approximates the spin
    // as a flip at the halfway point instead of a continuous squish.
    characterOptions.isMirrorX = cos(s->angle) < 0;
    character("c", VEC_XY(s->pos));
    characterOptions.isMirrorX = false;
    s->pos.y += s->vy;
    s->vy *= 0.9;
    s->angle += 0.2;
    if (s->angle > M_PI * 2) {
      play(POWER_UP);
      addScore(s->score, s->pos.x, s->pos.y);
      s->isAlive = false;
      continue;
    }
  }
}

void addGameFindAStar() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
