#include "cglp.h"

static char *title = "SKY RAFTSMAN";
static char *description = "[Tap] Jump";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 2, .isDarkColor = false};

#define JUMP_POWER 2
#define GRAVITY 0.1

typedef struct {
  Vector pos;
  float size;
  float rotationSpeed;
  float angle;
  Vector vel;
  float yDist;
  bool isAlive;
} Log;
#define MAX_LOG_COUNT 32
static Log logs[MAX_LOG_COUNT];
static int logIndex;
static float nextLogDist;

typedef struct {
  Vector pos;
  Vector size;
  bool isAlive;
} Cloud;
#define MAX_CLOUD_COUNT 64
static Cloud clouds[MAX_CLOUD_COUNT];
static int cloudIndex;
static float nextCloudDist;

typedef struct {
  Vector pos;
  bool isJumping;
  int currentLogIndex;  // index into logs instead of a live pointer
  Vector vel;
} Raftsman;
static Raftsman raftsman;

static float scrollDist;

static void updateClouds() {
  FOR_EACH(clouds, ci) {
    ASSIGN_ARRAY_ITEM(clouds, ci, Cloud, c);
    SKIP_IS_NOT_ALIVE(c);
    c->pos.y += scrollDist / 2;
    if (c->pos.y < -20) {
      c->isAlive = false;
      continue;
    }
  }
  nextCloudDist += scrollDist;
  if (nextCloudDist < 0) {
    int cc = rndi(3, 6);
    float cx = rnd(0, 100);
    TIMES(cc, i) {
      ASSIGN_ARRAY_ITEM(clouds, cloudIndex, Cloud, nc);
      vectorSet(&nc->pos, cx + rnd(0, 20) * RNDPM(), 120 + rnd(0, 10));
      vectorSet(&nc->size, rnd(15, 25), rnd(5, 15));
      nc->isAlive = true;
      cloudIndex = wrap(cloudIndex + 1, 0, MAX_CLOUD_COUNT);
    }
    nextCloudDist += rnd(120, 160);
  }
}

static void updateRaftsman() {
  Log *curLog = &logs[raftsman.currentLogIndex];
  if (input.isJustPressed && !raftsman.isJumping) {
    play(JUMP);
    raftsman.isJumping = true;
    vectorSet(&raftsman.vel, 0, 0);
    addWithAngle(&raftsman.vel, curLog->angle, JUMP_POWER);
    vectorAdd(&raftsman.pos, raftsman.vel.x * 2, raftsman.vel.y * 2);
    vectorSet(&curLog->vel, -raftsman.vel.x, -raftsman.vel.y);
  }
  if (raftsman.isJumping) {
    vectorAdd(&raftsman.pos, raftsman.vel.x * difficulty, raftsman.vel.y * difficulty);
    vectorMul(&raftsman.vel, 0.99);
    float gravDiv = 1;
    if (input.isPressed) {
      gravDiv = 2;
    }
    raftsman.vel.y += GRAVITY / gravDiv;
    raftsman.pos.y += scrollDist;
  } else {
    vectorSet(&raftsman.pos, curLog->pos.x, curLog->pos.y);
    addWithAngle(&raftsman.pos, curLog->angle, curLog->size / 2);
  }
  if ((raftsman.pos.x < 3 && raftsman.vel.x < 0) || (raftsman.pos.x > 97 && raftsman.vel.x > 0)) {
    raftsman.vel.x = -raftsman.vel.x;
  }
  if (raftsman.pos.y > 100) {
    play(EXPLOSION);
    gameOver();
  }
}

static void updateLogs() {
  FOR_EACH(logs, li) {
    ASSIGN_ARRAY_ITEM(logs, li, Log, log);
    SKIP_IS_NOT_ALIVE(log);
    vectorAdd(&log->pos, log->vel.x * difficulty, log->vel.y * difficulty);
    log->yDist += log->vel.y * difficulty;
    if (log->yDist > 20) {
      log->yDist -= 20;
      float s = floor(log->vel.y * difficulty * 5);
      if (s > 0) {
        if (log->pos.y < 100) {
          play(COIN);
          addScore(s, log->pos.x, log->pos.y);
        } else {
          addScore(s, log->pos.x, 99);
        }
      }
    }
    vectorMul(&log->vel, 0.99);
    log->pos.y += scrollDist;
    if ((log->pos.x < log->size / 2 && log->vel.x < 0) ||
        (log->pos.x > 100 - log->size / 2 && log->vel.x > 0)) {
      log->vel.x = -log->vel.x;
    }
    log->angle += log->rotationSpeed * difficulty;
    if (log->pos.y < -99) {
      log->isAlive = false;
      continue;
    }
  }
  nextLogDist += scrollDist;
  if (nextLogDist < 0 && !raftsman.isJumping) {
    ASSIGN_ARRAY_ITEM(logs, logIndex, Log, nl);
    vectorSet(&nl->pos, rnd(10, 90), 125);
    nl->size = rnd(15, 25);
    nl->rotationSpeed = rnd(0.04, 0.08) * RNDPM();
    nl->angle = 0;
    vectorSet(&nl->vel, 0, 0);
    nl->yDist = 0;
    nl->isAlive = true;
    logIndex = wrap(logIndex + 1, 0, MAX_LOG_COUNT);
    nextLogDist = 100;
  }
}

static void drawGame() {
  color = LIGHT_CYAN;
  FOR_EACH(clouds, ci) {
    ASSIGN_ARRAY_ITEM(clouds, ci, Cloud, c);
    SKIP_IS_NOT_ALIVE(c);
    box(c->pos.x, c->pos.y, c->size.x, c->size.y);
  }
  color = RED;
  box(raftsman.pos.x, raftsman.pos.y, 6, 6);
  color = YELLOW;
  FOR_EACH(logs, li) {
    ASSIGN_ARRAY_ITEM(logs, li, Log, log);
    SKIP_IS_NOT_ALIVE(log);
    if (log->pos.y > 100 + log->size / 2) {
      color = LIGHT_YELLOW;
      rect(log->pos.x - log->size / 2, 98, log->size, 2);
    } else {
      color = YELLOW;
      thickness = 3;
      Collision c = arc(log->pos.x, log->pos.y, log->size / 2, log->angle, log->angle + M_PI * 2);
      if (c.isColliding.rect[RED] && raftsman.isJumping) {
        play(CLICK);
        raftsman.isJumping = false;
        raftsman.currentLogIndex = li;
        log->angle = angleTo(&log->pos, raftsman.pos.x, raftsman.pos.y);
      }
    }
  }
}

static void update() {
  if (!ticks) {
    INIT_UNALIVED_ARRAY(logs);
    logIndex = 0;
    TIMES(5, i) {
      ASSIGN_ARRAY_ITEM(logs, logIndex, Log, log);
      if (i == 0) {
        vectorSet(&log->pos, 50, 75 + i * 30);
        log->size = 20;
        log->rotationSpeed = -0.05;
      } else {
        vectorSet(&log->pos, rnd(10, 90), 75 + i * 30);
        log->size = rnd(15, 25);
        log->rotationSpeed = rnd(0.04, 0.08) * RNDPM();
      }
      log->angle = 0;
      vectorSet(&log->vel, 0, 0);
      log->yDist = 0;
      log->isAlive = true;
      logIndex = wrap(logIndex + 1, 0, MAX_LOG_COUNT);
    }
    vectorSet(&raftsman.pos, 0, 0);
    raftsman.isJumping = false;
    raftsman.currentLogIndex = 0;
    vectorSet(&raftsman.vel, 0, 0);
    nextLogDist = 0;
    INIT_UNALIVED_ARRAY(clouds);
    cloudIndex = 0;
    nextCloudDist = 0;
    scrollDist = 0;
  }

  scrollDist = -0.05;
  COUNT_IS_ALIVE(logs, logsAliveCount);
  if (raftsman.pos.y > 50 && logsAliveCount > 0) {
    scrollDist += (50 - raftsman.pos.y) * 0.2;
  } else if (raftsman.pos.y < 20) {
    scrollDist += (20 - raftsman.pos.y) * 0.1;
  }
  scrollDist *= difficulty;

  updateClouds();
  updateRaftsman();
  updateLogs();
  drawGame();
}

void addGameSkyRaftsman() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
