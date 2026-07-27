#include "cglp.h"
#include "random.h"
#include <string.h>

static char *title = "SKY GOLF";
static char *description = "[Hold]\n Adjust power\n[Release]\n Shoot";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "      ",
    " ll   ",
    "llll  ",
    "llll  ",
    " ll   ",
    "      ",
}};
static int charactersCount = 1;

// Upstream sets isShowingScore:false; this port has no such option, so the score UI always shows.
static Options options = {
    .viewSizeX = 150, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef enum {
  STATE_TITLE,
  STATE_IN_GAME,
  STATE_GO_TO_NEXT_HOLE,
  STATE_GIVE_UP,
  STATE_HOLE_OUT
} State;
static State state;

typedef enum { BALL_SHOT, BALL_POWER, BALL_FLY } BallState;

typedef struct {
  Vector pos;
  Vector prevPos;
  Vector vel;
  float angle;
  float angleVel;
  float power;
  float basePower;
  float prevBasePower;
  BallState state;
} Ball;
static Ball ball;

typedef enum { GROUND_FAIRWAY, GROUND_SAND, GROUND_WATER, GROUND_TREE, GROUND_FLAG } GroundType;

// Argument to addGround() - distinct from GroundType, since "hole" resolves
// into a flag/fairway ground and never becomes a stored ground type itself.
typedef enum {
  ADD_GROUND_FAIRWAY,
  ADD_GROUND_SAND,
  ADD_GROUND_WATER,
  ADD_GROUND_TREE,
  ADD_GROUND_HOLE
} AddGroundType;

typedef struct {
  GroundType type;
  float height;  // only meaningful when type == GROUND_TREE
} Ground;

#define MAX_GROUND_COUNT 25
typedef struct {
  Vector pos;
  Ground grounds[MAX_GROUND_COUNT];
  int groundsCount;
} Platform;

#define MAX_PLATFORM_COUNT 3
static Platform platforms[MAX_PLATFORM_COUNT];
static int platformCount;

typedef struct {
  Vector pos;
  Vector size;
  char *text;
} Button;
static Button easyButton;
static Button mediumButton;
static Button hardButton;

static int ballCount;
static int holeCount;
static int courseDifficulty;
static int instructionTicks;
static int holeStartingTicks;
static int courseTime;
static int goToNextHoleTicks;
static int giveUpTicks;
static int holeOutTicks;

#define DIFFICULTY_COUNT 3
#define MAX_HOLE_COUNT 9
static int holeSeeds[DIFFICULTY_COUNT][MAX_HOLE_COUNT] = {
    {71, 45, 9, 0, 0, 0, 0, 0, 0},
    {49, 7, 98, 31, 54, 99, 0, 0, 0},
    {15, 4, 67, 5, 90, 53, 79, 85, 78},
};
static int holeCounts[DIFFICULTY_COUNT] = {3, 6, 9};

// Upstream's own from-scratch Random class is the same xorshift128 generator random.h
// already implements, so this reuses that API directly instead of duplicating it.
static Random courseRandom;

static void initTitle();
static void updateTitle();
static void initInGame(int difficultyLevel);
static void updateInGame();
static void drawBallAndTime();
static void goToNextHole();
static void initBall();
static void updateShotState();
static void updatePowerState();
static void updateFlyState();
static void backToPrevBallPos();
static void initBallShotState();
static void drawHole();
static int groundColor(GroundType type);
static void drawGround(float x, float y, float w, GroundType type);
static void drawTree(float x, float y, float h);
static void drawFlag(float x, float y);
static void initGoToNextHole();
static void updateGoToNextHole();
static void initGiveUp();
static void updateGiveUp();
static void initHoleOut();
static void updateHoleOut();
static void createHole(int seed);
static void addPlatform(Vector *pos, int width, bool hasHole, bool hasTeeing);
static void addGround(Ground *grounds, int groundsCount, AddGroundType type);
static void drawTime(float timeVal, float x, float y);
static void getPaddedNumber(char *out, int v, int digit);
static void drawButton(Button *b);
static bool isButtonClicked(Button *b);

static void drawButton(Button *b) {
  color = WHITE;
  rect(b->pos.x, b->pos.y, b->size.x, b->size.y);
  color = BLACK;
  text(b->text, b->pos.x + 2, b->pos.y + 1);
}

static bool isButtonClicked(Button *b) {
  return input.isJustPressed && input.pos.x >= b->pos.x &&
         input.pos.x <= b->pos.x + b->size.x && input.pos.y >= b->pos.y &&
         input.pos.y <= b->pos.y + b->size.y;
}

static void getPaddedNumber(char *out, int v, int digit) {
  char buf[16];
  strcpy(buf, "0000");
  strcat(buf, intToChar(v));
  int len = strlen(buf);
  int start = len - digit;
  int i = 0;
  while (i < digit) {
    out[i] = buf[start + i];
    i++;
  }
  out[i] = 0;
}

static void drawTime(float timeVal, float x, float y) {
  int t = (int)floor(timeVal * 100 / 50);
  if (t >= 10 * 60 * 100) {
    t = 10 * 60 * 100 - 1;
  }
  int minutes = t / 6000;
  int seconds = (t % 6000) / 100;
  int hundredths = t % 100;
  char minutesText[8];
  char secondsText[8];
  char hundredthsText[8];
  getPaddedNumber(minutesText, minutes, 1);
  getPaddedNumber(secondsText, seconds, 2);
  getPaddedNumber(hundredthsText, hundredths, 2);
  char timeText[24];
  strcpy(timeText, minutesText);
  strcat(timeText, "'");
  strcat(timeText, secondsText);
  strcat(timeText, "\"");
  strcat(timeText, hundredthsText);
  text(timeText, x, y);
}

static int groundColor(GroundType type) {
  if (type == GROUND_SAND) {
    return YELLOW;
  } else if (type == GROUND_WATER) {
    return BLUE;
  } else if (type == GROUND_FLAG) {
    return WHITE;
  }
  return GREEN;  // fairway or tree
}

static void drawGround(float x, float y, float w, GroundType type) {
  color = groundColor(type);
  rect(x, y, w, -3);
}

static void drawTree(float x, float y, float h) {
  float h2 = floor(h / 2);
  color = RED;
  rect(x + 1, y, 3, -h2);
  color = GREEN;
  rect(x, y - h2, 5, -h2);
}

static void drawFlag(float x, float y) {
  color = LIGHT_YELLOW;
  rect(x + 1, y, 2, -10);
  color = LIGHT_RED;
  rect(x + 3, y - 6, 5, -4);
}

static void drawHole() {
  TIMES(platformCount, i) {
    Platform *p = &platforms[i];
    color = RED;
    rect(p->pos.x, p->pos.y, p->groundsCount * 6, -2);
    GroundType pgt = p->grounds[0].type;
    float x = p->pos.x;
    float bx = p->pos.x;
    TIMES(p->groundsCount, gi) {
      Ground *g = &p->grounds[gi];
      if (g->type != pgt) {
        drawGround(bx, p->pos.y - 2, x - bx, pgt);
        bx = x;
        pgt = g->type;
      }
      if (g->type == GROUND_TREE) {
        drawTree(x, p->pos.y - 5, g->height);
      } else if (g->type == GROUND_FLAG) {
        drawFlag(x, p->pos.y - 5);
      }
      x += 6;
    }
    drawGround(bx, p->pos.y - 2, x - bx, pgt);
  }
}

static void addGround(Ground *grounds, int groundsCount, AddGroundType type) {
  int w;
  int x;
  if (type == ADD_GROUND_HOLE) {
    w = getIntRandom(&courseRandom, 3, 6);
    x = groundsCount - w - getIntRandom(&courseRandom, 0, 3);
  } else {
    w = getIntRandom(&courseRandom, 3, groundsCount / 2);
    x = getIntRandom(&courseRandom, 0, groundsCount - w);
  }
  int bh = getIntRandom(&courseRandom, 10, 20);
  TIMES(w, i) {
    if (type == ADD_GROUND_HOLE) {
      if (x + i == (int)floor(x + w / 2.0)) {
        grounds[x + i].type = GROUND_FLAG;
      } else {
        grounds[x + i].type = GROUND_FAIRWAY;
      }
    } else if (type == ADD_GROUND_TREE) {
      int height = (int)floor(bh + getIntRandom(&courseRandom, -5, 6));
      grounds[x + i].type = GROUND_TREE;
      grounds[x + i].height = height;
    } else if (type == ADD_GROUND_FAIRWAY) {
      grounds[x + i].type = GROUND_FAIRWAY;
    } else if (type == ADD_GROUND_SAND) {
      grounds[x + i].type = GROUND_SAND;
    } else if (type == ADD_GROUND_WATER) {
      grounds[x + i].type = GROUND_WATER;
    }
  }
}

static void addPlatform(Vector *pos, int width, bool hasHole, bool hasTeeing) {
  ASSIGN_ARRAY_ITEM(platforms, platformCount, Platform, plat);
  vectorSet(&plat->pos, pos->x, pos->y);
  plat->groundsCount = width;
  TIMES(width, i) {
    plat->grounds[i].type = GROUND_FAIRWAY;
    plat->grounds[i].height = 0;
  }
  addGround(plat->grounds, width, ADD_GROUND_TREE);
  if (getRandom(&courseRandom, 0, 1) < 0.7) {
    addGround(plat->grounds, width, ADD_GROUND_FAIRWAY);
  }
  if (getRandom(&courseRandom, 0, 1) < 0.8) {
    addGround(plat->grounds, width, ADD_GROUND_SAND);
  }
  if (getRandom(&courseRandom, 0, 1) < 0.5) {
    addGround(plat->grounds, width, ADD_GROUND_WATER);
  }
  if (hasHole) {
    addGround(plat->grounds, width, ADD_GROUND_HOLE);
  }
  if (hasTeeing) {
    TIMES(5, i) {
      plat->grounds[i].type = GROUND_FAIRWAY;
      plat->grounds[i].height = 0;
    }
  }
  platformCount++;
}

static void createHole(int seed) {
  setRandomSeed(&courseRandom, seed);
  platformCount = 0;
  int pc = getIntRandom(&courseRandom, 1, 3);
  int y = 90;
  int w = 25;
  if (getRandom(&courseRandom, 0, 1) < 0.5) {
    w = getIntRandom(&courseRandom, 12, 20);
    if (getRandom(&courseRandom, 0, 1) < 0.5) {
      y = getIntRandom(&courseRandom, 30, 70);
    }
  }
  ball.pos.y = y - 7;
  Vector pos0;
  vectorSet(&pos0, 0, y);
  addPlatform(&pos0, w, false, true);
  TIMES(pc, i) {
    int pw = getIntRandom(&courseRandom, 9, 20);
    // Split into separate statements (unlike upstream's inline vec(getInt(),getInt())) since C's
    // argument evaluation order is unspecified and this must draw x before y to match the RNG stream.
    int platX = getIntRandom(&courseRandom, 0, 150 - pw * 6);
    int platY = getIntRandom(&courseRandom, 30, 70);
    Vector posN;
    vectorSet(&posN, platX, platY);
    bool isLast = i == pc - 1;
    addPlatform(&posN, pw, isLast, false);
  }
}

static void initBall() {
  vectorSet(&ball.pos, 5, 83);
  vectorSet(&ball.prevPos, 0, 0);
  vectorSet(&ball.vel, 0, 0);
  ball.angle = 0;
  ball.angleVel = -1;
  ball.power = 0;
  ball.basePower = 1;
  ball.prevBasePower = 1;
  ball.state = BALL_SHOT;
}

static void initTitle() {
  state = STATE_TITLE;
  vectorSet(&easyButton.pos, 15, 45);
  vectorSet(&easyButton.size, 50, 7);
  easyButton.text = "Easy";
  vectorSet(&mediumButton.pos, 15, 55);
  vectorSet(&mediumButton.size, 50, 7);
  mediumButton.text = "Medium";
  vectorSet(&hardButton.pos, 15, 65);
  vectorSet(&hardButton.size, 50, 7);
  hardButton.text = "Hard";
  initBall();
  createHole(103);
}

static void updateTitle() {
  drawHole();
  color = BLACK;
  text("SKY GOLF", 9, 38);
  drawButton(&easyButton);
  text("3 holes", 75, 48);
  drawButton(&mediumButton);
  text("6 holes", 75, 58);
  drawButton(&hardButton);
  text("9 holes", 75, 68);
  text("Click button to start", 20, 79);
  if (isButtonClicked(&easyButton)) {
    initInGame(0);
  } else if (isButtonClicked(&mediumButton)) {
    initInGame(1);
  } else if (isButtonClicked(&hardButton)) {
    initInGame(2);
  }
}

static void initBallShotState() {
  ball.state = BALL_SHOT;
  ball.power = 0.1;
  // Upstream starts a seeded procedural BGM here; this port has no such generator.
}

static void goToNextHole() {
  state = STATE_IN_GAME;
  initBall();
  createHole(holeSeeds[courseDifficulty][holeCount]);
  vectorSet(&ball.prevPos, ball.pos.x, ball.pos.y);
  holeStartingTicks = 120;
  holeCount++;
  ballCount += 5;
  initBallShotState();
}

static void initInGame(int difficultyLevel) {
  courseDifficulty = difficultyLevel;
  ballCount = 0;
  holeCount = 0;
  courseTime = 0;
  goToNextHole();
}

static void backToPrevBallPos() {
  play(EXPLOSION);
  vectorSet(&ball.pos, ball.prevPos.x, ball.prevPos.y);
  ball.basePower = ball.prevBasePower;
}

static void initHoleOut() {
  state = STATE_HOLE_OUT;
  holeOutTicks = 0;
  // Upstream plays a seeded victory jingle here; omitted (no procedural BGM in this port).
}

static void initGoToNextHole() {
  if (holeCount == holeCounts[courseDifficulty]) {
    initHoleOut();
    return;
  }
  // Upstream plays a seeded transition jingle here; omitted (no procedural BGM in this port).
  state = STATE_GO_TO_NEXT_HOLE;
  goToNextHoleTicks = 0;
}

static void initGiveUp() {
  state = STATE_GIVE_UP;
  giveUpTicks = 0;
}

static void updateShotState() {
  ball.angle += ball.angleVel * 0.05;
  if ((ball.angle < -M_PI && ball.angleVel < 0) || (ball.angle > 0 && ball.angleVel > 0)) {
    ball.angleVel = -ball.angleVel;
    ball.angle += ball.angleVel * 0.05 * 2;
  }
  color = LIGHT_BLACK;
  Vector tip;
  vectorSet(&tip, ball.pos.x, ball.pos.y);
  addWithAngle(&tip, ball.angle, 9);
  thickness = 2;
  line(ball.pos.x, ball.pos.y, tip.x, tip.y);
  if (input.isJustPressed) {
    // Upstream stops the procedural BGM here; omitted (no procedural BGM in this port).
    play(SELECT);
    ball.state = BALL_POWER;
  }
}

static void updatePowerState() {
  ball.power += 0.2;
  color = LIGHT_BLACK;
  Vector tip;
  vectorSet(&tip, ball.pos.x, ball.pos.y);
  addWithAngle(&tip, ball.angle, ball.power);
  thickness = 2;
  line(ball.pos.x, ball.pos.y, tip.x, tip.y);
  if (ball.power > 9 || input.isJustReleased) {
    play(LASER);
    vectorSet(&ball.vel, 0, 0);
    addWithAngle(&ball.vel, ball.angle, ball.power * 0.5 * ball.basePower);
    ball.state = BALL_FLY;
    ballCount--;
  }
}

static void updateFlyState() {
  Vector p;
  color = TRANSPARENT;
  vectorSet(&p, ball.pos.x, ball.pos.y);
  vectorAdd(&p, ball.vel.x, 0);
  Collision ch = character("a", p.x, p.y);
  if (ch.isColliding.rect[RED] || ch.isColliding.rect[GREEN] || ch.isColliding.rect[YELLOW] ||
      ch.isColliding.rect[BLUE] || (ball.vel.x < 0 && ball.pos.x < 2) ||
      (ball.vel.x > 0 && ball.pos.x > 148)) {
    ball.vel.x *= -0.8;
    ball.vel.y *= 0.8;
  }
  vectorSet(&p, ball.pos.x, ball.pos.y);
  vectorAdd(&p, 0, ball.vel.y);
  Collision cv = character("a", p.x, p.y);
  if (cv.isColliding.rect[RED] || cv.isColliding.rect[GREEN] || cv.isColliding.rect[YELLOW] ||
      cv.isColliding.rect[BLUE]) {
    float vr = 0.8;
    if (ball.vel.y > 0 && cv.isColliding.rect[BLUE]) {
      play(CLICK);
      vr = 0.4;
      color = BLUE;
      particle(ball.pos.x, ball.pos.y + 2, 3, 1, -M_PI / 2, M_PI / 4);
    } else if (ball.vel.y > 0 && cv.isColliding.rect[YELLOW]) {
      play(CLICK);
      vr = 0.5;
      color = YELLOW;
      particle(ball.pos.x, ball.pos.y + 2, 3, 1, -M_PI / 2, M_PI / 4);
    } else {
      play(HIT);
    }
    ball.vel.y *= -vr;
    ball.vel.x *= vr;
    if (ball.vel.y < 0 && vectorLength(&ball.vel) < 0.5) {
      if (cv.isColliding.rect[WHITE]) {
        initGoToNextHole();
        return;
      } else if (ballCount <= 0) {
        initGiveUp();
        return;
      }
      initBallShotState();
      if (cv.isColliding.rect[YELLOW]) {
        ball.basePower = 0.5;
      } else {
        ball.basePower = 1;
      }
      if (cv.isColliding.rect[BLUE]) {
        color = BLUE;
        particle(ball.pos.x, ball.pos.y + 2, 9, 0.5, -M_PI / 2, M_PI / 2);
        backToPrevBallPos();
      }
      vectorSet(&ball.prevPos, ball.pos.x, ball.pos.y);
      ball.prevBasePower = ball.basePower;
      return;
    }
  }
  vectorAdd(&ball.pos, ball.vel.x, ball.vel.y);
  vectorMul(&ball.vel, 0.98);
  ball.vel.y += 0.1;
  if (ball.pos.y > 110) {
    if (ballCount <= 0) {
      initGiveUp();
      return;
    }
    backToPrevBallPos();
    initBallShotState();
  }
}

static void drawBallAndTime() {
  color = BLACK;
  character("a", 3, 4);
  char ballText[16];
  strcpy(ballText, "x");
  strcat(ballText, intToChar(ballCount));
  text(ballText, 9, 3);
  drawTime((float)courseTime, 110, 3);
}

static void updateInGame() {
  drawHole();
  if (ball.state == BALL_SHOT && ball.basePower < 1) {
    color = YELLOW;
  } else {
    color = BLACK;
  }
  character("a", ball.pos.x, ball.pos.y);
  if (ball.state == BALL_SHOT) {
    updateShotState();
  } else if (ball.state == BALL_POWER) {
    // Upstream stops the procedural BGM here; omitted (no procedural BGM in this port).
    updatePowerState();
  } else if (ball.state == BALL_FLY) {
    updateFlyState();
  }
  color = BLACK;
  if (instructionTicks > 0) {
    instructionTicks--;
    text("[Hold] to adjust power", 20, 60);
    text("[Release] to shoot", 20, 68);
  }
  if (holeStartingTicks > 0) {
    holeStartingTicks--;
    char holeText[16];
    strcpy(holeText, "HOLE ");
    strcat(holeText, intToChar(holeCount));
    text(holeText, 10, 95);
  }
  courseTime++;
  drawBallAndTime();
}

static void updateGoToNextHole() {
  drawHole();
  color = BLACK;
  text("GO TO NEXT HOLE", 30, 50);
  drawBallAndTime();
  goToNextHoleTicks++;
  if (goToNextHoleTicks > 150 || input.isJustPressed) {
    goToNextHole();
  }
}

static void updateGiveUp() {
  drawHole();
  color = BLACK;
  text("GIVE UP", 20, 50);
  giveUpTicks++;
  if (giveUpTicks > 300 || input.isJustPressed) {
    initTitle();
  }
}

static void updateHoleOut() {
  drawHole();
  color = BLACK;
  text("HOLE OUT!", 70, 50);
  drawBallAndTime();
  holeOutTicks++;
  if (holeOutTicks > 600 || input.isJustPressed) {
    initTitle();
  }
}

static void update() {
  if (!ticks) {
    // Upstream's document.title = "SKY GOLF" was a browser tab-title side effect;
    // title already carries the same string for this console's title screen.
    instructionTicks = 200;
    initTitle();
  }
  if (state == STATE_TITLE) {
    updateTitle();
  } else if (state == STATE_IN_GAME) {
    updateInGame();
  } else if (state == STATE_GO_TO_NEXT_HOLE) {
    updateGoToNextHole();
  } else if (state == STATE_GIVE_UP) {
    updateGiveUp();
  } else if (state == STATE_HOLE_OUT) {
    updateHoleOut();
  }
}

// usesMouse=true: only the title screen's difficulty buttons need input.pos; gameplay itself doesn't.
void addGameSkyGolf() {
  addGame(title, description, characters, charactersCount, options, true, update);
}
