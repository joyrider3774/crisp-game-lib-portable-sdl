#include "cglp.h"

static char *title = "MOLE N";
static char *description = "[Tap]\n Whack a number";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
    "      ",
}};
static int charactersCount = 0;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

#define MOLEN_NUMBER_COUNT 9
static int numbers[MOLEN_NUMBER_COUNT][MOLEN_NUMBER_COUNT];
static int currentNumbers[MOLEN_NUMBER_COUNT][MOLEN_NUMBER_COUNT];
static float numbersY;
static float penaltyY;
static int targetNumber;
static int nextTargetNumberCount;

typedef struct {
  Vector pos;
  int value;
  float removeTicks;
  float score;
  bool isAlive;
} MolenMole;
#define MOLEN_MAX_MOLE_COUNT 32
static MolenMole moles[MOLEN_MAX_MOLE_COUNT];
static int moleIndex;
static float nextMoleDist;
static float moleMoveTicks;
static Vector hitPos;
static float hitTicks;

static Vector offsets[4];

static int getRandomNumber() {
  int hi = (int)clamp((sqrt(difficulty) - 1) * 5 + 2.5, 1, 10);
  return rndi(1, hi);
}

static bool existsMole(Vector *p) {
  if (!(p->x >= 0 && p->x < MOLEN_NUMBER_COUNT && p->y >= 0 && p->y < MOLEN_NUMBER_COUNT)) {
    return true;
  }
  bool exists = false;
  FOR_EACH(moles, i) {
    ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->pos.x == p->x && m->pos.y == p->y) {
      exists = true;
    }
  }
  return exists;
}

static void update() {
  // Never reads a Collision result anywhere in this file - the tapped
  // cell is computed directly from input.pos via grid math, and mole
  // occupancy/movement is direct existsMole() grid lookup, so the
  // engine's own O(n^2) hitbox scan (see checkHitBox() in cglp.c) is pure
  // waste here. Restored automatically when the next real game starts,
  // via resetDrawState() in initInGame().
  hasCollision = false;
  if (!ticks) {
    TIMES(MOLEN_NUMBER_COUNT, x) {
      TIMES(MOLEN_NUMBER_COUNT, y) {
        numbers[x][y] = rndi(0, 10);
        currentNumbers[x][y] = 0;
      }
    }
    numbersY = 0;
    penaltyY = 0;
    targetNumber = 1;
    nextTargetNumberCount = 0;
    INIT_UNALIVED_ARRAY(moles);
    moleIndex = 0;
    nextMoleDist = 0;
    moleMoveTicks = 0;
    vectorSet(&hitPos, 0, 0);
    hitTicks = 0;
    vectorSet(&offsets[0], 1, 0);
    vectorSet(&offsets[1], 0, 1);
    vectorSet(&offsets[2], -1, 0);
    vectorSet(&offsets[3], 0, -1);
  }
  float scr = sqrt(difficulty) * 0.02 + penaltyY;
  penaltyY *= 0.9;
  numbersY += scr;
  if (numbersY > 0) {
    TIMES(MOLEN_NUMBER_COUNT, y) {
      TIMES(MOLEN_NUMBER_COUNT, x) {
        if (y < MOLEN_NUMBER_COUNT - 1) {
          numbers[x][MOLEN_NUMBER_COUNT - y - 1] = numbers[x][MOLEN_NUMBER_COUNT - y - 2];
        } else {
          numbers[x][0] = rndi(0, 10);
        }
      }
    }
    FOR_EACH(moles, i) {
      ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
      SKIP_IS_NOT_ALIVE(m);
      m->pos.y++;
    }
    hitPos.y++;
    numbersY -= 11;
  }
  if (input.isJustPressed) {
    play(LASER);
    Vector p;
    vectorSet(&p, floor((input.pos.x - 1) / 11), floor((input.pos.y - numbersY) / 11));
    hitPos = p;
    hitTicks = 40;
    bool pInRect = p.x >= 0 && p.x < MOLEN_NUMBER_COUNT && p.y >= 0 && p.y < MOLEN_NUMBER_COUNT;
    if (pInRect) {
      bool isTargetRemoved = false;
      FOR_EACH(moles, i) {
        ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
        SKIP_IS_NOT_ALIVE(m);
        if (m->removeTicks == 0 && m->pos.x == p.x && m->pos.y == p.y) {
          if (m->value == targetNumber) {
            isTargetRemoved = true;
          } else {
            m->removeTicks = 60;
            m->score = -m->value;
            penaltyY += 1;
          }
        }
      }
      if (isTargetRemoved) {
        targetNumber = getRandomNumber();
        FOR_EACH(moles, i) {
          ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
          SKIP_IS_NOT_ALIVE(m);
          m->removeTicks = 60;
          m->score = m->value;
        }
      }
      penaltyY += 1;
    }
  }
  COUNT_IS_ALIVE(moles, aliveMoleCount);
  if (aliveMoleCount == 0) {
    nextMoleDist = 0;
  }
  nextMoleDist -= scr;
  if (nextMoleDist < 0) {
    Vector pos;
    vectorSet(&pos, 0, 0);
    TIMES(9, i) {
      pos.x = rndi(0, MOLEN_NUMBER_COUNT);
      if (!existsMole(&pos)) {
        nextTargetNumberCount--;
        int value;
        if (nextTargetNumberCount <= 0) {
          value = targetNumber;
          nextTargetNumberCount = rndi(4, 7);
        } else {
          value = getRandomNumber();
        }
        ASSIGN_ARRAY_ITEM(moles, moleIndex, MolenMole, nm);
        nm->pos = pos;
        nm->value = value;
        nm->removeTicks = 0;
        nm->score = 0;
        nm->isAlive = true;
        moleIndex = wrap(moleIndex + 1, 0, MOLEN_MAX_MOLE_COUNT);
        break;
      }
    }
    nextMoleDist += rnd(9, 12);
  }
  moleMoveTicks -= sqrt(sqrt(difficulty));
  if (moleMoveTicks < 0) {
    FOR_EACH(moles, i) {
      ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
      SKIP_IS_NOT_ALIVE(m);
      if (m->removeTicks > 0) {
        continue;
      }
      int w = rndi(0, 4);
      Vector p;
      TIMES(4, k) {
        Vector o = offsets[w];
        vectorSet(&p, m->pos.x + o.x, m->pos.y + o.y);
        if (p.y > 0 && !existsMole(&p)) {
          play(HIT);
          m->pos = p;
          break;
        }
        w = (int)wrap(w + 1, 0, 4);
      }
    }
    moleMoveTicks += 99;
  }
  FOR_EACH(moles, i) {
    ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    if (m->removeTicks > 0) {
      m->removeTicks -= sqrt(difficulty);
      if (m->removeTicks <= 0) {
        color = RED;
        float px = m->pos.x * 11 + 6;
        float py = m->pos.y * 11 + 6 + numbersY;
        if (m->score != 0) {
          if (m->score > 0) {
            play(POWER_UP);
          } else {
            play(COIN);
          }
          particle(px, py, 9, 2, 0, M_PI * 2);
          addScore(m->score, px, py);
        }
        m->isAlive = false;
        continue;
      }
    } else if (m->pos.y >= MOLEN_NUMBER_COUNT - 1) {
      m->removeTicks = 60;
      if (m->value == targetNumber) {
        play(EXPLOSION);
        FOR_EACH(moles, k) {
          ASSIGN_ARRAY_ITEM(moles, k, MolenMole, mk);
          SKIP_IS_NOT_ALIVE(mk);
          mk->removeTicks = 60;
        }
        color = LIGHT_RED;
        rect(m->pos.x * 11 + 1, m->pos.y * 11 + 1 + numbersY, 11, 11);
        gameOver();
      }
    }
  }
  TIMES(MOLEN_NUMBER_COUNT, y) {
    TIMES(MOLEN_NUMBER_COUNT, x) { currentNumbers[x][y] = numbers[x][y]; }
  }
  FOR_EACH(moles, i) {
    ASSIGN_ARRAY_ITEM(moles, i, MolenMole, m);
    SKIP_IS_NOT_ALIVE(m);
    int n;
    if (m->removeTicks > 0) {
      n = m->value + 10;
    } else {
      n = (int)wrap(currentNumbers[(int)m->pos.x][(int)m->pos.y] + m->value, 0, 10);
    }
    currentNumbers[(int)m->pos.x][(int)m->pos.y] = n;
  }
  // The JS version draws each digit at 2x scale via a text `scale` option
  // this port doesn't support; drawn at normal size instead (visual only).
  TIMES(MOLEN_NUMBER_COUNT, y) {
    TIMES(MOLEN_NUMBER_COUNT, x) {
      int n = currentNumbers[x][y];
      if (n >= 10) {
        n -= 10;
        color = RED;
      } else {
        if (hitTicks > 0 && x == (int)hitPos.x && y == (int)hitPos.y) {
          color = GREEN;
        } else {
          color = BLUE;
        }
      }
      text(intToChar(n), x * 11 + 6, y * 11 + 6 + numbersY);
    }
  }
  hitTicks -= sqrt(difficulty);
  color = WHITE;
  rect(0, 0, 100, 7);
  rect(0, 93, 100, 7);
  color = BLACK;
  text("TARGET =", 3, 96);
  color = RED;
  text(intToChar(targetNumber), 3 + 6 * 9, 96);
}

void addGameMoleN() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
