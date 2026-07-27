#include "cglp.h"
#include "string.h"

static char *title = "MICROBIAL FOCUS";
static char *description = "[Hold]\n Adjust focus depth";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 9, .isDarkColor = false};

typedef struct {
  Vector pos;
  float depth;
  float size;
  bool moving;
  Vector moveDir;
  Vector velocity;
  float anger;
  float maxAnger;
  bool isAlive;
} MicrobialfocusMicrobe;
// Vircon32 port note: upstream itself caps concurrent microbes at 24 (see
// the spawn gate below) - this array is sized to that exact, explicit cap.
#define MICROBIALFOCUS_MAX_MICROBE_COUNT 24
static MicrobialfocusMicrobe microbes[MICROBIALFOCUS_MAX_MICROBE_COUNT];
static int microbeIndex;

static float focusDepth;
static int focusDirection;
static int holdCycle;
static float nextMicrobeSpawn;
static float multiplier;

static void update() {
  // Never reads a Collision result - focus matches are decided by depth comparison.
  hasCollision = false;
  if (!ticks) {
    focusDepth = 0.5;
    focusDirection = 1;
    INIT_UNALIVED_ARRAY(microbes);
    microbeIndex = 0;
    holdCycle = 1;
    nextMicrobeSpawn = 0;
    multiplier = 1;
  }

  // Focus control system
  if (input.isJustPressed) {
    if (holdCycle == 0) {
      focusDirection = 1;
      holdCycle = 1;
    } else {
      focusDirection = -1;
      holdCycle = 0;
    }
    play(SELECT);
  }

  if (input.isPressed) {
    focusDepth += focusDirection * 0.015 * sqrt(difficulty);
    if (focusDepth < 0 || focusDepth > 1) {
      focusDepth = clamp(focusDepth, 0, 1);
      play(LASER);
      multiplier = 1;
    }
  }

  multiplier *= 0.999;

  // Add new microbes periodically from screen edge
  COUNT_IS_ALIVE(microbes, microbeCountNow);
  if (ticks >= nextMicrobeSpawn && microbeCountNow < 24) {
    nextMicrobeSpawn = ticks + (50 / difficulty / difficulty) * sqrt(microbeCountNow + 1);
    float newDepth = 0.5;
    int di;
    for (di = 0; di < 8; di++) {
      newDepth = rnd(0.05, 0.95);
      if (fabs(newDepth - focusDepth) > 0.15) {
        break;
      }
    }

    int edge = (int)rnd(0, 4);
    float microbeSize = rnd(4, 8);
    Vector newPos;
    Vector initialVelocity;
    if (edge == 0) {
      vectorSet(&newPos, rnd(10, 90), -microbeSize + 1);
      vectorSet(&initialVelocity, rnd(-0.2, 0.2), rnd(0.5, 1));
    } else if (edge == 1) {
      vectorSet(&newPos, 100 + microbeSize - 1, rnd(10, 90));
      vectorSet(&initialVelocity, rnd(-1, -0.5), rnd(-0.2, 0.2));
    } else if (edge == 2) {
      vectorSet(&newPos, rnd(10, 90), 100 + microbeSize - 1);
      vectorSet(&initialVelocity, rnd(-0.2, 0.2), rnd(-1, -0.5));
    } else {
      vectorSet(&newPos, -microbeSize + 1, rnd(10, 90));
      vectorSet(&initialVelocity, rnd(0.5, 1), rnd(-0.2, 0.2));
    }

    ASSIGN_ARRAY_ITEM(microbes, microbeIndex, MicrobialfocusMicrobe, nm);
    nm->pos = newPos;
    nm->depth = newDepth;
    nm->size = microbeSize;
    nm->moving = false;
    vectorSet(&nm->moveDir, 0, 0);
    nm->velocity = initialVelocity;
    nm->anger = 0;
    nm->maxAnger = floor(600 / sqrt(sqrt(difficulty)));
    nm->isAlive = true;
    microbeIndex = wrap(microbeIndex + 1, 0, MICROBIALFOCUS_MAX_MICROBE_COUNT);

    play(SELECT);
  }

  // Update microbes
  FOR_EACH(microbes, mi) {
    ASSIGN_ARRAY_ITEM(microbes, mi, MicrobialfocusMicrobe, m);
    SKIP_IS_NOT_ALIVE(m);
    float depthDiff = fabs(m->depth - focusDepth);
    bool isInFocus = depthDiff < 0.1;

    m->anger++;

    float angerRatio = m->anger / m->maxAnger;
    if (angerRatio >= 0.8 && ((int)m->anger) % 30 == 0) {
      play(HIT);
    } else if (angerRatio >= 0.6 && ((int)m->anger) % 60 == 0) {
      play(HIT);
    }

    if (isInFocus) {
      if (!m->moving) {
        m->moving = true;
        Vector directionFromCenter;
        vectorSet(&directionFromCenter, m->pos.x - 50, m->pos.y - 50);
        float length = vectorLength(&directionFromCenter);
        if (length > 0) {
          vectorSet(&m->moveDir, directionFromCenter.x / length, directionFromCenter.y / length);
        } else {
          float ang = rnd(0, M_PI * 2);
          vectorSet(&m->moveDir, cos(ang), sin(ang));
        }
      }
      float acceleration = 0.02;
      m->velocity.x += m->moveDir.x * acceleration;
      m->velocity.y += m->moveDir.y * acceleration;
      vectorAdd(&m->pos, m->velocity.x, m->velocity.y);
    } else {
      m->velocity.x *= 0.99;
      m->velocity.y *= 0.99;
      vectorAdd(&m->pos, m->velocity.x, m->velocity.y);
      if (fabs(m->velocity.x) < 0.01 && fabs(m->velocity.y) < 0.01) {
        m->moving = false;
        vectorSet(&m->velocity, 0, 0);
      }
    }
  }

  // Remove off-screen microbes and award scores
  FOR_EACH(microbes, mri) {
    ASSIGN_ARRAY_ITEM(microbes, mri, MicrobialfocusMicrobe, mr);
    SKIP_IS_NOT_ALIVE(mr);
    if (mr->pos.x < -mr->size || mr->pos.x > 100 + mr->size || mr->pos.y < -mr->size ||
        mr->pos.y > 100 + mr->size) {
      int roundedMultiplier = (int)round(multiplier);
      float scoreX = clamp(mr->pos.x, 10, 90);
      float scoreY = clamp(mr->pos.y, 20, 99);
      addScore(roundedMultiplier, scoreX, scoreY);
      multiplier += 1;
      play(POWER_UP);
      mr->isAlive = false;
      continue;
    }
  }

  // Draw microbes
  FOR_EACH(microbes, mdi) {
    ASSIGN_ARRAY_ITEM(microbes, mdi, MicrobialfocusMicrobe, md);
    SKIP_IS_NOT_ALIVE(md);
    float depthDiff2 = fabs(md->depth - focusDepth);
    float blur = depthDiff2 * 15;
    bool isInFocus2 = depthDiff2 < 0.1;

    float angerRatio2 = md->anger / md->maxAnger;
    float angerRadius = angerRatio2 * md->size;

    bool shouldFlash = angerRatio2 > 0.6;
    float flashPeriod = 21 - (angerRatio2 - 0.6) * 20;
    bool isFlashFrame = false;
    if (shouldFlash) {
      isFlashFrame = ((int)(md->anger / flashPeriod)) % 2 == 0;
    }
    int angerColor = RED;
    int blurredAngerColor = LIGHT_RED;
    if (isFlashFrame) {
      angerColor = YELLOW;
      blurredAngerColor = LIGHT_YELLOW;
    }

    if (isInFocus2) {
      // Sharp, in-focus microbe
      color = GREEN;
      arc(md->pos.x, md->pos.y, md->size, 0, M_PI * 2);

      if (rnd(0, 1) < 0.3) {
        particle(md->pos.x, md->pos.y, 1, rnd(0.5, 1.5), 0, M_PI * 2);
      }

      if (angerRadius > 0.5) {
        color = angerColor;
        arc(md->pos.x, md->pos.y, angerRadius, 0, M_PI * 2);

        if (angerRatio2 > 0.7 && rnd(0, 1) < angerRatio2) {
          particle(md->pos.x, md->pos.y, rndi(2, 5), rnd(1, 3), 0, M_PI * 2);
        }
      }
    } else {
      // Blurred, out-of-focus microbe (fixed two-copy offset pattern)
      float blurAmount = blur * 0.5;

      color = LIGHT_GREEN;
      arc(md->pos.x - blurAmount, md->pos.y, md->size * (0.8 + blurAmount * 0.1), 0, M_PI * 2);
      arc(md->pos.x + blurAmount, md->pos.y, md->size * (0.8 + blurAmount * 0.1), 0, M_PI * 2);

      if (angerRadius > 0.5) {
        color = blurredAngerColor;
        arc(md->pos.x - blurAmount, md->pos.y, angerRadius * (0.8 + blurAmount * 0.1), 0, M_PI * 2);
        arc(md->pos.x + blurAmount, md->pos.y, angerRadius * (0.8 + blurAmount * 0.1), 0, M_PI * 2);

        if (angerRatio2 > 0.7 && rnd(0, 1) < angerRatio2 * 0.5) {
          particle(md->pos.x, md->pos.y, rnd(1, 3), rnd(0.8, 2), rnd(0, M_PI * 2), M_PI * 2);
        }
      }
    }

    // Check for game over
    if (md->anger >= md->maxAnger) {
      color = RED;
      float tx = clamp(md->pos.x, 3, 97);
      float ty = clamp(md->pos.y, 3, 97);
      text("X", tx, ty);
      play(EXPLOSION);
      gameOver();
    }
  }

  // Draw focus depth indicator
  color = BLUE;
  rect(10, 90, 80, 3);
  color = CYAN;
  rect(10 + focusDepth * 80 - 2, 89, 4, 5);

  // Draw multiplier
  // isSmallText dropped (no small-font variant in this port); drawn at normal size.
  color = BLACK;
  char multiplierText[16];
  strcpy(multiplierText, "x");
  strcat(multiplierText, intToChar((int)round(multiplier)));
  text(multiplierText, 3, 9);
}

void addGameMicrobialFocus() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
