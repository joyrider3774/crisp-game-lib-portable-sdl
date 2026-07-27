#include "cglp.h"

static char *title = "CHALKBOARD";
static char *description = "[Hold]    Write\n[Release] Save\nOverwriting costs a lot";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 5, .isDarkColor = false};

#define CHALKBOARD_MAX_ENERGY 100
#define CHALKBOARD_WRITE_GAIN 0.1
#define CHALKBOARD_IDLE_DECAY 0.1
#define CHALKBOARD_OVERLAP_PENALTY 10
#define CHALKBOARD_THICKNESS 3
#define CHALKBOARD_SEGMENT_SPACING 2
#define CHALKBOARD_FRESH_FRAMES 4

static Vector pos;
static int dirIndex;
static float speed;
static int turnTicks;
static float nextTurnTicks;
static Vector cam;
static float energy;
static Vector lastWritePos;
static float scoreCarry;
static bool blinkWasOn;
static int holdFrames;
static int releaseFrames;
static Vector nibScreen;

typedef struct {
  Vector p1;
  Vector p2;
  int age;
  bool isAlive;
} Seg;
// Matches upstream's 1000-entry cap; its extra distance-based pruning pass is redundant here.
#define CHALKBOARD_MAX_SEG_COUNT 1000
static Seg segs[CHALKBOARD_MAX_SEG_COUNT];
static int segIndex;

static float getTurnInterval() { return rndi(30, 150) / sqrt(difficulty); }

static void initGameState() {
  vectorSet(&pos, 0, 0);
  dirIndex = 0;
  speed = 0.9;
  turnTicks = 0;
  nextTurnTicks = 120;
  vectorSet(&cam, 0, 0);
  energy = 60;
  INIT_UNALIVED_ARRAY(segs);
  segIndex = 0;
  vectorSet(&lastWritePos, pos.x, pos.y);
  int sy = 10;
  while (sy < 90) {
    ASSIGN_ARRAY_ITEM(segs, segIndex, Seg, s);
    vectorSet(&s->p1, 120 - 50, sy - 50);
    vectorSet(&s->p2, 120 - 50, sy + 10 - 50);
    s->age = 0;
    s->isAlive = true;
    segIndex = wrap(segIndex + 1, 0, CHALKBOARD_MAX_SEG_COUNT);
    sy += 10;
  }
  scoreCarry = 0;
  holdFrames = 0;
  releaseFrames = 0;
  blinkWasOn = false;
}

static void stepMovement() {
  turnTicks++;
  if (turnTicks >= nextTurnTicks) {
    turnTicks = 0;
    nextTurnTicks = getTurnInterval();
    dirIndex = (dirIndex + 1) % 4;
    play(LASER);
  }
  speed = 0.9 * sqrt(difficulty);
  if (dirIndex == 0) {
    pos.x += speed;
  } else if (dirIndex == 1) {
    pos.y += speed;
  } else if (dirIndex == 2) {
    pos.x -= speed;
  } else {
    pos.y -= speed;
  }
  vectorSet(&cam, pos.x - 50, pos.y - 50);
}

static void drawBackground() {
  color = GREEN;
  rect(0, 0, 100, 100);
}

static void drawSegments() {
  float minX = -20;
  float minY = -20;
  float maxX = 120;
  float maxY = 120;
  FOR_EACH(segs, i) {
    ASSIGN_ARRAY_ITEM(segs, i, Seg, s);
    SKIP_IS_NOT_ALIVE(s);
    float centerX = (s->p1.x + s->p2.x) / 2 - cam.x;
    float centerY = (s->p1.y + s->p2.y) / 2 - cam.y;
    if (centerX < minX || centerX > maxX || centerY < minY || centerY > maxY) {
      continue;
    }
    float len = distanceTo(&s->p1, s->p2.x, s->p2.y);
    float ang = angleTo(&s->p1, s->p2.x, s->p2.y);
    if (s->age < CHALKBOARD_FRESH_FRAMES) {
      color = LIGHT_BLACK;
    } else {
      color = WHITE;
    }
    thickness = CHALKBOARD_THICKNESS;
    bar(centerX, centerY, len, ang);
    s->age++;
  }
}

static void maybeAddSegment() {
  float d = distanceTo(&lastWritePos, pos.x, pos.y);
  if (d >= CHALKBOARD_SEGMENT_SPACING) {
    ASSIGN_ARRAY_ITEM(segs, segIndex, Seg, s);
    vectorSet(&s->p1, lastWritePos.x, lastWritePos.y);
    vectorSet(&s->p2, pos.x, pos.y);
    s->age = 0;
    s->isAlive = true;
    segIndex = wrap(segIndex + 1, 0, CHALKBOARD_MAX_SEG_COUNT);
    vectorSet(&lastWritePos, pos.x, pos.y);
  }
}

static void drawHud() {
  float w = clamp((energy / CHALKBOARD_MAX_ENERGY) * 92, 0, 92);
  float y = 91;
  color = BLACK;
  rect(4, y, 92, 5);
  color = YELLOW;
  rect(4, y, w, 5);
}

static void doWritingAndEnergy() {
  vectorSet(&nibScreen, pos.x - cam.x, pos.y - cam.y);
  color = TRANSPARENT;
  Collision cWhite = box(nibScreen.x, nibScreen.y, CHALKBOARD_THICKNESS, CHALKBOARD_THICKNESS);

  if (input.isPressed) {
    holdFrames++;
    releaseFrames = 0;
  } else {
    releaseFrames++;
    holdFrames = 0;
  }
  float rampHold = clamp(pow((float)holdFrames / 120.0, 1.5), 0, 1);
  float rampRelease = clamp(pow((float)releaseFrames / 120.0, 1.5), 0, 1);

  if (input.isPressed) {
    if (input.isJustPressed) {
      vectorSet(&lastWritePos, pos.x, pos.y);
    }
    if (cWhite.isColliding.rect[WHITE]) {
      energy -= CHALKBOARD_OVERLAP_PENALTY;
      play(HIT);
      particle(nibScreen.x, nibScreen.y, 6, 1, 0, M_PI * 2);
    } else {
      float rawGain = CHALKBOARD_WRITE_GAIN * (3 * rampHold) * sqrt(difficulty);
      float applied = clamp(rawGain, 0, CHALKBOARD_MAX_ENERGY - energy);
      energy += applied;
      scoreCarry += rawGain;
      while (scoreCarry >= 1) {
        addScore(1, VEC_XY(nibScreen));
        scoreCarry -= 1;
      }
    }
    energy = clamp(energy, 0, CHALKBOARD_MAX_ENERGY);
    maybeAddSegment();
  } else {
    energy -= CHALKBOARD_IDLE_DECAY * (3 * rampRelease) * sqrt(difficulty);
  }

  color = RED;
  box(nibScreen.x, nibScreen.y, CHALKBOARD_THICKNESS, CHALKBOARD_THICKNESS);

  float arm = 8;
  float gap;
  if (input.isPressed) {
    gap = 0;
  } else {
    gap = 3;
  }
  float s = CHALKBOARD_THICKNESS;
  color = LIGHT_RED;
  if (dirIndex == 0) {
    rect(nibScreen.x + s / 2 + gap, nibScreen.y - s / 2, arm, s);
  } else if (dirIndex == 1) {
    rect(nibScreen.x - s / 2, nibScreen.y + s / 2 + gap, s, arm);
  } else if (dirIndex == 2) {
    rect(nibScreen.x - s / 2 - arm - gap, nibScreen.y - s / 2, arm, s);
  } else {
    rect(nibScreen.x - s / 2, nibScreen.y - s / 2 - arm - gap, s, arm);
  }

  float warnFrames = 30;
  int blinkPeriod = 8;
  float remaining = nextTurnTicks - turnTicks;
  if (remaining <= warnFrames) {
    bool blinkOn = ticks % blinkPeriod < blinkPeriod / 2;
    if (blinkOn && !blinkWasOn) {
      play(JUMP);
    }
    blinkWasOn = blinkOn;
    if (blinkOn) {
      int nextDir = (dirIndex + 1) % 4;
      float angle;
      if (nextDir == 0) {
        angle = 0;
      } else if (nextDir == 1) {
        angle = M_PI / 2;
      } else if (nextDir == 2) {
        angle = M_PI;
      } else {
        angle = -M_PI / 2;
      }
      float warnArm = arm + 2;
      float warnGap = gap + 1;
      Vector center;
      vectorSet(&center, nibScreen.x, nibScreen.y);
      addWithAngle(&center, angle, warnGap + warnArm * 0.5);
      color = RED;
      thickness = s;
      bar(center.x, center.y, warnArm, angle);
    }
  } else {
    blinkWasOn = false;
  }

  if (energy <= 0) {
    gameOver();
    play(EXPLOSION);
  }
}

static void update() {
  if (!ticks) {
    initGameState();
  }
  stepMovement();
  drawBackground();
  drawSegments();
  doWritingAndEnergy();
  drawHud();
}

void addGameChalkboard() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
