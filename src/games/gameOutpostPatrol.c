#include "cglp.h"

static char *title = "OUTPOST\nPATROL";
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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 7, .isDarkColor = false};

#define OUTPOSTPATROL_TRACK_RADIUS 40
#define OUTPOSTPATROL_JUMP_DURATION 30

typedef struct {
  Vector pos;
  float angle;
  bool isJumping;
  float jumpHeight;
} OutpostPatrolSentry;
static OutpostPatrolSentry sentry;
static float jumpTicks;

typedef struct {
  Vector pos;
  float angle;
  bool isAlive;
} OutpostPatrolObstacle;
#define OUTPOSTPATROL_MAX_OBSTACLE_COUNT 16
static OutpostPatrolObstacle obstacles[OUTPOSTPATROL_MAX_OBSTACLE_COUNT];
static int obstacleIndex;
static float nextObstacleTicks;

typedef struct {
  Vector pos;
  float angle;
  float height;
  bool isAlive;
} OutpostPatrolBomb;
// 16->1024: bombs never expire on their own (only removed by a precisely-timed
// jump hit) and spawn every ~105/difficulty ticks, so uncleared bombs pile up
// close to linearly with play time - simulation shows the count already blows
// past 16 within the first minute even with zero bombs ever destroyed.
#define OUTPOSTPATROL_MAX_BOMB_COUNT 1024
static OutpostPatrolBomb bombs[OUTPOSTPATROL_MAX_BOMB_COUNT];
static int bombIndex;
static float nextBombTicks;

typedef struct {
  float angle;
  float width;
  bool isAlive;
} OutpostPatrolWave;
#define OUTPOSTPATROL_MAX_WAVE_COUNT 16
static OutpostPatrolWave waves[OUTPOSTPATROL_MAX_WAVE_COUNT];
static int waveIndex;

static void update() {
  if (!ticks) {
    vectorSet(&sentry.pos, 50, 50);
    sentry.angle = 0;
    sentry.isJumping = false;
    sentry.jumpHeight = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    nextObstacleTicks = 0;
    INIT_UNALIVED_ARRAY(bombs);
    bombIndex = 0;
    nextBombTicks = 0;
    INIT_UNALIVED_ARRAY(waves);
    waveIndex = 0;
  }
  sentry.angle += 0.02 * difficulty;
  vectorSet(&sentry.pos, 50, 50);
  Vector sentryOfs;
  vectorSet(&sentryOfs, OUTPOSTPATROL_TRACK_RADIUS, 0);
  rotate(&sentryOfs, sentry.angle);
  vectorAdd(&sentry.pos, sentryOfs.x, sentryOfs.y);
  if (input.isJustPressed && !sentry.isJumping) {
    play(JUMP);
    sentry.isJumping = true;
    jumpTicks = 0;
  }
  if (sentry.isJumping) {
    float jumpAdd;
    if (input.isPressed) {
      jumpAdd = 1;
    } else {
      jumpAdd = 2;
    }
    jumpTicks += jumpAdd * difficulty;
    sentry.jumpHeight = 10 * sin(jumpTicks / OUTPOSTPATROL_JUMP_DURATION * M_PI);
    if (jumpTicks >= OUTPOSTPATROL_JUMP_DURATION) {
      play(HIT);
      sentry.isJumping = false;
      sentry.jumpHeight = 0;
    }
  }
  nextObstacleTicks -= difficulty;
  if (nextObstacleTicks < 0) {
    float angle = sentry.angle + M_PI;
    float minAo = M_PI;
    FOR_EACH(obstacles, i) {
      ASSIGN_ARRAY_ITEM(obstacles, i, OutpostPatrolObstacle, ob);
      SKIP_IS_NOT_ALIVE(ob);
      float ao = fabs(wrap(ob->angle - angle, -M_PI, M_PI));
      if (ao < minAo) {
        minAo = ao;
      }
    }
    if (minAo > M_PI * 0.2) {
      play(LASER);
      ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, OutpostPatrolObstacle, no);
      vectorSet(&no->pos, 50, 50);
      no->angle = angle;
      no->isAlive = true;
      obstacleIndex = wrap(obstacleIndex + 1, 0, OUTPOSTPATROL_MAX_OBSTACLE_COUNT);
    }
    nextObstacleTicks += rnd(60, 150);
  }
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, OutpostPatrolObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->angle -= 0.01 * difficulty;
    vectorSet(&ob->pos, 50, 50);
    Vector ofs;
    vectorSet(&ofs, OUTPOSTPATROL_TRACK_RADIUS, 0);
    rotate(&ofs, ob->angle);
    vectorAdd(&ob->pos, ofs.x, ofs.y);
  }
  nextBombTicks -= difficulty;
  if (nextBombTicks < 0) {
    play(CLICK);
    ASSIGN_ARRAY_ITEM(bombs, bombIndex, OutpostPatrolBomb, nb);
    vectorSet(&nb->pos, 50, 50);
    nb->angle = rnd(0, 2 * M_PI);
    nb->height = 10;
    nb->isAlive = true;
    bombIndex = wrap(bombIndex + 1, 0, OUTPOSTPATROL_MAX_BOMB_COUNT);
    nextBombTicks += rnd(90, 120);
  }
  FOR_EACH(bombs, i) {
    ASSIGN_ARRAY_ITEM(bombs, i, OutpostPatrolBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    b->angle -= 0.015 * difficulty;
    vectorSet(&b->pos, 50, 50);
    Vector ofs;
    vectorSet(&ofs, OUTPOSTPATROL_TRACK_RADIUS, 0);
    rotate(&ofs, b->angle);
    vectorAdd(&b->pos, ofs.x, ofs.y);
  }
  color = LIGHT_BLACK;
  thickness = 3;
  arc(50, 50, OUTPOSTPATROL_TRACK_RADIUS, 0, M_PI * 2);
  color = YELLOW;
  FOR_EACH(waves, i) {
    ASSIGN_ARRAY_ITEM(waves, i, OutpostPatrolWave, w);
    SKIP_IS_NOT_ALIVE(w);
    w->width += 0.1 * difficulty;
    thickness = 4;
    arc(50, 50, OUTPOSTPATROL_TRACK_RADIUS, w->angle - w->width / 2, w->angle + w->width / 2);
    if (w->width > 1) {
      w->isAlive = false;
    }
  }
  color = RED;
  FOR_EACH(obstacles, i) {
    ASSIGN_ARRAY_ITEM(obstacles, i, OutpostPatrolObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    Collision obCol = box(ob->pos.x, ob->pos.y, 5, 5);
    if (obCol.isColliding.rect[YELLOW]) {
      play(COIN);
      addScore(1, SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      ob->isAlive = false;
      continue;
    }
  }
  if (sentry.isJumping) {
    color = CYAN;
  } else {
    color = BLUE;
  }
  Collision sentryCol = box(sentry.pos.x, sentry.pos.y - sentry.jumpHeight, 7, 7);
  if (sentryCol.isColliding.rect[RED] && !sentry.isJumping) {
    play(EXPLOSION);
    gameOver();
  }
  color = YELLOW;
  FOR_EACH(bombs, i) {
    ASSIGN_ARRAY_ITEM(bombs, i, OutpostPatrolBomb, b);
    SKIP_IS_NOT_ALIVE(b);
    Collision bCol = box(b->pos.x, b->pos.y - b->height, 6, 6);
    if ((bCol.isColliding.rect[BLUE] || bCol.isColliding.rect[CYAN]) && sentry.isJumping) {
      play(POWER_UP);
      ASSIGN_ARRAY_ITEM(waves, waveIndex, OutpostPatrolWave, nw);
      nw->angle = b->angle;
      nw->width = 0;
      nw->isAlive = true;
      waveIndex = wrap(waveIndex + 1, 0, OUTPOSTPATROL_MAX_WAVE_COUNT);
      b->isAlive = false;
      continue;
    }
  }
}

void addGameOutpostPatrol() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
