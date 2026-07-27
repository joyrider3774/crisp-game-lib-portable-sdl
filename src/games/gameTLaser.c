#include "cglp.h"
#include "string.h"

static char *title = "T LASER";
static char *description = "[Slide]\n Move sight";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  gG  ",
        "  gG  ",
        "L gG l",
        "LLgGll",
        " LgGl ",
        " LgGl ",
    },
    {
        "     r",
        " l  l ",
        "l    l",
        "l    l",
        " l  l ",
        "r     ",
    },
};
static int charactersCount = 2;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = true};

#define TLASER_SIGHT_SPEED 1.5

typedef struct {
  Vector pos;
  float z;
} Star;
#define TLASER_STAR_COUNT 50
static Star stars[TLASER_STAR_COUNT];

typedef struct {
  Vector pos;
  float z;
  Vector vel;
  int lockIndex; // -1 = not locked onto
  int lockTicks;
  bool isAlive;
} Enemy;
// Once locked, an enemy never expires on its own - it only dies when the
// single laser eventually reaches it (processed one at a time in lock
// order), while any UNLOCKED enemy reaching the screen edge ends the game.
// So normal play forces the player to lock everything, and spawn batches
// (3-6 every rnd(60,99)/difficulty ticks) arrive faster than the one laser
// can clear them as difficulty climbs - the locked backlog grows without
// bound over a long session, so sized with real headroom.
#define TLASER_MAX_ENEMY_COUNT 512
static Enemy enemies[TLASER_MAX_ENEMY_COUNT];
static int enemyIndex;

#define TLASER_LASER_HISTORY_MAX 19
typedef struct {
  bool isAlive;
  Vector pos;
  float z;
  Vector vel;
  int lockIndex;
  Vector posHistory[TLASER_LASER_HISTORY_MAX];
  int posHistoryCount;
  int posHistoryStart;
} Laser;
static Laser laser;

static Vector sightPos;
static int nextLockIndex;
static float nextEnemyTicks;
static int multiplier;

// Push a new trailing position onto the laser's history, keeping at
// most the last TLASER_LASER_HISTORY_MAX positions (mirrors upstream's
// posHistory.push(...); if (length > 19) posHistory.shift();).
static void pushLaserHistory(Vector *p) {
  int idx = wrap(laser.posHistoryStart + laser.posHistoryCount, 0, TLASER_LASER_HISTORY_MAX);
  laser.posHistory[idx] = *p;
  if (laser.posHistoryCount < TLASER_LASER_HISTORY_MAX) {
    laser.posHistoryCount++;
  } else {
    laser.posHistoryStart = wrap(laser.posHistoryStart + 1, 0, TLASER_LASER_HISTORY_MAX);
  }
}

static void update() {
  if (!ticks) {
    vectorSet(&sightPos, 50, 50);
    INIT_UNALIVED_ARRAY(enemies);
    enemyIndex = 0;
    nextEnemyTicks = 0;
    nextLockIndex = 0;
    laser.isAlive = false;
    TIMES(TLASER_STAR_COUNT, sti) {
      vectorSet(&stars[sti].pos, rnd(0, 99), rnd(-20, 120));
      stars[sti].z = rnd(0.5, 3);
    }
    multiplier = 1;
  }

  TIMES(TLASER_STAR_COUNT, i) {
    Star *s = &stars[i];
    s->pos.y += 0.3 / sqrt(s->z);
    if (s->pos.y > 120) {
      s->pos.y -= 140;
    }
    int m = i % 3;
    if (m == 0) {
      color = LIGHT_CYAN;
    } else if (m == 1) {
      color = LIGHT_YELLOW;
    } else {
      color = LIGHT_RED;
    }
    rect(VEC_XY(s->pos), 1, 1);
  }

  color = BLACK;
  // Upstream drags the sight via mouse/touch; redesigned to direct d-pad movement.
  if (input.left.isPressed) {
    sightPos.x -= TLASER_SIGHT_SPEED;
  }
  if (input.right.isPressed) {
    sightPos.x += TLASER_SIGHT_SPEED;
  }
  if (input.up.isPressed) {
    sightPos.y -= TLASER_SIGHT_SPEED;
  }
  if (input.down.isPressed) {
    sightPos.y += TLASER_SIGHT_SPEED;
  }
  sightPos.x = clamp(sightPos.x, 0, 99);
  sightPos.y = clamp(sightPos.y, 0, 99);
  character("b", VEC_XY(sightPos));

  int ti = 9999999;
  int teIndex = -1;
  FOR_EACH(enemies, ei) {
    ASSIGN_ARRAY_ITEM(enemies, ei, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    if (e->lockIndex >= 0 && e->lockIndex < ti) {
      ti = e->lockIndex;
      teIndex = ei;
    }
  }

  if (!laser.isAlive && teIndex >= 0) {
    play(EXPLOSION);
    laser.isAlive = true;
    vectorSet(&laser.pos, 50, 99);
    laser.z = 1;
    vectorSet(&laser.vel, 0, -1);
    laser.lockIndex = 0;
    vectorSet(&laser.posHistory[0], 50, 99);
    laser.posHistoryCount = 1;
    laser.posHistoryStart = 0;
    color = BLACK;
    particle(50, 99, 30, 3, -M_PI_2, M_PI_4);
    multiplier = 1;
  }
  if (laser.isAlive) {
    float vz = 0;
    if (teIndex >= 0) {
      Enemy *te = &enemies[teIndex];
      Vector diff;
      vectorSet(&diff, te->pos.x, te->pos.y);
      vectorAdd(&diff, -laser.pos.x, -laser.pos.y);
      vectorMul(&diff, 0.5);
      vectorAdd(&laser.vel, diff.x, diff.y);
      vectorMul(&laser.vel, 0.5);
      vz = (te->z - laser.z) * 0.1;
      laser.z += vz;
    } else {
      float len = vectorLength(&laser.vel);
      // Real division by zero would be undefined for a zero-length vector -
      // guard the normalize step's length divide.
      if (len > 0) {
        vectorMul(&laser.vel, 1.0 / len);
      }
      vectorMul(&laser.vel, 5);
    }
    Vector step;
    step = laser.vel;
    vectorMul(&step, 1.0 / laser.z);
    vectorMul(&step, 1.0 / (1 + fabs(vz) * 9));
    vectorAdd(&laser.pos, step.x, step.y);
    Vector newHistPos;
    newHistPos = laser.pos;
    pushLaserHistory(&newHistPos);

    bool hasPrev = false;
    Vector pp;
    for (int k = 0; k < laser.posHistoryCount; k++) {
      int idx = wrap(laser.posHistoryStart + k, 0, TLASER_LASER_HISTORY_MAX);
      Vector p = laser.posHistory[idx];
      if (hasPrev) {
        if (rnd(0, 1) < (float)k / 9) {
          color = PURPLE;
        } else {
          color = BLACK;
        }
        float rs = 5 / laser.z;
        thickness = 3 / laser.z;
        float x1 = pp.x + rnd(0, rs) * RNDPM();
        float y1 = pp.y + rnd(0, rs) * RNDPM();
        float x2 = p.x + rnd(0, rs) * RNDPM();
        float y2 = p.y + rnd(0, rs) * RNDPM();
        line(x1, y1, x2, y2);
      }
      pp = p;
      hasPrev = true;
    }
    float o = 99 / laser.z;
    if (!(laser.pos.x >= -o && laser.pos.x <= 100 + o && laser.pos.y >= -o && laser.pos.y <= 100 + o)) {
      laser.isAlive = false;
      multiplier = 1;
    }
  }

  nextEnemyTicks--;
  if (nextEnemyTicks < 0) {
    int c = rndi(3, 6);
    Vector vel;
    vectorSet(&vel, 0, 0.2 + rnd(0, 0.3) * RNDPM());
    vectorMul(&vel, sqrt(difficulty));
    float startY;
    if (vel.y > 0) {
      startY = -5;
    } else {
      startY = 105;
    }
    Vector spos;
    vectorSet(&spos, rnd(10, 90), startY);
    float z = rnd(0.5, 3);
    vectorMul(&vel, 1.0 / sqrt(z));
    TIMES(c, ci) {
      ASSIGN_ARRAY_ITEM(enemies, enemyIndex, Enemy, ne);
      ne->pos = spos;
      ne->z = z;
      ne->vel = vel;
      ne->lockIndex = -1;
      ne->lockTicks = 9;
      ne->isAlive = true;
      enemyIndex = wrap(enemyIndex + 1, 0, TLASER_MAX_ENEMY_COUNT);
      spos.y -= vel.y * 30;
    }
    nextEnemyTicks += rnd(60, 99) / difficulty;
  }

  FOR_EACH(enemies, ei2) {
    ASSIGN_ARRAY_ITEM(enemies, ei2, Enemy, e);
    SKIP_IS_NOT_ALIVE(e);
    vectorAdd(&e->pos, e->vel.x, e->vel.y);
    color = BLACK;
    characterOptions.isMirrorX = false;
    characterOptions.isMirrorY = e->vel.y > 0;
    characterOptions.rotation = 0;
    Collision ec = character("a", VEC_XY(e->pos));
    if (ei2 == teIndex && ec.isColliding.rect[PURPLE]) {
      play(HIT);
      particle(e->pos.x, e->pos.y, 9, 2 / sqrt(e->z), 0, M_PI * 2);
      addScore(multiplier, VEC_XY(e->pos));
      multiplier++;
      e->isAlive = false;
      continue;
    }
    if (e->lockIndex < 0 && ec.isColliding.character['b']) {
      play(SELECT);
      e->lockIndex = nextLockIndex;
      nextLockIndex++;
    }
    if (e->lockIndex >= 0) {
      if (e->lockTicks > 0) {
        e->lockTicks--;
      }
      float a = -M_PI_2 - e->lockTicks * 0.3;
      float r = 5 + e->lockTicks * 5;
      thickness = 1;
      TIMES(4, li) {
        float pa = a;
        a += M_PI * 2 / (3 + (1.0 / 8) * e->lockTicks);
        Vector p1;
        vectorSet(&p1, r, 0);
        rotate(&p1, pa);
        vectorAdd(&p1, e->pos.x, e->pos.y);
        Vector p2;
        vectorSet(&p2, r, 0);
        rotate(&p2, a);
        vectorAdd(&p2, e->pos.x, e->pos.y);
        line(VEC_XY(p1), VEC_XY(p2));
      }
    }
    if (e->lockIndex < 0) {
      bool offscreen;
      if (e->vel.y < 0) {
        offscreen = e->pos.y < 0;
      } else {
        offscreen = e->pos.y > 99;
      }
      if (offscreen) {
        color = RED;
        characterOptions.isMirrorX = false;
        characterOptions.isMirrorY = false;
        characterOptions.rotation = 0;
        text("X", e->pos.x, clamp(e->pos.y, 5, 95));
        play(RANDOM);
        gameOver();
      }
    }
  }
  color = BLACK;
  characterOptions.isMirrorX = false;
  characterOptions.isMirrorY = false;
  characterOptions.rotation = 0;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameTLaser() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
