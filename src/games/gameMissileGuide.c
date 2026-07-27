#include "cglp.h"

static char *title = "MISSILE GUIDE";
static char *description = "[Hold] Freeze";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
} MissileguideMissile;
static MissileguideMissile missile;

typedef struct {
  float x;
  float y;
  float pulsePhase;
  bool isAlive;
} MissileguideOutpost;
// Concurrent count stays low (~3-4 typical) since spawn interval and travel
// speed both scale with difficulty in offsetting ways - sized well above
// worst-case estimates.
#define MISSILEGUIDE_MAX_OUTPOST_COUNT 32
static MissileguideOutpost outposts[MISSILEGUIDE_MAX_OUTPOST_COUNT];
static int outpostIndex;

typedef struct {
  float y;
  float gapX;
  float gapW;
  bool isAlive;
} MissileguideObstacle;
#define MISSILEGUIDE_MAX_OBSTACLE_COUNT 32
static MissileguideObstacle obstacles[MISSILEGUIDE_MAX_OBSTACLE_COUNT];
static int obstacleIndex;

typedef struct {
  float x;
  float y;
  int age;
  bool isAlive;
} MissileguideTrail;
// Trail spawns at most once per frame and lives at most 9 frames.
#define MISSILEGUIDE_MAX_TRAIL_COUNT 32
static MissileguideTrail trails[MISSILEGUIDE_MAX_TRAIL_COUNT];
static int trailIndex;

static float scrollSpeed;
static float nextSpawn;
static float fuel;
static int combo;
static float lastOutpostX;
static bool wasFrozen;
static float squashTime;

static void update() {
  if (!ticks) {
    missile.x = 50;
    missile.y = 65;
    missile.vx = 0;
    missile.vy = 0;
    INIT_UNALIVED_ARRAY(outposts);
    outpostIndex = 0;
    INIT_UNALIVED_ARRAY(obstacles);
    obstacleIndex = 0;
    scrollSpeed = 0.6;
    nextSpawn = 30;
    fuel = 100;
    combo = 0;
    lastOutpostX = 50;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    wasFrozen = false;
    squashTime = 0;
  }

  scrollSpeed = 0.6 * difficulty;
  bool frozen = input.isPressed;

  if (frozen && !wasFrozen) {
    squashTime = 10;
    color = LIGHT_CYAN;
    particle(missile.x, missile.y, 12, 1, 0, M_PI * 2);
  }
  if (!frozen && wasFrozen) {
    squashTime = -10;
    color = CYAN;
    particle(missile.x, missile.y, 8, 2, -M_PI / 2, M_PI / 4);
  }
  wasFrozen = frozen;

  if (squashTime > 0) {
    squashTime--;
  }
  if (squashTime < 0) {
    squashTime++;
  }

  if (frozen) {
    fuel -= 0.4;
  } else {
    fuel = fmin(fuel + 0.1, 100);
  }

  if (fuel <= 0) {
    play(EXPLOSION);
    color = RED;
    particle(missile.x, missile.y, 30, 3, 0, M_PI * 2);
    gameOver();
  }

  if (fuel < 20 && ticks % 10 == 0) {
    color = RED;
    particle(missile.x, missile.y + 5, 3, 0.5, M_PI / 2, M_PI / 4);
  }

  color = LIGHT_BLACK;
  rect(3, 97, fuel * 0.94, 2);

  nextSpawn--;
  if (nextSpawn <= 0) {
    float newX = lastOutpostX + rnd(-40, 40);
    newX = clamp(newX, 12, 88);
    if (rnd(0, 1) < 0.8) {
      ASSIGN_ARRAY_ITEM(outposts, outpostIndex, MissileguideOutpost, no);
      no->x = newX;
      no->y = -5;
      no->pulsePhase = rnd(0, M_PI * 2);
      no->isAlive = true;
      outpostIndex = wrap(outpostIndex + 1, 0, MISSILEGUIDE_MAX_OUTPOST_COUNT);
      lastOutpostX = newX;
    }

    if (ticks > 60 && rnd(0, 1) < 0.6) {
      float gapW = clamp(30, 16, 30);
      float gapX = newX + rnd(-20, 20);
      gapX = clamp(gapX, gapW / 2 + 5, 100 - gapW / 2 - 5);
      ASSIGN_ARRAY_ITEM(obstacles, obstacleIndex, MissileguideObstacle, nob);
      nob->y = -10;
      nob->gapX = gapX;
      nob->gapW = gapW;
      nob->isAlive = true;
      obstacleIndex = wrap(obstacleIndex + 1, 0, MISSILEGUIDE_MAX_OBSTACLE_COUNT);
    }
    nextSpawn = clamp(50 / difficulty, 25, 50);
  }

  float prevX = missile.x;
  float prevY = missile.y;

  if (!frozen) {
    bool targetFound = false;
    float targetX = 0;
    float targetY = 0;
    float minDist = 999;
    FOR_EACH(outposts, oi) {
      ASSIGN_ARRAY_ITEM(outposts, oi, MissileguideOutpost, o);
      SKIP_IS_NOT_ALIVE(o);
      if (o->y > missile.y - 60 && o->y < missile.y + 40) {
        float d = fabs(o->y - missile.y);
        if (d < minDist) {
          minDist = d;
          targetX = o->x;
          targetY = o->y;
          targetFound = true;
        }
      }
    }
    if (targetFound) {
      float dx = targetX - missile.x;
      float dy = targetY - missile.y;
      missile.x += clamp(dx * 0.03 * difficulty, -0.8, 0.8);
      missile.y += clamp(dy * 0.01 * difficulty, -0.8, 0.8);
    }
  }

  missile.x = clamp(missile.x, 6, 94);
  missile.y = clamp(missile.y, 60, 90);

  missile.vx = missile.x - prevX;
  missile.vy = missile.y - prevY;

  if (!frozen && (fabs(missile.vx) > 0.1 || fabs(missile.vy) > 0.1)) {
    ASSIGN_ARRAY_ITEM(trails, trailIndex, MissileguideTrail, nt);
    nt->x = missile.x;
    nt->y = missile.y;
    nt->age = 0;
    nt->isAlive = true;
    trailIndex = wrap(trailIndex + 1, 0, MISSILEGUIDE_MAX_TRAIL_COUNT);
  }
  FOR_EACH(trails, ti) {
    ASSIGN_ARRAY_ITEM(trails, ti, MissileguideTrail, t);
    SKIP_IS_NOT_ALIVE(t);
    t->age++;
    if (t->age > 8) {
      t->isAlive = false;
      continue;
    }
    float alpha = 1 - t->age / 8.0;
    color = LIGHT_CYAN;
    float size = 3 * alpha;
    box(t->x, t->y + t->age * 0.5, size, size * 1.5);
  }

  float mWidth = 4;
  float mHeight = 7;
  if (squashTime > 0) {
    mWidth = 4 + squashTime * 0.3;
    mHeight = 7 - squashTime * 0.4;
  } else if (squashTime < 0) {
    mWidth = 4 + squashTime * 0.2;
    mHeight = 7 - squashTime * 0.5;
  }

  float tilt = clamp(missile.vx * 0.3, -0.4, 0.4);

  color = CYAN;
  thickness = mWidth;
  barCenterPosRatio = 0.5;
  bar(missile.x, missile.y, mHeight, -M_PI / 2 + tilt);

  if (frozen) {
    color = LIGHT_CYAN;
    float pulseSize = 8 + sin(ticks * 0.2) * 2;
    thickness = 2;
    arc(missile.x, missile.y, pulseSize, 0, M_PI * 2);
  }

  FOR_EACH(outposts, oi2) {
    ASSIGN_ARRAY_ITEM(outposts, oi2, MissileguideOutpost, o2);
    SKIP_IS_NOT_ALIVE(o2);
    o2->y += scrollSpeed;
    o2->pulsePhase += 0.15;
    float pulse = 1 + sin(o2->pulsePhase) * 0.15;
    float size = 6 * pulse;
    color = YELLOW;
    Collision col = box(o2->x, o2->y, size, size);
    if (!frozen && col.isColliding.rect[CYAN]) {
      play(COIN);
      combo++;
      addScore(1 + floor(combo / 3.0), SCORE_NO_POPUP_X, SCORE_NO_POPUP_Y);
      fuel = fmin(fuel + 12, 100);
      particle(o2->x, o2->y, 12, 2, 0, M_PI * 2);
      o2->isAlive = false;
      continue;
    }
    if (o2->y > 105) {
      combo = 0;
      o2->isAlive = false;
      continue;
    }
  }

  FOR_EACH(obstacles, oi3) {
    ASSIGN_ARRAY_ITEM(obstacles, oi3, MissileguideObstacle, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->y += scrollSpeed;
    color = RED;
    float leftW = ob->gapX - ob->gapW / 2;
    float rightX = ob->gapX + ob->gapW / 2;
    float rightW = 100 - rightX;
    bool hitLeft = false;
    bool hitRight = false;
    if (leftW > 2) {
      Collision colL = box(leftW / 2, ob->y, leftW, 5);
      hitLeft = colL.isColliding.rect[CYAN];
    }
    if (rightW > 2) {
      Collision colR = box(rightX + rightW / 2, ob->y, rightW, 5);
      hitRight = colR.isColliding.rect[CYAN];
    }
    if (!frozen && (hitLeft || hitRight)) {
      play(EXPLOSION);
      color = YELLOW;
      particle(missile.x, missile.y, 25, 3, 0, M_PI * 2);
      gameOver();
    }
    if (ob->y > 105) {
      ob->isAlive = false;
      continue;
    }
  }
}

void addGameMissileGuide() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
