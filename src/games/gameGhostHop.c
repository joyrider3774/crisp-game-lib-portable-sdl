#include "cglp.h"
#include "string.h"

static char *title = "GHOST HOP";
static char *description = "[Tap] Hop";

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
  float squash;
} Target;
#define GHOSTHOP_TARGET_COUNT 5
static Target targets[GHOSTHOP_TARGET_COUNT];

typedef struct {
  int idx;
  float squash;
  float stretch;
} Player;
static Player player;

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  bool isAlive;
} Bullet;
#define GHOSTHOP_MAX_BULLET_COUNT 32
static Bullet bullets[GHOSTHOP_MAX_BULLET_COUNT];
static int bulletIndex;

typedef struct {
  float x;
  float y;
  int life;
  bool isAlive;
} Trail;
#define GHOSTHOP_MAX_TRAIL_COUNT 16
static Trail trails[GHOSTHOP_MAX_TRAIL_COUNT];
static int trailIndex;

typedef struct {
  float x;
  float y;
  float vx;
  float vy;
  int life;
  int col;
  bool isAlive;
} Particle;
#define GHOSTHOP_MAX_PARTICLE_COUNT 64
static Particle particles[GHOSTHOP_MAX_PARTICLE_COUNT];
static int particleIndex;

static bool jumping;
static int jumpTargetIdx;
static float jumpT;
static float jumpFromX;
static float jumpFromY;
static float multiplier;

static void update() {
  if (!ticks) {
    TIMES(GHOSTHOP_TARGET_COUNT, ti) {
      targets[ti].x = 20 + (ti % 3) * 30;
      targets[ti].y = 25 + (ti / 3) * 40;
      targets[ti].vx = (rnd(0, 1) - 0.5) * 0.4;
      targets[ti].vy = (rnd(0, 1) - 0.5) * 0.4;
      targets[ti].squash = 0;
    }
    player.idx = 2;
    player.squash = 0;
    player.stretch = 0;
    INIT_UNALIVED_ARRAY(bullets);
    bulletIndex = 0;
    jumping = false;
    jumpTargetIdx = 0;
    jumpT = 0;
    jumpFromX = 50;
    jumpFromY = 50;
    multiplier = 1;
    INIT_UNALIVED_ARRAY(trails);
    trailIndex = 0;
    INIT_UNALIVED_ARRAY(particles);
    particleIndex = 0;
  }

  float bulletSpd = 0.5 * difficulty;

  int spawnInterval = (int)ceil(50.0 / difficulty);
  if (ticks % spawnInterval == 0) {
    int side = (int)rnd(0, 4);
    float bx;
    float by;
    Target *t = &targets[player.idx];
    if (side == 0) {
      bx = -5;
      by = rnd(0, 100);
    } else if (side == 1) {
      bx = 105;
      by = rnd(0, 100);
    } else if (side == 2) {
      bx = rnd(0, 100);
      by = -5;
    } else {
      bx = rnd(0, 100);
      by = 105;
    }
    float ang = atan2(t->y - by, t->x - bx);
    ASSIGN_ARRAY_ITEM(bullets, bulletIndex, Bullet, nb);
    nb->x = bx;
    nb->y = by;
    nb->vx = cos(ang) * bulletSpd;
    nb->vy = sin(ang) * bulletSpd;
    nb->isAlive = true;
    bulletIndex = wrap(bulletIndex + 1, 0, GHOSTHOP_MAX_BULLET_COUNT);
  }

  if (input.isJustPressed && !jumping) {
    int nextIdx = (player.idx + 1) % GHOSTHOP_TARGET_COUNT;
    jumping = true;
    jumpTargetIdx = nextIdx;
    jumpT = 0;
    Target *t = &targets[player.idx];
    jumpFromX = t->x;
    jumpFromY = t->y;
    player.stretch = 1.0;
    play(JUMP);
  }

  TIMES(GHOSTHOP_TARGET_COUNT, ti2) {
    Target *t = &targets[ti2];
    t->x += t->vx * sqrt(difficulty);
    t->y += t->vy * sqrt(difficulty);
    bool bounced = false;
    if (t->x < 15 || t->x > 85) {
      t->vx *= -1;
      bounced = true;
    }
    if (t->y < 15 || t->y > 85) {
      t->vy *= -1;
      bounced = true;
    }
    t->x = clamp(t->x, 15, 85);
    t->y = clamp(t->y, 15, 85);
    if (bounced) {
      t->squash = 0.5;
    }
    t->squash *= 0.85;
  }

  float px;
  float py;
  float eyeDirX = 0;
  float eyeDirY = 0;
  if (jumping) {
    jumpT += 0.08 * sqrt(difficulty);
    Target *tgt = &targets[jumpTargetIdx];
    px = jumpFromX + (tgt->x - jumpFromX) * jumpT;
    py = jumpFromY + (tgt->y - jumpFromY) * jumpT;
    eyeDirX = tgt->x - px;
    eyeDirY = tgt->y - py;
    player.stretch = (1 - jumpT) * 0.8;
    if (ticks % 3 == 0) {
      ASSIGN_ARRAY_ITEM(trails, trailIndex, Trail, nt);
      nt->x = px;
      nt->y = py;
      nt->life = 16;
      nt->isAlive = true;
      trailIndex = wrap(trailIndex + 1, 0, GHOSTHOP_MAX_TRAIL_COUNT);
    }
    if (jumpT >= 1) {
      player.idx = jumpTargetIdx;
      jumping = false;
      player.squash = 0.6;
      player.stretch = 0;
      play(POWER_UP);
      addScore(floor(multiplier), px, py);
      multiplier *= 2;
      TIMES(2, pj) {
        ASSIGN_ARRAY_ITEM(particles, particleIndex, Particle, np);
        np->x = px;
        np->y = py;
        np->vx = (rnd(0, 1) - 0.5) * 2;
        np->vy = rnd(0, 1) * 1.5;
        np->life = 12;
        np->col = CYAN;
        np->isAlive = true;
        particleIndex = wrap(particleIndex + 1, 0, GHOSTHOP_MAX_PARTICLE_COUNT);
      }
    }
  } else {
    Target *t = &targets[player.idx];
    px = t->x;
    py = t->y;
    Target *nextT = &targets[(player.idx + 1) % GHOSTHOP_TARGET_COUNT];
    eyeDirX = nextT->x - px;
    eyeDirY = nextT->y - py;
  }

  FOR_EACH(particles, particleIdx) {
    ASSIGN_ARRAY_ITEM(particles, particleIdx, Particle, p);
    SKIP_IS_NOT_ALIVE(p);
    p->x += p->vx;
    p->y += p->vy;
    p->vy += 0.1;
    p->life--;
    color = p->col;
    particle(p->x, p->y, 1, M_PI / 4, M_PI / 4, M_PI * 2);
    p->isAlive = p->life > 0;
  }

  FOR_EACH(trails, tri) {
    ASSIGN_ARRAY_ITEM(trails, tri, Trail, tr);
    SKIP_IS_NOT_ALIVE(tr);
    tr->life--;
    float alpha = (float)tr->life / 8;
    color = LIGHT_YELLOW;
    box(tr->x, tr->y, 5 * alpha, 5 * alpha);
    tr->isAlive = tr->life > 0;
  }

  player.squash *= 0.85;

  TIMES(GHOSTHOP_TARGET_COUNT, ti3) {
    Target *t = &targets[ti3];
    bool isNext = (ti3 == (player.idx + 1) % GHOSTHOP_TARGET_COUNT) && !jumping;
    if (isNext) {
      color = LIGHT_CYAN;
    } else {
      color = GREEN;
    }
    float baseSize;
    if (ti3 == player.idx && !jumping) {
      baseSize = 8;
    } else {
      baseSize = 5;
    }
    float breath = sin(ticks * 0.1 + ti3) * 0.3;
    float sw = baseSize * (1 + t->squash * 0.5 + breath * 0.1);
    float sh = baseSize * (1 - t->squash * 0.3 + breath * 0.1);
    box(t->x, t->y, sw, sh);
    bool isCurrent = ti3 == player.idx && !jumping;
    float eyeSpacing = 1.5;
    if (isCurrent || isNext) {
      float lookX = t->vx;
      float lookY = t->vy;
      float len = sqrt(lookX * lookX + lookY * lookY);
      if (len > 0.01) {
        lookX /= len;
        lookY /= len;
      }
      color = WHITE;
      box(t->x - eyeSpacing, t->y - 1, 2, 2);
      box(t->x + eyeSpacing, t->y - 1, 2, 2);
      color = BLACK;
      box(t->x - eyeSpacing + lookX * 0.5, t->y - 1 + lookY * 0.5, 1, 1);
      box(t->x + eyeSpacing + lookX * 0.5, t->y - 1 + lookY * 0.5, 1, 1);
    } else {
      color = BLACK;
      box(t->x - eyeSpacing, t->y - 1, 2, 0.5);
      box(t->x + eyeSpacing, t->y - 1, 2, 0.5);
    }
  }

  if (jumping) {
    color = YELLOW;
  } else {
    color = CYAN;
  }
  float breath2 = sin(ticks * 0.15) * 0.2;
  float pw = 5 * (1 + player.squash * 0.5 - player.stretch * 0.3 + breath2 * 0.1);
  float ph = 5 * (1 - player.squash * 0.3 + player.stretch * 0.5 + breath2 * 0.1);
  float tiltAngle = 0;
  if (jumping) {
    Target *tgt = &targets[jumpTargetIdx];
    tiltAngle = atan2(tgt->y - jumpFromY, tgt->x - jumpFromX);
  }
  if (fabs(tiltAngle) < 0.1) {
    box(px, py, pw, ph);
  } else {
    thickness = pw * 0.8;
    barCenterPosRatio = 0.5;
    bar(px, py, ph, tiltAngle);
  }
  float eyeLen = sqrt(eyeDirX * eyeDirX + eyeDirY * eyeDirY);
  if (eyeLen > 0.01) {
    eyeDirX /= eyeLen;
    eyeDirY /= eyeLen;
  }
  color = WHITE;
  float playerEyeSpacing = 1.5;
  box(px - playerEyeSpacing, py - 1, 2, 2);
  box(px + playerEyeSpacing, py - 1, 2, 2);
  color = BLACK;
  box(px - playerEyeSpacing + eyeDirX * 0.6, py - 1 + eyeDirY * 0.6, 1, 1);
  box(px + playerEyeSpacing + eyeDirX * 0.6, py - 1 + eyeDirY * 0.6, 1, 1);

  FOR_EACH(bullets, bi) {
    ASSIGN_ARRAY_ITEM(bullets, bi, Bullet, b);
    SKIP_IS_NOT_ALIVE(b);
    b->x += b->vx;
    b->y += b->vy;
    if (b->x < -10 || b->x > 110 || b->y < -10 || b->y > 110) {
      b->isAlive = false;
      continue;
    }
    color = RED;
    float bulletAngle = atan2(b->vy, b->vx);
    thickness = 2;
    barCenterPosRatio = 0.5;
    Collision col = bar(b->x, b->y, 4, bulletAngle);
    if (col.isColliding.rect[YELLOW]) {
      play(EXPLOSION);
      TIMES(3, pj2) {
        ASSIGN_ARRAY_ITEM(particles, particleIndex, Particle, np2);
        np2->x = b->x;
        np2->y = b->y;
        np2->vx = (rnd(0, 1) - 0.5) * 3;
        np2->vy = (rnd(0, 1) - 0.5) * 3;
        np2->life = 10;
        np2->col = RED;
        np2->isAlive = true;
        particleIndex = wrap(particleIndex + 1, 0, GHOSTHOP_MAX_PARTICLE_COUNT);
      }
      gameOver();
    }
  }

  multiplier = clamp(multiplier * 0.99, 1, 256);
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar((int)multiplier));
  color = BLACK;
  text(multText, 3, 9);
}

void addGameGhostHop() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
