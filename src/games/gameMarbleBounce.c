#include "cglp.h"
#include "string.h"

static char *title = "MARBLE BOUNCE";
static char *description = "[Tap] Bounce higher";

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
    .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 3, .isDarkColor = false};

typedef struct {
  Vector pos;
  float size;
} MarbleBounceMarble;
static MarbleBounceMarble marble;

typedef struct {
  Vector pos;
  Vector size;
} MarbleBounceGround;
static MarbleBounceGround ground;

typedef struct {
  Vector pos;
  float size;
  bool isTarget;
  bool spawning;
  float spawnSize;
  float targetSize;
  int lastHitTime;
  int createdTime;
  float warningTime;
  float explosionTime;
  bool isAlive;
} MarbleBouncePin;
// Regular pins never expire upstream either, and the periodic spawner's
// interval floors at 90/sqrt(difficulty) ticks while only one pin is
// consumed per target hit, so population trends upward over a normal long
// session and can reach the old 128-slot cap; raised for headroom.
#define MARBLEBOUNCE_MAX_PIN_COUNT 512
static MarbleBouncePin pins[MARBLEBOUNCE_MAX_PIN_COUNT];
static int pinIndex;

static Vector marbleVelocity;
static float gravity;
static float pinSpawnTimer;
static int multiplier;
static float pinSpawnInterval;
static float baseSpawnInterval;

// Shared by both the initial 8-pin setup and each new wave; retries up to 50 times for spacing.
static void spawnWaveOfPins(bool markSpawning) {
  int i;
  for (i = 0; i < 8; i++) {
    Vector newPos;
    int attempts = 0;
    bool validPosition = false;
    while (attempts < 50 && !validPosition) {
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
      validPosition = true;
      FOR_EACH(pins, pinIdx) {
        ASSIGN_ARRAY_ITEM(pins, pinIdx, MarbleBouncePin, p);
        SKIP_IS_NOT_ALIVE(p);
        if (distanceTo(&newPos, p->pos.x, p->pos.y) < 15) {
          validPosition = false;
        }
      }
      if (distanceTo(&newPos, marble.pos.x, marble.pos.y) < 15) {
        validPosition = false;
      }
      attempts++;
    }
    if (!validPosition) {
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
    }
    ASSIGN_ARRAY_ITEM(pins, pinIndex, MarbleBouncePin, np);
    np->pos = newPos;
    np->size = 2;
    np->isTarget = (i == 0);
    np->spawning = markSpawning;
    if (markSpawning) {
      np->spawnSize = 0;
    } else {
      np->spawnSize = 2;
    }
    np->targetSize = 2;
    np->lastHitTime = 0;
    np->createdTime = ticks;
    np->warningTime = 600;
    np->explosionTime = 900;
    np->isAlive = true;
    pinIndex = wrap(pinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
  }
}

static void update() {
  if (!ticks) {
    // Upstream's sss.setQuantize(0) has no equivalent in this port's sound engine; dropped.
    vectorSet(&marble.pos, 50, 80);
    marble.size = 3;
    INIT_UNALIVED_ARRAY(pins);
    pinIndex = 0;
    vectorSet(&ground.pos, 50, 95);
    vectorSet(&ground.size, 100, 5);
    vectorSet(&marbleVelocity, 0, 0);
    gravity = 0.15;
    pinSpawnTimer = 0;
    multiplier = 1;
    baseSpawnInterval = 360;
    pinSpawnInterval = baseSpawnInterval;
    spawnWaveOfPins(false);
  }

  marbleVelocity.y += gravity;

  pinSpawnTimer += sqrt(difficulty);
  if (pinSpawnTimer >= pinSpawnInterval) {
    pinSpawnTimer = 0;
    int attempts = 0;
    bool spawned = false;
    while (attempts < 20 && !spawned) {
      Vector newPos;
      vectorSet(&newPos, rnd(10, 90), rnd(20, 80));
      bool tooClose = false;
      FOR_EACH(pins, pi2) {
        ASSIGN_ARRAY_ITEM(pins, pi2, MarbleBouncePin, p2);
        SKIP_IS_NOT_ALIVE(p2);
        if (distanceTo(&newPos, p2->pos.x, p2->pos.y) < 15) {
          tooClose = true;
        }
      }
      if (distanceTo(&newPos, marble.pos.x, marble.pos.y) < 15) {
        tooClose = true;
      }
      if (!tooClose) {
        COUNT_IS_ALIVE(pins, pinAliveCount);
        if (pinAliveCount < MARBLEBOUNCE_MAX_PIN_COUNT) {
          ASSIGN_ARRAY_ITEM(pins, pinIndex, MarbleBouncePin, np2);
          np2->pos = newPos;
          np2->size = 2;
          np2->isTarget = false;
          np2->spawning = true;
          np2->spawnSize = 0;
          np2->targetSize = 2;
          np2->lastHitTime = 0;
          np2->createdTime = ticks;
          np2->warningTime = 600;
          np2->explosionTime = 900;
          np2->isAlive = true;
          pinIndex = wrap(pinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
          play(LASER);
          pinSpawnInterval = clamp(pinSpawnInterval - 30, 90, baseSpawnInterval);
        }
        spawned = true;
      }
      attempts++;
    }
  }

  if (input.isJustPressed) {
    marbleVelocity.y = clamp(marbleVelocity.y - 3, -5, 5);
    play(JUMP);
  }

  vectorAdd(&marble.pos, marbleVelocity.x, marbleVelocity.y);

  color = GREEN;
  rect(ground.pos.x - 50, ground.pos.y - 2.5, 100, 5);

  color = BLACK;
  char multText[16];
  strcpy(multText, "x");
  strcat(multText, intToChar(multiplier));
  text(multText, 3, 9);

  if (marble.pos.y + marble.size >= ground.pos.y - 2.5) {
    marble.pos.y = ground.pos.y - 2.5 - marble.size;
    marbleVelocity.y = -marbleVelocity.y * 0.7;
    if (fabs(marbleVelocity.x) < 0.5) {
      marbleVelocity.x += rnd(1, 2) * RNDPM();
    }
  }

  FOR_EACH(pins, pi3) {
    ASSIGN_ARRAY_ITEM(pins, pi3, MarbleBouncePin, pin);
    SKIP_IS_NOT_ALIVE(pin);

    if (pin->spawning) {
      pin->spawnSize += 0.1;
      if (pin->spawnSize >= pin->targetSize) {
        pin->spawnSize = pin->targetSize;
        pin->spawning = false;
      }
      pin->size = pin->spawnSize;
    }

    if (pin->isTarget) {
      float timeAlive = (ticks - pin->createdTime) * sqrt(difficulty);

      if (timeAlive > pin->explosionTime) {
        play(EXPLOSION);
        gameOver();
        continue;
      }

      if (timeAlive > pin->warningTime) {
        float warningProgress = (timeAlive - pin->warningTime) / (pin->explosionTime - pin->warningTime);
        float intensity = warningProgress;

        float vibrationStrength = 0.2 + intensity * 1.0;
        float vibrationSpeed = 0.2 + intensity * 0.8;
        float vibrationX = sin(ticks * vibrationSpeed) * vibrationStrength;
        float vibrationY = cos(ticks * vibrationSpeed * 1.3) * vibrationStrength;

        int particleFrequency = (int)floor(20 - intensity * 15);
        int particleCount = (int)floor(2 + intensity * 6);
        float particleSpeed = 0.5 + intensity * 2;

        int spawnFreq = (int)clamp(particleFrequency, 3, 20);
        if (ticks % spawnFreq == 0) {
          play(HIT);
          particle(pin->pos.x + vibrationX, pin->pos.y + vibrationY, particleCount, particleSpeed, rnd(0, M_PI * 2),
                   M_PI * 2);
        }

        int flashSpeed = (int)floor(20 - intensity * 15);
        int clampedFlash = (int)clamp(flashSpeed, 3, 20);
        int clampedFlashHalf = (int)clamp(flashSpeed / 2, 1, 10);
        if (ticks % clampedFlash < clampedFlashHalf) {
          color = RED;
        } else {
          color = YELLOW;
        }
        arc(pin->pos.x + vibrationX, pin->pos.y + vibrationY, pin->size, 0, M_PI * 2);
      } else {
        color = RED;
        arc(pin->pos.x, pin->pos.y, pin->size, 0, M_PI * 2);
      }
    } else {
      color = BLACK;
      arc(pin->pos.x, pin->pos.y, pin->size, 0, M_PI * 2);
    }
  }

  color = BLUE;
  Collision marbleCollision = arc(marble.pos.x, marble.pos.y, marble.size, 0, M_PI * 2);

  if (marbleCollision.isColliding.rect[BLACK]) {
    MarbleBouncePin *closestPin = NULL;
    float closestDistance = 1000000;
    FOR_EACH(pins, pi4) {
      ASSIGN_ARRAY_ITEM(pins, pi4, MarbleBouncePin, p4);
      SKIP_IS_NOT_ALIVE(p4);
      if (!p4->isTarget) {
        float distance = distanceTo(&marble.pos, p4->pos.x, p4->pos.y);
        if (distance < closestDistance) {
          closestDistance = distance;
          closestPin = p4;
        }
      }
    }

    if (closestPin != NULL) {
      int attempts = 0;
      while (attempts < 10) {
        float separationAngle = angleTo(&closestPin->pos, marble.pos.x, marble.pos.y);
        addWithAngle(&marble.pos, separationAngle, 1);

        color = TRANSPARENT;
        Collision stillCollidingCheck =
            arc(marble.pos.x, marble.pos.y, marble.size, 0, M_PI * 2);
        if (!stillCollidingCheck.isColliding.rect[BLACK]) {
          break;
        }
        attempts++;
      }

      int hitCooldown = 9;
      if (ticks - closestPin->lastHitTime > hitCooldown) {
        addScore(multiplier, marble.pos.x, marble.pos.y);
        closestPin->lastHitTime = ticks;
        play(HIT);
      }

      marbleVelocity.x = rnd(1, 2) * RNDPM();
      marbleVelocity.y = -fabs(marbleVelocity.y) * 0.8;
    }

    color = BLUE;
    marbleCollision = arc(marble.pos.x, marble.pos.y, marble.size, 0, M_PI * 2);
  }

  if (marbleCollision.isColliding.rect[RED] || marbleCollision.isColliding.rect[YELLOW]) {
    MarbleBouncePin *targetPin = NULL;
    FOR_EACH(pins, pi5) {
      ASSIGN_ARRAY_ITEM(pins, pi5, MarbleBouncePin, p5);
      SKIP_IS_NOT_ALIVE(p5);
      if (p5->isTarget) {
        float distance = distanceTo(&marble.pos, p5->pos.x, p5->pos.y);
        if (distance < marble.size + p5->size) {
          targetPin = p5;
        }
      }
    }

    if (targetPin != NULL) {
      float separationAngle = angleTo(&targetPin->pos, marble.pos.x, marble.pos.y);
      addWithAngle(&marble.pos, separationAngle, 2);
      marbleVelocity.x = rnd(2, 3) * RNDPM();
      marbleVelocity.y = -fabs(marbleVelocity.y) * 0.9;

      addScore(multiplier * 100, marble.pos.x, marble.pos.y);
      play(POWER_UP);

      pinSpawnInterval = baseSpawnInterval;
      pinSpawnTimer = 0;

      targetPin->isAlive = false;

      COUNT_IS_ALIVE(pins, remainingPinCount);
      if (remainingPinCount == 0) {
        multiplier++;
        play(RANDOM);  // Equivalent to "lucky" in JS
        spawnWaveOfPins(true);
      } else {
        int regularIndices[MARBLEBOUNCE_MAX_PIN_COUNT];
        int regularCount = 0;
        FOR_EACH(pins, pi6) {
          ASSIGN_ARRAY_ITEM(pins, pi6, MarbleBouncePin, p6);
          SKIP_IS_NOT_ALIVE(p6);
          if (!p6->isTarget) {
            regularIndices[regularCount] = pi6;
            regularCount++;
          }
        }
        if (regularCount > 0) {
          int chosen = regularIndices[rndi(0, regularCount)];
          ASSIGN_ARRAY_ITEM(pins, chosen, MarbleBouncePin, randomPin);
          randomPin->isTarget = true;
          randomPin->createdTime = ticks;
          randomPin->warningTime = 600;
          randomPin->explosionTime = 900;
        } else {
          ASSIGN_ARRAY_ITEM(pins, pinIndex, MarbleBouncePin, newTarget);
          vectorSet(&newTarget->pos, rnd(10, 90), rnd(20, 80));
          newTarget->size = 2;
          newTarget->isTarget = true;
          newTarget->spawning = true;
          newTarget->spawnSize = 0;
          newTarget->targetSize = 2;
          newTarget->lastHitTime = 0;
          newTarget->createdTime = ticks;
          newTarget->warningTime = 600;
          newTarget->explosionTime = 900;
          newTarget->isAlive = true;
          pinIndex = wrap(pinIndex + 1, 0, MARBLEBOUNCE_MAX_PIN_COUNT);
        }
      }

      color = BLUE;
      arc(marble.pos.x, marble.pos.y, marble.size, 0, M_PI * 2);
    }
  }

  if (marble.pos.x < 5) {
    marble.pos.x = 5;
    marbleVelocity.x = fabs(marbleVelocity.x);
  }
  if (marble.pos.x > 95) {
    marble.pos.x = 95;
    marbleVelocity.x = -fabs(marbleVelocity.x);
  }

  if (marble.pos.y < 5) {
    marble.pos.y = 5;
    marbleVelocity.y = fabs(marbleVelocity.y);
  }
}

void addGameMarbleBounce() {
  addGame(title, description, characters, charactersCount, options, false, update);
}
