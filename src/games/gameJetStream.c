#include "cglp.h"
#include "string.h"

static char *title = "JET STREAM";
static char *description = "[Tap] Cycle Lanes";

#define CS static char characters[][CHARACTER_HEIGHT][CHARACTER_WIDTH + 1]
CS = {
    {
        "  ll  ",
        "   l l",
        "  lll ",
        " l l  ",
        "  ll  ",
        " l  l ",
    },
    {
        "  ll  ",
        "   l  ",
        " lllll",
        "  l   ",
        " l l  ",
        "l  l  ",
    },
    {
        "  ll  ",
        " l l  ",
        "  lll ",
        "   l l",
        "  ll  ",
        "  l l ",
    },
};
static int charactersCount = 3;

static Options options = {
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 6, .isDarkColor = false};

typedef struct {
  Vector pos;
  int lane;
  int step;
  float animFrame;
  float boardSway;
  Vector knockbackVel;
  float buttonKnockbackRatio;
} JetstreamSurfer;
static JetstreamSurfer surfer;

typedef struct {
  Vector pos;
  Vector vel;
  float size;
  float life;
  float maxLife;
  bool isAlive;
} JetstreamWakeParticle;
#define JETSTREAM_MAX_WAKE_PARTICLE_COUNT 32
static JetstreamWakeParticle wakeParticles[JETSTREAM_MAX_WAKE_PARTICLE_COUNT];
static int wakeParticleIndex;

typedef struct {
  Vector pos;
  int lane;
  float rotation;
  float pulsePhase;
  bool shrinking;
  float shrinkScale;
  float shrinkSpeed;
  bool destroyed;
  float destroyScale;
  float destroyExpansion;
  bool isAlive;
} JetstreamWindBarrier;
// Concurrent count grows roughly as 1.3*sqrt(difficulty) (barrier speed and
// spawn cadence both scale with difficulty) - generous headroom applied for
// a long session.
#define JETSTREAM_MAX_WIND_BARRIER_COUNT 64
static JetstreamWindBarrier windBarriers[JETSTREAM_MAX_WIND_BARRIER_COUNT];
static int windBarrierIndex;

static float streamOffset;
static float barrierSpawnTimer;
static float baseSpawnInterval;
static int wakeParticleTimer;
static int wakeParticleInterval;
static int multiplier;

static void update() {
  if (!ticks) {
    vectorSet(&surfer.pos, 50, 25);
    surfer.lane = 0;
    surfer.step = 0;
    surfer.animFrame = 0;
    surfer.boardSway = 0;
    vectorSet(&surfer.knockbackVel, 0, 0);
    surfer.buttonKnockbackRatio = 0;
    INIT_UNALIVED_ARRAY(windBarriers);
    windBarrierIndex = 0;
    streamOffset = 0;
    INIT_UNALIVED_ARRAY(wakeParticles);
    wakeParticleIndex = 0;
    barrierSpawnTimer = 0;
    baseSpawnInterval = 120;
    wakeParticleTimer = 0;
    wakeParticleInterval = 3;
    multiplier = 1;
  }

  if (input.isJustPressed) {
    surfer.step = (surfer.step + 1) % 4;

    surfer.knockbackVel.x += surfer.buttonKnockbackRatio;
    surfer.knockbackVel.y -= 1;
    surfer.buttonKnockbackRatio += 1;

    if (surfer.step == 0) {
      surfer.lane = 0;
    } else if (surfer.step == 1) {
      surfer.lane = 1;
    } else if (surfer.step == 2) {
      surfer.lane = 2;
    } else if (surfer.step == 3) {
      surfer.lane = 1;
    }

    play(SELECT);
  }
  surfer.buttonKnockbackRatio *= 0.9;

  float targetY;
  if (surfer.lane == 0) {
    targetY = 21;
  } else if (surfer.lane == 1) {
    targetY = 46;
  } else {
    targetY = 71;
  }
  surfer.pos.y += (targetY - surfer.pos.y) * 0.1;

  vectorAdd(&surfer.pos, surfer.knockbackVel.x, surfer.knockbackVel.y);
  vectorMul(&surfer.knockbackVel, 0.9);

  float laneSpeed = 0.5 * sqrt(difficulty);
  if (surfer.lane == 0 || surfer.lane == 2) {
    surfer.pos.x -= laneSpeed;
  } else {
    surfer.pos.x += laneSpeed;
  }

  streamOffset += 2 * sqrt(difficulty);
  color = LIGHT_BLUE;
  thickness = 2;
  int si;
  for (si = 0; si < 5; si++) {
    float xUpper = 100 - (fmod(si * 25 + streamOffset, 125) - 25);
    line(xUpper, 25, xUpper - 15, 25);

    float xCenter = fmod(si * 25 + streamOffset, 125) - 25;
    thickness = 2;
    line(xCenter, 50, xCenter + 15, 50);

    float xLower = 100 - (fmod(si * 25 + streamOffset, 125) - 25);
    thickness = 2;
    line(xLower, 75, xLower - 15, 75);
  }

  if (rnd(0, 1) < 0.05) {
    surfer.animFrame++;
  }
  surfer.boardSway += 0.1;

  color = BLUE;
  int animLookup[4];
  animLookup[0] = 0;
  animLookup[1] = 1;
  animLookup[2] = 2;
  animLookup[3] = 1;
  int charIndex = animLookup[(int)floor(surfer.animFrame) % 4];

  char surferChar[2];
  surferChar[0] = 'a' + charIndex;
  surferChar[1] = 0;
  character(surferChar, VEC_XY(surfer.pos));

  color = YELLOW;
  float boardY = surfer.pos.y + 4;
  float swayAmount;
  if (surfer.lane == 0 || surfer.lane == 2) {
    swayAmount = sin(surfer.boardSway + streamOffset * 0.02) * 1;
  } else {
    swayAmount = sin(surfer.boardSway - streamOffset * 0.02) * 1;
  }

  float boardStartX = surfer.pos.x - 4;
  float boardEndX = surfer.pos.x + 4;
  float boardStartY = boardY + swayAmount;
  float boardEndY = boardY - swayAmount;

  thickness = 2;
  line(boardStartX, boardStartY, boardEndX, boardEndY);

  wakeParticleTimer++;

  if (wakeParticleTimer >= wakeParticleInterval) {
    float wakeX;
    float velocityX;
    if (surfer.lane == 0 || surfer.lane == 2) {
      wakeX = surfer.pos.x + 5;
      velocityX = 0.3;
    } else {
      wakeX = surfer.pos.x - 5;
      velocityX = -0.3;
    }

    ASSIGN_ARRAY_ITEM(wakeParticles, wakeParticleIndex, JetstreamWakeParticle, nw);
    vectorSet(&nw->pos, wakeX, boardY);
    vectorSet(&nw->vel, velocityX, -0.1);
    nw->size = 1;
    nw->life = 24;
    nw->maxLife = 24;
    nw->isAlive = true;
    wakeParticleIndex = wrap(wakeParticleIndex + 1, 0, JETSTREAM_MAX_WAKE_PARTICLE_COUNT);

    wakeParticleTimer = 0;
  }

  color = CYAN;
  FOR_EACH(wakeParticles, wpi) {
    ASSIGN_ARRAY_ITEM(wakeParticles, wpi, JetstreamWakeParticle, p);
    SKIP_IS_NOT_ALIVE(p);
    vectorAdd(&p->pos, p->vel.x, p->vel.y);
    p->life--;

    p->size = 1 + (1 - p->life / p->maxLife) * 2;

    if (p->life > 0) {
      rect(p->pos.x - p->size / 2, p->pos.y - p->size / 2, p->size, p->size);
    }

    if (p->life <= 0 || p->pos.x < -5 || p->pos.x > 105) {
      p->isAlive = false;
    }
  }

  barrierSpawnTimer += difficulty;

  float nextSpawnInterval = baseSpawnInterval + rnd(-30, 30);

  if (barrierSpawnTimer >= nextSpawnInterval) {
    int lane = rndi(0, 3);
    float yPos;
    if (lane == 0) {
      yPos = 25;
    } else if (lane == 1) {
      yPos = 50;
    } else {
      yPos = 75;
    }
    float spawnX;
    if (lane == 0 || lane == 2) {
      spawnX = -10;
    } else {
      spawnX = 110;
    }
    ASSIGN_ARRAY_ITEM(windBarriers, windBarrierIndex, JetstreamWindBarrier, nb);
    vectorSet(&nb->pos, spawnX, yPos);
    nb->lane = lane;
    nb->rotation = 0;
    nb->pulsePhase = rnd(0, 2 * M_PI);
    nb->shrinking = false;
    nb->shrinkScale = 1.0;
    nb->shrinkSpeed = 0.15;
    nb->destroyed = false;
    nb->destroyScale = 1.0;
    nb->destroyExpansion = 1.0;
    nb->isAlive = true;
    windBarrierIndex = wrap(windBarrierIndex + 1, 0, JETSTREAM_MAX_WIND_BARRIER_COUNT);

    barrierSpawnTimer = 0;
    baseSpawnInterval = 120 + rnd(-20, 20);
  }

  FOR_EACH(windBarriers, bi) {
    ASSIGN_ARRAY_ITEM(windBarriers, bi, JetstreamWindBarrier, b);
    SKIP_IS_NOT_ALIVE(b);

    if (!b->shrinking) {
      float barrierSpeed = 1 * sqrt(difficulty);
      if (b->lane == 0 || b->lane == 2) {
        b->pos.x += barrierSpeed;
        b->rotation += 0.15;
      } else {
        b->pos.x -= barrierSpeed;
        b->rotation -= 0.15;
      }
    }

    b->pulsePhase += 0.2;

    if (b->shrinking) {
      b->shrinkScale -= b->shrinkSpeed;
      if (b->shrinkScale <= 0) {
        b->isAlive = false;
        continue;
      }
    }

    if (b->destroyed) {
      b->destroyScale -= 0.1;
      b->destroyExpansion += 0.9;
      if (b->destroyScale <= 0) {
        b->isAlive = false;
        continue;
      }
    }

    float currentScale;
    if (b->shrinking) {
      currentScale = b->shrinkScale;
    } else if (b->destroyed) {
      currentScale = b->destroyScale;
    } else {
      currentScale = 1.0;
    }
    float expansionScale;
    if (b->destroyed) {
      expansionScale = b->destroyExpansion;
    } else {
      expansionScale = 1.0;
    }

    float pulseSize = (1 + sin(b->pulsePhase) * 0.3) * currentScale;

    color = LIGHT_PURPLE;
    bool windCollision = false;
    int wi;
    for (wi = 0; wi < 4; wi++) {
      float angle = b->rotation + (wi * M_PI) / 2;
      float windX = b->pos.x + cos(angle) * 6 * currentScale * expansionScale;
      float windY = b->pos.y + sin(angle) * 6 * currentScale * expansionScale;
      thickness = 3;
      Collision windCol = arc(windX, windY, 2 * currentScale, 0, M_PI * 2);

      if (!b->shrinking && !b->destroyed && windCol.isColliding.rect[CYAN]) {
        windCollision = true;
      }
    }

    if (windCollision) {
      b->destroyed = true;

      addScore(multiplier, VEC_XY(b->pos));

      // Vircon32 port note: upstream picks between three sound-seed
      // variants of the same effect by multiplier tier - this engine's
      // play() has no per-call seed, so all three tiers just play POWER_UP.
      play(POWER_UP);
      multiplier = clamp(multiplier + 1, 1, 16);
    }

    color = PURPLE;
    thickness = 3;
    Collision coreCol = arc(b->pos.x, b->pos.y, 3 * pulseSize, 0, M_PI * 2);

    if (!b->shrinking && !b->destroyed &&
        (coreCol.isColliding.character['a'] || coreCol.isColliding.character['b'] ||
         coreCol.isColliding.character['c'])) {
      if (b->lane == 0 || b->lane == 2) {
        surfer.knockbackVel.x += 3 * sqrt(difficulty);
      } else {
        surfer.knockbackVel.x -= 3 * sqrt(difficulty);
      }

      surfer.knockbackVel.y += rnd(-0.5, 0.5);

      b->shrinking = true;

      play(EXPLOSION);
    }

    if (!b->shrinking && !b->destroyed && (b->pos.x < -15 || b->pos.x > 115)) {
      multiplier = clamp(multiplier - 1, 1, 16);
      b->isAlive = false;
      continue;
    }
  }

  if (surfer.pos.x < -3 || surfer.pos.x > 103) {
    play(EXPLOSION);
    gameOver();
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  // Vircon32 port note: JS drew this with an unsupported isSmallText option;
  // drawn at normal text size instead (see gameSlimestretcher.c precedent).
  text(multText, 3, 9);
}

void addGameJetStream() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
