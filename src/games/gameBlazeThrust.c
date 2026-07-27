#include "cglp.h"
#include "string.h"

static char *title = "BLAZE THRUST";
static char *description = "[Hold] Thrust upward";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 1, .isDarkColor = false};

typedef struct {
  Vector pos;
  Vector velocity;
  float size;
  float windEffect;
} Flame;
static Flame flame;

typedef struct {
  Vector pos;
  float width;
  float force;
  bool isAlive;
} WindCurrent;
// Up to 20 are created at init, replenished up to a soft cap of 10 more
// while scrolling - sized with headroom above the initial population.
#define BLAZETHRUST_MAX_WIND_CURRENT_COUNT 40
static WindCurrent windCurrents[BLAZETHRUST_MAX_WIND_CURRENT_COUNT];
static int windCurrentIndex;

typedef struct {
  Vector pos;
  Vector velocity;
  int age;
  bool isPurple;
  float waveOffset;
  float waveAmplitude;
  bool isAlive;
} WindParticle;
// Sized generously above the ~50 concurrent estimated from spawn chance x lifetime.
#define BLAZETHRUST_MAX_WIND_PARTICLE_COUNT 256
static WindParticle windParticles[BLAZETHRUST_MAX_WIND_PARTICLE_COUNT];
static int windParticleIndex;

typedef struct {
  Vector pos;
  float size;
  float windEffect;
  float fallSpeed;
  bool isAlive;
} OxygenBubble;
// Hard-capped at 15 concurrent by the spawn gate itself.
#define BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT 16
static OxygenBubble oxygenBubbles[BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT];
static int oxygenBubbleIndex;

static float scrollOffset;
static float lastWindY;
static float nextBubbleDistance;
static float windAnimOffset;
static int blinkTimer;
static int multiplier;
static float holdTime;

static void update() {
  if (!ticks) {
    vectorSet(&flame.pos, 50, 90);
    vectorSet(&flame.velocity, 0, 0);
    flame.size = 9;
    flame.windEffect = 0.1;
    INIT_UNALIVED_ARRAY(windCurrents);
    windCurrentIndex = 0;
    INIT_UNALIVED_ARRAY(oxygenBubbles);
    oxygenBubbleIndex = 0;
    scrollOffset = 0;
    lastWindY = -100;
    nextBubbleDistance = rnd(10, 40);
    windAnimOffset = 0;
    INIT_UNALIVED_ARRAY(windParticles);
    windParticleIndex = 0;
    blinkTimer = 0;
    multiplier = 1;
    holdTime = 0;

    float currentY = 0;
    TIMES(20, wi) {
      float width = rnd(20, 40);
      ASSIGN_ARRAY_ITEM(windCurrents, windCurrentIndex, WindCurrent, w);
      vectorSet(&w->pos, 0, currentY);
      w->width = width;
      w->force = rnd(0.1, 0.3) * RNDPM();
      w->isAlive = true;
      windCurrentIndex = wrap(windCurrentIndex + 1, 0, BLAZETHRUST_MAX_WIND_CURRENT_COUNT);
      currentY += width + rnd(20, 40);
      lastWindY = currentY - rnd(20, 40);
    }
  }

  if (input.isPressed) {
    flame.velocity.y -= 0.2;
    holdTime++;
  } else {
    holdTime -= 3;
    if (holdTime < 0) {
      holdTime = 0;
    }
  }

  flame.velocity.y += 0.1;

  float scrollSpeed = 0;
  if (flame.pos.y < 60) {
    scrollSpeed = (60 - flame.pos.y) * 0.1;
    flame.pos.y += scrollSpeed;
    scrollOffset += scrollSpeed;
  }

  vectorAdd(&flame.pos, flame.velocity.x, flame.velocity.y);
  vectorMul(&flame.velocity, 0.95);

  FOR_EACH(windCurrents, wi2) {
    ASSIGN_ARRAY_ITEM(windCurrents, wi2, WindCurrent, w2);
    SKIP_IS_NOT_ALIVE(w2);
    w2->pos.y += scrollSpeed;
  }

  if (scrollSpeed > 0) {
    bool hasTopWind = false;
    int aliveWindCount = 0;
    FOR_EACH(windCurrents, wi3) {
      ASSIGN_ARRAY_ITEM(windCurrents, wi3, WindCurrent, w3);
      SKIP_IS_NOT_ALIVE(w3);
      aliveWindCount++;
      if (w3->pos.y < 0) {
        hasTopWind = true;
      }
    }
    if (!hasTopWind && aliveWindCount < 10) {
      float width = rnd(20, 40);
      float gap = rnd(20, 40);
      float newY = lastWindY - gap - width;
      ASSIGN_ARRAY_ITEM(windCurrents, windCurrentIndex, WindCurrent, nw);
      vectorSet(&nw->pos, 0, newY);
      nw->width = width;
      nw->force = rnd(0.1, 0.3) * RNDPM();
      nw->isAlive = true;
      windCurrentIndex = wrap(windCurrentIndex + 1, 0, BLAZETHRUST_MAX_WIND_CURRENT_COUNT);
      lastWindY = newY;
    }
  }

  FOR_EACH(windCurrents, wi4) {
    ASSIGN_ARRAY_ITEM(windCurrents, wi4, WindCurrent, w4);
    SKIP_IS_NOT_ALIVE(w4);
    if (w4->pos.y >= 120) {
      w4->isAlive = false;
      continue;
    }
  }

  bool anyWindAlive = false;
  float topWindY = 0;
  FOR_EACH(windCurrents, wi5) {
    ASSIGN_ARRAY_ITEM(windCurrents, wi5, WindCurrent, w5);
    SKIP_IS_NOT_ALIVE(w5);
    if (!anyWindAlive || w5->pos.y < topWindY) {
      topWindY = w5->pos.y;
      anyWindAlive = true;
    }
  }
  if (anyWindAlive) {
    lastWindY = topWindY;
  }

  if (flame.pos.x < 5) {
    flame.pos.x = 10;
    flame.velocity.x = fabs(flame.velocity.x) * 0.5;
  }
  if (flame.pos.x > 95) {
    flame.pos.x = 90;
    flame.velocity.x = -fabs(flame.velocity.x) * 0.5;
  }
  if (flame.pos.y < 5) {
    flame.pos.y = 10;
    flame.velocity.y = fabs(flame.velocity.y) * 0.3;
  }
  if (flame.pos.y > 95) {
    flame.pos.y = 90;
    flame.velocity.y = -fabs(flame.velocity.y) * 0.3;
  }

  windAnimOffset += 2;

  color = CYAN;
  FOR_EACH(windCurrents, wi6) {
    ASSIGN_ARRAY_ITEM(windCurrents, wi6, WindCurrent, w6);
    SKIP_IS_NOT_ALIVE(w6);
    rect(VEC_XY(w6->pos), 100, w6->width);
  }

  FOR_EACH(windParticles, wpi) {
    ASSIGN_ARRAY_ITEM(windParticles, wpi, WindParticle, p);
    SKIP_IS_NOT_ALIVE(p);
    p->pos.y += scrollSpeed;
    p->pos.x += p->velocity.x;
    float waveX = cos(p->age * 0.1 + p->waveOffset) * p->waveAmplitude;
    float waveY = sin(p->age * 0.1 + p->waveOffset) * p->waveAmplitude;
    p->pos.x += waveX;
    p->pos.y += waveY * 0.5;
    p->age++;
  }
  FOR_EACH(windParticles, pi2) {
    ASSIGN_ARRAY_ITEM(windParticles, pi2, WindParticle, p2);
    SKIP_IS_NOT_ALIVE(p2);
    if (!(p2->pos.y < 120 && p2->age < 180 && p2->pos.x > -10 && p2->pos.x < 110)) {
      p2->isAlive = false;
      continue;
    }
  }

  if (rnd(0, 1) < 0.15) {
    FOR_EACH(windCurrents, wi7) {
      ASSIGN_ARRAY_ITEM(windCurrents, wi7, WindCurrent, w7);
      SKIP_IS_NOT_ALIVE(w7);
      if (rnd(0, 1) < 0.25) {
        ASSIGN_ARRAY_ITEM(windParticles, windParticleIndex, WindParticle, np);
        vectorSet(&np->pos, 100, w7->pos.y + rnd(0, w7->width));
        vectorSet(&np->velocity, -rnd(1, 2), 0);
        np->age = 0;
        np->isPurple = true;
        np->waveOffset = rnd(0, M_PI * 2);
        np->waveAmplitude = rnd(0.2, 0.5);
        np->isAlive = true;
        windParticleIndex = wrap(windParticleIndex + 1, 0, BLAZETHRUST_MAX_WIND_PARTICLE_COUNT);
      }
    }

    for (int y = 0; y < 100; y += 20) {
      if (rnd(0, 1) < 0.15) {
        float particleY = y + rnd(0, 15);
        bool isInCyan = false;
        FOR_EACH(windCurrents, wi8) {
          ASSIGN_ARRAY_ITEM(windCurrents, wi8, WindCurrent, w8);
          SKIP_IS_NOT_ALIVE(w8);
          if (particleY >= w8->pos.y && particleY <= w8->pos.y + w8->width) {
            isInCyan = true;
          }
        }
        if (!isInCyan) {
          ASSIGN_ARRAY_ITEM(windParticles, windParticleIndex, WindParticle, np2);
          vectorSet(&np2->pos, 0, particleY);
          vectorSet(&np2->velocity, rnd(1, 2), 0);
          np2->age = 0;
          np2->isPurple = false;
          np2->waveOffset = rnd(0, M_PI * 2);
          np2->waveAmplitude = rnd(0.2, 0.5);
          np2->isAlive = true;
          windParticleIndex = wrap(windParticleIndex + 1, 0, BLAZETHRUST_MAX_WIND_PARTICLE_COUNT);
        }
      }
    }
  }

  FOR_EACH(windParticles, pi3) {
    ASSIGN_ARRAY_ITEM(windParticles, pi3, WindParticle, p3);
    SKIP_IS_NOT_ALIVE(p3);
    if (p3->isPurple) {
      color = LIGHT_CYAN;
    } else {
      color = LIGHT_GREEN;
    }
    rect(VEC_XY(p3->pos), 6, 2);
  }

  color = TRANSPARENT;
  Collision bgCol = box(VEC_XY(flame.pos), 1, 1);
  bool isOnCyan = bgCol.isColliding.rect[CYAN];

  if (isOnCyan) {
    flame.velocity.x -= flame.windEffect;
  } else {
    flame.velocity.x += flame.windEffect;
  }

  float shrinkRate = 0.0016 * difficulty * (1 + holdTime * 0.01);
  flame.size *= 1 - shrinkRate;
  flame.windEffect = flame.size * 0.02;

  if (flame.size <= 2) {
    play(EXPLOSION);
    gameOver();
  }

  blinkTimer++;

  bool shouldBlink = false;
  if (flame.size <= 5) {
    int blinkSpeed = (int)floor(fmax(5, 20 - (5 - flame.size) * 4));
    shouldBlink = blinkTimer % blinkSpeed < blinkSpeed / 2.0;
    if (blinkTimer % blinkSpeed == 0) {
      play(LASER);
    }
  }

  if (flame.size <= 5 && shouldBlink) {
    color = LIGHT_RED;
  } else {
    color = RED;
  }
  arc(VEC_XY(flame.pos), flame.size, 0, M_PI * 2);

  FOR_EACH(oxygenBubbles, oi) {
    ASSIGN_ARRAY_ITEM(oxygenBubbles, oi, OxygenBubble, ob);
    SKIP_IS_NOT_ALIVE(ob);
    ob->pos.y += scrollSpeed;
    ob->pos.y += ob->fallSpeed;
    bool isInCyanWind = false;
    FOR_EACH(windCurrents, wi9) {
      ASSIGN_ARRAY_ITEM(windCurrents, wi9, WindCurrent, w9);
      SKIP_IS_NOT_ALIVE(w9);
      if (ob->pos.y >= w9->pos.y && ob->pos.y <= w9->pos.y + w9->width) {
        isInCyanWind = true;
      }
    }
    if (isInCyanWind) {
      ob->pos.x -= ob->windEffect;
    } else {
      ob->pos.x += ob->windEffect;
    }
    if (ob->pos.x < 5) {
      ob->pos.x = 5;
    }
    if (ob->pos.x > 95) {
      ob->pos.x = 95;
    }
  }

  color = PURPLE;
  FOR_EACH(oxygenBubbles, oi2) {
    ASSIGN_ARRAY_ITEM(oxygenBubbles, oi2, OxygenBubble, ob2);
    SKIP_IS_NOT_ALIVE(ob2);
    Collision obCol = arc(VEC_XY(ob2->pos), ob2->size, 0, M_PI * 2);
    if (obCol.isColliding.rect[RED]) {
      play(COIN);
      addScore(multiplier, VEC_XY(ob2->pos));
      if (multiplier < 16) {
        multiplier++;
      }
      flame.size = flame.size + ob2->size * 0.2 * difficulty;
      if (flame.size > 25) {
        flame.size = 25;
      }
      flame.windEffect = flame.size * 0.02;

      int particleCount = (int)floor(ob2->size * 3);
      float ringRadius = ob2->size + 2;
      for (int pci = 0; pci < particleCount; pci++) {
        float angle = ((float)pci / particleCount) * M_PI * 2;
        float particleX = ob2->pos.x + cos(angle) * ringRadius;
        float particleY = ob2->pos.y + sin(angle) * ringRadius;
        particle(particleX, particleY, 1, 2, angle, 0.3);
      }
      ob2->isAlive = false;
      continue;
    }
    if (ob2->pos.y > 120) {
      if (multiplier > 1) {
        play(HIT);
      }
      multiplier = 1;
      ob2->isAlive = false;
      continue;
    }
  }

  if (scrollSpeed > 0) {
    int aliveBubbleCount = 0;
    FOR_EACH(oxygenBubbles, oi3) {
      ASSIGN_ARRAY_ITEM(oxygenBubbles, oi3, OxygenBubble, ob3);
      SKIP_IS_NOT_ALIVE(ob3);
      aliveBubbleCount++;
    }
    if (aliveBubbleCount < 15) {
      nextBubbleDistance -= scrollSpeed;
      if (nextBubbleDistance <= 0) {
        float size = rnd(3, 9);
        ASSIGN_ARRAY_ITEM(oxygenBubbles, oxygenBubbleIndex, OxygenBubble, nb);
        vectorSet(&nb->pos, rnd(10, 90), -size);
        nb->size = size;
        nb->windEffect = size * 0.02;
        nb->fallSpeed = 0.3 - (size - 3) * 0.015;
        nb->isAlive = true;
        oxygenBubbleIndex = wrap(oxygenBubbleIndex + 1, 0, BLAZETHRUST_MAX_OXYGEN_BUBBLE_COUNT);
        nextBubbleDistance = rnd(5, 55);
      }
    }
  }

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);
}

void addGameBlazeThrust() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
