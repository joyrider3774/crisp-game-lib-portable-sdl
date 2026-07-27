#include "cglp.h"
#include "string.h"

static char *title = "TEETER";
static char *description = "[Tap]\n Change angle";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    " llll ",
    "l llll",
    "llllll",
    "llllll",
    "llllll",
    " llll ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 60, .isDarkColor = false};

typedef struct {
  Vector pos;
  float angle;
  float width;
  bool isAlive;
} Bar;
// Bars never expire on their own - only "landed on a negative-barCount
// floor" removes them, while landing on a positive one adds up to 4 more;
// a lucky/skilled run can keep net-adding bars indefinitely across many
// rounds (game only ends when zero bars remain), so keep well above the
// ~30-40 that can physically fit without overlapping.
#define TEETER_MAX_BAR_COUNT 128
static Bar bars[TEETER_MAX_BAR_COUNT];
static int barIndex;

static int barAngleSign;
static bool hasBall;
static Vector ballTargetPos;
static Vector ballPos;

typedef struct {
  Vector pos;
  float width;
  float score;
  float barCount;
  bool isAlive;
} Floor;
#define TEETER_MAX_FLOOR_COUNT 16
static Floor floors[TEETER_MAX_FLOOR_COUNT];
static int floorIndex;

static bool addBar() {
  color = WHITE;
  FOR_EACH(bars, bi) {
    ASSIGN_ARRAY_ITEM(bars, bi, Bar, b);
    SKIP_IS_NOT_ALIVE(b);
    thickness = 6;
    bar(b->pos.x, b->pos.y, b->width + 5, b->angle);
    bar(b->pos.x, b->pos.y, b->width + 5, -b->angle);
  }
  color = TRANSPARENT;
  bool isPlaced = false;
  TIMES(99, ti) {
    float width = rnd(12, 18);
    float angle = rnd(0.2, 0.8) * RNDPM();
    float aw = width * cos(angle);
    float px = rnd(5 + aw / 2, 95 - aw / 2);
    float py = rnd(20, 70);
    thickness = 6;
    Collision c1 = bar(px, py, width + 5, angle);
    Collision c2 = bar(px, py, width + 5, -angle);
    if (!c1.isColliding.rect[WHITE] && !c2.isColliding.rect[WHITE]) {
      ASSIGN_ARRAY_ITEM(bars, barIndex, Bar, nb);
      vectorSet(&nb->pos, px, py);
      nb->angle = angle;
      nb->width = width;
      nb->isAlive = true;
      barIndex = wrap(barIndex + 1, 0, TEETER_MAX_BAR_COUNT);
      isPlaced = true;
      break;
    }
  }
  return isPlaced;
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(bars);
    barIndex = 0;
    TIMES(7, i) { addBar(); }
    barAngleSign = 1;
    hasBall = false;
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
  }
  if (!hasBall) {
    play(LASER);
    float x = 50;
    TIMES(99, i) {
      x = rnd(20, 80);
      bool isOnBar = false;
      FOR_EACH(bars, bi2) {
        ASSIGN_ARRAY_ITEM(bars, bi2, Bar, b2);
        SKIP_IS_NOT_ALIVE(b2);
        if (fabs(b2->pos.x - x) < (b2->width / 2) * cos(b2->angle)) {
          isOnBar = true;
        }
      }
      if (isOnBar) {
        break;
      }
    }
    vectorSet(&ballTargetPos, x, -4);
    ballPos = ballTargetPos;
    hasBall = true;
    INIT_UNALIVED_ARRAY(floors);
    floorIndex = 0;
    int waveStart = floorIndex;
    int waveCount = 0;
    float fw = 0;
    while (fw < 100) {
      float width = rnd(20, 30);
      ASSIGN_ARRAY_ITEM(floors, floorIndex, Floor, nf);
      vectorSet(&nf->pos, fw + width / 2, 95);
      nf->width = width;
      nf->score = floor(rnd(1, 3.1) * rnd(1, 3.1));
      nf->barCount = 0;
      nf->isAlive = true;
      floorIndex = wrap(floorIndex + 1, 0, TEETER_MAX_FLOOR_COUNT);
      waveCount++;
      fw += width;
    }
    int fi = rndi(0, waveCount);
    Floor *ff1 = &floors[(waveStart + fi) % TEETER_MAX_FLOOR_COUNT];
    ff1->score = 0;
    ff1->barCount = floor(rnd(1, 2.2) * rnd(1, 2.2));
    fi = rndi(0, waveCount);
    Floor *ff2 = &floors[(waveStart + fi) % TEETER_MAX_FLOOR_COUNT];
    ff2->score = 0;
    ff2->barCount = -floor(rnd(1, 2.2) * rnd(1, 2.2));
    TIMES(waveCount, wi) {
      Floor *wf = &floors[(waveStart + wi) % TEETER_MAX_FLOOR_COUNT];
      wf->pos.x -= (fw - 100) / 2;
    }
  }
  ballTargetPos.y += difficulty * 0.5;
  vectorAdd(&ballPos, (ballTargetPos.x - ballPos.x) * 0.5,
            (ballTargetPos.y - ballPos.y) * 0.5);
  color = BLACK;
  character("a", VEC_XY(ballPos));
  if (input.isJustPressed) {
    play(SELECT);
    barAngleSign *= -1;
  }
  FOR_EACH(bars, bi3) {
    ASSIGN_ARRAY_ITEM(bars, bi3, Bar, b3);
    SKIP_IS_NOT_ALIVE(b3);
    float a = b3->angle * barAngleSign;
    thickness = 3;
    Collision bc = bar(b3->pos.x, b3->pos.y, b3->width, a);
    if (bc.isColliding.character['a']) {
      play(HIT);
      float dir;
      if (a > 0) {
        dir = 1;
      } else {
        dir = -1;
      }
      vectorSet(&ballTargetPos, b3->pos.x, b3->pos.y);
      addWithAngle(&ballTargetPos, a, (b3->width / 2 + 7) * dir);
    }
  }
  float barCountDiff = 0;
  FOR_EACH(floors, fi2) {
    ASSIGN_ARRAY_ITEM(floors, fi2, Floor, f);
    SKIP_IS_NOT_ALIVE(f);
    char t[16];
    int tColor;
    if (f->score > 0) {
      strcpy(t, intToChar((int)f->score));
      if (f->score < 5) {
        tColor = LIGHT_BLACK;
      } else {
        tColor = BLACK;
      }
    } else if (f->barCount < 0) {
      strcpy(t, intToChar((int)f->barCount));
      tColor = RED;
    } else {
      strcpy(t, "+");
      strcat(t, intToChar((int)f->barCount));
      tColor = BLUE;
    }
    color = tColor;
    Collision fc = box(f->pos.x, f->pos.y, f->width - 1, 10);
    if (fc.isColliding.character['a']) {
      particle(f->pos.x, f->pos.y, 16, 1, 0, M_PI * 2);
      if (f->score > 0) {
        play(COIN);
        addScore(f->score, VEC_XY(f->pos));
        f->score++;
      } else {
        barCountDiff += f->barCount;
      }
      f->isAlive = false;
      continue;
    }
    color = WHITE;
    int tLen = strlen(t);
    text(t, clamp(f->pos.x - (tLen - 1) * 3, 3, 97 - (tLen - 1) * 6), f->pos.y);
    if (f->pos.x > 99 + f->width / 2) {
      f->isAlive = false;
      continue;
    }
  }
  if (barCountDiff > 0) {
    int sc = 0;
    TIMES((int)barCountDiff, bci) {
      play(POWER_UP);
      if (!addBar()) {
        sc++;
      }
    }
    if (sc > 0) {
      addScore(sc, VEC_XY(ballPos));
    }
  } else if (barCountDiff < 0) {
    play(EXPLOSION);
    color = BLACK;
    int removeCount = (int)(-barCountDiff);
    TIMES(removeCount, rci) {
      COUNT_IS_ALIVE(bars, aliveCount);
      if (aliveCount == 0) {
        break;
      }
      int target = rndi(0, aliveCount);
      int seen = 0;
      FOR_EACH(bars, bi4) {
        ASSIGN_ARRAY_ITEM(bars, bi4, Bar, b4);
        if (!b4->isAlive) {
          continue;
        }
        if (seen == target) {
          particle(b4->pos.x, b4->pos.y, 16, 1, 0, M_PI * 2);
          b4->isAlive = false;
          break;
        }
        seen++;
      }
    }
  }
  if (ballPos.y > 99) {
    COUNT_IS_ALIVE(bars, finalAliveCount);
    if (finalAliveCount == 0) {
      play(RANDOM);  // Equivalent to "lucky" in JS
      gameOver();
    }
    hasBall = false;
  }
}

void addGameTeeter() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
