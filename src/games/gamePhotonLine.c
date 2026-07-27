#include "cglp.h"
#include "string.h"

static char *title = "PHOTON LINE";
static char *description = "[Tap]  Turn\n[Hold] Flick";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "rllllr",
        "lrllrl",
        "ll lll",
        "llllll",
        "lrllrl",
        "rllllr",
    },
    {
        "r pprr",
        " ppprr",
        "pp ppp",
        "pppppp",
        "rrppp ",
        "rrpp r",
    },
    {
        "r yyrr",
        " yyyrr",
        "yy yyy",
        "yyyyyy",
        "rryyy ",
        "rryy r",
    },
    {
        "r ccrr",
        " cccrr",
        "cc ccc",
        "cccccc",
        "rrccc ",
        "rrcc r",
    },
    {
        "r ggrr",
        " gggrr",
        "gg ggg",
        "gggggg",
        "rrggg ",
        "rrgg r",
    },
    {
        "r    b",
        " r  b ",
        "  rb  ",
        "  br  ",
        " b  r ",
        "b    r",
    },
    {
        "   l  ",
        "llllll",
        "l    l",
        " llll ",
        " ll   ",
        "l llll",
    },
    {
        "   l l",
        "llllll",
        "l   l ",
        "lll ll",
        "l l ll",
        "l l ll",
    },
};
static int charactersCount = 8;

static Options options = {
    .viewSizeX = 200, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

typedef struct {
  float x;
  float vx;
  int index;
  int w;
  bool isReflected;
  bool isAlive;
} Photon;
#define MAX_PHOTON_COUNT 64
static Photon photons[MAX_PHOTON_COUNT];
static int photonIndex;
static float nextPhotonTicks;

static float playerAngle;
static bool playerIsRotating;
static float playerTa;
// Real JS lengths stay small (splice(5) on target, a soft appWidth-halving
// cap past 9 on hands) - sized with generous headroom above both.
#define MAX_HAND_LEN 16
static int hands[2][MAX_HAND_LEN];
static int handsLen[2];

static int target[2][MAX_HAND_LEN];
static int targetLen[2];

static int world;
static int step;
static float nextStepTicks;
static float appWidth;
static int colorCount;

#define MAX_NEXT_PHOTONS 16
static int nextPhotons[MAX_NEXT_PHOTONS];
static int nextPhotonsLen;

static void update() {
  // Never reads a Collision result anywhere in this file - a photon
  // "hitting" a hand is a direct x-threshold comparison (see "hit" below),
  // so the engine's own O(n^2) hitbox scan is pure waste here.
  hasCollision = false;
  if (!ticks) {
    playerAngle = 0;
    playerIsRotating = false;
    playerTa = 0;
    hands[0][0] = 0;
    handsLen[0] = 1;
    hands[1][0] = 1;
    handsLen[1] = 1;
    target[0][0] = 0;
    targetLen[0] = 1;
    target[1][0] = 1;
    targetLen[1] = 1;
    INIT_UNALIVED_ARRAY(photons);
    photonIndex = 0;
    nextPhotonTicks = 0;
    world = 1;
    step = 1;
    nextStepTicks = 0;
    appWidth = 30;
    colorCount = 2;
    nextPhotonsLen = 0;
  }
  if (nextStepTicks == 0) {
    if (step > clamp(world + 1, 1, 5)) {
      step = 1;
      world++;
      colorCount = clamp(rndi(2, world + 2), 2, 4);
      target[0][0] = rndi(0, colorCount);
      targetLen[0] = 1;
      target[1][0] = rndi(0, colorCount);
      targetLen[1] = 1;
      hands[0][0] = target[0][0];
      handsLen[0] = 1;
      hands[1][0] = target[1][0];
      handsLen[1] = 1;
    }
    int prevTarget[2][MAX_HAND_LEN];
    int prevTargetLen[2];
    TIMES(2, pIdx) {
      prevTargetLen[pIdx] = targetLen[pIdx];
      TIMES(targetLen[pIdx], pj) { prevTarget[pIdx][pj] = target[pIdx][pj]; }
    }
    TIMES(2, ti) {
      if (targetLen[ti] > 4) {
        targetLen[ti] = 4;
      }
    }
    float cr = 3.0 / colorCount;
    int ac = (int)round(rnd(1, world) * cr);
    int dc = (int)round(clamp(rnd(0, world) - 1, 0, 2) * cr);
    if (ac == dc) {
      dc--;
    }
    TIMES(dc, dci) {
      int h = rndi(0, 2);
      if (targetLen[h] > 0) {
        targetLen[h]--;
      }
    }
    TIMES(ac + dc, aci) {
      int h = rndi(0, 2);
      if (targetLen[h] < MAX_HAND_LEN) {
        target[h][targetLen[h]] = rndi(0, colorCount);
        targetLen[h]++;
      }
    }
    TIMES(2, ti2) {
      if (targetLen[ti2] > 5) {
        targetLen[ti2] = 5;
      }
    }
    bool isSame = targetLen[0] == prevTargetLen[0] && targetLen[1] == prevTargetLen[1];
    if (isSame) {
      TIMES(2, si) {
        TIMES(targetLen[si], sj) {
          if (prevTarget[si][sj] != target[si][sj]) {
            isSame = false;
          }
        }
      }
    }
    if (isSame && targetLen[0] > 0) {
      int lastIdx = targetLen[0] - 1;
      target[0][lastIdx] = (int)wrap(target[0][lastIdx] + 1, 0, colorCount);
    }
    step++;
    nextPhotonTicks = 60;
    appWidth = 30;
    nextPhotonsLen = 0;
  }
  if (nextStepTicks < 0 && nextStepTicks > -100) {
    char worldStepText[32];
    strcpy(worldStepText, "WORLD ");
    strcat(worldStepText, intToChar(world));
    strcat(worldStepText, "   STEP ");
    strcat(worldStepText, intToChar(step - 1));
    text(worldStepText, 50, 20);
  }
  if (nextStepTicks > 0) {
    character("g", 80, 50);
    character("h", 120, 50);
    if (appWidth > -3) {
      appWidth--;
      addScore(world, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
    }
  }
  nextStepTicks--;
  nextPhotonTicks--;
  if (nextPhotonTicks < 0) {
    if (nextPhotonsLen == 0) {
      TIMES(colorCount, npi) {
        nextPhotons[nextPhotonsLen] = npi;
        nextPhotonsLen++;
        nextPhotons[nextPhotonsLen] = npi;
        nextPhotonsLen++;
      }
      int extraCount = (int)ceil(colorCount * 0.4);
      TIMES(extraCount, eci) {
        nextPhotons[nextPhotonsLen] = 4;
        nextPhotonsLen++;
      }
      int nl = nextPhotonsLen;
      TIMES(99, shi) {
        int i1 = rndi(0, nl);
        int i2 = rndi(0, nl);
        int t = nextPhotons[i1];
        nextPhotons[i1] = nextPhotons[i2];
        nextPhotons[i2] = t;
      }
    }
    int w;
    if (rnd(0, 1) < 0.5) {
      w = 1;
    } else {
      w = -1;
    }
    float s = sqrt(difficulty) * 0.4;
    ASSIGN_ARRAY_ITEM(photons, photonIndex, Photon, np);
    if (w > 0) {
      np->x = appWidth;
    } else {
      np->x = 200 - appWidth;
    }
    np->vx = w * s;
    np->w = w;
    nextPhotonsLen--;
    np->index = nextPhotons[nextPhotonsLen];
    np->isReflected = false;
    np->isAlive = true;
    photonIndex = wrap(photonIndex + 1, 0, MAX_PHOTON_COUNT);
    nextPhotonTicks = rnd(60, 90) / sqrt(difficulty);
  }
  bool clearAll = false;
  FOR_EACH(photons, i) {
    ASSIGN_ARRAY_ITEM(photons, i, Photon, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx;
    char pcode[2];
    pcode[0] = 'b' + p->index;
    pcode[1] = 0;
    character(pcode, p->x, 60);
    if (p->isReflected) {
      if (p->x < appWidth || p->x > 200 - appWidth) {
        p->isAlive = false;
      }
      continue;
    }
    int sideBit;
    if (p->w > 0) {
      sideBit = 0;
    } else {
      sideBit = 1;
    }
    int angleBit;
    if (playerAngle < M_PI / 2 || playerAngle > (M_PI / 2) * 3) {
      angleBit = 0;
    } else {
      angleBit = 1;
    }
    int hi = (int)wrap(sideBit + angleBit, 0, 2);
    float hl = handsLen[hi] * 7 + 6;
    float x = 100 - hl * p->w;
    bool hit = false;
    if (p->w == 1 && p->x > x) {
      hit = true;
    } else if (p->w == -1 && p->x < x) {
      hit = true;
    }
    if (hit) {
      if (playerIsRotating) {
        play(LASER);
        p->isReflected = true;
        p->vx *= -5;
      } else {
        if (p->index == 4) {
          if (handsLen[hi] > 0) {
            play(HIT);
            particle(p->x, 60, 16, 1, 0, M_PI * 2);
            handsLen[hi]--;
          }
        } else {
          play(SELECT);
          if (handsLen[hi] < MAX_HAND_LEN) {
            hands[hi][handsLen[hi]] = p->index;
            handsLen[hi]++;
          }
          if (handsLen[hi] > 9) {
            appWidth /= 2;
          }
        }
        bool isMatching = true;
        TIMES(2, mi) {
          if (targetLen[mi] != handsLen[mi]) {
            isMatching = false;
          } else {
            TIMES(targetLen[mi], mj) {
              if (target[mi][mj] != hands[mi][mj]) {
                isMatching = false;
              }
            }
          }
        }
        if (isMatching && playerAngle > M_PI / 2) {
          playerIsRotating = true;
          playerTa += M_PI;
        }
        if (!isMatching) {
          isMatching = true;
          TIMES(2, mi2) {
            int otherIdx;
            if (mi2 == 0) {
              otherIdx = 1;
            } else {
              otherIdx = 0;
            }
            if (targetLen[mi2] != handsLen[otherIdx]) {
              isMatching = false;
            } else {
              TIMES(targetLen[mi2], mj2) {
                if (target[mi2][mj2] != hands[otherIdx][mj2]) {
                  isMatching = false;
                }
              }
            }
          }
          if (isMatching && playerAngle < M_PI / 2) {
            playerIsRotating = true;
            playerTa += M_PI;
          }
        }
        if (isMatching) {
          play(POWER_UP);
          clearAll = true;
          nextStepTicks = 60;
          nextPhotonTicks = 9999;
        }
        p->isAlive = false;
        continue;
      }
    }
  }
  if (clearAll) {
    INIT_UNALIVED_ARRAY(photons);
  }
  character("a", 100, 30);
  Vector p2;
  TIMES(2, wi) {
    vectorSet(&p2, 100, 30);
    TIMES(targetLen[wi], ii) {
      p2.x += 7 * (wi * 2 - 1);
      char tc[2];
      tc[0] = 'b' + target[wi][ii];
      tc[1] = 0;
      character(tc, VEC_XY(p2));
    }
  }
  if (!playerIsRotating && nextStepTicks < 0 && input.isJustPressed) {
    play(COIN);
    playerIsRotating = true;
    playerTa += M_PI;
  }
  if (playerIsRotating) {
    playerAngle += 0.2;
    if (playerAngle >= playerTa) {
      if (!input.isPressed) {
        playerAngle = playerTa;
        playerIsRotating = false;
      } else {
        playerTa += M_PI;
      }
    }
    if (playerAngle >= M_PI * 2) {
      playerAngle -= M_PI * 2;
      playerTa -= M_PI * 2;
    }
  }
  character("a", 100, 60);
  TIMES(2, hwi) {
    vectorSet(&p2, 100, 60);
    TIMES(handsLen[hwi], hii) {
      addWithAngle(&p2, playerAngle, 7 * (hwi * 2 - 1));
      char hc[2];
      hc[0] = 'b' + hands[hwi][hii];
      hc[1] = 0;
      character(hc, VEC_XY(p2));
    }
  }
  appWidth -= 0.02;
  rect(0, 56, appWidth + 3, 8);
  rect(200, 56, -appWidth - 3, 8);
  if (nextStepTicks < 0 && appWidth < -3) {
    play(RANDOM);  // Equivalent to "lucky" in JS
    gameOver();
  }
}

void addGamePhotonLine() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
