#include "cglp.h"

static char *title = "HIT BLOW UP";
static char *description = "[Tap]\n Select color";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        " ll   ",
        "llll  ",
        "llll  ",
        " ll   ",
            "      ",
        "      ",
},
    {
        " ll   ",
        "l  l  ",
        "l  l  ",
        " ll   ",
            "      ",
        "      ",
},
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 4, .isDarkColor = false};

#define HITBLOWUP_MAX_TARGET_LEN 6
#define HITBLOWUP_MAX_HIST_COUNT 64

static int colors[6] = {RED, GREEN, BLUE, YELLOW, CYAN, PURPLE};

typedef struct {
  int colorsArr[HITBLOWUP_MAX_TARGET_LEN];
  int len;
  int hit;
  int blow;
} HistEntry;
static HistEntry hist[HITBLOWUP_MAX_HIST_COUNT];
static int histCount;

static bool isGoingNextStage;
static int stageCount;
static int loopCount;
static int selectorBase[6];
static int selector[6];
static int selectorLen;
static float selectorY;
static int target[HITBLOWUP_MAX_TARGET_LEN];
static int targetLen;
static int current[HITBLOWUP_MAX_TARGET_LEN];
static int currentIndex;
static float hitBlowTicks;
static float nextStageTicks;
static int hitCount;
static int blowCount;

static void drawAnswer() {
  float tx = 50 - ((targetLen - 1) / 2.0) * 7;
  TIMES(targetLen, i) {
    color = colors[target[i]];
    character("a", tx, selectorY + 8);
    tx += 7;
  }
}

static void update() {
  // Never reads a Collision result anywhere in this file (including the
  // drawAnswer() helper above) - the selected color is computed
  // directly from input.pos via grid math (see "si" below), so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    isGoingNextStage = true;
    stageCount = 0;
  }
  if (isGoingNextStage) {
    loopCount = (int)floor(stageCount / 6.0);
    int s = stageCount % 6;
    if (s == 0) {
      TIMES(6, i) { selectorBase[i] = i; }
      TIMES(99, k) {
        int n1 = rndi(0, 6);
        int n2 = rndi(0, 6);
        int t = selectorBase[n1];
        selectorBase[n1] = selectorBase[n2];
        selectorBase[n2] = t;
      }
    }
    int sc = 3 + (int)floor((s + 1) / 2.0);
    selectorLen = sc;
    TIMES(sc, i) { selector[i] = selectorBase[i]; }
    int tc = 2 + (int)floor(s / 2.0);
    TIMES(sc, i) { target[i] = selector[i]; }
    TIMES(99, k) {
      int n1 = rndi(0, sc);
      int n2 = rndi(0, sc);
      int t = target[n1];
      target[n1] = target[n2];
      target[n2] = t;
    }
    targetLen = tc;
    histCount = 0;
    selectorY = 90;
    isGoingNextStage = false;
    TIMES(tc, i) { current[i] = -1; }
    currentIndex = 0;
    hitBlowTicks = -1;
    nextStageTicks = -1;
  }
  if (nextStageTicks < 0) {
    selectorY -= (pow(2, loopCount) / (targetLen * targetLen + selectorLen)) * 0.05;
  }
  color = LIGHT_BLACK;
  rect(0, selectorY, 99, 99 - selectorY);
  float sx = 50 - ((selectorLen - 1) / 2.0) * 10;
  TIMES(selectorLen, i) {
    color = colors[selector[i]];
    character("a", sx, selectorY + 3);
    sx += 10;
  }
  if (nextStageTicks < 0 && input.isJustPressed) {
    int si = (int)floor((input.pos.x - 50) / 10 + selectorLen / 2.0);
    if (si >= 0 && si < selectorLen) {
      play(SELECT);
      current[currentIndex] = selector[si];
      currentIndex++;
      if (currentIndex == targetLen) {
        int hit = 0;
        int blow = 0;
        TIMES(targetLen, ti) {
          if (target[ti] == current[ti]) {
            hit++;
          } else {
            bool foundInCurrent = false;
            TIMES(targetLen, cj) {
              if (current[cj] == target[ti]) {
                foundInCurrent = true;
              }
            }
            if (foundInCurrent) {
              blow++;
            }
          }
        }
        if (histCount < HITBLOWUP_MAX_HIST_COUNT) {
          HistEntry *he = &hist[histCount];
          TIMES(targetLen, ci) { he->colorsArr[ci] = current[ci]; }
          he->len = targetLen;
          he->hit = hit;
          he->blow = blow;
          histCount++;
        }
        hitBlowTicks = 60;
        TIMES(targetLen, ci) { current[ci] = -1; }
        currentIndex = 0;
        if (hit == targetLen) {
          addScore((selectorY - histCount * 6) * (loopCount + 1), 70,
                   (selectorY - histCount * 6) / 2 + 9);
          nextStageTicks = 60;
        }
        hitCount = hit;
        blowCount = blow;
      }
    }
  }
  float hy = selectorY - 3;
  TIMES(histCount, hi) {
    HistEntry *hs = &hist[hi];
    float hx = 50 - ((hs->len - 1) / 2.0) * 7;
    TIMES(hs->len, ci) {
      color = colors[hs->colorsArr[ci]];
      character("a", hx, hy);
      hx += 7;
    }
    color = BLACK;
    text(intToChar(hs->hit), 3, hy);
    text(intToChar(hs->blow), 96, hy);
    hy -= 6;
  }
  hitBlowTicks--;
  if (hitBlowTicks > 0) {
    text("HIT", 10, hy + 6);
    text("BLOW", 70, hy + 6);
    if ((int)hitBlowTicks % 10 == 0) {
      if (hitCount > 0) {
        play(POWER_UP);
        hitCount--;
      } else if (blowCount > 0) {
        play(COIN);
        blowCount--;
      }
    }
  }
  nextStageTicks--;
  if (nextStageTicks >= 0) {
    color = BLACK;
    rect(50, 0, 1, hy + 3);
    rect(48, 0, 5, 1);
    rect(48, hy + 2, 5, 1);
    drawAnswer();
    if (nextStageTicks == 0) {
      stageCount++;
      isGoingNextStage = true;
    }
    return;
  }
  float cx = 50 - ((targetLen - 1) / 2.0) * 7;
  TIMES(targetLen, ci) {
    if (current[ci] < 0) {
      color = LIGHT_BLACK;
      character("b", cx, hy);
    } else {
      color = colors[current[ci]];
      character("a", cx, hy);
    }
    cx += 7;
  }
  if (nextStageTicks < 0 && hy < 3) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    drawAnswer();
    gameOver();
  }
}

void addGameHitBlowUp() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
