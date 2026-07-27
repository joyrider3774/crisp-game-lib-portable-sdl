#include "cglp.h"
#include "string.h"

static char *title = "WINDOW WASHER";
static char *description = "[Hold] Ascend\n[Release] Descend\nClean windows!";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {{
    "  ll  ",
    " l  l ",
    "llllll",
    " l  l ",
    "      ",
    "      ",
}};
static int charactersCount = 1;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  Vector pos;
  float vx;
  float vy;
  float width;
  float height;
} Platform;
static Platform platform;

typedef struct {
  Vector pos;
  float width;
  float height;
  bool isCleaned;
} Window;
#define WINDOWWASHER_WINDOW_COUNT 10
static Window windows[WINDOWWASHER_WINDOW_COUNT];

typedef struct {
  Vector pos;
  float vx;
  bool isBird;
  bool isAlive;
} Obstacle;
#define WINDOWWASHER_MAX_OBSTACLE_COUNT 32
static Obstacle obstacles[WINDOWWASHER_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

static float nextObstacleDist;
static float screenScrollSpeed;
static int multiplier;

static void drawPlatform() {
  color = LIGHT_BLACK;
  rect(platform.pos.x - platform.width / 2, 0, 1, platform.pos.y);
  rect(platform.pos.x + platform.width / 2 - 1, 0, 1, platform.pos.y);
  color = BLUE;
  box(VEC_XY(platform.pos), platform.width, platform.height);
}

static void updateBackground() {
  color = LIGHT_CYAN;
  rect(0, 0, 3, 100);
  rect(97, 0, 3, 100);
}

static void updatePlatform() {
  if (input.isJustPressed) {
    play(LASER);
  }
  if (input.isPressed) {
    platform.vy = -1.5;
  } else {
    platform.vy = 1;
  }
  platform.pos.x += platform.vx;
  if (platform.pos.x > 90 || platform.pos.x < 10) {
    play(CLICK);
    platform.vx *= -1;
  }
  platform.pos.y += platform.vy;
  platform.pos.y = clamp(platform.pos.y, 5, 95);
  drawPlatform();
}

static void endGame() {
  play(EXPLOSION);
  gameOver();
}

static void updateWindows() {
  TIMES(WINDOWWASHER_WINDOW_COUNT, wi) {
    Window *w = &windows[wi];
    w->pos.y -= screenScrollSpeed;
    if (w->pos.y < -30) {
      w->pos.y += 330;
      if (w->isCleaned) {
        w->isCleaned = false;
      } else {
        play(HIT);
        multiplier--;
        if (multiplier < 1) {
          multiplier = 1;
        }
      }
    }
    if (w->isCleaned) {
      color = YELLOW;
    } else {
      color = CYAN;
    }
    Collision scratch = box(VEC_XY(w->pos), w->width, w->height);
    if (scratch.isColliding.rect[BLUE] && !w->isCleaned) {
      play(POWER_UP);
      w->isCleaned = true;
      addScore(multiplier, VEC_XY(w->pos));
      multiplier++;
    }
  }
}

static void updateObstacles() {
  FOR_EACH(obstacles, oi) {
    ASSIGN_ARRAY_ITEM(obstacles, oi, Obstacle, o);
    SKIP_IS_NOT_ALIVE(o);
    o->pos.y -= screenScrollSpeed;
    if (o->isBird) {
      o->pos.x += o->vx;
      if (o->pos.x > 95 || o->pos.x < 5) {
        o->vx *= -1;
      }
    }
    color = RED;
    if (o->isBird) {
      characterOptions.isMirrorX = !(o->vx > 0);
      characterOptions.isMirrorY = false;
      characterOptions.rotation = 0;
      Collision scratch = character("a", VEC_XY(o->pos));
      if (scratch.isColliding.rect[BLUE]) {
        endGame();
      }
    } else {
      Collision scratch = box(o->pos.x, o->pos.y, 10, 14);
      if (scratch.isColliding.rect[BLUE]) {
        endGame();
      }
    }
    if (o->pos.y < -10) {
      o->isAlive = false;
      continue;
    }
  }
  nextObstacleDist -= screenScrollSpeed;
  if (nextObstacleDist < 0) {
    bool isBird = rnd(0, 1) < 0.5;
    float px = rnd(15, 85);
    float py = 110;
    if (!isBird) {
      color = TRANSPARENT;
      Collision scratch = box(px, py, 14, 18);
      if (scratch.isColliding.rect[CYAN]) {
        isBird = true;
      }
    }
    ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, Obstacle, no);
    vectorSet(&no->pos, px, py);
    if (isBird) {
      if (rnd(0, 1) < 0.5) {
        no->vx = 0.5;
      } else {
        no->vx = -0.5;
      }
    } else {
      no->vx = 0;
    }
    no->isBird = isBird;
    no->isAlive = true;
    obstacleIndex = wrap(obstacleIndex + 1, 0, WINDOWWASHER_MAX_OBSTACLE_COUNT);
    nextObstacleDist += rnd(40, 50);
  }
}

static void updateScroll() {
  screenScrollSpeed = 0.5 + difficulty * 0.1;
}

static void initGameState() {
  vectorSet(&platform.pos, 50, 10);
  platform.vx = 1;
  platform.vy = 1;
  platform.width = 20;
  platform.height = 5;
  TIMES(WINDOWWASHER_WINDOW_COUNT, i) {
    Window *w = &windows[i];
    vectorSet(&w->pos, rnd(15, 85), 20 + i * 30);
    w->width = 15;
    w->height = 20;
    w->isCleaned = false;
  }
  INIT_UNALIVED_ARRAY(obstacles);
  obstacleIndex = 0;
  nextObstacleDist = 0;
  screenScrollSpeed = 0.5;
  multiplier = 1;
}

static void update() {
  if (!ticks) {
    initGameState();
  }
  updateBackground();
  updatePlatform();
  updateWindows();
  updateObstacles();
  drawPlatform();
  updateScroll();
  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 2, 10);
}

void addGameWindowWasher() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
